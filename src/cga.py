
# SPDX-FileCopyrightText: Korneliusz Osmenda <korneliuszo@gmail.com>
# SPDX-License-Identifier: MIT

import bioslayer
import time

async def await_nop():
    return

async def display_frames(self,file):
    regs = bioslayer.stackregs()
    regs["ax"] = 0x0006
    await bioslayer.stackcode8(self,regs,bytes([0xCD,0x10,0xCB]))

    awaitable = await_nop()
    addr = 0xB800
    frame = bytearray(8096)
    start = time.ticks_ms()
    while True:
        file.readinto(frame)
        if len(frame)!=8096:
            break
        await awaitable
        stop=time.ticks_ms()
        print("FPS: ",1000/(stop-start))
        start=stop
        awaitable = bioslayer.putmem(self,addr,0,frame[0:100*80])
        if addr == 0xB800:
            addr = 0xBA00
        else:
            addr = 0xB800
    await awaitable

async def display_netframes(self,file):
    regs = bioslayer.stackregs()
    regs["ax"] = 0x0006
    await bioslayer.stackcode8(self,regs,bytes([0xCD,0x10,0xCB]))

    awaitable = await_nop()
    addr = 0xB800
    start = time.ticks_ms()
    while True:
        frame = file.readexactly(100*80)
        await awaitable
        awaitable = bioslayer.putmem(self,addr,0,await frame)
        stop=time.ticks_ms()
        print("FPS: ",1000/(stop-start))
        start=stop
        if addr == 0xB800:
            addr = 0xBA00
        else:
            addr = 0xB800
    await awaitable
