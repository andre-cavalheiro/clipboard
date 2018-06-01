#include "clipboard.h"

//Data structures

//Clipboard region
struct data{
    void * payload;
    char hash[HASH_SIZE];
    bool from_parent;
    size_t size;
};

//Nodes of list
struct node{
    int sock;
    int id;
};

//Argument type for Remote clipboard handlers
struct argument{
    int id;
    int sock;
    bool isParent;
};

//Arguments for message spreaders
struct spread{
    int region;
    struct metaData info;
    void * payload;
    bool parent;
};

//Global Variables
struct data clipboard[REGION_SIZE];
bool new_data[REGION_SIZE];
bool from_parent[REGION_SIZE];
t_lista * head;

pthread_mutex_t mutex[REGION_SIZE];
pthread_mutex_t setup_mutex ;
pthread_mutex_t list_mutex;
pthread_mutex_t passingArgumentsToSpread;
pthread_mutex_t passingArgumentsToHandleClip;
pthread_rwlock_t rwlocks[REGION_SIZE];
pthread_mutex_t waitingList_mutex[REGION_SIZE];
pthread_cond_t cond[REGION_SIZE];

t_lista * waitingLists[REGION_SIZE];



//Remote and local Comunication functions
void * getLocalClipboardData(int region);
struct metaData getLocalClipboardInfo(int region);
struct data * setLocalRegion(int region, void * payload,size_t size,char*hash);
void * getRemoteData(int sock,struct metaData *info,bool compare,int* err,int* logout);
int sendDataToRemote(int client,struct metaData info, void* payload);

//Clipboard functionality Functions
void * handleLocalClient(void * );
void * handleClipboard(void * arg);
void * ClipHub (void * );
void * ClipHandleChild (void * _clip);
void * ClipHandleParent (void * _clip);
void * spreadTheWord(void *arg);
void * regionWatch(void * region_);
int ClipSync(int parent_id);

//Clipboard helper functions
void shutDownClipboard(int );
char * generateHash(int size);
void freePayload(void * payload);
void freeWaitingListNode(void * payload);
void * xmalloc(size_t size);

//Clipboard development functions
void printClipboard();
void printListClipboards();
void printWaitingList(int region);