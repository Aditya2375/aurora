#!/usr/bin/env python3
"""Real PS/2 command tests through QEMU's monitor, no shell shortcuts."""
import os, pathlib, socket, subprocess, tempfile, time
ROOT=pathlib.Path(__file__).resolve().parent.parent
QEMU=os.environ.get('QEMU','qemu-system-i386')
KEYS={' ':'spc','\n':'ret','/':'slash','.':'dot','-':'minus','_':'shift-minus',':':'shift-semicolon'}

def run(mem, full=False):
    with tempfile.TemporaryDirectory() as tmp:
        serial=pathlib.Path(tmp)/'serial.log'; mon=str(pathlib.Path(tmp)/'monitor')
        proc=subprocess.Popen([QEMU,'-m',str(mem),'-drive',f'format=raw,file={ROOT}/build/aurora.img',
            '-display','none','-nic','none','-serial',f'file:{serial}',
            '-monitor',f'unix:{mon},server=on,wait=off','-no-reboot'],stdout=subprocess.PIPE,stderr=subprocess.PIPE)
        def log(): return serial.read_text(errors='replace') if serial.exists() else ''
        def wait(predicate, timeout=12):
            end=time.monotonic()+timeout
            while time.monotonic()<end:
                if predicate(): return
                if proc.poll() is not None: raise AssertionError(proc.stderr.read().decode()+log())
                time.sleep(.025)
            raise AssertionError('QEMU condition timeout\n'+log())
        try:
            wait(lambda: 'SHELL_READY' in log())
            assert 'BOOTTEST protection PASS' in log() and 'BOOTTEST all PASS' in log()
            sock=socket.socket(socket.AF_UNIX);sock.connect(mon);sock.recv(4096)
            def cmd(text, expected=None):
                old=log().count('SHELL_READY')
                for c in text+'\n':
                    key=KEYS.get(c,('shift-'+c.lower()) if c.isupper() else c)
                    sock.sendall(f'sendkey {key} 1\n'.encode());time.sleep(.012)
                wait(lambda: log().count('SHELL_READY')>old)
                if expected is not None: assert expected in log(), (text,log())
            cmd('check','[PASS] hardware caught')
            cmd('mem','heap integrity    : ok')
            if full:
                cmd('mkdir projects')
                cmd('cd projects')
                cmd('write note.txt hello from kernel')
                cmd('cat note.txt','hello from kernel')
                cmd('append note.txt  and protected ram')
                cmd('cat note.txt','hello from kernel and protected ram')
                cmd('pwd','/home/projects')
                cmd('write /sys/security.txt bad','read-only system entry')
                cmd('rm /home/projects','invalid path or operation')
                cmd('alloc 1048576','allocated, touched and freed 1048576 bytes; heap ok')
                cmd('alloc 9999999999999999999','alloc: expected')
                cmd('check','recovered diagnostic page faults: 9')
                cmd('cd ..')
                cmd('rm projects','directory not empty')
                cmd('rm projects/note.txt')
                cmd('rm projects')
                cmd('ls')
            assert 'KERNEL EXCEPTION' not in log()
            (ROOT/'build'/f'qemu-{mem}.log').write_text(log())
            print(f'QEMU {mem} MiB: protection, allocator, shell'+(', filesystem mutations PASS' if full else ' PASS'))
        finally:
            proc.terminate()
            try: proc.communicate(timeout=3)
            except subprocess.TimeoutExpired: proc.kill();proc.communicate()
for size in (16,32,128,256): run(size, size==128)
