
# SPDX-FileCopyrightText: Korneliusz Osmenda <korneliuszo@gmail.com>
# SPDX-License-Identifier: MIT

from machine import Pin
from rp2 import PIO, StateMachine, asm_pio
import asyncio
from uctypes import addressof
import picopcmcia_low


HOST_IO=const(27)
PICO_AD0=const(0)
HOST_IOIS16=const(30)
HOST_WAIT=const(28)
PICO_MUX=const(29)
HOST_IREQ=const(31)

#mux0     A25 A24 
#         A23 A22 A21 A20 
#         A19 A18 A17 A16  
#         A15 A14 A13 A12
#         A11 A10 A9  A8
#         A7  A6  A5  A4
#         A3  A2  A1  A0
#mux1     A25 CE2 
#         CE1 INPACK N/C WE  
#         OE  REG IOWR IORD 
#         D15 D14 D13 D12 
#         D11 D10 D9  D8 
#         D7  D6  D5  D4 
#         D3  D2  D1  D0
#         set=WAIT(1=allow to run),MUX (LSB)
#         sideset=INPACK(DIR)

@asm_pio(
    out_init=(PIO.IN_LOW,)*26,
    set_init=(PIO.OUT_HIGH,PIO.OUT_HIGH),
    sideset_init=(PIO.OUT_LOW),
    side_pindir=True,
    push_thresh=26,
    pull_thresh=32,
    autopush=True,
    autopull=True,
    in_shiftdir=PIO.SHIFT_LEFT,
    out_shiftdir=PIO.SHIFT_RIGHT
)
def pcmcia_prog():
    mov(pins,null).side(0)
    jmp("entry")
    label("us")
    set(pins,0b10)
    jmp(not_x,"no_iord")
    nop().side(1)
    label("no_iord")
    out(pins,16)
    out(pindirs,16) [7] #tpd time??
    label("entry")
    wrap_target()
    set(pins,0b11)
    wait(1,gpio,27)
    mov(null,null).side(0b0) #pindir bug workaround
    set(pins,0b01)
    wait(0,gpio,27)
    set(pins,0b00) ## wait even faster to comply
    in_(pins,26)
    set(pins,0b10)[5] 
    # 0b10 - always wait (comply with 35ns attr req)
    # 0b11 - wait on decode (200ns@300MHz)
    in_(pins,26)
    out(x,32) # 0- not us 1-inpack cycle, 2-normal cycle
    jmp(x_dec,"us")
    wrap()

def init():
    Pin(HOST_IREQ, Pin.OUT).value(0)# RDY hack
    sm = StateMachine(0, pcmcia_prog,
        in_base=Pin(PICO_AD0),
        out_base=Pin(PICO_AD0),           
        set_base=Pin(HOST_WAIT),
        sideset_base=Pin(22),
        )
    Pin(HOST_IO, Pin.IN)
    sm.active(0)
    sm.restart()
    sm.active(1)
    #assume IO 8bit if not forced by PCIC
    #
    Pin(HOST_IOIS16, Pin.OUT).value(1) 

    picopcmcia_low.init()

def ready():
    Pin(HOST_IREQ, Pin.OUT).value(1)
