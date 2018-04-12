#include "clipboard.h"
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
int main(){

		int fd = clipboard_connect("./");
		
		if(fd== -1){
			exit(-1);
		}
		char dados[10];
		int dados_int;
		fgets(dados, 10, stdin);
		write(fd, dados, 10);
		read(fd+1, &dados_int, sizeof(dados_int));
		printf("Received %d\n", dados_int);
		
		exit(0);
	}
