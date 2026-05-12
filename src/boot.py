import machine
import picopcmcia
import micropython
micropython.alloc_emergency_exception_buf(100)

uart1 = machine.UART(0,baudrate=1000000)

uart1.write("\r\n\r\n\r\n\r\nWOOTBOOT!!\r\n")

picopcmcia.init()

@micropython.viper
def u(ctx):
    uart1.write(hex(ctx.mux0()))
    uart1.write("\r\n")
    uart1.write(hex(ctx.mux1()))
    uart1.write("\r\n")
    ctx.set_data(0xffff55aa)

picopcmcia.picopcmcia_low.attr_irq(0,handler=u,start=0,mask=0,hard=False)
