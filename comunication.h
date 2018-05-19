#include "clipboard.h"


//handshake struct
struct metaData{
    int action;         //0->client wants to send data to server. 1->client is requesting data from server. 2->Local client is logging out. 3-> wait 4-> Remote end connection
    int region;
    char hash[HASH_SIZE];
    size_t msg_size;
};

//Actual Data struct
struct data{
    void * payload;
    char hash[HASH_SIZE];
    size_t size;
};




//Global Variables
struct data clipboard[REGION_SIZE];
bool new_data[REGION_SIZE];
bool from_parent[REGION_SIZE];
t_lista * head;
pthread_mutex_t mutex[REGION_SIZE];
pthread_mutex_t setup_mutex ;
pthread_mutex_t list_mutex;
pthread_rwlock_t rwlocks[REGION_SIZE];
pthread_cond_t cond[REGION_SIZE];



void * getLocalClipboardData(int region);
struct metaData getLocalClipboardInfo(int region);
void setLocalRegion(int region, void * payload,size_t size,char*hash);
void * getRemoteData(int sock,struct metaData *info,bool compare);
int sendDataToRemote(int client,struct metaData info, void* payload);



//Clipboard Functions
/***********OLD*/
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
/********/