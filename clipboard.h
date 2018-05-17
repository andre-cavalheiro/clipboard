#include <sys/types.h>
#include "socket_lib.h"

#define SOCK_LOCAL_ADDR "/tmp/CLIPBOARD"       //FIXME -> devia ser sem o tmp e CLIPBOARD_SOCKET, é só para o debugger funcionar.
#define REGION_SIZE 10
#define HASH_SIZE  10


//handshake struct
struct metaData{
    int action;         //0->client wants to send data to server. 1->client is requesting data from server. 2->Local client is logging out. 3-> wait 4-> Remote end connection
    int region;
    char * hash;
    size_t msg_size;
};

//Actual Data struct
struct data{
    void * payload;
    char * hash;
    size_t size;
};



int clipboard_connect(char * clipboard_dir);
int clipboard_copy(int clipboard_id, int region, void *buf, size_t count);
int clipboard_paste(int clipboard_id, int region, void *buf, size_t count);
int clipboard_wait(int clipboard_id, int region, void *buf, size_t count);
void clipboard_close(int clipboard_id);

int checkParams(int clipboard_id, int region);  //Isto devia tar noutro sitio nao?