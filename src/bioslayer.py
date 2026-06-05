import struct

async def readentry(self):
    return dict(zip(
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
