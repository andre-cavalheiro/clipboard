#include "comunication.h"




/*
gcc -Wall -o socketlib.o -c socket_lib.c && gcc -Wall -pthread -o clipboard.o clipboard.c socketlib.o && mv clipboard.o cmake-build-debug/ && ./cmake-build-debug/clipboard.o
 */

//Should we create another process/thread for error handling?


int main(int argc, char** argv) {
    //Global variable inicialization
    head = iniLista();
    pthread_mutex_init ( &setup_mutex, NULL);
    pthread_mutex_init ( &list_mutex, NULL);
    for(int i=0;i<REGION_SIZE;i++){
        pthread_mutex_init ( &mutex[i], NULL);
    }

    //Clipboard setup
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
    printf("[main] socket in %s\n",delete_me);
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
void * ClipHub (void * useless){    //FIXME useless
    //Create Socket
    int sock = createSocket(AF_INET,SOCK_STREAM);
    //Generating random socket
    srand(getpid());
    int port = rand()%63714 + 1024; //generate random port between 1024 and 64738
    //Bind Local socket
    InternetServerSocket(sock, port ,5);

    printf("[Cliphub] Waiting for connections on port : %d \n", port);
    // ClipHub setup
    pthread_t clipboard_comm;
    int * clip_read = malloc(sizeof(int));
    int * clip_write = malloc(sizeof(int));
    int err;

    //Wait for remote clipboard connections
    while(1){
        //printf("[Cliphub] Ready to accept \n");
        if((*clip_read = accept(sock, NULL, NULL)) == -1) {
            perror("accept");
            exit(-1);
        }
        printf("[Cliphub] New clipboard connected \n");
        //After a remote clipboard connects, it creates 2 sockets with it:
        // One to receive updates
        if(pthread_create(&clipboard_comm, NULL, ClipHandleChild, clip_read) != 0){
            perror("[Cliphub] Creating thread");
            exit(-1);
        }
        //And one to spread updates whenever there's one
        if((*clip_write = accept(sock, NULL, NULL)) == -1) {
            perror("[Cliphub] accept");
            exit(-1);
        }
        head=criaNovoNoLista(head,clip_write,&err);
        if(err !=0){
            //FIXME handle error
            printf("Error creating new node\n");
        }

    }
    pthread_exit(NULL);
}

/**
 *
 * @param _clip
 * @return
 */
void * ClipHandleChild (void * _clip){

    int * clip_ = _clip;
    int clip = *clip_;
    void * payload = NULL;
    struct metaData info;

    printf("[Handle Child] A child contacted me for the first time, sending all the data \n");

    /*Whenever a remote clipboard connects to this one
     the current local clipboard is send out*/
    for(int i=0;i<REGION_SIZE;i++){
        info = getLocalClipboardInfo(i);
        info.action = 0;
        payload = getLocalClipboardData(i);
        if(sendDataToRemote(clip,info,payload)!=0){
            //FIXME
        }
    }
    printf("[Handle Child] Child is updated \n");


    //Wait for an update by the remote clipboard
    while(1){
        if((payload = getRemoteData(clip,&info,false)) == NULL){
            //with compare set as false, if received is NULL there's an error.
            printf("[Handle Child] Error Receiving \n");
            //FIXME
        }
        printf("[Handle Child] Received [%d] - %s \t %s \n",info.region,info.hash ,(char*)payload);
        setLocalRegion(info.region,payload,info.msg_size,info.hash);
        printf("[Handle Child] Updated Local Region %d\n",info.region);
        pthread_mutex_lock(&mutex[info.region]);
        //Send signal to spread the message
        new_data[info.region]=true;
        from_parent[info.region]=false;
        pthread_mutex_unlock(&mutex[info.region]);
        pthread_cond_signal(&cond[info.region]);

        for(int i=0; i<REGION_SIZE; i++){
            printf("[Handle Child]\t[%d]: %s \t %s\n",i,clipboard[i].hash,(char*)clipboard[i].payload);
        }

    }


    pthread_exit(NULL);
}

/**
 *
 * @param _clip
 * @return
 */
void * ClipHandleParent (void * _clip){

    int * clip_ = _clip;
    int clip = *clip_;
    void * payload = NULL;
    struct metaData info;

    //Wait for an update by the remote clipboard
    while(1){
        if((payload = getRemoteData(clip,&info,true)) == NULL){
            //with compare set as true, if received is NULL maybe we already had the data.
            printf("[ClipHandleParent] I already had this data OR error \n");
            continue;
        }
        printf("[ClipHandleParent] Received [%d] - %s \t %s \n",info.region,info.hash ,(char*)payload);
        setLocalRegion(info.region,payload,info.msg_size,info.hash);
        printf("[ClipHandleParent] Updated Local Region %d\n",info.region);
        for(int i=0; i<REGION_SIZE; i++){
            printf("\t[%d]: %s \t %s\n",i,clipboard[i].hash,(char*)clipboard[i].payload);
        }

        //Send signal to spread the message
        pthread_mutex_lock(&mutex[info.region]);
        new_data[info.region]=true;
        from_parent[info.region]=true;
        pthread_mutex_unlock(&mutex[info.region]);

        pthread_cond_signal(&cond[info.region]);

        for(int i=0; i<REGION_SIZE; i++){
            printf("[ClipHandleParent] \t[%d]: %s \t %s\n",i,clipboard[i].hash,(char*)clipboard[i].payload);
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
    bool parent = 0;

    while(1){
        pthread_mutex_lock(&mutex[region]);
        //Wait for clipboard region to receive new update
        pthread_cond_wait(&cond[region],&mutex[region]);
        //Get New Data when unlocked
        //printf("[Region watch - %d] Out of Conditional wait \n", region);
        info = getLocalClipboardInfo(region);
        info.action = 0;
        payload = getLocalClipboardData(region);
        //printf("[Region watch] [%d] - %s - %zd - %s\n",info.region,info.hash,info.msg_size,(char*)payload);
        //Cleanup for condition wait
        new_data[region] = false;
        parent = from_parent[region];   //check if info came from parent
        from_parent[region] = 0;
        pthread_mutex_unlock(&mutex[region]);
        //printf("[Region watch] Cleaned things up \n");

        //Run through clipboards list and spread the new data
        if(parent){
            printf("[Region watch] Message came from parent \n");
            list_size = numItensNaLista(head) -1;
        }else{
            printf("[Region watch] Message came from son/local \n");
            list_size = numItensNaLista(head);
        }

        pthread_mutex_lock(&list_mutex);
        t_lista * aux = head;
        t_lista * prev = NULL;
        printf("[Region watch] Starting to spread the message. List has size %d \n",list_size);
        if(list_size != 0){
            for(int i=0;i<list_size;i++){
                //printf("[Region watch] list position %d \n",i);
                client = getItemLista(aux);
                if(sendDataToRemote(*client,info,payload)!=0){
                    printf("[Region watch] Failed to spread \n");
                    aux = free_node(&prev, aux,freePayload);
                    continue;
                }
                prev = aux;
                aux = getProxElementoLista(aux);
                printf("[Region watch] [%s] sent to node %d \n",(char*)payload,i);
            }
        }
        pthread_mutex_unlock(&list_mutex);
        printf("[Region Watch] Message [%s] is spread",(char*)payload);
    }
    pthread_exit(NULL);
}


/**
 * @param parent_id
 * @return
 */
int ClipSync(int parent_id){
    struct metaData info;
    void * received;
    printf("[ClipSync] Initiating process\n");
    //Request Data
    for(int i=0;i<REGION_SIZE;i++){
        if((received = getRemoteData(parent_id,&info,false)) == NULL){
            //with compare set as false, if received is NULL there's an error.
            printf("[ClipSync] Error Receiving \n");
        }
        printf("[ClipSync] Received [%d] - %s \t %s \n",info.region,info.hash ,(char*)received);
        setLocalRegion(i,received,info.msg_size,info.hash);
        printf("[ClipSync] Updated Local Region %d\n",i);
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