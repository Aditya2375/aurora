#include "shell.h"
#include "kbd.h"
#include "kprint.h"
#include "vga.h"
#include "heap.h"
#include "pmm.h"
#include "serial.h"
#include "string.h"
#include "timer.h"
#include "memory.h"
#include "fs.h"
#define LINE_MAX 128
#define HIST_MAX 16
static char history[HIST_MAX][LINE_MAX];
static int hist_len, cwd;
static void hist_push(const char *line) {
    if (hist_len && !strcmp(history[hist_len-1],line)) return;
    if (hist_len==HIST_MAX) {
        for(int i=1;i<HIST_MAX;i++) strcpy(history[i-1],history[i]);
        hist_len--;
    }
    strcpy(history[hist_len++],line);
}
static void prompt(void) {
    char path[FS_PATH];
    if(fs_path(cwd,path,sizeof(path))<0) strcpy(path,"?");
    kprintf_attr(ATTR_ACCENT,"aurora");
    kprintf_attr(ATTR_DIM,":");
    kprintf_attr(ATTR_TITLE,"%s",path);
    kprintf_attr(ATTR_DIM,"$ ");
    serial_write("SHELL_READY\n");
}
static void erase(int n) { while(n-->0) kprintf("\b"); }
static void read_line(char *line) {
    int len=0, selected=hist_len;
    char draft[LINE_MAX]; draft[0]=0;
    for(;;) {
        char c=kbd_getc();
        if(c=='\n') { kprintf("\n"); line[len]=0; return; }
        if(c==17 || c==18) {
            if(selected==hist_len) { memcpy(draft,line,(size_t)len); draft[len]=0; }
            if(c==17 && selected>0) selected--;
            if(c==18 && selected<hist_len) selected++;
            erase(len);
            strcpy(line,selected==hist_len?draft:history[selected]);
            len=(int)strlen(line); kprintf("%s",line); continue;
        }
        if(c=='\b') { if(len) {len--;kprintf("\b");} continue; }
        if(c=='\t') c=' ';
        if(c>=32 && c<=126 && len<LINE_MAX-1) {line[len++]=c;kprintf("%c",(int)c);}
    }
}
static void help(void) {
    kprintf_attr(ATTR_TITLE,"Aurora command desk\n");
    kprintf("  help  about  mem  check  uptime  echo  history\n");
    kprintf("  pwd  ls [path]  cd path  tree  stat path  cat path\n");
    kprintf("  mkdir path  touch path  rm path\n");
    kprintf("  write path text  append path text\n");
    kprintf("  color  clear  halt\n");
    kprintf_attr(ATTR_DIM,"Up/down: history. Files live in RAM and reset on reboot.\n");
}
static void mem(void) {
    kprintf("e820 total memory : %u MiB\n",(unsigned)(pmm_total_memory()/1048576));
    kprintf("pmm free pages    : %u of %u\n",(unsigned)pmm_free_blocks(),(unsigned)pmm_usable_blocks());
    kprintf("heap              : %u bytes used, %u bytes mapped\n",(unsigned)heap_used(),(unsigned)heap_total());
    kprintf("heap integrity    : %s\n",heap_check()?"ok":"BROKEN");
    kprintf("RAM filesystem    : %d / %d nodes, %d-byte file limit\n",fs_used(),FS_NODES,FS_DATA-1);
    kprintf("paging            : 4 KiB, supervisor, CR0.WP enabled\n");
}
static void check(void) {
    int safe=memory_check();
    kprintf_attr(safe?ATTR_OK:ATTR_ERR,"[%s] hardware caught null read, stack-guard read, code write\n",safe?"PASS":"FAIL");
    kprintf("       recovered diagnostic page faults: %u\n",memory_faults_caught());
    kprintf_attr(heap_check()?ATTR_OK:ATTR_ERR,"[%s] kernel heap metadata\n",heap_check()?"PASS":"FAIL");
    kprintf_attr(ATTR_WARN,"LIMIT: ring-0 shell, no userspace isolation, no NX or ASLR.\n");
    kprintf("Read /sys/security.txt for the full threat model.\n");
}
static char *word(char **p) {
    while(**p==' ') (*p)++;
    char *start=*p;
    while(**p && **p!=' ') (*p)++;
    if(**p) *(*p)++=0;
    return start;
}
static void fs_result(int e) { if(e<0) kprintf_attr(ATTR_ERR,"fs: %s\n",fs_error(e)); }
static void list(int id) {
    struct fs_info f;
    if(fs_stat(id,&f)<0) { fs_result(FS_NOT_FOUND);return; }
    if(!f.dir) {kprintf("  %s  %u bytes\n",f.name,(unsigned)f.size);return;}
    for(int i=1;i<FS_NODES;i++) if(!fs_stat(i,&f) && f.parent==id) {
        kprintf_attr(f.dir?ATTR_ACCENT:ATTR_TITLE,"  %s%s",f.name,f.dir?"/":"");
        kprintf_attr(ATTR_DIM,"  %s  %u bytes\n",f.readonly?"ro":"rw",(unsigned)f.size);
    }
}
static void tree(int id,int depth) {
    struct fs_info f;
    for(int i=1;i<FS_NODES;i++) if(!fs_stat(i,&f) && f.parent==id) {
        for(int j=0;j<depth;j++) kprintf("  ");
        kprintf("%s%s\n",f.name,f.dir?"/":"");
        if(f.dir && depth<FS_NODES) tree(i,depth+1);
    }
}
static void alloc(const char *arg) {
    unsigned n=0;
    if(!*arg) {kprintf("usage: alloc <1..67108864>\n");return;}
    for(const char *p=arg;*p;p++) {
        if(*p<'0'||*p>'9'||n>(67108864u-(unsigned)(*p-'0'))/10) {
            kprintf_attr(ATTR_ERR,"alloc: expected 1..67108864 bytes\n");return;
        }
        n=n*10+(unsigned)(*p-'0');
    }
    void *p=kmalloc(n);
    if(!p) {kprintf("allocation refused (zero or insufficient contiguous RAM)\n");return;}
    memset(p,0x5A,n);
    kfree(p);
    kprintf("allocated, touched and freed %u bytes; heap %s\n",n,heap_check()?"ok":"BROKEN");
}
static void execute(char *line) {
    while(*line==' ') line++;
    if(!*line) return;
    hist_push(line);
    char *p=line, *cmd=word(&p);
    while(*p==' ') p++;
    if(!strcmp(cmd,"help")) help();
    else if(!strcmp(cmd,"about")) {
        kprintf_attr(ATTR_TITLE,"Aurora 0.2 / protected playground\n");
        kprintf("A real BIOS-bootable x86 kernel in C and assembly.\n");
        kprintf("4 KiB page protection, interrupt-driven keyboard and timer,\n");
        kprintf("E820 page allocator, checked heap, bounded RAM filesystem.\n");
        kprintf("No host command execution. No guest network driver. MIT licensed.\n");
        kprintf_attr(ATTR_WARN,"Educational, not a replacement for a maintained desktop OS.\n");
    }
    else if(!strcmp(cmd,"mem")) mem();
    else if(!strcmp(cmd,"check")) check();
    else if(!strcmp(cmd,"alloc")) alloc(p);
    else if(!strcmp(cmd,"echo")) kprintf("%s\n",p);
    else if(!strcmp(cmd,"pwd")) {char path[FS_PATH];if(!fs_path(cwd,path,sizeof(path)))kprintf("%s\n",path);}
    else if(!strcmp(cmd,"tree")) {kprintf("/\n");tree(0,1);}
    else if(!strcmp(cmd,"ls")) {int id=*p?fs_resolve(cwd,p):cwd;if(id<0)fs_result(id);else list(id);}
    else if(!strcmp(cmd,"cd")) {
        int id=fs_resolve(cwd,*p?p:"/home");struct fs_info f;
        if(id<0) fs_result(id);else {fs_stat(id,&f);if(!f.dir)fs_result(FS_NOT_DIR);else cwd=id;}
    }
    else if(!strcmp(cmd,"cat")) {
        const char *data;size_t len;int e=fs_read(cwd,p,&data,&len);
        if(e<0)fs_result(e);else {kprintf("%s",data);if(!len||data[len-1]!='\n')kprintf("\n");}
    }
    else if(!strcmp(cmd,"stat")) {
        int id=fs_resolve(cwd,p);struct fs_info f;
        if(id<0)fs_result(id);else {fs_stat(id,&f);kprintf("%s: %s, %s, %u bytes, node %d\n",f.name,f.dir?"directory":"file",f.readonly?"read-only":"read-write",(unsigned)f.size,id);}
    }
    else if(!strcmp(cmd,"mkdir")||!strcmp(cmd,"touch")) {
        int id=fs_resolve(cwd,p);
        if(!strcmp(cmd,"touch") && id>=0) {struct fs_info f;fs_stat(id,&f);if(f.dir)fs_result(FS_IS_DIR);}
        else fs_result(fs_create(cwd,p,!strcmp(cmd,"mkdir")));
    }
    else if(!strcmp(cmd,"rm")) fs_result(fs_remove(cwd,p));
    else if(!strcmp(cmd,"write")||!strcmp(cmd,"append")) {
        char *path=word(&p);
        fs_result(fs_write(cwd,path,p,!strcmp(cmd,"append")));
    }
    else if(!strcmp(cmd,"history")) {for(int i=0;i<hist_len;i++)kprintf("  %d  %s\n",i+1,history[i]);}
    else if(!strcmp(cmd,"uptime")) {
        uint64_t t=timer_ticks()/TIMER_HZ;
        kprintf("up %llu:%llu%llu:%llu%llu\n",(unsigned long long)(t/3600),(unsigned long long)((t/60%60)/10),(unsigned long long)(t/60%10),(unsigned long long)(t%60/10),(unsigned long long)(t%10));
    }
    else if(!strcmp(cmd,"color")) {for(int b=0;b<8;b++){for(int f=0;f<16;f++)kprintf_attr(VGA_ATTR(f,b),"%c",(int)0xDB);kprintf("\n");}}
    else if(!strcmp(cmd,"clear")) vga_clear();
    else if(!strcmp(cmd,"halt")) {kprintf("CPU halted. Reboot from the browser to start again.\n");serial_write("GUEST_HALTED\n");for(;;)__asm__ volatile("cli; hlt");}
    else kprintf("unknown command '%s'; type help\n",cmd);
}
void shell_run(void) {
    cwd=fs_resolve(0,"/home");
    char line[LINE_MAX];
    for(;;) {prompt();read_line(line);execute(line);}
}
