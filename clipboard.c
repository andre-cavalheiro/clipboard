#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "clipboard.h"
#include "socket_lib.h"

#include <unistd.h>

#define MAX_CLIENTS 10


/*
gcc -Wall -o socketlib.o -c socket_lib.c && gcc -Wall -pthread -o clipboard.o clipboard.c socketlib.o && mv clipboard.o cmake-build-debug/ && ./cmake-build-debug/clipboard.o
 */

//Functions
void * handleClient(void * client_);


//Global Variables
int numClients = 0;
struct node clipboard[REGION_SIZE];


int main(int argc, char** argv) {
    //handle command line arguments  -- should handle errors
    /*
    char * ip = malloc(16);
    int port;
    getopt(argc, argv, "c:");
    ip=optarg;
    port = atoi(argv[optind]);
    printf("%s   %d\n",ip, port);
    */

    //Create Local Socket
    int sock = createSocket(AF_UNIX,SOCK_STREAM);
    //Bind Local socket
    UnixServerSocket(sock,SOCK_LOCAL_ADDR,5);

    // Local clients setup
    pthread_t clipboard_comm;
    pthread_t * threads = malloc(sizeof(pthread_t)*MAX_CLIENTS);
    int * clients = malloc(sizeof(int)*MAX_CLIENTS);


    //Clipboard setup
    for(int i;i<REGION_SIZE;i++){
        clipboard[i].payload = NULL;
        clipboard[i].size = 0; //Cannot be negative because size_t
    }

    /*PLACEHOLDER FOR DISTRIUTED CLIPBOARD COMMUNICATION
    ->receive from function arguments if its solo(empty) or connected(ip addr)???*/
    /*if(pthread_create(clipboard_com, NULL, clipboardHub, argv[]) != 0){
        printf("Creating thread");
        exit(-1);
    }
    */

    while(1){
        printf("Ready to accept \n");
        if((clients[numClients] = accept(sock, NULL, NULL)) == -1) {         //Handle when numClients >= MAX_CLIENTS
            perror("accept");
            exit(-1);
        }
        if(pthread_create(&threads[numClients], NULL, handleClient, &clients[numClients]) != 0){
            printf("Creating thread");
            exit(-1);
        }
        numClients++;
    }

    return 0;
}


void * handleClient(void * client_){
    int * client = client_;
    void * bytestream = malloc(sizeof(struct metaData));
    struct metaData info;
    while(1){
        printf("[Thread] Ready to receive \n");
        bytestream = handleHandShake(*client, sizeof(struct metaData));
        memcpy(&info,bytestream,sizeof(struct metaData));
        switch (info.action){
            case 0:
                //client wants to send data to server (Copy)
                printf("[Thread] Client wants to copy region %d with size %zd\n",info.region,info.msg_size);
                if(clipboard[info.region].size != 0){
                    free(clipboard[info.region].payload);
                }
                clipboard[info.region].size = info.msg_size;
                clipboard[info.region].payload = malloc(info.msg_size);
                clipboard[info.region].payload = receiveData(*client,info.msg_size);       //Isto vai partir para quando as mensagens tiverem constantemente a mudar de tamanho
                printf("[Thread] Copy completed: %s \n",(char*)clipboard[info.region].payload);
                break;
            case 1:
                //client is requesting data from server (Paste)
                printf("[Thread] Client wants to paste region %d\n",info.region);
                info.msg_size = clipboard[info.region].size;
                //Informar cliente do tamanho da mensagem
                memcpy(bytestream,&info,sizeof(struct metaData));
                handShake(*client,bytestream,sizeof(struct metaData));
                //Enviar mensagem
                sendData(*client,clipboard[info.region].size,clipboard[info.region].payload);
                printf("Paste completed\n");
                break;
            default:
                //We should clean things up
                pthread_exit(NULL);
                break;
        }
        printf("[Thread] Final clipboard state: \n");
        for(int i=0; i<REGION_SIZE; i++){
            printf("\t[%d]: %s \n",i,(char*)clipboard[i].payload);
        }

    }
    pthread_exit(NULL);
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
