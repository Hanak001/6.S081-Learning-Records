#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    if(argc<=1){
        fprintf(2,"sleep: user forgets to pass an argument");
        exit(1);
    }
    if(argc>2){
        fprintf(2,"sleep: too much argument");
        exit(1);
    }
    if(argc==2){
        int tik=atoi(argv[1]);
        int ret=sleep(tik);
        if(ret!=0){
            fprintf(2,"sleep: return -1 error");
            exit(1);
        }

    }
    exit(0);
}




