#define INBOUND_FIFO "INBOUND_FIFO"
#define OUTBOUND_FIFO "OUTBOUND_FIFO"
#define READ_REQUEST 0
#define WRITE_REQUEST 1
#include <sys/types.h>

/* _msg structure
 * 
 * Structure used for communication between APP and clipboard
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

typedef struct _msg{
	int option;
	int region;
	void* message;
	size_t msg_size;

 }

int clipboard_connect(char * clipboard_dir);
int clipboard_copy(int clipboard_id, int region, void *buf, size_t count);
int clipboard_paste(int clipboard_id, int region, void *buf, size_t count);

