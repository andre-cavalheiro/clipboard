#include "socket_lib.h"

//Compile: gcc -Wall -o socketlib.o -c socket_lib.c && gcc -Wall -pthread -o clipboard.o clipboard.c socketlib.o

//test using testClient.c : 


int main(){
    //Creating
    int sockUnCl = createSocket(AF_UNIX,SOCK_STREAM);
    int sockInCl = createSocket(AF_INET,SOCK_STREAM);
    printf("%d %d \n",sockUnCl,sockInCl);

    //Client
    UnixClientSocket(sockUnCl,SOCK_LOCAL_ADDR);
    printf("boop\n");


    //Getting information from stdin and sending it
    size_t real_size = sizeof(char)*100;
    char * str = (char*)malloc(real_size);
    while(strcmp(str,"exit\n") != 0) {
        printf("Write a message\n");
        str = fgets(str,real_size, stdin);
        printf("Initiating Handshake:\n\t%s\tsize is %zd\n",str,real_size);

        //AF_UNIX
        handShake(sockUnCl,&real_size,1,1);
        printf("Sending data\n");
        sendData(sockUnCl,real_size,str);
    }

    return 0;
}