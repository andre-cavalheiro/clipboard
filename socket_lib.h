#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <unistd.h>


int createSocket(int domain, int type);
int UnixServerSocket(int sockId, char * path, int maxQueueLength);
int InternetServerSocket(int sockId, int port,int maxQueueLength);
int UnixClientSocket(int sockId , char * path);
int InternetClientSocket(int sockId, char *ip, int port);
int handShake(int sockId, void * info, size_t size);
void * handleHandShake(int clientId, size_t size);
int sendData(int sockId, size_t size, void * msg);
void * receiveData(int sockId,size_t size);

