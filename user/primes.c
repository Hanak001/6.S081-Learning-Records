#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int num[40];
void process(int p[]){
    close(p[1]);
    int pi[2],prime;
    pipe(pi);
    if(read(p[0],&prime,4)==4){
        printf("prime %d\n",prime);
        int pid=fork();
        if(pid>0){
            close(pi[0]);
            int number[40];
            int i=0;
            while(read(p[0],&number[i],4)==4){
                if(number[i]%prime!=0){
                    write(pi[1],&number[i],4);
                }
                i++;
            }
            close(pi[1]);
            close(p[0]);
            wait(0);
            exit(0);
        }
        else if(pid==0){
            process(pi);
            exit(0);
        }
    }
    

}

int main(int argc, char *argv[])
{
    int p[2];
    pipe(p);
    int pid=fork();
    if(pid>0){
        close(p[0]);
        for(int i=2;i<36;i++){
            num[i]=i;
            if(write(p[1],&num[i],4)!=4){
                fprintf(2,"write error\n");
            }
        }
        close(p[1]);
        wait(0);
        exit(0);
    }else{
        process(p);
    }
    return 0;
}
