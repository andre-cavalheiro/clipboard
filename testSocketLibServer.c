#include "socket_lib.h"

//Compiling everything    gcc -Wall -c -o lib.o socket_lib.c && gcc -Wall -o testServer.o testSocketLibServer.c lib.o && gcc -Wall -o testClient.o testSocketLibClient.c lib.o

int main(){
    DECLARE(int,5);

    //Create Sockets
    int sockUn = createSocket(AF_UNIX,SOCK_STREAM);
    int sockIn = createSocket(AF_INET,SOCK_STREAM);
    printf("%d %d \n",sockUn,sockIn);


    //Server
    UnixServerSocket(sockUn,SOCK_LOCAL_ADDR,5);
    InternetServerSocket(sockIn,3000,5);

    //Un connection handling
    int clientUn;
    size_t sizeOfMessage;
    //Accept connection from a single client
    if((clientUn = accept(sockUn, NULL, NULL)) == -1) {
        perror("accept");
        exit(-1);
    }
    char * data = NULL;
    while(1){
        printf("Ready to accept \n");
        sizeOfMessage = handleHandShake(clientUn);
        printf("About to read data \n");
        data = receiveData(clientUn,sizeOfMessage);
        printf("Received data: %s \n",data);

    }
    close (clientUn);

    return 0;
}