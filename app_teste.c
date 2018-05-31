#include "clipboard.h"



int main(int argc, char ** argv){
    char * command = malloc(2);    //Action to be done in this program
    char * clipboardString = NULL;  //String that will be used to sent/receive data to/from the clipboard
    void * bytestream = malloc(sizeof(struct metaData));    //Pointer to be used in handshakes
    int region = -1, verify = 0;
    size_t maxsize = 4095;  //stin buffer max size
    //Helper variables to receive dynamic input
    FILE* stringFile = NULL;
    char ch,enable;
    size_t size = 0;


    //Connect
	int sock = clipboard_connect("./");
    //int sock = clipboard_connect("./cmake-build-debug/");
    if(sock == -1){
		exit(-1);
	}

    //Receive input and make requests to clipboard
	while(1) {
		printf("\nRun \tc - copy\tp - paste\ta - show all\tw - wait\te - exit\n");
		//Receive command
        command = fgets(command,4,stdin);
        region = -1;

        //Paste
		if(strcmp(command,"p\n") == 0){
			printf("What region?\t");
			scanf("%d",&region);
            getchar();
            clipboardString = malloc(maxsize);
            if((verify=clipboard_paste(sock,region,clipboardString,maxsize)) > 0){
                printf("[%d](%d) - %s \n",region,verify,clipboardString);
            }else{
                printf("Error while pasting\n");
            }
			continue;   //FIXME am i supposed to be here?
		}
		//Copy
		else if(strcmp(command,"c\n") == 0) {
			printf("What region? \t");
			scanf("%d",&region);
			getchar();
            printf("What would you like to copy\t");

            //Get endless string from stdin (max is 4095 bytes)
            stdin = freopen(NULL,"r",stdin);
            fgets(&enable,0,stdin);
            stringFile=fopen("/tmp/stringFile","w+");
            size=0;
            while(1){
                ch = fgetc(stdin);
                if( ch =='\n')
                    break ;
                size++;
                fputc(ch,stringFile);
            }
            clipboardString = malloc(size+1);
            fseek(stringFile, 0, SEEK_SET );
            fgets(clipboardString,size+1,stringFile);
            fclose(stringFile);

            printf("=== size [%zd] ===\n",size);
            printf("%s\n======",clipboardString);
            //copy string
            if(clipboard_copy(sock,region,clipboardString,strlen(clipboardString)) == 0){
                printf("Error copying \n");
            }
			continue;
		}
		//Display all
		else if(strcmp(command,"a\n") == 0){
            clipboardString = malloc(maxsize);
            for(region=0;region<REGION_SIZE;region++){
                if((verify=clipboard_paste(sock,region,clipboardString,maxsize)) > 0){
                    printf("[%d](%d) - %s\n",region,verify,clipboardString);
                }else{
                    printf("Error while pasting\n");
                }
            }
			continue;
        }
        //Wait
        else if(strcmp(command,"w\n") == 0){
            printf("What region?\t");
            scanf("%d",&region);
            getchar();
            clipboardString = malloc(maxsize);
            if((verify=clipboard_wait(sock,region,clipboardString,maxsize)) > 0){
                printf("[%d](%d) - %s \n",region,verify,clipboardString);
            }else{
                printf("Error while pasting\n");
            }

            continue;
        }
        //Exit
        else if(strcmp(command,"e\n") == 0){
            printf("I'm out \n");
            clipboard_close(sock);
            break;
        }


        if(clipboardString != NULL){
            free(clipboardString);
            clipboardString = NULL;
        }
    }
    free(bytestream);
    exit(0);
}
