#include<stdio.h>
#include<unistd.h>

int main(){
    long pid = 0;
    long num = 500;
    
    pid = syscall(222,num);
    printf("num:%ld\n",pid);

    return 0;
}
