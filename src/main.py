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
            l23[i] = 0x06
        picopcmcia.picopcmcia_low.set_l1_entry(0x1700,3)
        picopcmcia.picopcmcia_low.set_l1_entry(0x1600,3)
        picopcmcia.picopcmcia_low.set_l1_entry(0x1500,3)
        picopcmcia.picopcmcia_low.set_l1_entry(0x1400,3)


u=tracecrom(open("common.bin","rb").read())


ring = micropython.RingIO(100)
picopcmcia.picopcmcia_low.set_trace(ring)
async def s(ring):
    uart1.write(b"DUMP\r\n")
    sreader = asyncio.StreamReader(ring)
    while True:
        data = await sreader.read(16)
        pdata = struct.unpack('<IIII',data)
        uart1.write(hex(pdata[0]))
        uart1.write(b"\t")
        uart1.write(hex(pdata[1]))
        uart1.write(b"\t")
        uart1.write(hex(pdata[2]))
        uart1.write(b"\t")
        uart1.write(hex(pdata[3]))
        uart1.write(b"\r\n")

import aiorepl
async def main():
    repl = asyncio.create_task(aiorepl.task())
    tracer = asyncio.create_task(s(ring))
    await asyncio.gather(tracer, repl)


asyncio.run(main())