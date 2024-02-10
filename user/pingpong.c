#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

char* buf[1];
int main(int argc, char *argv[])
{
    if(argc>1){
        fprintf(2,"pingpong: to much argument\n");
        exit(1);
    }
    if(argc==1){
        int p[2];
        if(pipe(p)<0){
            fprintf(2,"pingpong: create pipe error\n");
            exit(1);
        }
        int pid=fork();
        if(pid>0){
            if(write(p[1]," ",1)!=1){
                fprintf(2,"pingpong: father write pipe fail\n");
                exit(1);
            }
            close(p[1]);
            wait(0);
            if(read(p[0],buf,1)!=1){
                fprintf(2,"pingpong: father read pipe fail\n");
                exit(1);
            }
            close(p[0]);
            fprintf(2,"%d: received pong\n",getpid());
            // printf("siezofbuf:%d",sizeof(buf));
            exit(0);

        }
        else if(pid==0){
            if(read(p[0],buf,1)!=1){
                fprintf(2,"pingpong: child read pipe fail\n");
                exit(1);
            }
            close(p[0]);
            fprintf(2,"%d: received ping\n",getpid());
            // printf("siezofbuf:%d",sizeof(buf));
            if(write(p[1]," ",1)!=1){
                fprintf(2,"pingpong: child write pipe fail\n");
                exit(1);
            }
            close(p[1]);
            exit(0);
        }

    }
    return 0;
}
