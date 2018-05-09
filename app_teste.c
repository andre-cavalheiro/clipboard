#include "clipboard.h"
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


/*
gcc -Wall -o lib.o -c library.c  && gcc -o app_test.o app_teste.c socketlib.o library.c && mv app_test.o cmake-build-debug/ && ./cmake-build-debug/app_test.o

 */

int main(){

	int sock = clipboard_connect("./");
	if(sock == -1){
		exit(-1);
	}

	size_t size = sizeof(char)*100;
	char * str = malloc(size);
	char * input = malloc(size);
	char * data = malloc(size);
	void * bytestream = malloc(sizeof(struct metaData));
	struct metaData info;
	int region = -1;


	while(strcmp(str,"exit\n") != 0) {
		printf("Run \tc - copy\tp - paste\ta - show all\n");
		str = fgets(str,size, stdin);
		//Paste
		if(strcmp(str,"p\n") == 0){
			region = -1;
			printf("What region? \n");
			scanf("%d",&region);
            getchar();  //Ignore \n from scanf
            if(region > 0 && region < 10){
				if(clipboard_paste(sock,region,str,size) == 0){
					printf("[%d] - %s",region,(char*)bytestream);
				}else{
					printf("Error pasting \n");
				}
			}else{
				printf("Invalid region idiot \n");
			}
			continue;
		}
		//Copy
		else if(strcmp(str,"c\n") == 0) {
			region = -1;
			printf("What region? \n");
			scanf("%d",&region);
			getchar();  //Ignore \n from scanf
            printf("What would you like to copy\n");
			input=fgets(input,size,stdin);
			if(region > 0 && region < 10){
				if(clipboard_copy(sock,region,input,size) != 0){
					printf("Error copying \n");
				}
			}else{
				printf("Invalid region idiot! \n");
			}
			continue;
		}
		//Display all
		else if(strcmp(str,"a\n") == 0){

			continue;
		}

	}
	exit(0);
}
