#define READ_REQUEST 0
#define WRITE_REQUEST 1
#define SOCKET_QUEUE_LENGTH 5
#define SOCK_ADDRESS "./Sock_420"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>


/** msg structure
 * 
 * Structure used for communication between APP and clipboard
 *
 * int option - 		READ_REQUEST - requests to read a message from region
 * 						WRITE_REQUEST - request to write a message to region
 * 
 * int region - 		region number to be accessed
 * 
 *
 * size_t msg_size - 	message size
 * 
 * */
struct msg{
	int option;			//make it a boolean?
	int region;
	size_t msg_size;
 };




int clipboard_connect(char * clipboard_dir);
int clipboard_copy(int clipboard_id, int region, void *buf, size_t count);
//int clipboard_paste(int clipboard_id, int region, void *buf, size_t count);

