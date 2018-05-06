#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <unistd.h>

#define SOCK_LOCAL_ADDR "./socket_420"      //Criar name com pid()? Se sim então têm de ser em /tmp e temos de garantir unlink
#define COPY_RQST = 0
#define PASTE_RQSR = 1


struct msg{
    int action;
    int region;
    size_t msg_size;
};



int createSocket(int domain, int type);
int UnixServerSocket(int sockId, char * path, int maxQueueLength);
int InternetServerSocket(int sockId, int port,int maxQueueLength);
int UnixClientSocket(int sockId , char * path);
int InternetClientSocket(int sockId, char *ip, int port);
int handShake(int sockId, size_t * size, int action,int region);
struct msg * handleHandShake(int clientId);
int sendData(int sockId, size_t size, char * msg);
char * receiveData(int sockId,size_t size);

