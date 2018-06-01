#include <sys/types.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/time.h>
#include <poll.h>

#include "list.h"
#include "socket_lib.h"

#define SOCK_LOCAL_ADDR "CLIPBOARD_SOCKET"    //when SOCKET is put instead of SOCK, the string is cut! why???
#define REGION_SIZE 10
#define HASH_SIZE  10
#define SIG_SOCKET "SIG_SOCKET"
//handshake struct
/*0->client wants to send data to server. 1->client is requesting data from server.
2->Local client is logging out.     3-> wait    4-> Remote end this connection or refusing information*/
struct metaData{
    int action;
    int region;
    char hash[HASH_SIZE];
    size_t msg_size;
};
