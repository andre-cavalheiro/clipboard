#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
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
void * ClipHandle (void * _clip);
void * ClipOuterHandle (void * _clip);
int ClipSync(int parent_id);

//Global Variables
struct node clipboard[REGION_SIZE];

pthread_mutex_t mutex[REGION_SIZE] = PTHREAD_MUTEX_INITIALIZER;
pthread_rwlock_t rwlocks[REGION_SIZE];
bool new_data[REGION_SIZE]; 


int main(int argc, char** argv) {

	//Clipboard setup
    for(int i; i < REGION_SIZE; i++){
        clipboard[i].payload = NULL;
        clipboard[i].size = 0; //Cannot be negative because size_t
        clipboard[i].time = time(NULL);
        pthread_rwlock_init(rwlocks[i], NULL);
    }
    //Signal handlers
    struct sigaction shutdown;
    shutdown.sa_handler = shutDownClipboard;
    sigaction(SIGINT,&shutdown,NULL);

    //handle command line arguments  -- should handle errors
    int opt = getopt(argc, argv, "c:");
   	pthread_t clipboard_hub;
    if(opt == 'c'){
        char * ip = malloc(16);
        ip = optarg;
        int port = atoi(argv[optind]);
        printf("%s   %d\n",ip, port);

        //create socket for parent clipboard comms
        int parent_sock = createSocket(AF_INET,SOCK_STREAM);
        InternetClientSocket(parent_sock, ip, port);

	    //creates clipboard hub to receive/transmit data between clipboards
	    if(pthread_create(&clipboard_hub, NULL, ClipHub, &parent_sock) != 0){
	    	perror("Creating thread\n");
	    	exit(-1);
	    }
        //new thread to handle this connection
    	pthread_t parent_connection;
    	if(pthread_create(&parent_connection, NULL, ClipHandle, &parent_sock) != 0){
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

    //Create Local Socket
    int sock = createSocket(AF_UNIX,SOCK_STREAM);
    //Bind Local socket
    UnixServerSocket(sock,SOCK_LOCAL_ADDR,5);

    // Local clients setup
    pthread_t localHandler;
    int * client = malloc(sizeof(int));

    while(1){
        printf("Ready to accept \n");
        if((*client = accept(sock, NULL, NULL)) == -1) {
            perror("accept");
            exit(-1);
        }
        if(pthread_create(&localHandler, NULL, ClipHandle, client) != 0){
            printf("Creating thread");
            exit(-1);
        }
    }

    return 0;
}

/** LOCAL CLIENTS WILL ALWAYS HAVE THE MOST RECENT DATA THEREFORE IT IT UNECESSARY TO CHECK TIME BUT WE HAVE TO SEND IT
 *
 * @param client_
 * @return
 */
void * handleClient(void * client__){
    int * client_ = client__;
    int client = *client_;
    void * bytestream_cpy = NULL;
    void * bytestream_pst = malloc(sizeof(struct metaData));
    struct metaData info;

    while(1){
        printf("[Thread] Ready to receive \n");
        bytestream_cpy = handleHandShake(client, sizeof(struct metaData));
        memcpy(&info,bytestream_cpy,sizeof(struct metaData));
        switch (info.action){
            case 0:
                //client wants to send data to server (Copy)
                printf("[Thread] Client wants to copy region %d with size %zd\n",info.region,info.msg_size);

                //********CRITICAL REGION*************
                pthread_mutex_lock(mutex[info.region]);
                pthread_rwlock_wrlock(rwlocks[info.region]);
                if(clipboard[info.region].size != 0){
                    free(clipboard[info.region].payload);
                }
                clipboard[info.region].size = info.msg_size;
                clipboard[info.region].payload = malloc(info.msg_size);
                clipboard[info.region].payload = receiveData(client,info.msg_size);
                pthread_rwlock_unlock (rwlocks[info.region]);
                new_data[info.region] = true;
                pthread_mutex_unlock(mutex[info.region]);
                //**********CRITICAL REGION***************

                printf("[Thread] Copy completed: %s \n",(char*)clipboard[info.region].payload);
                break;
            case 1:
                //client is requesting data from server (Paste)
                printf("[Thread] Client wants to paste region %d\n",info.region);

                //***************CRITICAL REGION**************** - SAVE TO AUX STRING TO SHRINK REGION
                pthread_rwlock_rdlock(rwlocks[info.region]);
                info.msg_size = clipboard[info.region].size;
                //Informar cliente do tamanho da mensagem
                memcpy(bytestream_pst,&info,sizeof(struct metaData));
                handShake(client,bytestream_pst,sizeof(struct metaData));
                //Enviar mensagem
                sendData(client,clipboard[info.region].size,clipboard[info.region].payload);
                pthread_rwlock_unlock(rwlocks[info.region]);
                //************+CRITICAL REGION**************

                printf("Paste completed\n");
                break;
            case 2:
                //client is logging out
                shutDownThread(bytestream_cpy,bytestream_pst);
                pthread_exit(NULL);
                break;
            default:
                //Error:
                //FIXME handle this case in client side.
                shutDownThread(bytestream_cpy,bytestream_pst);
                pthread_exit(NULL);
                break;
        }
        printf("[Thread] Final clipboard state: \n");
        for(int i=0; i<REGION_SIZE; i++){
            printf("\t[%d]: %s \n",i,(char*)clipboard[i].payload);
        }
    }
}


/**
 *
 *
 *
 */
void * ClipHub (void * parent_){

	int * parent = parent_;
	if (parent != NULL){
		//new thread to handle parent connection
		int parent_id = *parent;
    	pthread_t parent_connection;
    	ClipSync(parent_id);
    	if(pthread_create(&parent_connection, NULL, ClipHandle, parent) != 0){
    	perror("Creating thread\n");
    	exit(-1);
    	}
	}
	//Create Socket
    int sock = createSocket(AF_INET,SOCK_STREAM);
    //Bind Local socket
    InternetServerSocket(sock, INADDR_LOOPBACK,5);


    // ClipHub setup
    pthread_t clipboard_comm;
    int * clip = malloc(sizeof(int));

    while(1){
        printf("Ready to accept \n");
        if((*clip = accept(sock, NULL, NULL)) == -1) {
            perror("accept");
            exit(-1);
        }
        if(pthread_create(&clipboard_comm, NULL, ClipHandle, clip) != 0){
            perror("Creating thread");
            exit(-1);
        }
    }
    return 0;    
}


/**	OUTSIDERS MAY NOT HAVE THE MOST RECENT DATA THEREFORE IT IS NECESSARY TO CROSS CHECK THE VALIDITY OF THE DATA WE RECEIVE
 *
 * clip -> socket id
 *
 */
void * ClipHandle (void * _clip){

	//This thread will handle local check(and update the outside), while another one checks for updates from the outside (and update local)
    int * clip_ = _clip;
    int clip = *clip_;
    pthread_t secondary_comm;

    void * bytestream_cpy = NULL;
    void * bytestream_pst = malloc(sizeof(struct metaData));
    struct metaData info;

    //send eveything to new clipboard
    int i;

    //create secondary thread
    if(pthread_create(&secondary_comm, NULL, ClipOuterHandle, &clip) != 0){
        perror("Creating thread");
        exit(-1);
    }

    while(1){

		//***************CRITICAL REGION**************** - SAVE TO AUX STRING TO SHRINK REGION
		pthread_mutex_lock(mutex[info.region]);

		pthread_rwlock_rdlock(rwlocks[info.region]);
	    info.msg_size = clipboard[info.region].size;
	    //Informar cliente do tamanho da mensagem
	    memcpy(bytestream_pst,&info,sizeof(struct metaData));
	    handShake(clip,bytestream_pst,sizeof(struct metaData));
	    //Enviar mensagem
	    sendData(clip,clipboard[info.region].size,clipboard[info.region].payload);
	    pthread_rwlock_unlock(rwlocks[info.region]);

	    pthread_mutex_unlock(mutex[info.region]);
	    //*************CRITICAL REGION****************
	}
    return(0);
}

/**
 *
 * clip -> socket id
 *
 */
void * ClipOuterHandle (void * _clip){

	//This thread will handle local check(and update the outside), while another one checks for updates from the outside (and update local)
    int * clip_ = _clip;
    int clip = *clip_;

	void * bytestream_cpy = NULL;
    void * bytestream_pst = malloc(sizeof(struct metaData));
    struct metaData info;

    while(1){
        printf("[Thread] Ready to receive \n");
        bytestream_cpy = handleHandShake(clip, sizeof(struct metaData));
        memcpy(&info,bytestream_cpy,sizeof(struct metaData));
        switch (info.action){
            case 0:
                //client wants to send data to server (Copy)
                printf("[Thread] Client wants to copy region %d with size %zd\n",info.region,info.msg_size);

                //**************CRITICAL REGION*****************
                pthread_mutex_lock(mutex[info.region]);
                pthread_rwlock_wrlock(rwlocks[info.region]);
                if(clipboard[info.region].size != 0){
                    free(clipboard[info.region].payload);
                }
                clipboard[info.region].size = info.msg_size;
                clipboard[info.region].payload = malloc(info.msg_size);
                clipboard[info.region].payload = receiveData(clip,info.msg_size);
                pthread_rwlock_unlock(rwlocks[info.region]);
                new_data[info.region] = true;
                pthread_mutex_unlock(mutex[info.region]);
                //****************CRITICAL REGION****************

                break;
            case 1:
                //client is requesting data from server (Paste)
            	//It's not supposed to do so!
                break;
            case 2:
                //client is logging out
                shutDownThread(bytestream_cpy,bytestream_pst);
                pthread_exit(NULL);
                break;
            default:
                //Error:
                //FIXME handle this case in client side.
                shutDownThread(bytestream_cpy,bytestream_pst);
                pthread_exit(NULL);
                break;
        }
    }
}


int ClipSync(int parent_id){
	char * bytestream = malloc(sizeof(struct metaData));
	struct metaData info;
    void * received;
    int i,count = 0;

    //Request Data
    for( i = 0; i < REGION_SIZE ; i++){
		info.region=i;
		info.action=1;
		info.msg_size=-1;
		memcpy(bytestream,&info,sizeof(struct metaData));
		if(handShake(parent_id,bytestream,sizeof(struct metaData)) != 0){
	        free(bytestream);
	        exit(0);
	    }

	    //Get size of data
	    if((bytestream = handleHandShake(parent_id,sizeof(struct metaData))) == NULL){
	        free(bytestream);
	        exit(0);
	    }
	    memcpy(&info,bytestream,sizeof(struct metaData));
	    free(bytestream);
	    received = malloc(info.msg_size);                                   //FIXME Check if malloc is necessary, i dont think so

	    //Get data
		if((received = receiveData(parent_id,info.msg_size)) == NULL){
	        exit(0);
		}
	    //Handle data CRITICAL REGION
	    if(info.msg_size > count ){
	        //FIXME - devia copiar o que pode para o buffer. Talvez memcpy só com count
	        printf("[clipboard_paste] Given buffer is not big enough\n");
	        free(received);
	        exit(count);
	    } else if(info.msg_size > 0){
	        //printf("\t[clipboard_paste] Paste successful\n");
	        pthread_rwlock_rdlock(rwlocks[info.region]);
	        memcpy(clipboard[i].payload,received,info.msg_size);
	        free(received);
	        clipboard[i].size = info.msg_size;
	        clipboard[i].time = info.time;
	        pthread_rwlock_unlock(rwlocks[info.region]);
	    } else{
	        //printf("\t[clipboard_paste] Region was empty \n "); CRITICAL??
	        free(received);
	        pthread_rwlock_rdlock(rwlocks[i]);
			clipboard[i].size = 0; //Buf unchanged, region was empty
			pthread_rwlock_unlock(rwlocks[i]);
	    }
	}
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
    unlink(SOCK_LOCAL_ADDR);
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
