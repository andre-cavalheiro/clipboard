#include "socket_lib.h"

int main(){
    //Creating
    int sockUnCl = createSocket(AF_UNIX,SOCK_STREAM);
    int sockInCl = createSocket(AF_INET,SOCK_STREAM);
    printf("%d %d \n",sockUnCl,sockInCl);

    //Client
    UnixClientSocket(sockUnCl,SOCK_LOCAL_ADDR);
    printf("boop\n");
    /*InternetClientSocket(sockInCl,"localhost",3000);
    printf("boop\n");*/

    //Getting information from stdin and sending it
    size_t size = sizeof(char)*100;
    char * str = (char*)malloc(size);

    struct metaData info;
    info.action = 0;
    info.region = 0;
    info.msg_size = size;

    char * bytestream = malloc(sizeof(struct metaData));


    while(strcmp(str,"exit\n") != 0) {
        memcpy(bytestream,&info,sizeof(struct metaData));
        printf("Write a message\n");
        str = fgets(str,size, stdin);

        printf("Initiating Handshake\n");
        handShake(sockUnCl,bytestream,sizeof(struct metaData));
        printf("Sending data\n");
        sendData(sockUnCl,size,str);
        info.region ++;

    }
    printf("Clipboard state: \n");
    char * data = malloc(MAX_STR_SIZE);;
    for(info.action = 1,info.region = 0; info.region < 10; info.region++){
        memcpy(bytestream,&info,sizeof(struct metaData));
        handShake(sockUnCl,bytestream,sizeof(struct metaData));
        data = receiveData(sockUnCl,MAX_STR_SIZE);
        printf("\t[%d] - %s \n",info.action,data);
    }


    return 0;
}