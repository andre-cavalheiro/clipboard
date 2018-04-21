#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <unistd.h>

#define SOCK_LOCAL_ADDR "./socket_420"  //Criar name com pid()? Se sim então têm de ser em /tmp e temos de garantir unlink

/*
#include <sys/types.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
 */



int createSocket(int domain, int type);
int UnixServerSocket(int sockId, char * path, int maxQueueLength);
int InternetServerSocket(int sockId, int port,int maxQueueLength);
int UnixClientSocket(int sockId , char * path);
int InternetClientSocket(int sockId, char *ip, int port);
int handShakeUn(int sockId, size_t * size);
size_t handleHandShake(int clientId);
int sendData(int sockId, size_t size, char * msg);
char * receiveData(int sockId,size_t size);

