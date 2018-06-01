#include "header.h"


/**
 * This function is responsible for dealing with the local clients.
 *
 * @param client_
 * @return
 */
void * handleLocalClient(void * client__){
    int * client_ = client__;
    int client = *client_;
    pthread_mutex_unlock(&setup_mutex);

    void * payload = NULL;
    void * bytestream = NULL;
    struct metaData info;
    struct data * data = NULL;
    int error = 0;

    while(1){

        //Wait for local client to make request
        bytestream = handleHandShake(client, sizeof(struct metaData));
        if(bytestream == NULL){
            pthread_exit(NULL);
        }
        memcpy(&info,bytestream,sizeof(struct metaData));
        //Handle request accordingly
        switch (info.action){
            case 0:
                //client wants to send data to server (Copy)
                if((payload = receiveData(client,info.msg_size))==NULL){
                    pthread_exit(NULL);
                }

                //***************CRITICAL REGION****************
                pthread_mutex_lock(&mutex[info.region]);
                data=setLocalRegion(info.region,payload,info.msg_size,NULL);
                //Signal RegionWatch to spread new data
                pthread_mutex_unlock(&mutex[info.region]);
                //***************CRITICAL REGION****************

                //***************CRITICAL REGION****************
                pthread_mutex_lock(&waitingList_mutex[info.region]);
                waitingLists[info.region] = criaNovoNoLista(waitingLists[info.region],data,&error);
                pthread_mutex_unlock(&waitingList_mutex[info.region]);
                //***************CRITICAL REGION****************
                data->from_parent = 0;

                if(error==1){
                    perror("\t[Local] Error adding new info to list\n");
                    pthread_exit(NULL);
                }
                pthread_cond_broadcast(&cond[info.region]);

                break;
            case 1:
                //client is requesting data from server (Paste)


                //***************CRITICAL REGION****************
                pthread_rwlock_rdlock(&rwlocks[info.region]);

                info = getLocalClipboardInfo(info.region);
                payload = getLocalClipboardData(info.region);

                pthread_rwlock_unlock(&rwlocks[info.region]);
                //***************CRITICAL REGION****************

                memcpy(bytestream,&info, sizeof(struct metaData));
                if(handShake(client,bytestream,sizeof(struct metaData)) != 0){
                    pthread_exit(NULL);
                }
                if(sendData(client,info.msg_size,payload) != info.msg_size){
                    pthread_exit(NULL);
                }

                break;
            case 2:
                //client is logging out
                if(bytestream != NULL){
                    free(bytestream);
                }
                pthread_exit(NULL);
                break;
            case 3:
                    //client is requesting wait
                    /***************CRITICAL REGION****************/
                    pthread_mutex_lock(&mutex[info.region]);

                    pthread_cond_wait(&cond[info.region],&mutex[info.region]);

                    pthread_rwlock_rdlock(&rwlocks[info.region]);

                    info = getLocalClipboardInfo(info.region);
                    payload = getLocalClipboardData(info.region);

                    pthread_rwlock_unlock(&rwlocks[info.region]);

                    pthread_mutex_unlock(&mutex[info.region]);
                    /***************CRITICAL REGION****************/
                    memcpy(bytestream,&info, sizeof(struct metaData));
                    if(handShake(client,bytestream,sizeof(struct metaData)) != 0){
                        pthread_exit(NULL);
                    }
                    if(sendData(client,info.msg_size,payload) != info.msg_size){
                        pthread_exit(NULL);
                    }
                    break;
            default:
                //Error:
                if(bytestream != NULL){
                    free(bytestream);
                }
                pthread_exit(NULL);
                break;
        }
        if(bytestream != NULL){
            free(bytestream);
        }
    }
}

/**
 * This function waits for connections to the internet socket and launches threads to deal with them
 *
 * @param parent_
 * @return
 */
void * ClipHub (void * useless){
    int sock, port, err, id =1;
    pthread_t clipboard_comm;
    int clip_read,clip_write;
    struct node * listNode = NULL;
    struct argument * sonArg = xmalloc(sizeof(struct argument));
    
    //Generating random port
    srand(getpid());
    port = rand()%63714 + 1024; //generate random port between 1024 and 64738
    //Create Socket
    sock = createSocket(AF_INET,SOCK_STREAM);
    //Bind Local socket
    InternetServerSocket(sock, port ,5);
    printf("Waiting for connections on port : %d \n", port);

    
    sonArg->isParent=false;

    //Wait for remote clipboard connections
    while(1){
        if((clip_read = accept(sock, NULL, NULL)) == -1) {
            perror("[Cliphub] First accept");
            exit(-1);
        }
        sonArg->id = id;
        sonArg->sock = clip_read;
        //After a remote clipboard connects, it creates 2 sockets with it:
        // One to receive updates
        pthread_mutex_lock(&passingArgumentsToHandleClip);
        if(pthread_create(&clipboard_comm, NULL, handleClipboard,sonArg ) != 0){
            perror("[Cliphub] Creating thread");
            exit(-1);
        }
        //And one to spread updates whenever there's one
        if((clip_write = accept(sock, NULL, NULL)) == -1) {
            perror("[Cliphub] Second accept");
            exit(-1);
        }
        listNode = xmalloc(sizeof(struct node));
        listNode->sock = clip_write;
        listNode->id = id++;
        pthread_mutex_lock(&list_mutex);
        head = criaNovoNoLista(head,listNode,&err);
        pthread_mutex_unlock(&list_mutex);
        if(err !=0){
            perror("\t[ClipHub] Error creating new node, exiting...\n");
            exit(-1);
        }

    }
    pthread_exit(NULL);
}


/**
 * This function is responsible for receiving information from other clipboards and handling
 * the data accordingly: Either save it to the local clipboard or refusing it.
 *
 * @param arg
 * @return
 */
void * handleClipboard(void * arg){
    struct argument * arg_ = arg;
    struct argument remoteClipboard;
    remoteClipboard.sock = arg_->sock;
    remoteClipboard.isParent = arg_->isParent;
    remoteClipboard.id = arg_->id;
    pthread_mutex_unlock(&passingArgumentsToHandleClip);

    void * payload = NULL;
    struct metaData info;
    struct data * data = NULL;
    int error=0, logout = 0,i;
    t_lista * aux=NULL;
    struct node * node;

    /*Whenever a remote clipboard connects to this one,
     the current local clipboard is send out*/
    if(!remoteClipboard.isParent){
        for(i=0;i<REGION_SIZE;i++){
            info = getLocalClipboardInfo(i);
            info.action = 0;
            payload = getLocalClipboardData(i);
            if(sendDataToRemote(remoteClipboard.sock,info,payload)!= 0){
                pthread_mutex_lock(&list_mutex);
                aux = head;
                while(aux != NULL){
                    node = getItemLista(aux);
                    if(remoteClipboard.id == node->id){
                        printf("\t[HandleClipboard] Removing [%d] from connected clipboards \n",remoteClipboard.id);
                        head = free_node(head, aux,freePayload);
                        break;
                    }
                    aux = getProxElementoLista(aux);
                }
                pthread_mutex_unlock(&list_mutex);            }
        }
    }


    //Wait for an update by the remote clipboard
    while(1){
        //Receive information about the new data and verify if it's actually new
        payload = getRemoteData(remoteClipboard.sock, &info, remoteClipboard.isParent,&error,&logout);
        if(logout == 1 || error == 1){
            //Handle clipboard disconnection
            printf("\tRemote Clipboard is logging out\n" );
            aux = head;
            pthread_mutex_lock(&list_mutex);
            while(aux != NULL){
                    node = getItemLista(aux);
                    if(remoteClipboard.id == node->id){
                        printf("Removing [%d] from connected clipboards \n",remoteClipboard.id);
                        head = free_node(head, aux,freePayload);
                        break;
                    }
                    aux = getProxElementoLista(aux);
            }
            pthread_mutex_unlock(&list_mutex);
            pthread_exit(NULL);

        }
        if(payload != NULL){
            //Update Local Clipboard
            data=setLocalRegion(info.region,payload,info.msg_size,info.hash);

            strncpy(data->hash,info.hash,HASH_SIZE);
            data->from_parent = remoteClipboard.isParent;
            pthread_mutex_lock(&waitingList_mutex[info.region]);
            waitingLists[info.region] = criaNovoNoLista(waitingLists[info.region],data,&error);
            pthread_mutex_unlock(&waitingList_mutex[info.region]);
            if(error==1){
                perror("\t[HandleClipboard] Error adding new info to list, exiting....\n");
                exit(-1);
            }

            //***************CRITICAL REGION****************
            pthread_mutex_lock(&mutex[info.region]);
            from_parent[info.region]=remoteClipboard.isParent;
            //Send signal to spread new data
            pthread_cond_broadcast(&cond[info.region]);
            pthread_mutex_unlock(&mutex[info.region]);
            //***************CRITICAL REGION****************

        }
    }
}




/**
 * This function waits for changes to a particular region of the local clipboard
 * and spreads the information to other clipboards
 *
 * @param region_
 * @return
 */
void * regionWatch(void * region_){
    int * region__ = region_;
    int region = *region__;
    pthread_mutex_unlock(&setup_mutex);

    t_lista * aux = NULL;
    struct data * data = NULL;

    struct spread * messageToSpread = xmalloc(sizeof(struct spread));
    struct metaData info;
    void * payload = NULL;
    pthread_t hermes;
    bool parent = 0;

    messageToSpread->region=region;
    messageToSpread->parent=parent;
    info.region = region;

    while(1){
        pthread_mutex_lock(&mutex[region]);

        //Wait for clipboard region to receive new update
        while(getItemLista(waitingLists[region]) == NULL){
            pthread_cond_wait(&cond[region],&mutex[region]);
        }
        pthread_mutex_lock(&waitingList_mutex[region]);
        aux = getLastNode(waitingLists[region]);
        data = getItemLista(aux);
        pthread_mutex_unlock(&waitingList_mutex[region]);


        //Get New Data when unlocked
        info.action = 0;
        info.msg_size = data->size;
        strncpy(info.hash,data->hash,HASH_SIZE);

        payload = xmalloc(data->size);
        memcpy(payload,data->payload,data->size);

        //Cleanup for condition wait
        pthread_mutex_lock(&waitingList_mutex[region]);
        waitingLists[region] = free_node(waitingLists[region],aux,freeWaitingListNode);
        pthread_mutex_unlock(&waitingList_mutex[info.region]);

        pthread_mutex_unlock(&mutex[region]);

        messageToSpread->payload = xmalloc(info.msg_size);
        memcpy(messageToSpread->payload,payload,info.msg_size);
        messageToSpread->info.msg_size = info.msg_size;
        strncpy(messageToSpread->info.hash,info.hash,HASH_SIZE);
        messageToSpread->info.action=-1;
        messageToSpread->info.region= info.region;

        //Launch thread to spread new information
        pthread_mutex_lock(&passingArgumentsToSpread);

        if( pthread_create(&hermes, NULL, spreadTheWord, messageToSpread) != 0){
            perror("[Region watch ] Error Creating thread ");
            exit(-1);
        }
        free(payload);
    }
    pthread_exit(NULL);
}

/**
 *
 * @return
 */
void * spreadTheWord(void *arg){
    struct spread * argument;
    int i;
    argument = arg;
    struct spread messageToSpread;
    messageToSpread.region = argument->region;
    messageToSpread.info = argument->info;
    messageToSpread.payload = argument->payload;
    messageToSpread.parent = argument->parent;
    pthread_mutex_unlock(&passingArgumentsToSpread);

    int list_size;
    t_lista * aux=NULL,*aux2=NULL;
    struct node *node = NULL;
    //Run through clipboards list and spread the new data
    if(messageToSpread.parent){
        list_size = numItensNaLista(head) - 1;
    }else{
        list_size = numItensNaLista(head);
    }
    pthread_mutex_lock(&list_mutex);
    aux = head;
    if(list_size != 0){
        for(i=0;i<list_size;i++){
            node = getItemLista(aux);
            if(sendDataToRemote(node->sock,messageToSpread.info,messageToSpread.payload)!=0){
                perror("\tFailed to spread new information, removing remote clipboard\n");
                aux2=aux;
                aux = getProxElementoLista(aux);
                head = free_node(head, aux2,freePayload);
                continue;
            }
            aux = getProxElementoLista(aux);
        }
    }
    pthread_mutex_unlock(&list_mutex);
    pthread_exit(NULL);
}


/**
 * This function receives the 10 clipboard regions from the parent clipboard and
 * writes them to the local clipboard.
 *
 * @param parent_id
 * @return
 */
int ClipSync(int parent_id){
    struct metaData info;
    void * received;
    int error, logout,i;
    //Request Data

    for( i=0;i<REGION_SIZE;i++){
        received = getRemoteData(parent_id,&info,false,&error,&logout);
        if(error == 1){
            //Handle error
            return -1;
        }
        setLocalRegion(i,received,info.msg_size,info.hash);
    }

    return(0);
}


/**
 *
 * @param payload
 */
void freeWaitingListNode(void * payload){
    //FIXME
    struct data * data = payload;
    free(data->payload);
}