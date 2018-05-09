#include <sys/types.h>
#include "socket_lib.h"

#define SOCK_LOCAL_ADDR "/tmp/socket_42"  //Criar name com pid()? Se sim então têm de ser em /tmp e temos de garantir unlink
#define MAX_STR_SIZE 200
#define REGION_SIZE 10


//handshake struct
struct metaData{
    int action;         //0->client wants to send data to server. 1->client is requesting data from server
    int region;
    size_t msg_size;
};

//Actual Data struct
struct node {
    void * payload;
    size_t size;
    //will also have the time of the data reception
};


int clipboard_connect(char * clipboard_dir);
int clipboard_copy(int clipboard_id, int region, void *buf, size_t count);
int clipboard_paste(int clipboard_id, int region, void *buf, size_t count);
int clipboard_wait(int clipboard_id, int region, void *buf, size_t count);
void clipboard_close(int clipboard_id);

int checkParams(int clipboard_id, int region);  //Isto devia tar noutro sitio nao?