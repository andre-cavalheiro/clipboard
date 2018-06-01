#include "socket_lib.h"


//To compile only the library: gcc -Wall -c -o lib.o socket_lib.c

/**
 * Creates a socket with specified domains and types
 *
 * @param domain
 * @param type
 * @return
 */
int createSocket(int domain, int type){
    //Create
    int sockId = socket(domain , type , 0);
    if (sockId == -1){
        perror("socket: ");
        return -1;
    }
    return sockId;
}


/**
 * Binds Unix socket to address with specified path
 *
 * @param path
 * @param maxQueueLength
 * @return
 */
int UnixServerSocket(int sockId, char * path, int maxQueueLength){
    //bind
    struct sockaddr_un local_addr ;
    local_addr.sun_family = AF_UNIX;
    strcpy(local_addr.sun_path, path);
    if(bind(sockId,(struct sockaddr *)&local_addr, sizeof(struct sockaddr_un)) == -1) {
        //Try to unlink before throwing error
        unlink(path);
        if(bind(sockId,(struct sockaddr *)&local_addr, sizeof(struct sockaddr_un)) == -1) {
            perror("bind Unix: ");
            return -1;
        }
    }
    //Listen
    if (listen(sockId,maxQueueLength)==-1){
        perror("listen");
        return -1;
    }
    return sockId;
}


/**
 * Binds Internet socket to all available IPs, with specified port and
 * enables connections to it
 *
 * @param port
 * @param maxQueueLength
 * @return
 */
int InternetServerSocket(int sockId, int port,int maxQueueLength){
    //Bind
    struct sockaddr_in local_addr;
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(port);
    local_addr.sin_addr.s_addr= INADDR_ANY;
    if(bind(sockId,(struct sockaddr *)&local_addr,sizeof(struct sockaddr_in))==-1){
        perror("bind internet: ");
        return -1;
    }
    //Listen
    if (listen(sockId,maxQueueLength)==-1){
        perror("listen");
        return -1;
    }
    return sockId;
}

/**
 * Connects Unix (client) socket to local (server) socket specified by path
 *
 * @param path
 * @return
 */
int UnixClientSocket(int sockId , char * path){
    //Connect
    struct sockaddr_un local_addr ;
    local_addr.sun_family = AF_UNIX;
    strcpy(local_addr.sun_path, path);
    if(connect(sockId, (const struct sockaddr *) &local_addr, sizeof(struct sockaddr_un))==-1){
        perror("connect: ");
        return -1;
    }

    return  sockId;
}


/**
 *  Connects Internet (client) socket to the (server) socket specified by ip:port
 *
 * @param ip
 * @param port
 * @return
 */
int InternetClientSocket(int sockId, char *ip, int port){
    //connect
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_aton(ip,&server_addr.sin_addr);
    if(connect(sockId, (const struct sockaddr *) &server_addr, sizeof(struct sockaddr_in))==-1){
        perror("connect: ");
        return -1;
    }
    return sockId;
}


/**
 * Informs (server) Unix socket about the the type of comunication that's about to happen and awaits confirmation
 * from it

 *
 * @param sockId
 * @param type
 * @return
 */
int handShake(int sockId, void * info, size_t size){
    if((write(sockId, info, size ))==-1){
        perror("handshake write: ");
        return -1;
    }
    int received;
    //Receive integer 1 to confirm reception
    if((read(sockId,&received,sizeof(int)))==-1){
        perror("handshake read: ");
        return -1;
    }
    if(received != 1){
        perror("handshake miscommunication \n");
        return -1;
    }
    return 0;
}


/**
 * Receives information from the (client) socket about the type of comunication that's about to happen and
 * acknowledges it
 *
 * @param clientId
 * @return
 */
void * handleHandShake(int clientId, size_t size){
    void * received = malloc(size);
    if( read(clientId,received,size)==-1){
        perror("handleHandshake read: ");
        return NULL;
    }
    //Send integer 1 to confirm reception
    int confirm = 1;
    if((write(clientId,&confirm, sizeof(int)))==-1){
        perror("handleHandshake write: ");
        free(received);
        return NULL;
    }
    return received;
}

/**
 * Sends Data
 *
 * @param sockId
 * @param size
 * @param msg
 * @return
 */
int sendData(int sockId, size_t size, void * msg){
    size_t count = size;
    size_t sentBytes = 0;
    while(count > 0){
        if((sentBytes = send(sockId,msg,size,0))==-1){
            perror("sendData write: ");
            return (size - count);
        }
        count -= sentBytes;
    }
    return size;
}

/**
 * Receives Data
 *
 * @param sockId
 * @param size
 * @return
 */
void * receiveData(int sockId,size_t size){
    size_t count = size;
    size_t readBytes = 0;
    void * str = malloc(size+1);
    while(count > 0){
        if((readBytes = recv(sockId,str+readBytes,size,0)) == -1){
            perror("sendData write: ");
            free(str);
            return NULL;
        }
        count -= readBytes;
    }
    return str;
}
