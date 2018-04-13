#define READ_REQUEST 0
#define WRITE_REQUEST 1
#define SOCKET_NAME "clipboardSocket"
#define SOCKET_QUEUE_LENGTH 5
#define SOCK_ADDRESS "./clipboardSocket"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>


/** _msg structure
 * 
 * Structure used for communication between APP and clipboard
 *
 * int option - 		READ_REQUEST - requests to read a message from region
 * 						WRITE_REQUEST - request to write a message to region
 * 
 * int region - 		region number to be accessed
 * 
 * void* message - 		message to be sent to clipboard
 * 
 * size_t msg_size - 	message size
 * 
 * */
struct _msg{
	int option;			//make it a boolean?
	int region;
	void* message;
	size_t msg_size;

 };

int clipboard_connect(char * clipboard_dir);
int clipboard_copy(int clipboard_id, int region, void *buf, size_t count);
int clipboard_paste(int clipboard_id, int region, void *buf, size_t count);

