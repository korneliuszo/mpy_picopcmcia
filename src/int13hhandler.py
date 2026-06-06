import bioslayer

CHS = (80,2,18)

def tolba(self,chs):
    c,h,s = CHS
    return (chs[0]*h + chs[1])*s + chs[2] - 1

def paramstochs(self):
    c = (self.regs["cx"]>>8) | ((self.regs["cx"]&0xC0)<<(8-6))
    h = ((self.regs["dx"] &0xff00)>>8 )
    s = self.regs["cx"]&0x3f
    return (c,h,s)

def readsector(file,offset,size):
    file.seek(offset*512)
    return file.read(size*512)

def writesector(file,offset,data):
    file.seek(offset*512)
    file.write(data)


async def read_cmd(self,file):
    l = self.regs["ax"]&0xff
    data = readsector(file,tolba(self,paramstochs(self)),l)
    await bioslayer.putmem(self,self.regs["es"],self.regs["bx"],data)
    self.regs["ax"] = l | (0x00<<8)
    self.regs["rf"] &= ~0x0001
    return True

async def write_cmd(self,file):
    l = self.regs["ax"]&0xff
    data = await bioslayer.getmem(self,self.regs["es"],self.regs["bx"],data)
    writesector(file,tolba(self,paramstochs(self)),data)    
    self.regs["ax"] = l | (0x00<<8)
    self.regs["rf"] &= ~0x0001
    return True

async def get_chs_cmd(self,file):
    c,h,s = CHS
    if c > 1024:
        c=1024
    h -= 1
    c -= 1
    self.regs["bx"] = 0
    self.regs["cx"] = ((c&0xff) <<8) | ((c&0x300)>>(8-6)) | s
    self.regs["dx"] = (h << 8) | 0x01
    self.regs["di"] = 0 # I hope DBT is not needed for now
    self.regs["es"] = 0
    self.regs["ax"] &= 0x00ff
    self.regs["rf"] &= ~0x0001
    return True

async def reset_cmd(self,file):
    self.regs["ax"] = 0x0000
    self.regs["rf"] &= ~0x0001
    return True

async def not_implemented_no_error(self,file):
    self.regs["ax"] = 0x0101
    self.regs["rf"] |= 0x0001
    return True

async def implemented_noop(self,file):
    self.regs["ax"] = 0x0000
    self.regs["rf"] &= ~0x0001
    return True

async def disk_status(self,file):
    self.regs["ax"] = 0x0000
    self.regs["rf"] &= 0x0001
    return True


async def handler(self,file):
    print(self.regs)
    ret = False
    try:
        ret = await {
            0x00: reset_cmd,
            0x02: read_cmd,
            0x03: write_cmd,
            0x08: get_chs_cmd,
            0x23: not_implemented_no_error, #set features
            0x24: not_implemented_no_error, #set multiple blocks
            0x25: not_implemented_no_error, #identify drive
            0x01: implemented_noop, # status
            0x09: implemented_noop, # init disk
            0x0d: implemented_noop, # areset
            0x0c: implemented_noop, # seek
            0x47: implemented_noop, # eseek
            0x10: implemented_noop, # driveready
            0x11: implemented_noop, # recalibrate
            0x14: implemented_noop, # diagnostic
            0x04: implemented_noop, # verify
            0x16: disk_status, # disk change

        }[self.regs["ax"]>>8](self,file)
    except KeyError:
        print("Error: ",hex(self.regs["ax"]>>8))
        self.regs["ax"] = 0x0101
        self.regs["rf"] |= 0x0001
        ret=True
    if ret:
        self.regs["rettype"]=1
        await bioslayer.callback_end(self)
    return ret    

