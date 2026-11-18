#include "harness.h"
#include "fs.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>
static uint32_t rng=0xA1704A;
static uint32_t next(void) { rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng; }
int main(void) {
    fs_init();
    int initial=fs_used();
    int home=fs_resolve(0,"/home");
    CHECK(home>0,"home exists");
    CHECK(fs_resolve(home,"../sys/./commands.txt")>0,"dot navigation");
    CHECK(fs_resolve(0,"../../../../")==0,"cannot escape root");
    CHECK(fs_create(home,"/sys/evil",0)==FS_READ_ONLY,"system dir protected");
    CHECK(fs_write(home,"/sys/security.txt","tamper",0)==FS_READ_ONLY,"system file protected");
    CHECK(fs_remove(home,"/sys")==FS_READ_ONLY,"cannot remove system directory");
    CHECK(fs_create(home,"work",1)>0,"create directory");
    CHECK(fs_write(home,"work/note","hello",0)==0,"create by write");
    CHECK(fs_write(home,"work/note"," world",1)==0,"append");
    const char *data;size_t n;
    CHECK(fs_read(home,"work/note",&data,&n)==0 && n==11 && !strcmp(data,"hello world"),"read contents");
    CHECK(fs_resolve(home,"work/note/..") == FS_NOT_DIR,"file cannot traverse");
    CHECK(fs_resolve(home,"work/note/")==FS_NOT_DIR,"file trailing slash rejected");
    CHECK(fs_remove(home,"work")==FS_NOT_EMPTY,"nonempty directory protected");
    CHECK(fs_create(home,"work/note",1)==FS_EXISTS,"no duplicate node");
    CHECK(fs_create(home,".",1)==FS_INVALID,"dot create rejected");
    CHECK(fs_create(home,"a b",0)==FS_INVALID,"space in name rejected");
    CHECK(fs_create(home,"",0)==FS_INVALID,"empty name rejected");
    CHECK(fs_create(home,"xxxxxxxxxxxxxxxxxxxxxxxx",0)==FS_INVALID,"name limit enforced");
    CHECK(fs_write(home,"work/note","x",0)==0,"overwrite");
    CHECK(fs_read(home,"work/note",&data,&n)==0 && n==1 && !strcmp(data,"x"),"overwrite truncates");
    char big[FS_DATA+1];memset(big,'A',sizeof(big));big[FS_DATA]=0;
    CHECK(fs_write(home,"work/note",big,0)==FS_TOO_BIG,"oversized write rejected");
    big[FS_DATA-1]=0;
    CHECK(fs_write(home,"work/note",big,0)==0,"maximum file fits");
    CHECK(fs_write(home,"work/note","B",1)==FS_TOO_BIG,"append quota atomic");
    CHECK(fs_read(home,"work/note",&data,&n)==0 && n==FS_DATA-1 && data[n-1]=='A',"failed append unchanged");
    CHECK(fs_read(home,"work",&data,&n)==FS_IS_DIR,"directory read rejected");
    int work=fs_resolve(home,"work");
    CHECK(fs_remove(work,".")==FS_INVALID,"cwd cannot be removed");
    char path[FS_PATH];
    CHECK(fs_path(work,path,sizeof(path))==0 && !strcmp(path,"/home/work"),"canonical path");
    CHECK(fs_path(work,path,4)==FS_TOO_BIG,"path output bounded");
    CHECK(fs_remove(home,"work/note")==0,"remove file");
    CHECK(fs_remove(home,"work")==0,"remove empty dir");
    CHECK(fs_used()==initial,"no node leak");
    int dir=home;
    while(1) {
        int e=fs_create(dir,"abcdefghijklmnopqrstuvw",1);
        if(e<0) {CHECK(e==FS_TOO_BIG,"canonical depth bounded");break;}
        dir=e;
    }
    fs_init();home=fs_resolve(0,"/home");
    for(int i=0;i<FS_NODES-initial;i++) {
        char name[20];snprintf(name,sizeof(name),"file%d",i);
        CHECK(fs_create(home,name,0)>0,"fill node capacity");
    }
    CHECK(fs_create(home,"one-more",0)==FS_FULL,"full filesystem refused");
    CHECK(fs_remove(home,"file0")==0,"free node");
    CHECK(fs_write(home,"new","reused",0)==0,"deleted node reused");
    /* Deterministic state-machine stress with a separate content model. */
    fs_init();home=fs_resolve(0,"/home");
    char model[12][FS_DATA];int live[12]={0};memset(model,0,sizeof(model));
    for(int k=0;k<12000;k++) {
        unsigned i=next()%12,op=next()%4;
        char name[16],text[24];snprintf(name,sizeof(name),"f%u",i);snprintf(text,sizeof(text),"v%u",next());
        if(op==0 || op==1) {
            size_t len=strlen(text),old=op==1?strlen(model[i]):0;
            int e=fs_write(home,name,text,op==1);
            if(old+len>=FS_DATA) CHECK(e==FS_TOO_BIG,"stress quota");
            else {CHECK(e==0,"stress write");memcpy(model[i]+old,text,len+1);live[i]=1;}
        } else if(op==2) {
            int e=fs_remove(home,name);
            CHECK(e==(live[i]?0:FS_NOT_FOUND),"stress remove");live[i]=0;model[i][0]=0;
        } else {
            int e=fs_read(home,name,&data,&n);
            CHECK(e==(live[i]?0:FS_NOT_FOUND),"stress read status");
            if(live[i]) CHECK(n==strlen(model[i])&&!strcmp(data,model[i]),"stress contents");
        }
    }
    TEST_RESULTS();
}
