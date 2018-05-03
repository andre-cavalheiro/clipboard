#include "socket_lib.h"

int main(){
    //Creating
    int sockUnCl = createSocket(AF_UNIX,SOCK_STREAM);
    int sockInCl = createSocket(AF_INET,SOCK_STREAM);
    printf("%d %d \n",sockUnCl,sockInCl);

    //Client
    UnixClientSocket(sockUnCl,SOCK_LOCAL_ADDR);
    printf("boop\n");
    InternetClientSocket(sockInCl,"localhost",3000);
    printf("boop\n");

    //Getting information from stdin and sending it
    size_t real_size = sizeof(char)*100;
    char * str = (char*)malloc(real_size);
    while(strcmp(str,"exit\n") != 0) {
        printf("Write a message\n");
        str = fgets(str,real_size, stdin);
        printf("Initiating Handshake:\n\t%s\tsize is %zd\n",str,real_size);
        //AF_UNIX
        /*handShakeUn(sockUnCl,&real_size);
        printf("Sending data\n");
        sendData(sockUnCl,real_size,str);*/
        //AF_INET
        handShakeUn(sockInCl,&real_size);
        printf("Sending data\n");
        sendData(sockInCl,real_size,str);
    }



    return 0;
}