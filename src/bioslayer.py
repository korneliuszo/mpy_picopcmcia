
# SPDX-FileCopyrightText: Korneliusz Osmenda <korneliuszo@gmail.com>
# SPDX-License-Identifier: MIT

import struct

async def readentry(self):
    self.regs = dict(zip(
        (
            "irq",
            "codeplace",
            "es",
            "ds",
            "di",
            "si",
            "bp",
            "bx",
            "dx",
            "cx",
            "ax",
            "f", #use for stackcode base
            "ph1",
            "ph2",
            "ip",
            "cs",
            "rf", #only in irq
            "rettype"
        ),
        (struct.unpack('<BBHHHHHHHHHHHHHHHB',
    await self.sreader.readexactly(33)))))
    await finish_cmd(self,0x03)

async def finish_cmd(self,req):
    if (await self.sreader.readexactly(1))[0] != req:
        raise Exception("Not synced")
    if self.send.any() != 0:
        raise Exception("Not synced2")


async def callback_end(self):
    self.send.write(bytes([
            0x02,])+
        struct.pack("<HHHHHHHHHHHHHHHB",
            self.regs["es"],
            self.regs["ds"],
            self.regs["di"],
            self.regs["si"],
            self.regs["bp"],
            self.regs["bx"],
            self.regs["dx"],
            self.regs["cx"],
            self.regs["ax"],
            self.regs["f"], #use for stackcode base
            self.regs["ph1"],
            self.regs["ph2"],
            self.regs["ip"],
            self.regs["cs"],
            self.regs["rf"], #only in irq
            self.regs["rettype"]
            ))
    await finish_cmd(self,0x02)
    await callback_end_noset(self)

async def callback_end_noset(self):
    self.send.write(bytes([0x03]))

async def chain(self,callback):
    self.regs["ph1"] = callback[0]
    self.regs["ph2"] = callback[1]
    self.regs["rettype"]=0

def stackregs():
    return {"es":0,
            "ds":0,
            "di":0,
            "si":0,
            "bx":0,
            "dx":0,
            "cx":0,
            "ax":0,
            "f":0
        }

async def stackcode8(self,regs,code):
    self.send.write(
        bytes([
            0x01])+
        struct.pack("<HHHHHHHHH",
            regs["es"],
            regs["ds"],
            regs["di"],
            regs["si"],
            regs["bx"],
            regs["dx"],
            regs["cx"],
            regs["ax"],
            regs["f"]
            )+code)
    regs_out = dict(zip(
        (
            "es",
            "ds",
            "di",
            "si",
            "bx",
            "dx",
            "cx",
            "ax",
            "f", #use for stackcode base
        ),
        (struct.unpack('<HHHHHHHHH',
    await self.sreader.readexactly(18)))))

    await finish_cmd(self,0x01)
    return regs_out

async def putmem(self,seg,addr,data):
    l=len(data)
    for i in range(0,l,512):
        l2=min(512,l-i)        
        await putmems(self,seg,addr+i,data[i:i+l2])

async def putmems(self,seg,addr,data):
    print("Put: ",seg,addr,len(data))
    self.send.write(
        bytes([0x06])+
        struct.pack(">HHH",seg,addr,len(data))+
        data)
    await finish_cmd(self,0x06)

async def getmem(self,seg,addr,l):
    ret = bytearray(l)
    for i in range(0, l, 512):
        l2 = min(512,l-i)
        ret[i:i+l2] = await getmem_pages(self,seg,addr+i,l2)

async def getmems(self,seg,addr,l):
    self.send.write(
        bytes([0x07])+
        struct.pack(">HHH",seg,addr,l))
    ret =  await self.sreader.readexactly(l)
    await finish_cmd(self,0x07)
    return ret

async def putstr(self,line):
    regs = stackregs()
    regs["f"]=self.regs["f"]
    for b in line:
        regs["ax"]=(0x0e<<8)|ord(b)
        await stackcode8(self,regs,bytes([0xCD,0x10,0xCB]))

async def install_irq(self,irq):
    self.send.write(
        bytes([0x08,irq]))
    ret =  await self.sreader.readexactly(4)
    await finish_cmd(self,0x08)
    return struct.unpack("<HH",ret)

async def getch(self):
    regs = stackregs()
    regs["f"]=self.regs["f"]
    regs["ax"]=0x0100
    return (await stackcode8(self,regs,bytes([0xCD,0x16,0xCB])))["ax"]

