#include "clipboard.h"
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>

int main(){

    int fd = clipboard_connect("./");
    if(fd== -1){
        exit(-1);
    }
    printf("Clipboard connected\n");
    char * str = (char*)malloc(sizeof(char)*100) ;

    while(strcmp(str,"exit\n") != 0){
        printf("Ready to receive message\n");
        str=fgets(str,100,stdin);
        clipboard_copy(fd,3,str,100);
    }

    exit(0);
}
