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

	size_t size = 100;
	char * command = malloc(2);
	char * clipboardString = malloc(size);
	void * bytestream = malloc(sizeof(struct metaData));
	struct metaData info;
	int region = -1;


	while(1) {
		printf("Run \tc - copy\tp - paste\ta - show all\tw - wait\te - exit\n");
		command = fgets(command,size, stdin);
        region = -1;

        //Paste
		if(strcmp(command,"p\n") == 0){
			printf("What region? \n");
			scanf("%d",&region);
            getchar();  //Ignore \n from scanf
            if(region >= 0 && region < 10){
				if(clipboard_paste(sock,region,clipboardString,size) == 0){
					printf("[%d] - %s",region,(char*)clipboardString);
				}else{
					printf("Error pasting \n");
				}
			}else{
				printf("Invalid region idiot \n");
			}
			continue;
		}
		//Copy
		else if(strcmp(command,"c\n") == 0) {
			printf("What region? \n");
			scanf("%d",&region);
			getchar();  //Ignore \n from scanf
            printf("What would you like to copy\n");
			clipboardString=fgets(clipboardString,size,stdin);
			if(region >= 0 && region < 10){
				if(clipboard_copy(sock,region,clipboardString,size) != 0){
					printf("Error copying \n");
				}
			}else{
				printf("Invalid region idiot! \n");
			}
			continue;
		}
		//Display all
		else if(strcmp(command,"a\n") == 0){
            for(region=0;region<10;region++){
                if(clipboard_paste(sock,region,clipboardString,size) == 0){
                    printf("[%d] - %s \n",region,clipboardString);
                }else{
                    printf("Error Displaying all regions \n");
                }
            }
			continue;
		}
        else if(strcmp(command,"w\n") == 0){
            //clipboard wait
            continue;
        }
        else if(strcmp(command,"e\n") == 0){
            //Clipboard_close...
            break;
        }
    }
	exit(0);
}
