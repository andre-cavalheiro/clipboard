#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "socket_lib.h"

#define MAX_CLIENTS 10
#define REGION_SIZE 10

//gcc -Wall -pthread -o threads.o threads.c socketlib.o

void * handleClient(void * client_);


int main(int argc, char** argv) {
    //Create Sockets
    int sock = createSocket(AF_UNIX,SOCK_STREAM);
    //Bind socket
    UnixServerSocket(sock,SOCK_LOCAL_ADDR,5);

    // connection handling
    int numClients = 0;
    pthread_t clipboard_comm;
    pthread_t * threads = malloc(sizeof(pthread_t)*MAX_CLIENTS);
    int * clients = malloc(sizeof(int)*MAX_CLIENTS);

    //create Clipboard Regions, assuming char for now, should work as it is used for byte streams anyways

    char** clipboard = (char*) malloc(REGION_SIZE*sizeof(char*));

    //init of empty strings(will paste "" instead of seg_fault)
    for(i = 0; i < REGION_SIZE;i++){
        clipboard[i] = (char*)malloc(sizeof(char));
        clipboard[i][0] = "\0";
    }

    /*PLACEHOLDER FOR DISTRIUTED CLIPBOARD COMMUNICATION
        ->receive from function arguments if its solo(empty) or connected(ip addr)???
    if(pthread_create(clipboard_com, NULL, clipboardHub, argv[]) != 0){
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
    printf("Shall we begin? \n");           //Afinal printfs funcionam em threads
    int* client = (int*)client_;
    printf("%d \n",*client);
    char* data = malloc(sizeof(char)*100);
    //Receive data
    printf("Ready to receive \n");
    while(1){
        struct msg* info = handleHandShake(*client);

        //Client requested a copy
        if info->action == COPY_RQST{
            data = receiveData(*client,info->msg_size);
            printf("Received data: %s \n\n",data);

            region_copy(data, info->region);

            if(strcmp(data,"exit")==0){
                break;
            }
        }

        //Client requested a paste
        data = region_paste(info->region);

        //send data to client
    }
    pthread_exit(0);

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