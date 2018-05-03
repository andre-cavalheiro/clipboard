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
    int client;
    int numClients = 0;
    pthread_t * threads = malloc(sizeof(pthread_t)*MAX_CLIENTS);

    while(1){
        printf("Ready to accept \n");
        if((client = accept(sock, NULL, NULL)) == -1) {         //Handle when numClients >= MAX_CLIENTS
            perror("accept");
            exit(-1);
        }
        if(pthread_create(&threads[numClients], NULL, handleClient, &client) != 0){
            printf("Creating thread");
            exit(-1);
        }

    }

    return 0;
}


void * handleClient(void * client_){
    int * client = client_;
    char * data = NULL;
    size_t sizeOfMessage;
    FILE *f = fopen("threadOutput.log", "w");
    if(f == NULL){
       perror("File");
    }
    while(1){
        fprintf(f,"Ready to receive \n");
        sizeOfMessage = handleHandShake(*client);
        fprintf(f,"About to read data \n");
        data = receiveData(*client,sizeOfMessage);
        fprintf(f,"Received data: %s \n\n",data);
        if(strcmp(data,"exit")==0){
            break;
        }
    }
    fclose(f);
    pthread_exit(0);

}