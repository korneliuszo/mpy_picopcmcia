import machine
import picopcmcia
import micropython
import array
import uio
import builtins
import asyncio
import struct

import attr_rom

micropython.alloc_emergency_exception_buf(10000)

machine.freq(300000000)

uart1 = machine.UART(0,baudrate=1000000)

uart1.write(b"\r\n\r\n\r\n\r\nWOOTBOOT!!\r\n")

picopcmcia.init()

class tracerom:

    @micropython.viper
    def u(self:object,ctx:object):
        #mux0=uint(ctx.mux0())
        #data=uint(ptr16(self.rom)[mux0])
        data=uint(0xaa55)
        mask=uint(0xffff)
        ctx.set_data(data,mask)
        
    @micropython.viper
    def vinit(self:object):
        self.ptr16=ptr16(self.rom)

    def __init__(self):
        self.rom = array.array("b",
        [
            (0x55),
            (0x5A),
            (0xA5),
            (0xAA)
        ]
        )
        rom = attr_rom.rom(self.rom)
        picopcmcia.picopcmcia_low.irq(1,handler=rom)
        l20=picopcmcia.picopcmcia_low.get_l2(0)
        for i in range(len(l20)):
            l20[i] = 0x01
        l21=picopcmcia.picopcmcia_low.get_l2(1)
        for i in range(len(l21)):
            l21[i] = 0x02
        l22=picopcmcia.picopcmcia_low.get_l2(2)
        for i in range(len(l22)):
            l22[i] = 0x05
        picopcmcia.picopcmcia_low.set_l1_entry(0x1300,2)
        picopcmcia.picopcmcia_low.set_l1_entry(0x1200,2)
        picopcmcia.picopcmcia_low.set_l1_entry(0x1100,2)
        picopcmcia.picopcmcia_low.set_l1_entry(0x1000,2)


u=tracerom()


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