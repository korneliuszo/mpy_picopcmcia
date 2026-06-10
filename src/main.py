
# SPDX-FileCopyrightText: Korneliusz Osmenda <korneliuszo@gmail.com>
# SPDX-License-Identifier: MIT

import machine
import picopcmcia
import micropython
import array
import uio
import builtins
import asyncio
import uasyncio
import struct
import time

import attr_rom
import common_rom
import ioiface
import included_roms

import bioslayer
import int13hhandler

micropython.alloc_emergency_exception_buf(10000)

machine.freq(300000000)

uart1 = machine.UART(0,baudrate=1000000)

uart1.write(b"\r\n\r\n\r\n\r\nWOOTBOOT!!\r\n")

picopcmcia.init()


def thr(tsf):
    picopcmcia.picopcmcia_low.init()
    tsf.set()
    while True:
        time.sleep_ms(1000)

import _thread

class tracerom:
    def __init__(self,rom):
        picopcmcia.picopcmcia_low.irq(1,handler=attr_rom.rom(rom))
        l22=picopcmcia.picopcmcia_low.get_l2(2)
        for i in range(len(l22)):
            l22[i] = 0x05
        picopcmcia.picopcmcia_low.set_l1_entry(0x1300,2)
        picopcmcia.picopcmcia_low.set_l1_entry(0x1200,2)
        picopcmcia.picopcmcia_low.set_l1_entry(0x1100,2)
        picopcmcia.picopcmcia_low.set_l1_entry(0x1000,2)


u=tracerom(included_roms.attr())

class tracecrom:
    def __init__(self,rom):
        picopcmcia.picopcmcia_low.irq(2,handler=common_rom.rom(rom))
        picopcmcia.picopcmcia_low.irq(5,handler=common_rom.ram(rom))
        l23=picopcmcia.picopcmcia_low.get_l2(3)
        for i in range(len(l23)):
            l23[i] = 0x09
        picopcmcia.picopcmcia_low.set_l1_entry(0x1700,3)
        picopcmcia.picopcmcia_low.set_l1_entry(0x1600,3)
        picopcmcia.picopcmcia_low.set_l1_entry(0x1500,3)
        picopcmcia.picopcmcia_low.set_l1_entry(0x1400,3)
        l26=picopcmcia.picopcmcia_low.get_l2(6)
        for i in range(len(l26)):
            l26[i] = (5<<2)|1
        picopcmcia.picopcmcia_low.set_l1_entry(0x0F00,6)
        picopcmcia.picopcmcia_low.set_l1_entry(0x0E00,6)
        picopcmcia.picopcmcia_low.set_l1_entry(0x0D00,6)
        picopcmcia.picopcmcia_low.set_l1_entry(0x0C00,6)        


u=tracecrom(bytearray(included_roms.rom_8088()))

async def t():
    while True:
        await asyncio.sleep_ms(1000)
        val = picopcmcia.picopcmcia_low.get_ticks() / 300
        out = "Max us: %f\r\n" % (val,) 
        uart1.write(out)

ioiface.init()
picopcmcia.picopcmcia_low.irq(3,handler=ioiface.RD())
picopcmcia.picopcmcia_low.irq(4,handler=ioiface.WR())
l24=picopcmcia.picopcmcia_low.get_l2(4)
l24[0x1f] = (3<<2)|2
#for i in range(len(l24)):
#    l24[i] = (3<<2)|1

picopcmcia.picopcmcia_low.set_l1_entry(0x1A00,4)
l25=picopcmcia.picopcmcia_low.get_l2(5)
l25[0x1f] = (4<<2)|1
#for i in range(len(l25)):
#    l25[i] = (4<<2)|1

picopcmcia.picopcmcia_low.set_l1_entry(0x1900,5)

INT19_CBK=None
INT13_CBK=None

import vfs
import sdcard
vfs.mount(sdcard.SDCard(machine.SPI(0,baudrate=30000000),machine.Pin(37)),"/sd")

fdfile=open("/sd/TFT.raw","rb+")

netstream = None

async def new_conn(reader,writer):
    global netstream
    netstream = reader

installed = False

class Testioface():
    def __init__(self,worker):
        print("init")
        self.send=micropython.RingIO(16100)
        self.recv=micropython.RingIO(8050)
        worker.set_send(self.send)
        worker.set_recv(self.recv)
        self.sreader = asyncio.StreamReader(self.recv)
        self.task = asyncio.create_task(self.coro())

    async def coro(self):
        global INT19_CBK
        global INT13_CBK
        global fdfile
        global netstream
        global installed
        await bioslayer.readentry(self)
        if self.regs["irq"] == 0:
            if installed:
                await bioslayer.putstr(self,"PICOPCMCIA")
                import cga
                while netstream == None:
                    await asyncio.sleep_ms(1000)
                await cga.display_netframes(self,netstream)         
                #await bioslayer.callback_end(self)

                return
            else:
                await bioslayer.putstr(self,"PICOPCMCIA")
                INT19_CBK = await bioslayer.install_irq(self,0x19)
                await bioslayer.callback_end_noset(self)
                return
        elif self.regs["irq"] == 1:
            if self.regs["codeplace"] == 0x19:
                await bioslayer.putstr(self,"PICOPCMCIA")
                await bioslayer.putmem(self,0,0x7c00,included_roms.rom_8088())
                installed = True
                regs = bioslayer.stackregs()
                await bioslayer.stackcode8(self,regs,bytes([
                    0xEA, 0x03, 0x00, 0xc0, 0x07]))
      
        print("No handler for:\n")
        print(self.regs)
        

def Testiofacecallback(worker):
    print("preinit")
    return Testioface(worker)

ioiface.register_entry(0,Testiofacecallback)

picopcmcia.ready()

import aiorepl
async def main():
    tsf = asyncio.ThreadSafeFlag()
    _thread.start_new_thread(thr,(tsf,))
    await tsf.wait()
    picopcmcia.ready()
    repl = asyncio.create_task(aiorepl.task())
    time = asyncio.create_task(t())
    server = asyncio.create_task(uasyncio.start_server(new_conn,"0.0.0.0",5555))
    await asyncio.gather(repl, time, server)


asyncio.run(main())
