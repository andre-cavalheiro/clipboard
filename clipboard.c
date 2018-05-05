#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "socket_lib.h"

#define MAX_CLIENTS 10

//gcc -Wall -pthread -o threads.o threads.c socketlib.o

void * handleClient(void * client_);


int main(int argc, char** argv) {
    //Create Sockets
    int sock = createSocket(AF_UNIX,SOCK_STREAM);
    //Bind socket
    UnixServerSocket(sock,SOCK_LOCAL_ADDR,5);

    // connection handling
    int numClients = 0;
    pthread_t * threads = malloc(sizeof(pthread_t)*MAX_CLIENTS);
    int * clients = malloc(sizeof(int)*MAX_CLIENTS);



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
    int * client = (int*)client_;
    printf("%d \n",*client);
    char * data = malloc(sizeof(char)*100);
    //Receive data
    while(1){
        printf("Ready to receive \n");
        struct msg * info = handleHandShake(*client);
        printf("About to read data \n");
        data = receiveData(*client,info->msg_size);
        printf("Received data: %s \n\n",data);
        if(strcmp(data,"exit")==0){
            break;
        }
    }
    pthread_exit(0);

}