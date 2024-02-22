#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void direcProcess(char* path,char* name){
    char buf[512],* p;
    int fd;
    struct dirent de;
    struct stat st;
    strcpy(buf,path);
    p=buf+strlen(buf);
    *p++='/';
    if((fd = open(path, 0)) < 0){
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    if(fstat(fd, &st) < 0){
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    if(st.type!=T_DIR){
        fprintf(2, "find: path is not dir %s\n", path);
    }

    while(read(fd, &de, sizeof(de)) == sizeof(de)){
        if(de.inum == 0)
            continue;
        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = 0;
        if(strcmp(de.name,name)==0){
            printf("%s\n",buf);
        }  
        if(stat(buf, &st) < 0){
                printf("find: cannot stat %s\n", buf);
                continue;
            }
        if(st.type==T_DIR&&!(strcmp(de.name,".")==0||strcmp(de.name,"..")==0)){
                direcProcess(buf,name);
            }
    }
    close(fd);
}

int main(int argc, char *argv[])
{
    char* path=".",*file_name=" ";
    if(argc<2){
        fprintf(2,"no argument");
        exit(1);
    }
    if(argc==2){
        file_name=argv[1];
    }
    else if(argc==3){
        path=argv[1];
        file_name=argv[2];
    }
    direcProcess(path,file_name);
    exit(0);



    return 0;
}
