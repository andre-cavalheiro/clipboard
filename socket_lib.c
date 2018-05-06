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
    int sockId= socket(domain , type , 0);
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
    if(bind(sockId,(struct sockaddr *)&local_addr, sizeof(struct sockaddr)) == -1) {
        //Try to unlink before throwing error
        unlink(path);
        if(bind(sockId,(struct sockaddr *)&local_addr, sizeof(struct sockaddr)) == -1) {
            perror("bind");
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
    if(bind(sockId,(struct sockaddr *)&local_addr,sizeof(struct sockaddr))==-1){
        perror("bind: ");
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
 * Informs (server) socket about the size of data that it's about to be sent and awaits confirmation
 * from it
 *
 * @param sockId
 * @param size
 * @return
 */
//size deixar de ser ponteiro
int handShake(int sockId, size_t * size, int action,int region){        //action: 1-> escreve   0->lê
    char * info_toStr = (char*)malloc(sizeof(char)* sizeof(struct msg));
    struct msg * info = malloc(sizeof(struct msg));
    info->msg_size = *size;
    info->region = region;
    info->action = action;

    if(memcpy(info_toStr, info, sizeof(struct msg) ) == NULL){
        perror("Mem copy");
        return -1;
    };

    printf("\tsize: %zd \n", *size);
    if((write(sockId, info_toStr , sizeof(struct msg) ))==-1){
        perror("handshake write: ");
        return -1;
    }

    size_t received;
    if((read(sockId,&received,sizeof(size_t)))==-1){
        perror("handshake read: ");
        return -1;
    }
    printf("\tReceived size: %zd \n", received);
    if(*size != received){
        printf("handshake miscommunication \n");
        return -1;
    }
    printf("\tSuccessful \n");
    return 0;
}


/**
 * Receives information from the (client) socket about the amount of data that it's about to be sent and
 * acknowledges it
 *
 * @param clientId
 * @return
 */
struct msg * handleHandShake(int clientId){
    char * received = malloc(sizeof(struct msg));
    if((read(clientId,&received,sizeof(size_t)))==-1){
        perror("handleHandshake read: ");
        //return -1;
    }
    struct msg * info = malloc(sizeof(struct msg));
    memcpy(info,received,sizeof(struct msg));
    size_t size = info->msg_size;
    printf("boop 3\n");
    if((write(clientId,&size, sizeof(size_t)))==-1){
        perror("handleHandshake write: ");
        //return -1;
    }
    printf("\tSuccessful handshake \n");
    return info;
}

/**
 * Sends string
 *
 * @param sockId
 * @param size
 * @param msg
 * @return
 */
int sendData(int sockId, size_t size, char * msg){
    size_t count = size;
    size_t sentBytes = 0;
    while(count > 0){
        if((sentBytes = send(sockId,msg,size,0))==-1){
            perror("sendData write: ");
            return -1;
        }
        printf("\t Wrote %zd bytes \n",sentBytes);
        count -= sentBytes;
    }
    return 0;
}

/**
 * Receives string
 *
 * @param sockId
 * @param size
 * @return
 */
char * receiveData(int sockId,size_t size){
    size_t count = size;
    size_t readBytes = 0;
    char * str = (char *)malloc(size+1);
    printf("\t [%d] Entering read loop \n",sockId);
    while(count > 0){
        if((readBytes = recv(sockId,str+readBytes,size,0)) == -1){
            perror("sendData write: ");
            exit(-1);
        }
        printf("\t [%d] Read %zd bytes --> %s \n",sockId,readBytes, str);
        count -= readBytes;
    }
    return str;
}




/***
    >Teoria de Sockets:


    -[accept] da parte do servidor reage a um [connect] da parte do client, devolvendo um socket especifico
    para a ligação com esse cliente em particular. A ligação entre o servidor e ESSE cliente, só acabará quando
    for feito o [close] do socket devolvido pelo [accept].

    -[read/write] vs [recv/send] : O read/write sao mais genericos e funcionam para qualquer file descriptor
    o recv/send sao mais especificos para sockets e permitem usar flags que podem ser uteis.


    >Duvidas:

    -Se eu enviar muitas coisas com um client e depois fizer read com esse mesmo cliente antes do servidor
    ler o que está num socket ele lê aquilo que mandou??
 */