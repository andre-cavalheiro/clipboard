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
	char * command = malloc(2);                      //Action to be done in this program
	char * clipboardString = malloc(size);              //String that will be used to sent/receive data to/from the clipboard
	void * bytestream = malloc(sizeof(struct metaData));    //Pointer to be used in handshakes
	struct metaData info;
	int region = -1;
    int verify = 0;


	while(1) {
		printf("\nRun \tc - copy\tp - paste\ta - show all\tw - wait\te - exit\n");
		command = fgets(command,size, stdin);
        region = -1;

        //Paste
		if(strcmp(command,"p\n") == 0){
			printf("What region?\t");
			scanf("%d",&region);
            getchar();          //Ignore \n from scanf
            if((verify=clipboard_paste(sock,region,clipboardString,size)) > 0){
                printf("[%d] - %s",region,clipboardString);
            }else if(verify == -1){
                printf("[%d] - (empty slot)\n",region);
            }else{
                printf("Error while pasting\n");
            }
			continue;
		}
		//Copy
		else if(strcmp(command,"c\n") == 0) {
			printf("What region? \t");
			scanf("%d",&region);
			getchar();  //Ignore \n from scanf
            printf("What would you like to copy\t");
			clipboardString=fgets(clipboardString,size,stdin);
            if(clipboard_copy(sock,region,clipboardString,size) == 0){
                printf("Error copying \n");
            }
			continue;
		}
		//Display all
		else if(strcmp(command,"a\n") == 0){
            for(region=0;region<10;region++){
                if((verify=clipboard_paste(sock,region,clipboardString,size)) > 0){
                    printf("[%d] - %s",region,clipboardString);
                }else if(verify == -1){
                    printf("[%d] - (empty slot)\n",region);
                }else{
                    printf("Error while pasting\n");
                }
            }
			continue;
		}
        else if(strcmp(command,"w\n") == 0){
            //clipboard wait
            continue;
        }
        else if(strcmp(command,"e\n") == 0){
            //Close connection
            clipboard_close(sock);
            break;
        }
    }
	exit(0);
}
