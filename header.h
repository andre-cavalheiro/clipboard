#include "clipboard.h"

//Data structures

//Clipboard region
struct data{
    void * payload;
    char hash[HASH_SIZE];
    size_t size;
};

//Argument type for Remote clipboard handlers
struct argument{
    int id;
    int sock;
    bool isParent;
};

//Nodes of list
struct node{
    int sock;
    int id;
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



//Remote and local Comunication functions
void * getLocalClipboardData(int region);
struct metaData getLocalClipboardInfo(int region);
void setLocalRegion(int region, void * payload,size_t size,char*hash);
void * getRemoteData(int sock,struct metaData *info,bool compare,int* err,int* logout);
int sendDataToRemote(int client,struct metaData info, void* payload);


//Clipboard functionality Functions
void * handleLocalClient(void * );
void * handleClipboard(void * arg);
void shutDownClipboard(int );
void * ClipHub (void * );
void * ClipHandleChild (void * _clip);
void * ClipHandleParent (void * _clip);
int ClipSync(int parent_id);
char * generateHash(int size, int randFactor,int randFactor2);
void * regionWatch(void * region_);
void freePayload(void * payload);


void printClipboard();
void printList();