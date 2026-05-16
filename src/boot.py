import machine
import picopcmcia
import micropython
import array
import uio
import builtins

micropython.alloc_emergency_exception_buf(10000)

uart1 = machine.UART(0,baudrate=1000000)

uart1.write(b"\r\n\r\n\r\n\r\nWOOTBOOT!!\r\n")

picopcmcia.init()

class tracerom:
    def __init__(self):
        self.rom = bytearray(
            (0x55,0xAA)+
            (0x5A,0xA5)+
            (0xA5,0x5A)+
            (0xAA,0x55)
        )
        self.ridx = array.array('I',(0,))
        self.widx = array.array('I',(0,))
        self.fifo = array.array('I',(0,)*1024) #hardcoded

        picopcmcia.picopcmcia_low.attr_irq(0,handler=self.u,start=0,mask=0,hard=True)


    @micropython.viper
    def s(self,data):
        pdata=ptr32(data)
        uart1.write(hex(pdata[0]))
        uart1.write(b"\r\n")
        uart1.write(hex(pdata[1]))
        uart1.write(b"\r\n")
        uart1.write(hex(pdata[2]))
        uart1.write(b"\r\n")
        uart1.write(hex(pdata[3]))
        uart1.write(b"\r\n")

    @micropython.viper
    def u(self:object,ctx:object):
    #    self.v(ctx.mux0,ctx.mux1,ctx.offset,)
    #@micropython.viper
    #def v(mux0:uint,mux1:uint,offset:uint,)
        mux0=uint(ctx.mux0())
        mux1=uint(ctx.mux1())
        offset=uint(ctx.offset())

        idxp=ptr32(self.widx)
        idx=uint(idxp[0])

        data=ptr32(self.fifo)
        data[idx+0]=idx
        data[idx+1]=mux0
        data[idx+2]=mux1
        data[idx+3]=offset

        idx=(idx+uint(4)) & uint(1023)
        idxp[0]=idx
        ctx.set_data(uint(ptr16(self.rom)[uint(offset)]),uint(0xffff))

u=tracerom()

