#!/usr/bin/env python3
"""Execute the real loader E820 routine in Unicorn with adversarial BIOS replies.
Requires nasm and Python unicorn; neither the routine nor checks are reimplemented.
"""
import pathlib,subprocess,tempfile,os
from unicorn import Uc,UC_ARCH_X86,UC_MODE_16,UC_HOOK_INTR,UC_HOOK_CODE
from unicorn.x86_const import *
ROOT=pathlib.Path(__file__).resolve().parent.parent
loader=(ROOT/'boot/loader.asm').read_text()
routine=loader[loader.index('e820_load:'):loader.index('print_hang:')]
asm='''[BITS 16]
[ORG 0]
%define E820_SEG 0x0500
%define E820_MAX 64
%define E820_COUNT 0x4FF0
mov sp, 0xF000
call e820_load
mov ax, 0xCAFE
hlt
'''+routine+'''print_hang:
mov ax,0xDEAD
hlt
msg_e820 db 0
'''
with tempfile.TemporaryDirectory() as d:
    source=pathlib.Path(d)/'probe.asm';binary=pathlib.Path(d)/'probe.bin';source.write_text(asm)
    subprocess.run([os.environ.get('NASM','nasm'),'-f','bin',str(source),'-o',str(binary)],check=True)
    code=binary.read_bytes()
def execute(replies,passed,label,check_attr=False):
    uc=Uc(UC_ARCH_X86,UC_MODE_16);uc.mem_map(0,0x100000);uc.mem_write(0x10000,code)
    uc.reg_write(UC_X86_REG_CS,0x1000);uc.reg_write(UC_X86_REG_DS,0x1000)
    index=0
    def interrupt(u,n,_):
        nonlocal index
        assert n==0x15
        r=replies[min(index,len(replies)-1)];index+=1
        flags=u.reg_read(UC_X86_REG_EFLAGS)
        u.reg_write(UC_X86_REG_EFLAGS,(flags|1) if r.get('cf') else (flags&~1))
        u.reg_write(UC_X86_REG_EAX,r.get('sig',0x534D4150));u.reg_write(UC_X86_REG_ECX,r.get('size',24))
        u.reg_write(UC_X86_REG_EBX,r.get('next',0))
        import struct
        addr=u.reg_read(UC_X86_REG_ES)*16+u.reg_read(UC_X86_REG_DI)
        data=struct.pack('<QQII',r.get('base',0x100000),0x4000000,1,r.get('attrs',1))
        u.mem_write(addr,data[:r.get('size',24)])
    def instruction(u,a,size,_):
        if u.mem_read(a,1)==b'\xf4':u.emu_stop()
    uc.hook_add(UC_HOOK_INTR,interrupt);uc.hook_add(UC_HOOK_CODE,instruction)
    uc.emu_start(0x10000,0x20000,count=100000)
    result=uc.reg_read(UC_X86_REG_AX)
    assert result==(0xCAFE if passed else 0xDEAD),(label,hex(result))
    if passed:
        assert int.from_bytes(uc.mem_read(0x4FF0,2),'little')==len(replies)
        if check_attr: assert int.from_bytes(uc.mem_read(0x5000+20,4),'little')==1
    print('E820:',label,'PASS')
execute([{}],True,'valid 24-byte terminal')
execute([{'size':20}],True,'legacy 20-byte normalized (attr read back as 1)',check_attr=True)
execute([{'next':1},{}],True,'complete continuation')
execute([{'next':1}]*63+[{}],True,'exactly 64 complete records')
execute([{'cf':True}],False,'unsupported BIOS fail closed')
execute([{'sig':0}],False,'invalid signature fail closed')
for size in (0,19,21,23,25):execute([{'size':size}],False,f'invalid record size {size}')
execute([{'next':1}]*64,False,'truncated 64-record map rejected')
execute([{'next':1},{'cf':True}],False,'error mid-map rejected')
