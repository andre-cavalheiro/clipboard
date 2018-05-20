#include <sys/types.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/time.h>

#include "list.h"
#include "socket_lib.h"


#define SOCK_LOCAL_ADDR "/tmp/CLIP"       //FIXME -> devia ser sem o tmp e CLIPBOARD_SOCKET, é só para o debugger funcionar.
#define REGION_SIZE 10
#define HASH_SIZE  10

//handshake struct
/*0->client wants to send data to server. 1->client is requesting data from server.
2->Local client is logging out.     3-> wait    4-> Remote end this connection or refusing information*/
struct metaData{
    int action;
    int region;
    char hash[HASH_SIZE];
    size_t msg_size;
};


//Local Functions
int clipboard_connect(char * clipboard_dir);
int clipboard_copy(int clipboard_id, int region, void *buf, size_t count);
int clipboard_paste(int clipboard_id, int region, void *buf, size_t count);
int clipboard_wait(int clipboard_id, int region, void *buf, size_t count);
void clipboard_close(int clipboard_id);
int checkParams(int clipboard_id, int region);  //Isto devia tar noutro sitio nao?

