//
// Created by andre on 5/21/18.
//

#include "clipboard.h"

#define CLIENTS 3


int main(int argc, char ** argv) {
    if (argc < CLIENTS + 1) {
        printf("Missing socket name\n");
        exit(1);
    }
    char *command = malloc(2);    //Action to be done in this program
    char *clipboardString = NULL;  //String that will be used to sent/receive data to/from the clipboard
    void *bytestream = malloc(sizeof(struct metaData));    //Pointer to be used in handshakes
    int region = -1, verify = 0;
    size_t maxsize = 4095;  //stin buffer max size
    //Helper variables to receive dynamic input
    FILE *stringFile = NULL;
    char ch, enable;
    size_t size = 0;

    char **socket = malloc(sizeof(char*)*CLIENTS);
    char *dir = malloc(5);    //FIXME Create socket name (i dont think we'll keep this in the final version, but let's keep it until the very last)
    dir = "/tmp/";

    for(int i=0;i<CLIENTS;i++){
        socket[i]=malloc(100 + strlen(argv[i]));
        strcpy(socket[i], dir);
        strcat(socket[i], argv[i + 1]);
        printf("%s\n", socket[i]);
    }

    //Connect
    int *sock = malloc(sizeof(int)*CLIENTS);
    for(int i=0;i<CLIENTS;i++){
        sock[i] = clipboard_connect(socket[i]);

        if(sock[i] == -1){
            exit(-1);
        }
    }
    char c[CLIENTS];
    char **string = malloc(CLIENTS*sizeof(char*));
    char **output = malloc(CLIENTS*sizeof(char*));
    FILE** f = malloc(sizeof(FILE*)*CLIENTS);
    FILE** outputFile = malloc(sizeof(FILE *)*CLIENTS);
    int *numChar=malloc(sizeof(int)*CLIENTS);
    char ** nameOfFile = malloc(CLIENTS*sizeof(char*));

    for(int i=0;i<CLIENTS;i++){
        nameOfFile[i] = malloc(100);
        sprintf(nameOfFile[i],"message%d",i);
        f[i]=fopen(nameOfFile[i],"r");

        numChar[i] = 0;
        while(getchar() != EOF){
            ++numChar[i];
        }
        string[i] = malloc(numChar[i]);
        output[i] = malloc(numChar[i]);
        string[i] = fgets(string[i] ,numChar[i],f[i]);
        printf("%s\n\n ",string[i]);
    }

    for(int i=0;i<CLIENTS;i++){
        clipboard_copy(sock[i],0,string[i],numChar[i]);
        printf("copy %d\n",i);
    }
    for(int i=0;i<CLIENTS;i++){
        sprintf(nameOfFile[i],"-out%d",i);
        f[i]=fopen(nameOfFile[i],"r");
        outputFile[i]=fopen(nameOfFile[i],"w");
        clipboard_paste(sock[i],0,output[i],numChar[i]);
        fprintf(outputFile[i],output[i]);
        printf("Output %d",i);
    }
    for(int i=0;i<CLIENTS;i++){
        fclose(outputFile[i]);
        fclose(f[i]);

    }
}
