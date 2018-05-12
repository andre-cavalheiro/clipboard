#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "clipboard.h"
#include "socket_lib.h"


/*
gcc -Wall -o socketlib.o -c socket_lib.c && gcc -Wall -pthread -o clipboard.o clipboard.c socketlib.o && mv clipboard.o cmake-build-debug/ && ./cmake-build-debug/clipboard.o
 */

//Functions
void * handleClient(void * );
void shutDownThread(void * ,void *);
void shutDownClipboard(int );

//Global Variables
struct node clipboard[REGION_SIZE];


int main(int argc, char** argv) {
    //handle command line arguments  -- should handle errors
    int connected_mode;
    int opt = getopt(argc, argv, "c:");
    if(opt == 'c'){
        connected_mode = 1;         // Variavel que indica se é suposto comunicar com outros clipboards ou se é só para correr localmente (tá no enunciado que é suposto dar das 2 maneiras)
        char * ip = malloc(16);
        ip = optarg;
        int port = atoi(argv[optind]);
        printf("%s   %d\n",ip, port);
    }else{
        connected_mode = 0;
    }


    //Create Local Socket
    int sock = createSocket(AF_UNIX,SOCK_STREAM);
    //Bind Local socket
    UnixServerSocket(sock,SOCK_LOCAL_ADDR,5);

    // Local clients setup
    pthread_t clipboard_comm;
    pthread_t localHandler;
    int client;


    //Clipboard setup
    for(int i;i<REGION_SIZE;i++){
        clipboard[i].payload = NULL;
        clipboard[i].size = 0; //Cannot be negative because size_t
    }

    //Signal handlers
    struct sigaction shutdown;
    shutdown.sa_handler = shutDownClipboard;
    sigaction(SIGINT,&shutdown,NULL);

    /*PLACEHOLDER FOR DISTRIUTED CLIPBOARD COMMUNICATION
    ->receive from function arguments if its solo(empty) or connected(ip addr)???*/
    /*if(pthread_create(clipboard_com, NULL, clipboardHub, argv[]) != 0){
        printf("Creating thread");
        exit(-1);
    }
    */

    while(1){
        printf("Ready to accept \n");
        if((client = accept(sock, NULL, NULL)) == -1) {
            perror("accept");
            exit(-1);
        }
        if(pthread_create(&localHandler, NULL, handleClient, &client) != 0){
            printf("Creating thread");
            exit(-1);
        }
    }

    return 0;
}


/**
 *
 * @param client_
 * @return
 */
void * handleClient(void * client_){
    int * client = client_;
    void * bytestream_cpy = NULL;
    void * bytestream_pst = malloc(sizeof(struct metaData));
    struct metaData info;

    while(1){
        printf("[Thread] Ready to receive \n");
        bytestream_cpy = handleHandShake(*client, sizeof(struct metaData));
        memcpy(&info,bytestream_cpy,sizeof(struct metaData));
        switch (info.action){
            case 0:
                //client wants to send data to server (Copy)
                printf("[Thread] Client wants to copy region %d with size %zd\n",info.region,info.msg_size);
                if(clipboard[info.region].size != 0){
                    free(clipboard[info.region].payload);
                }
                clipboard[info.region].size = info.msg_size;
                clipboard[info.region].payload = malloc(info.msg_size);
                clipboard[info.region].payload = receiveData(*client,info.msg_size);
                printf("[Thread] Copy completed: %s \n",(char*)clipboard[info.region].payload);
                break;
            case 1:
                //client is requesting data from server (Paste)
                printf("[Thread] Client wants to paste region %d\n",info.region);
                info.msg_size = clipboard[info.region].size;
                //Informar cliente do tamanho da mensagem
                memcpy(bytestream_pst,&info,sizeof(struct metaData));
                handShake(*client,bytestream_pst,sizeof(struct metaData));
                //Enviar mensagem
                sendData(*client,clipboard[info.region].size,clipboard[info.region].payload);
                printf("Paste completed\n");
                break;
            case 2:
                //client is logging out
                shutDownThread(bytestream_cpy,bytestream_pst);
                pthread_exit(NULL);
                break;
            default:
                //Error:
                //FIXME handle this case in client side.
                shutDownThread(bytestream_cpy,bytestream_pst);
                pthread_exit(NULL);
                break;
        }
        printf("[Thread] Final clipboard state: \n");
        for(int i=0; i<REGION_SIZE; i++){
            printf("\t[%d]: %s \n",i,(char*)clipboard[i].payload);
        }

    }
}

/* Main thread for the distributed clipboard system
all threads must be able to access the clipboard directly (global??)
void* clipboardHub(void* ip_addr){


    //CONNECTED MODE
    if ip_addr != NULL{ ??

        socket to master for connection request

    }

    // await connection from other clipboards
    while(1){

        same as for local communication
    }
}
*/

/**
 *
 * @param sock
 */
void shutDownClipboard(int sig){
    //Free clipboard
    for (int i=0;i<REGION_SIZE;i++){
        //Free clipboard
        if(clipboard[i].payload != NULL){
            free(clipboard[i].payload);
        }

    }
    unlink(SOCK_LOCAL_ADDR);
}


/**
 *
 * @param sock
 * @param bytestream_cpy
 * @param bytestream_pst
 */
void shutDownThread(void *bytestream_cpy,void * bytestream_pst ){
    free(bytestream_cpy);
    if(bytestream_cpy != NULL){
        free(bytestream_cpy);
    }
}