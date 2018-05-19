#include "comunication.h"

/**
 *
 * @param region
 * @return
 */

void * getLocalClipboardData(int region){
    void * payload = NULL;

    pthread_rwlock_rdlock(&rwlocks[region]);
    payload = malloc(clipboard[region].size);
    memcpy(payload,clipboard[region].payload,clipboard[region].size);
    pthread_rwlock_unlock(&rwlocks[region]);

    return payload;
}

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
void setLocalRegion(int region, void * payload,size_t size,char*hash){
    if(hash == NULL){
        hash = generateHash(HASH_SIZE,getpid(),pthread_self());
    }

    pthread_rwlock_wrlock(&rwlocks[region]);
    clipboard[region].size = size;
    strncpy(clipboard[region].hash,hash,HASH_SIZE);
    clipboard[region].payload = payload;
    pthread_rwlock_unlock(&rwlocks[region]);
}

/**
 * NULL means not updated, not error
 *
 * @param parent_id
 * @param info
 * @return
 */
void * getRemoteData(int sock,struct metaData *info,bool compare){
    void * bytestream;
    void * received;

    if((bytestream = handleHandShake(sock,sizeof(struct metaData))) == NULL){
        free(bytestream);
        return NULL;
    }
    memcpy(info,bytestream,sizeof(struct metaData));

    struct metaData localInfo = getLocalClipboardInfo(info->region);

    //Compare hashes if it's supposed to
    if(compare && (strcmp(localInfo.hash, info->hash) == 0)) {
            info->action = 4;
            memcpy(bytestream, info, sizeof(struct metaData));
            if (handShake(sock, bytestream, sizeof(struct metaData)) != 0) {
                free(bytestream);
                return NULL;    //error
            }
            return NULL;    //Not error
    }

    info->action = 1;
    memcpy(bytestream,info,sizeof(struct metaData));
    if(handShake(sock,bytestream,sizeof(struct metaData)) != 0){
        free(bytestream);
        return NULL;    //error
    }


    //Get data
    if((received = receiveData(sock,info->msg_size)) == NULL){
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
    void * bytestream = malloc(sizeof(struct metaData));
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
        printf("[senDataToRemote] Client refused information [%s]\n",(char*)payload);
    }
    free(bytestream);
    return 0;
}




