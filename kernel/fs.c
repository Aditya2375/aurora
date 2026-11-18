#include "fs.h"
#include "string.h"

/* Fixed capacity RAM filesystem. No parsing of untrusted disk structures, no
 * dynamic pointers in nodes, and mutations validate before changing anything. */
struct node { int live, parent, dir, readonly; size_t len;
              char name[FS_NAME]; char data[FS_DATA]; };
static struct node nodes[FS_NODES];
static int valid(int id) { return id >= 0 && id < FS_NODES && nodes[id].live; }
static int child(int parent, const char *name) {
    for (int i = 1; i < FS_NODES; i++)
        if (nodes[i].live && nodes[i].parent == parent &&
            !strcmp(nodes[i].name, name)) return i;
    return FS_NOT_FOUND;
}
static int component(const char **p, char *name) {
    while (**p == '/') (*p)++;
    size_t n = 0;
    while (**p && **p != '/') {
        unsigned char c = (unsigned char)**p;
        if (n >= FS_NAME - 1 || c < 33 || c > 126 || c == '\\') return FS_INVALID;
        name[n++] = *(*p)++;
    }
    name[n] = 0;
    return (int)n;
}
int fs_resolve(int cwd, const char *path) {
    if (!path || !*path || strnlen(path, FS_PATH) >= FS_PATH || !valid(cwd))
        return FS_INVALID;
    int id = *path == '/' ? 0 : cwd;
    const char *p = path;
    while (*p) {
        char name[FS_NAME];
        int n = component(&p, name);
        if (n < 0) return n;
        if (!n) break;
        if (!nodes[id].dir) return FS_NOT_DIR;
        if (!strcmp(name, ".")) continue;
        if (!strcmp(name, "..")) { id = nodes[id].parent; continue; }
        id = child(id, name);
        if (id < 0) return id;
    }
    if (path[strlen(path)-1] == '/' && !nodes[id].dir) return FS_NOT_DIR;
    return id;
}
static int destination(int cwd, const char *path, int *parent, char *name) {
    if (!path || !*path || strnlen(path, FS_PATH) >= FS_PATH) return FS_INVALID;
    size_t len = strlen(path);
    if (path[len-1] == '/') return FS_INVALID;
    size_t start = len;
    while (start && path[start-1] != '/') start--;
    if (len-start >= FS_NAME || !len || len == start) return FS_INVALID;
    const char *p = path + start;
    if (component(&p, name) < 1 || !strcmp(name, ".") || !strcmp(name, ".."))
        return FS_INVALID;
    char prefix[FS_PATH];
    if (!start) { prefix[0]='.'; prefix[1]=0; }
    else { memcpy(prefix, path, start); prefix[start]=0; }
    *parent = fs_resolve(cwd, prefix);
    if (*parent < 0) return *parent;
    if (!nodes[*parent].dir) return FS_NOT_DIR;
    if (nodes[*parent].readonly) return FS_READ_ONLY;
    char canonical[FS_PATH];
    if (fs_path(*parent,canonical,sizeof(canonical)) < 0 ||
        strlen(canonical)+strlen(name)+2 >= FS_PATH) return FS_TOO_BIG;
    return FS_OK;
}
int fs_create(int cwd, const char *path, int dir) {
    int parent; char name[FS_NAME];
    int e = destination(cwd, path, &parent, name);
    if (e) return e;
    if (child(parent, name) >= 0) return FS_EXISTS;
    for (int i=1; i<FS_NODES; i++) if (!nodes[i].live) {
        memset(&nodes[i], 0, sizeof(nodes[i]));
        nodes[i].live=1; nodes[i].parent=parent; nodes[i].dir=!!dir;
        strcpy(nodes[i].name, name);
        return i;
    }
    return FS_FULL;
}
int fs_stat(int id, struct fs_info *out) {
    if (!valid(id) || !out) return FS_NOT_FOUND;
    out->id=id; out->parent=nodes[id].parent; out->dir=nodes[id].dir;
    out->readonly=nodes[id].readonly; out->size=nodes[id].len;
    strcpy(out->name,nodes[id].name);
    return 0;
}
int fs_remove(int cwd, const char *path) {
    int id=fs_resolve(cwd,path);
    if (id<0) return id;
    if (!id || nodes[id].readonly || nodes[nodes[id].parent].readonly) return FS_READ_ONLY;
    /* Do not orphan a cwd, including one below a directory being removed. */
    for (int p=cwd;;p=nodes[p].parent) {
        if (p==id) return FS_INVALID;
        if (!p) break;
    }
    for (int i=1;i<FS_NODES;i++)
        if (nodes[i].live && nodes[i].parent==id) return FS_NOT_EMPTY;
    memset(&nodes[id],0,sizeof(nodes[id]));
    return 0;
}
int fs_write(int cwd, const char *path, const char *text, int append) {
    if (!text) return FS_INVALID;
    size_t n=strnlen(text,FS_DATA);
    if (n>=FS_DATA) return FS_TOO_BIG;
    int id=fs_resolve(cwd,path);
    if (id==FS_NOT_FOUND) id=fs_create(cwd,path,0);
    if (id<0) return id;
    if (nodes[id].dir) return FS_IS_DIR;
    if (nodes[id].readonly || nodes[nodes[id].parent].readonly) return FS_READ_ONLY;
    size_t offset=append ? nodes[id].len : 0;
    if (n>FS_DATA-1-offset) return FS_TOO_BIG;
    /* memmove permits data from this node to be used as a write source. */
    memmove(nodes[id].data+offset,text,n);
    nodes[id].data[offset+n]=0;
    if (offset+n < nodes[id].len)
        memset(nodes[id].data+offset+n+1,0,nodes[id].len-offset-n);
    nodes[id].len=offset+n;
    return 0;
}
int fs_read(int cwd, const char *path, const char **data, size_t *len) {
    int id=fs_resolve(cwd,path);
    if (id<0) return id;
    if (!data || !len) return FS_INVALID;
    if (nodes[id].dir) return FS_IS_DIR;
    *data=nodes[id].data; *len=nodes[id].len;
    return 0;
}
int fs_path(int id, char *out, size_t cap) {
    if (!valid(id) || !out || cap<2) return FS_INVALID;
    int chain[FS_NODES], n=0;
    size_t need=1;
    for (int p=id;p;p=nodes[p].parent) {
        if (n>=FS_NODES) return FS_INVALID;
        chain[n++]=p; need+=strlen(nodes[p].name)+1;
    }
    if (need+1>cap) return FS_TOO_BIG;
    size_t pos=0; out[pos++]='/';
    while(n) {
        const char *s=nodes[chain[--n]].name;
        size_t len=strlen(s); memcpy(out+pos,s,len); pos+=len;
        if(n) out[pos++]='/';
    }
    out[pos]=0;
    return 0;
}
int fs_used(void) { int n=0; for(int i=0;i<FS_NODES;i++) n+=!!nodes[i].live; return n; }
const char *fs_error(int code) {
    switch(code) {
    case FS_NOT_FOUND:return "not found"; case FS_NOT_DIR:return "not a directory";
    case FS_EXISTS:return "already exists"; case FS_FULL:return "filesystem full (48 nodes)";
    case FS_INVALID:return "invalid path or operation"; case FS_READ_ONLY:return "read-only system entry";
    case FS_NOT_EMPTY:return "directory not empty"; case FS_IS_DIR:return "is a directory";
    case FS_TOO_BIG:return "capacity exceeded"; default:return "filesystem error";
    }
}
void fs_init(void) {
    memset(nodes,0,sizeof(nodes)); nodes[0].live=nodes[0].dir=1;
    int sys=fs_create(0,"/sys",1);
    fs_create(0,"/home",1);
    fs_create(0,"/tmp",1);
    fs_write(0,"/home/welcome.txt","Welcome to Aurora. This file lives in kernel RAM.\nTry: write note.txt hello from a real kernel\nThen: cat note.txt\nRAM files vanish on reboot. Never put secrets in this demo.\n",0);
    fs_write(0,"/sys/security.txt","Aurora 0.2 security boundary\n\nON: 4 KiB supervisor pages, CR0.WP, read-only code and constants.\nON: unmapped null page and stack guard; fail-closed exceptions.\nON: bounded shell, file names, file contents and node capacity.\nOFF: user processes, ring-3 isolation, NX, ASLR, secure boot.\nOFF: networking, accounts, encryption and persistent storage.\nAll shell commands run in ring 0. Not a production OS.\nThe browser and v86 are part of the trusted computing base.\n",0);
    fs_write(0,"/sys/commands.txt","help   about   mem   check   uptime   echo   history\npwd    ls [path]    cd path    cat path    tree\nmkdir path    touch path    rm path\nwrite path text    append path text    stat path\ncolor    clear    halt\n\nPaths support /, . and ..; names cannot contain spaces.\nwrite and append store the remainder of the command verbatim.\nFiles: 1023 bytes each, 48 total nodes. RAM only.\n",0);
    for(int i=1;i<FS_NODES;i++) if(nodes[i].live && nodes[i].parent==sys) nodes[i].readonly=1;
    nodes[sys].readonly=1;
}
