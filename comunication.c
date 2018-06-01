#include "header.h"


/**
 *
 * @param region
 * @return
 */

void * getLocalClipboardData(int region){
    void * payload = NULL;

    pthread_rwlock_rdlock(&rwlocks[region]);
    payload = xmalloc(clipboard[region].size+10);
    memcpy(payload,clipboard[region].payload,clipboard[region].size+5);
    pthread_rwlock_unlock(&rwlocks[region]);

    return payload;
}

/**
 *
 * @param region
 * @return
 */
struct metaData getLocalClipboardInfo(int region){
    struct metaData info;
    info.region = region;

    pthread_rwlock_rdlock(&rwlocks[region]);
    strncpy(info.hash,clipboard[region].hash,HASH_SIZE);
    info.msg_size = clipboard[region].size;
    pthread_rwlock_unlock(&rwlocks[region]);

    return info;
}

/**
 *
 * @param region
 * @param payload
 * @param hash
 * @param size
 */
struct data * setLocalRegion(int region, void * payload,size_t size,char*hash){
    struct data * data = xmalloc(sizeof(struct data));
    if(hash == NULL){
        hash = generateHash(HASH_SIZE);
    }

    pthread_rwlock_wrlock(&rwlocks[region]);
    clipboard[region].size = size;
    strncpy(clipboard[region].hash,hash,HASH_SIZE);
    clipboard[region].payload = payload;
    pthread_rwlock_unlock(&rwlocks[region]);

    data->payload = xmalloc(size);
    memcpy(data->payload,payload,size);
    data->size = size;
    strncpy(data->hash,hash,HASH_SIZE);
    //When leaving this function, the from_parent is still to be defined
    return data;
}

/**
 *
 * @param parent_id
 * @param info
 * @return
 */
void * getRemoteData(int sock,struct metaData *info,bool compare,int* error,int* logout){
    void * bytestream;
    void * received;
    struct metaData localInfo;

    if((bytestream = handleHandShake(sock,sizeof(struct metaData))) == NULL){
        free(bytestream);
        *error = 1;
        return NULL;
    }
    memcpy(info,bytestream,sizeof(struct metaData));
    if(info->action == 4){
        *logout = 1;
        return NULL;
    }
    localInfo = getLocalClipboardInfo(info->region);

    //Compare hashes if it's supposed to
    if(compare && (strcmp(localInfo.hash, info->hash) == 0)) {
            info->action = 4;
            memcpy(bytestream, info, sizeof(struct metaData));
            if (handShake(sock, bytestream, sizeof(struct metaData)) != 0) {
                free(bytestream);
                *error = 1;
                return NULL;
            }
            return NULL;    //Not error
    }

    info->action = 1;
    memcpy(bytestream,info,sizeof(struct metaData));
    if(handShake(sock,bytestream,sizeof(struct metaData)) != 0){
        free(bytestream);
        *error = 1;
        return NULL;    //error
    }


    //Get data
    if((received = receiveData(sock,info->msg_size)) == NULL){
        *error = 1;
        return NULL;
    }
    free(bytestream);
    return received;

}


/**
 *
 *
 * @param client
 * @param info
 * @param payload
 * @return
 */
int sendDataToRemote(int client,struct metaData info, void* payload){
    void * bytestream = xmalloc(sizeof(struct metaData));
    memcpy(bytestream,&info,sizeof(struct metaData));

    if(handShake(client,bytestream,sizeof(struct metaData)) == -1){
        //this socket is dead, removing the corpse(node)...
        printf("[sendRemoteData] DEAD NODE\n");
        free(bytestream);
        return -1;
    }
    //printf("[sendRemoteData] Handshake went good \n");
    if((bytestream = handleHandShake(client,sizeof(struct metaData)))==NULL){
        printf("[senDataToRemote] handleHandShake FAILED! \n");
        exit(0);
    }
    //printf("[sendRemoteData] Handle handshake went good \n");
    memcpy(&info,bytestream,sizeof(struct metaData));
    //If remote informs me that he does not have the message send it
    if(info.action != 4){
        //printf("[sendRemoteData] Sending data \n");
        if((sendData(client,info.msg_size,payload)) != info.msg_size){
            printf("[senDataToRemote]] THE ENTIRE MESSAGE WAS NOT SEND\n");
        }
        //printf("[sendRemoteData] Done sending\n");
    }else{
        printf("[senDataToRemote] Client refused information [%s]\n",info.hash);
    }
    free(bytestream);
    return 0;
}



/**
 *  Function to end the clipboard
 *  It releases the used resources and informs other clipboards.
 *  This function is used as a handler for the SIGINT signal
 *
 * @param sock
 */
void shutDownClipboard(int sig) {
    int list_size;
    int * client;
    t_lista * aux = head;
    struct metaData info;
    void * bytestream = xmalloc(sizeof(struct metaData));

    info.action = 4;
    memcpy(bytestream,&info, sizeof(struct metaData));

    //Free clipboard and print final output
    for (int i = 0; i < REGION_SIZE; i++) {
        //Lock region just in case
        pthread_mutex_lock(&mutex[i]);
        pthread_rwlock_wrlock(&rwlocks[i]);
        //Free clipboard
        if (clipboard[i].payload != NULL) {
            free(clipboard[i].payload);
        }
    }

    //Unlink local socket
    unlink(SOCK_LOCAL_ADDR);

    //Inform other clipboards of my death and free memory
    pthread_mutex_lock(&list_mutex);
    list_size = numItensNaLista(head);
    for (int i = 0; i < list_size; i++) {
        client = getItemLista(aux);
        handShake(*client,bytestream,sizeof(struct metaData));
        aux = getProxElementoLista(aux);
    }
    free(bytestream);
    libertaLista(head,freePayload);
}

/**
 *  Generate a random hash which identifies each piece of data
 *
 * @param size
 * @param randFactor
 * @param randFactor2
 * @return
 */
char* generateHash(int size){
    char charset[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWX"; //60 characters
    char hashTable[15][4];
    char * hash = xmalloc(size+1);
    int randomLine,randomColumn;

    //Generate random table
    for(int i = 0;i<15;i++){
        for(int j=0;j<4;j++){
            hashTable[i][j]=charset[rand()%60];
        }
    }

    //Generate random Hash
    randomLine = rand()%15;
    randomColumn = rand()%4;

    for(int i=0; i< size-1; i++){
        hash[i] = hashTable[randomLine][randomColumn];

        randomLine = rand()%15;
        randomColumn = rand()%4;
    }
    hash[size-1]='\0';
    return hash;
}


/**
 * Free list payload which is an int pointer to a socket file descriptor
 * @param payload
 */
void freePayload(void * payload){
    free(payload);
}


/**
 * Print current clipboard
 */
void printClipboard(){
    for(int i=0; i<REGION_SIZE; i++){
        printf("   [%d]-[%s] \t %s [%zd bytes]\n",i,clipboard[i].hash,(char*)clipboard[i].payload,clipboard[i].size);
    }
}


/**
 *
 * @param size
 * @return
 */
void * xmalloc(size_t size){
    void * ptr = malloc(size);
    if(ptr==NULL){
        perror("Error alocating memory");
        exit(-1);
    }
    return ptr;
}

