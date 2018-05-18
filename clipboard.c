#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/time.h>
#include "list.h"
#include "clipboard.h"
#include "socket_lib.h"

/*
gcc -Wall -o socketlib.o -c socket_lib.c && gcc -Wall -pthread -o clipboard.o clipboard.c socketlib.o && mv clipboard.o cmake-build-debug/ && ./cmake-build-debug/clipboard.o
 */

//Should we create another process/thread for error handling?

//Functions
void * handleClient(void * );
void shutDownThread(void * ,void *);
void shutDownClipboard(int );
void * ClipHub (void * parent_);
void * ClipHandleChild (void * _clip);
void * ClipHandleParent (void * _clip);
int ClipSync(int parent_id);
char * generateHash(int size, int randFactor,int randFactor2);
void * regionWatch(void * region_);
void freePayload(void * payload);

//Global Variables
struct data clipboard[REGION_SIZE];
t_lista * head = NULL;
bool new_data[REGION_SIZE];
bool from_parent[REGION_SIZE];
pthread_mutex_t mutex[REGION_SIZE] = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t setup_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t list_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_rwlock_t rwlocks[REGION_SIZE];
pthread_cond_t cond[REGION_SIZE];


int main(int argc, char** argv) {
    head = iniLista();

    //****Clipboard setup*****
    pthread_t localHandler;
    for(int i=0;i<REGION_SIZE;i++){
        clipboard[i].payload = NULL;
        clipboard[i].size = 0; //Cannot be negative because size_t
        clipboard[i].hash[0] = '\0';
    }

    //handle command line arguments  -- should handle errors
    int opt = getopt(argc, argv, "c:");
    pthread_t clipboard_hub;
    if(opt == 'c'){
        char * ip = malloc(16);
        ip = optarg;
        int port = atoi(argv[optind]);
        printf("%s   %d\n",ip, port);

        //create socket for parent clipboard comms
        int parent_sock_write = createSocket(AF_INET,SOCK_STREAM);
        InternetClientSocket(parent_sock_write, ip, port);
        int parent_sock_read = createSocket(AF_INET,SOCK_STREAM);
        InternetClientSocket(parent_sock_read, ip, port);
        int err =0;
        int * aux = malloc(sizeof(int));
        *aux = parent_sock_write;
        head = criaNovoNoLista(head,aux,&err);
        if(err != 0){
            printf("Error creating node\n");
        }
        //Receives clipboard from parent
        ClipSync(parent_sock_write);

        //creates clipboard hub to receive/transmit data child clipboards
        if(pthread_create(&clipboard_hub, NULL, ClipHub,NULL) != 0){
            perror("Creating thread\n");
            exit(-1);
        }
        //Create Thread that handles comunication with the parent
        pthread_t parent_connection;
        if(pthread_create(&parent_connection, NULL, ClipHandleParent, &parent_sock_read) != 0){
            perror("Creating thread\n");
            exit(-1);
        }
    }
    else{
        //creates clipboard hub to receive/transmit data between clipboards
        if(pthread_create(&clipboard_hub, NULL, ClipHub, NULL) != 0){
            perror("Creating thread\n");
            exit(-1);
        }
    }
    int region[REGION_SIZE];        //FIXME later
    for(int i=0;i<REGION_SIZE;i++){
        region[i]=i;
        //CHEESY CRITICAL REGION FOR localHandler
        pthread_mutex_lock(&setup_mutex);
        pthread_create(&localHandler,NULL,regionWatch,&region[i]);
    }

    //Create Local Socket
    int sock = createSocket(AF_UNIX,SOCK_STREAM);
    //Bind Local socket
    char * delete_me = malloc(100);
    char * pid = malloc(20);    //temporary to allow multiple clipboard in same dir
    snprintf (pid, 40, "%d",getpid());
    strcpy(delete_me, SOCK_LOCAL_ADDR);
    strcat(delete_me,pid);
    printf("[main] %s\n",delete_me);
    UnixServerSocket(sock,delete_me,5);


    // Local clients setup
    int * client = malloc(sizeof(int));

    //Signal handlers
    struct sigaction shutdown;
    shutdown.sa_handler = shutDownClipboard;
    sigaction(SIGINT,&shutdown,NULL);

    while(1){
        printf("Ready to accept \n");
        if((*client = accept(sock, NULL, NULL)) == -1) {
            perror("accept");
            exit(-1);
        }
        //CHEESY CRITICAL REGION FOR localHandler
        pthread_mutex_lock(&setup_mutex);
        if(pthread_create(&localHandler, NULL, handleClient, client) != 0){
            printf("Creating thread");
            exit(-1);
        }

    }

    return 0;
}


/**
 *
 * @param client_
 * @return
 */
void * handleClient(void * client__){
    int * client_ = client__;
    int client = *client_;
    pthread_mutex_unlock(&setup_mutex);

    void * data_bytestream = NULL;
    void * bytestream_cpy = NULL;
    void * bytestream_pst = malloc(sizeof(struct metaData));
    char * hash;
    struct metaData info;

    while(1){
        printf("[Local] Ready to receive \n");
        bytestream_cpy = handleHandShake(client, sizeof(struct metaData));
        memcpy(&info,bytestream_cpy,sizeof(struct metaData));
        switch (info.action){
            case 0:
                //client wants to send data to server (Copy)
           		data_bytestream = receiveData(client,info.msg_size);
		        hash = generateHash(HASH_SIZE,getpid(),pthread_self());

                printf("[Local] Client wants to copy region %d with size %zd\n",info.region,info.msg_size);

                //***************CRITICAL REGION****************
                pthread_mutex_lock(&mutex[info.region]);
		        pthread_rwlock_wrlock(&rwlocks[info.region]);

                if(clipboard[info.region].size != 0){
                    free(clipboard[info.region].payload);
                }
                clipboard[info.region].size = info.msg_size;
                clipboard[info.region].payload = malloc(info.msg_size);
                clipboard[info.region].payload = data_bytestream;
                memcpy(clipboard[info.region].hash,hash, HASH_SIZE);
                pthread_rwlock_unlock(&rwlocks[info.region]);

                new_data[info.region] = true;
                pthread_cond_signal(&cond[info.region]);
		        pthread_mutex_unlock(&mutex[info.region]);
		       //***************CRITICAL REGION****************

                printf("[Local] Final clipboard state: \n");
                for(int i=0; i<REGION_SIZE; i++){
                    printf("\t[%d]: %s \t %s\n",i,clipboard[i].hash,(char*)clipboard[i].payload);
                }
                break;
            case 1:
                //client is requesting data from server (Paste)
                printf("[Local] Client wants to paste region %d\n",info.region);

                //***************CRITICAL REGION****************
                pthread_rwlock_rdlock(&rwlocks[info.region]);
                info.msg_size = clipboard[info.region].size;
                //Informar cliente do tamanho da mensagem
                memcpy(bytestream_pst,&info,sizeof(struct metaData));
                handShake(client,bytestream_pst,sizeof(struct metaData));

                //this sucks, but other option is to malloc which is shit too!!
                sendData(client,info.msg_size,clipboard[info.region].payload);

                pthread_rwlock_unlock(&rwlocks[info.region]);
                //***************CRITICAL REGION****************

                printf("Paste completed\n");
                break;
            case 2:
                //client is logging out
                shutDownThread(bytestream_cpy,bytestream_pst);
                pthread_exit(NULL);
                break;
            /*case 3:
            	//client is requesting wait
            	printf("[Local] Client wants to wait for a new addition to region %d\n",info.region);
            	/***************CRITICAL REGION****************
 				pthread_mutex_lock(&mutex[info.region]);

 				while(!new_data[info.region]){  //FIXME
 					pthread_cond_wait(&cond[info.region],&mutex[info.region]);
 				}

		        pthread_rwlock_rdlock(&rwlocks[info.region]);
		        info.msg_size = clipboard[info.region].size;
                //Informar cliente do tamanho da mensagem
                memcpy(bytestream_pst,&info,sizeof(struct metaData));
                handShake(client,bytestream_pst,sizeof(struct metaData));

                //this sucks, but other option is to malloc which is shit too!!
                sendData(client,info.msg_size,clipboard[info.region].payload);
		        pthread_rwlock_unlock(&rwlocks[info.region]);
		        pthread_mutex_unlock(&mutex[info.region]);
		        /***************CRITICAL REGION****************
		        break;
 				}*/
            default:
                //Error:
                //FIXME handle this case in client side.
                shutDownThread(bytestream_cpy,bytestream_pst);
                pthread_exit(NULL);
                break;
        }
    }
}

/**
 *
 * @param parent_
 * @return
 */
void * ClipHub (void * useless){
    srand(getpid());
    int port = rand()%63714 + 1024; //generate random port between 1024 and 64738
    //Create Socket
    int sock = createSocket(AF_INET,SOCK_STREAM);
    //Bind Local socket
    InternetServerSocket(sock, port ,5);  //FIXME INADDR_LOOPBACK

    printf("[Cliphub] Created socket in port %d \n", port);
    // ClipHub setup
    pthread_t clipboard_comm;
    int * clip_read = malloc(sizeof(int));
    int * clip_write = malloc(sizeof(int));
    int err;

    while(1){
        printf("[Cliphub] Ready to accept \n");
        if((*clip_read = accept(sock, NULL, NULL)) == -1) {
            perror("accept");
            exit(-1);
        }
        printf("[Cliphub] First receive \n");
        if(pthread_create(&clipboard_comm, NULL, ClipHandleChild, clip_read) != 0){
            perror("[Cliphub] Creating thread");
            exit(-1);
        }

        printf("[Cliphub] Second receive \n");
        if((*clip_write = accept(sock, NULL, NULL)) == -1) {
            perror("[Cliphub] accept");
            exit(-1);
        }
        printf("[Cliphub] Adding child to list \n");
        head=criaNovoNoLista(head,clip_write,&err);
        if(err !=0){
            //FIXME handle error
            printf("Error creating new node\n");
        }

    }
    pthread_exit(NULL);
}


/**	OUTSIDERS MAY NOT HAVE THE MOST RECENT DATA THEREFORE IT IS NECESSARY TO CROSS CHECK THE VALIDITY OF THE DATA WE RECEIVE
 *
 * clip -> socket id
 *
 */
void * ClipHandleChild (void * _clip){

    int * clip_ = _clip;
    int clip = *clip_;
    void * bytestream_cpy = NULL;
    void * bytestream_pst = NULL;
    void * payload = NULL;
    struct metaData info;

    printf("[Handle Child] A child contacted me for the first time, sending all the data \n");
    for(int i=0;i<REGION_SIZE;i++){
        bytestream_pst = handleHandShake(clip,sizeof(struct metaData));
        memcpy(&info,bytestream_pst,sizeof(struct metaData));

        //***************CRITICAL REGION****************
        pthread_rwlock_rdlock(&rwlocks[info.region]);
        info.msg_size = clipboard[info.region].size;
        strncpy(info.hash,clipboard[info.region].hash,HASH_SIZE);
        payload = clipboard[info.region].payload;
        pthread_rwlock_unlock(&rwlocks[info.region]);
        //***************CRITICAL REGION****************

        memcpy(bytestream_pst,&info,sizeof(struct metaData));
        handShake(clip,bytestream_pst,sizeof(struct metaData));

        sendData(clip,info.msg_size , payload);
    }
    printf("Child is updated \n");


    while(1){
        bytestream_pst = handleHandShake(clip,sizeof(struct metaData));
        printf("[Handle Child] A child contacted me \n");
        memcpy(&info,bytestream_pst,sizeof(struct metaData));
        info.action = 1;
        memcpy(bytestream_pst,&info,sizeof(struct metaData));
        handShake(clip,bytestream_pst,sizeof(struct metaData));
        printf("[Handle Child] About to Receive new data \n");
        bytestream_cpy = receiveData(clip,info.msg_size);

        //***************CRITICAL REGION**************** - SAVE TO AUX STRING TO SHRINK REGION
        pthread_mutex_lock(&mutex[info.region]);
        pthread_rwlock_wrlock(&rwlocks[info.region]);
        printf("[Handle Parent] Beggining substitution\n ");
        if(clipboard[info.region].size != 0){
            free(clipboard[info.region].payload);
        }
        clipboard[info.region].size = info.msg_size;
        clipboard[info.region].payload = bytestream_cpy;
        strncpy(clipboard[info.region].hash,info.hash,HASH_SIZE);

        new_data[info.region]=true;
        pthread_cond_signal(&cond[info.region]);


        pthread_rwlock_unlock(&rwlocks[info.region]);
        pthread_mutex_unlock(&mutex[info.region]);
        //*************CRITICAL REGION****************

        printf("[Handle Child] Clipboard updated \n");
    }
    pthread_exit(NULL);
}   //FIXME WITH STUFF FROM HANDLEPARENT

/**
 *
 * @param _clip
 * @return
 */
void * ClipHandleParent (void * _clip){

    int * clip_ = _clip;
    int clip = *clip_;
    void * bytestream_cpy = malloc(sizeof(struct metaData));
    void * bytestream_pst = NULL;
    void * data;
    struct metaData info;


    while(1){
        if((bytestream_pst = handleHandShake(clip,sizeof(struct metaData)))== NULL){
            printf("[Handle Parent] handle handshake failed \n");
            exit(0);
        }
        printf("[Handle Parent] My father contacted me \n");
        memcpy(&info,bytestream_pst,sizeof(struct metaData));

        pthread_rwlock_rdlock(&rwlocks[info.region]);
        if(strcmp(clipboard[info.region].hash,info.hash)==0){
            printf("[Handle Parent] I already have this [%d] mine:%s \t new:%s \n",info.region,clipboard[info.region].hash,info.hash);
            info.action = 4;
            memcpy(bytestream_cpy,&info,sizeof(struct metaData));
            handShake(clip,bytestream_cpy,sizeof(struct metaData));
            continue;
        }
        info.action = 1;
        memcpy(bytestream_cpy,&info,sizeof(struct metaData));
        if(handShake(clip,bytestream_cpy,sizeof(struct metaData)) != 0){
            printf("[Handle Parent] handshake failed \n");
            exit(0);
        }
        printf("[Handle Parent] New data incoming\n");
        pthread_rwlock_unlock(&rwlocks[info.region]);
        if((data = receiveData(clip,info.msg_size))==NULL){
            printf("[Handle Parent] The entire data WAS NOT received \n");
        }

        //***************CRITICAL REGION**************** - SAVE TO AUX STRING TO SHRINK REGION
        pthread_mutex_lock(&mutex[info.region]);
        pthread_rwlock_wrlock(&rwlocks[info.region]);
        printf("[Handle Parent] Beginning substitution\n ");
        if(clipboard[info.region].size != 0){
            free(clipboard[info.region].payload);
        }
        clipboard[info.region].size = info.msg_size;
        memcpy(clipboard[info.region].payload,data,info.msg_size);
        strncpy(clipboard[info.region].hash,info.hash,HASH_SIZE);

        new_data[info.region]=true;
        from_parent[info.region]=true;
        pthread_cond_signal(&cond[info.region]);


        pthread_rwlock_unlock(&rwlocks[info.region]);
        pthread_mutex_unlock(&mutex[info.region]);
        printf("[Handle Parent] Clipboard updated\n");

        //*************CRITICAL REGION****************
        free(data);
        for(int i=0; i<REGION_SIZE; i++){
            printf("\t[%d]: %s \t %s\n",i,clipboard[i].hash,(char*)clipboard[i].payload);
        }

    }
    pthread_exit(NULL);
}



/**
 *
 * @param region_
 * @return
 */
void * regionWatch(void * region_){
    int * region__ = region_;
    int region = *region__;
    pthread_mutex_unlock(&setup_mutex);

    struct metaData info;
    int list_size=0;
    int * client = NULL;
    void * payload = NULL;
    void * bytestream = malloc(sizeof(struct metaData));
    bool parent = 0;
    printf("[Region watch] Watching for %d \n",region);

    info.region = region;

    while(1){
        //************** CRITICAL REGION ***************
        pthread_mutex_lock(&mutex[region]);
        //while(!new_data[region]){
            pthread_cond_wait(&cond[region],&mutex[region]);
        //}
        printf("[Region watch - %d] Out of Conditional wait \n",region);
        info.action = 0;
        pthread_rwlock_rdlock(&rwlocks[region]);

        strncpy(info.hash,clipboard[info.region].hash,HASH_SIZE);
        info.msg_size = clipboard[region].size;
        payload = clipboard[region].payload;
        memcpy(bytestream,&info,sizeof(struct metaData));

        pthread_rwlock_unlock(&rwlocks[region]);
        printf("[Region watch] Got info from clipboard \n");
        new_data[region] = false;
        parent = from_parent[region];   //check if info came from parent
        from_parent[region] = 0;
        pthread_mutex_unlock(&mutex[region]);
        printf("[Region watch] Cleaned things up \n");
        //************** CRITICAL REGION **********************

        if(parent){
            printf("[Region watch] Message came from parent \n");
            list_size = numItensNaLista(head) -1;
        }else{
            printf("[Region watch] Message came from son/local \n");
            list_size = numItensNaLista(head);
        }

        //******** SHITTY LIST * CRITICAL REGION ****************
        pthread_mutex_lock(&list_mutex);
        t_lista * aux = head;
        t_lista * prev = NULL;
        printf("[Region watch] Starting to spread the message. List has size %d \n",list_size);
        if(list_size != 0){
            for(int i=0;i<list_size;i++){
                printf("[Region watch] list position %d \n",i);
                client = getItemLista(aux);
                if(handShake(*client,bytestream,sizeof(struct metaData)) == -1){
                    //this socket is dead, removing the corpse(node)...
                    printf("[Region watch] dead node\n");
                    aux = free_node(&prev, aux,freePayload);
                    continue;
                }
                printf("[Region watch] Handshake went good \n");
                if((bytestream = handleHandShake(*client,sizeof(struct metaData)))==NULL){
                    printf("handleHandShake failed! \n");
                    exit(0);
                }
                printf("[Region watch] Handle handshake went good \n ");
                memcpy(&info,bytestream,sizeof(struct metaData));
                if(info.action != 4){
                    printf("[Region watch] Sending data \n ");
                    if((sendData(*client,info.msg_size,payload)) != info.msg_size){
                        printf("[Region watch] The entire message WASN'T send\n");
                    }
                }
                prev = aux;
                aux = getProxElementoLista(aux);
            }
            printf("[Region watch] out of loop\n");
        }
        //*********** CRITICAL REGION ****************
        printf("[Region watch] Done \n");
        pthread_mutex_unlock(&list_mutex);
    }
    pthread_exit(NULL);
}


/**
 *
 * @param parent_id
 * @return
 */
int ClipSync(int parent_id){
    char * bytestream = malloc(sizeof(struct metaData));
    struct metaData info;
    void * received;
    printf("[ClipSync] Initiating process\n");
    //Request Data
    for(int i = 0; i < REGION_SIZE ; i++){
        info.region=i;
        info.action=1;
        info.msg_size=0;
        memcpy(bytestream,&info,sizeof(struct metaData));

        if(handShake(parent_id,bytestream,sizeof(struct metaData)) != 0){
            free(bytestream);
            exit(0);
        }

        if((bytestream = handleHandShake(parent_id,sizeof(struct metaData))) == NULL){
            free(bytestream);
            exit(0);
        }
        memcpy(&info,bytestream,sizeof(struct metaData));

        //Get data
        if((received = receiveData(parent_id,info.msg_size)) == NULL){
            printf("Error receiving \n");
            exit(0);
        }
        printf("[ClipSync] Received [%d] - %s \t %s \n",info.region,info.hash ,(char*)received);

   		//***** CRITICAL REGION *******
        pthread_rwlock_wrlock(&rwlocks[info.region]);
        clipboard[info.region].size = info.msg_size;
        strncpy(info.hash,clipboard[info.region].hash,HASH_SIZE);
        clipboard[info.region].payload = received;
        pthread_rwlock_unlock(&rwlocks[info.region]);
        //***** CRITICAL REGION *******
    }
    printf("[ClipSync] Done \n");

    return(0);
}

/**
 *
 * @param sock
 */
void shutDownClipboard(int sig){
    //Free clipboard
    for (int i=0;i<REGION_SIZE;i++){
        //Free clipboard
        if(clipboard[i].payload != NULL){
            free(clipboard[i].payload);
        }

    }
    //unlink(SOCK_LOCAL_ADDR);
}


/**
 *
 * @param sock
 * @param bytestream_cpy
 * @param bytestream_pst
 */
void shutDownThread(void *bytestream_cpy,void * bytestream_pst ){
    free(bytestream_cpy);
    if(bytestream_cpy != NULL){
        free(bytestream_cpy);
    }
}


/**
 *
 * @param size
 * @param randFactor
 * @param randFactor2
 * @return
 */
char* generateHash(int size, int randFactor,int randFactor2){  //pid, pthread, numo nanoseconds since ...
    //62 elementos, tirei o w,W para fazer 60.
    char charset[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWX";

    char * hash = malloc(size);

    //Generate table
    srand(randFactor+randFactor2);
    /*char hashTable[15][4] = {
            {"0fuK"},
            {"1gvL"},
            {"2hxM"},
            {"3iyN"},
            {"4jzO"},
            {"5kAP"},
            {"6lBQ"},
            {"7mCR"},
            {"8nDS"},
            {"9oET"},
            {"apFU"},
            {"bqGV"},
            {"crHX"},
            {"dsIY"},
            {"etJZ"},
    };*/
    char hashTable[15][4];
    for(int i = 0;i<15;i++){
        for(int j=0;j<4;j++){
            hashTable[i][j]=charset[rand()%60];
        }
    }

    //Generate Hash
    struct timeval tv;
    int msec = -1;
    if (gettimeofday(&tv, NULL) == 0) {
        msec = ((tv.tv_sec % 86400) * 1000 + tv.tv_usec / 1000);    //Get milliseconds since midnight
    }else{
        printf("Error\n");
    }

    srand(msec);
    int randomLine = rand()%15;
    int randomColumn = rand()%4;


    for(int i=0; i< size-1; i++){
        hash[i] = hashTable[randomLine][randomColumn];

        randomLine = rand()%15;
        randomColumn = rand()%4;
    }
    hash[size-1]='\0';
    return hash;
}

/**
 *
 * @param payload
 */
void freePayload(void * payload){
    free(payload);
}