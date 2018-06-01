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
        //printf("[Local] Ready to receive \n");

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

                //printf("[Local] Client wants to copy region %d with size %zd\n",info.region,info.msg_size);

                //***************CRITICAL REGION****************
                pthread_mutex_lock(&mutex[info.region]);
                data=setLocalRegion(info.region,payload,info.msg_size,NULL);
                new_data[info.region] = true;
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
                    pthread_exit(NULL);
                    printf("\t[Local] Error adding new info to list\n");
                }

                pthread_cond_broadcast(&cond[info.region]);


                printClipboard();

                break;
            case 1:
                //client is requesting data from server (Paste)

                //printf("[Local] Client wants to paste region %d\n",info.region);

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

                //printf("[Local] Paste completed\n");
                break;
            case 2:
                //client is logging out
                printf("[Local] Client is exiting\n");
                if(bytestream != NULL){
                    free(bytestream);
                }
                pthread_exit(NULL);
                break;
            case 3:
                    //client is requesting wait
                    printf("[Local] Client wants to wait for a new addition to region %d\n",info.region);
                    /***************CRITICAL REGION****************/
                    pthread_mutex_lock(&mutex[info.region]);

                    pthread_cond_wait(&cond[info.region],&mutex[info.region]);
                    printf("Initating wait \n");

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
    printf("[Cliphub] Waiting for connections on port : %d \n", port);

    
    sonArg->isParent=false;

    //Wait for remote clipboard connections
    while(1){
        //printf("[Cliphub] Ready to accept \n");
        if((clip_read = accept(sock, NULL, NULL)) == -1) {
            perror("accept");
            exit(-1);
        }
        sonArg->id = id;
        sonArg->sock = clip_read;
        printf("[Cliphub] New clipboard connected \n");
        //After a remote clipboard connects, it creates 2 sockets with it:
        // One to receive updates
        pthread_mutex_lock(&passingArgumentsToHandleClip);
        if(pthread_create(&clipboard_comm, NULL, handleClipboard,sonArg ) != 0){
            perror("[Cliphub] Creating thread");
            exit(-1);
        }
        //And one to spread updates whenever there's one
        if((clip_write = accept(sock, NULL, NULL)) == -1) {
            perror("[Cliphub] accept");
            exit(-1);
        }
        listNode = xmalloc(sizeof(struct node));
        listNode->sock = clip_write;
        listNode->id = id++;
        pthread_mutex_lock(&list_mutex);
        head = criaNovoNoLista(head,listNode,&err);
        printList();
        pthread_mutex_unlock(&list_mutex);
        if(err !=0){
            printf("\t[ClipHub] !!!!!!!! Error creating new node\n");
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
    t_lista * aux=NULL,*prev=NULL;
    struct node * node;

    /*Whenever a remote clipboard connects to this one,
     the current local clipboard is send out*/
    if(!remoteClipboard.isParent){
        printf("[HandleClipboard] A child contacted me for the first time, sending all the data \n");
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
                        head = free_node(head,&prev, aux,freePayload);
                        break;
                    }
                    prev = aux;
                    aux = getProxElementoLista(aux);
                }
                printList();
                pthread_mutex_unlock(&list_mutex);            }
        }
        printf("[HandleClipboard] Child is updated \n");
    }


    //Wait for an update by the remote clipboard
    while(1){
        //Receive information about the new data and verify if it's actually new
        payload = getRemoteData(remoteClipboard.sock, &info, remoteClipboard.isParent,&error,&logout);
        if(logout == 1 || error == 1){
            //Handle clipboard disconnection
            printf("\t[HandleClipboard] Remote Client is logging out\n" );
            aux = head;
            prev = NULL;
            pthread_mutex_lock(&list_mutex);
            while(aux != NULL){
                    node = getItemLista(aux);
                    if(remoteClipboard.id == node->id){
                        printf("\t[HandleClipboard] Removing [%d] from connected clipboards \n",remoteClipboard.id);
                        head = free_node(head,&prev, aux,freePayload);
                        break;
                    }
                    prev = aux;
                    aux = getProxElementoLista(aux);
            }
            printList();
            pthread_mutex_unlock(&list_mutex);
            pthread_exit(NULL);

        }
        if(payload == NULL){
            printf("\t[HandleClipboard] I already had this information \n");
        }else{

            //Update Local Clipboard
            data=setLocalRegion(info.region,payload,info.msg_size,info.hash);
            printf("[HandleClipboard] Received and updated: [%d] - %s \n",info.region,info.hash );

            strncpy(data->hash,info.hash,HASH_SIZE);
            data->from_parent = remoteClipboard.isParent;
            pthread_mutex_lock(&waitingList_mutex[info.region]);
            waitingLists[info.region] = criaNovoNoLista(waitingLists[info.region],data,&error);
            pthread_mutex_unlock(&waitingList_mutex[info.region]);
            if(error==1){
                printf("\t[HandleClipboard] Error adding new info to list\n");
            }

            //***************CRITICAL REGION****************
            pthread_mutex_lock(&mutex[info.region]);
            new_data[info.region]=true;
            from_parent[info.region]=remoteClipboard.isParent;
            //Send signal to spread new data
            pthread_cond_broadcast(&cond[info.region]);
            pthread_mutex_unlock(&mutex[info.region]);
            //***************CRITICAL REGION****************


            printClipboard();
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
    t_lista * aux2 = NULL;
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
        //printf("[Region watch - %d] Out of Conditional wait \n", region);
        info.action = 0;
        info.msg_size = data->size;
        strncpy(info.hash,data->hash,HASH_SIZE);

        payload = xmalloc(data->size);
        memcpy(payload,data->payload,data->size);

        printf("[Region watch] [%d] - %s - %zd \n",info.region,info.hash,info.msg_size);

        //Cleanup for condition wait
        pthread_mutex_lock(&waitingList_mutex[info.region]);
        if(getProxElementoLista(aux)!= NULL){
            aux2=waitingLists[region];
            while(aux2 != aux){
                aux2=getProxElementoLista(aux2);
            }
        }else{
            aux2=NULL;
        }

        waitingLists[region] = free_node(waitingLists[region],&aux2,aux,freeWaitingListNode);

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

        printf("[Region watch] launching spreadTheWord of (%s) \n",(char*)messageToSpread->info.hash);
        if( pthread_create(&hermes, NULL, spreadTheWord, messageToSpread) != 0){
            printf("[Region watch ] Error Creating thread ");
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
    t_lista * aux=NULL, *prev=NULL;
    struct node *node = NULL;
    //Run through clipboards list and spread the new data
    if(messageToSpread.parent){
        printf("[spreadTheWord] Message came from parent \n");
        list_size = numItensNaLista(head) - 1;
    }else{
        printf("[spreadTheWord] Message came from son/local \n");
        list_size = numItensNaLista(head);
    }
    pthread_mutex_lock(&list_mutex);
    printList();
    aux = head;
    prev = NULL;
    if(list_size != 0){
        printf("[spreadTheWord] Starting to spread the message. List has size %d \n",list_size);
        for(i=0;i<list_size;i++){
            node = getItemLista(aux);
            if(sendDataToRemote(node->sock,messageToSpread.info,messageToSpread.payload)!=0){
                printf("\t[spreadTheWord] !!!!!!!!!!!!!Failed to spread!!!!!!!!!!!!!!!!!!!!!! \n");
                head = free_node(head,&prev, aux,freePayload);
                continue;
            }
            prev = aux;
            aux = getProxElementoLista(aux);
            printf("[spreadTheWord] Node %d (%d) done [%s] \n",i,node->id,messageToSpread.info.hash);
        }
    }
    pthread_mutex_unlock(&list_mutex);
    printf("[spreadTheWord] Message [%s] is spread \n",messageToSpread.info.hash);
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
    printf("[ClipSync] Initiating process\n");
    //Request Data

    for( i=0;i<REGION_SIZE;i++){
        received = getRemoteData(parent_id,&info,false,&error,&logout);
        if(error == 1){
            //Handle error
            return -1;
        }
        printf("[ClipSync] Received [%d] - %s \n",info.region,info.hash );
        setLocalRegion(i,received,info.msg_size,info.hash);
        printf("[ClipSync] Updated Local Region %d\n",i);
    }
    printf("[ClipSync] Done \n");

    return(0);
}


void printList(){
    int * client;
    t_lista* aux = head;
    printf("====\n");
    while(aux != NULL){
        client = getItemLista(aux);
        printf("%d\t",*client);
        aux = getProxElementoLista(aux);
    }
    printf("\n====\n");
}

void freeWaitingListNode(void * payload){
    struct data * data = payload;
    free(data->payload);
}