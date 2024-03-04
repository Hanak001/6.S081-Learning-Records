#include "kernel/types.h"
#include "user/user.h"
#include "kernel/param.h"


 int main(int argc, char *argv[])
 {
   char buf;
   char *xargv[MAXARG];
   for(int i=1;i<argc;i++){
      xargv[i-1]=argv[i];
   }
   while(read(0,&buf,1)>0){
      char myargv[MAXARG];
      memset(myargv,0,sizeof(myargv));
      char *i=myargv;
      *i++=buf;
      while(read(0,&buf,1)>0&&(buf!='\n')){
         *i++=buf;
      }
      xargv[argc-1]=myargv;
      if(fork()==0){
         // printf("xargv: %s",myargv);
         exec(argv[1],xargv);
         exit(0);
      }
      else{
         wait(0);
      }
      
   }

   
   exit(0);
   return 0;
 }
 