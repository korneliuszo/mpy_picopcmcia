import machine
import picopcmcia
import micropython
import array
import uio
import builtins
import asyncio
import struct

import attr_rom
import common_rom
import ioiface

micropython.alloc_emergency_exception_buf(10000)

machine.freq(300000000)

uart1 = machine.UART(0,baudrate=1000000)

uart1.write(b"\r\n\r\n\r\n\r\nWOOTBOOT!!\r\n")

picopcmcia.init()

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


u=tracerom(open("attr.bin","rb").read())

class tracecrom:
    def __init__(self,rom):
        picopcmcia.picopcmcia_low.irq(2,handler=common_rom.rom(rom))
        l23=picopcmcia.picopcmcia_low.get_l2(3)
        for i in range(len(l23)):
            l23[i] = 0x09
        picopcmcia.picopcmcia_low.set_l1_entry(0x1700,3)
        picopcmcia.picopcmcia_low.set_l1_entry(0x1600,3)
        picopcmcia.picopcmcia_low.set_l1_entry(0x1500,3)
        picopcmcia.picopcmcia_low.set_l1_entry(0x1400,3)


u=tracecrom(open("common.bin","rb").read())


ring = micropython.RingIO(10000)
picopcmcia.picopcmcia_low.set_trace(ring)
async def s(ring):
    uart1.write(b"DUMP\r\n")
    sreader = asyncio.StreamReader(ring)
    while True:
        data = await sreader.read(16)
        pdata = struct.unpack('<IIII',data)
        out = "0x%04X 0x%07X 0x%07X 0x%02X\r\n" % pdata 
        uart1.write(out)

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
l24[0] = (3<<2)|2
picopcmcia.picopcmcia_low.set_l1_entry(0x1A00,4)
l25=picopcmcia.picopcmcia_low.get_l2(5)
l25[0] = (4<<2)|1
picopcmcia.picopcmcia_low.set_l1_entry(0x1900,5)

class Testioface():

    def __init__(self,worker):
        print("init")
        self.send=micropython.RingIO(100)
        self.recv=micropython.RingIO(100)
        worker.set_send(self.send)
        worker.set_recv(self.recv)
        self.task = asyncio.create_task(self.thread())

    async def thread(self):
        sreader = asyncio.StreamReader(self.recv)
        while True:
            data = await sreader.read(1)
            print("pong" + hex(data[0]))
            self.send.write(data)

def Testiofacecallback(worker):
    print("preinit")
    return Testioface(worker)

ioiface.register_entry(0,Testiofacecallback)

picopcmcia.ready()

import aiorepl
async def main():
    repl = asyncio.create_task(aiorepl.task())
    tracer = asyncio.create_task(s(ring))
    time = asyncio.create_task(t())
    await asyncio.gather(tracer, time, repl)


asyncio.run(main())