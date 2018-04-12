#include "clipboard.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


/*This function should try to do a connect to a server whose socket is created in the
clipboard_dir. This function returns the file descriptor of the newly created and connected socket on error
*/
int clipboard_connect(char * clipboard_dir){
	struct sockaddr_un local_addr;		//Isto torna só valido para AF_UNIX

	/*Concat clipboard_dir to SOCKET_NAME to variable path 	-----  NOT IMPLEMENTES
	char *path = malloc(strlen(clipboard_dir)+strlen(SOCKET_NAME)+1);//+1 for the null-terminator
	strcpy(path, clipboard_dir);
	strcat(path, SOCKET_NAME);*/
	//Create Socket
	int sock_fd= socket(AF_UNIX,SOCK_STREAM , 0);
	if (sock_fd == -1){
		perror("socket: ");
		return -1;
	}
	//Bind socket to local address
	local_addr.sun_family = AF_UNIX;
	strcpy(local_addr.sun_path, SOCK_ADDRESS);
	if(bind(sock_fd, (struct sockaddr *)&local_addr, sizeof(local_addr)) == -1) {
		perror("bind");
		return -1;
	}
	printf(" socket created and binded \n");
	//Enabeling connections
	if(listen(sock_fd,SOCKET_QUEUE_LENGTH) == -1){
		perror("listen");
		return -1;
	}
	return sock_fd;

}

/* This function receives the value returned by clipboard_connect and uses it to send/copy
data to clipboard, using a socket (all reads and writes will be done in the same file
descriptor)
*/
int clipboard_copy(int clipboard_id, int region, void *buf, size_t count){

	//allocates memory for structures
	char* msg = (char*)malloc(sizeof(_msg));
	_msg* send_msg = (_msg*)malloc(sizeof(_msg));
	
	//fills structure for transit
	send_msg->option = WRITE_REQUEST;
	send_msg->region = region;
	send_msg->message = buf;
	send_msg->msg_size = count;
	
	memcpy(msg,send_msg, sizeof(_msg));
	
	//sends message to clipboard
	if (send(clipboard_id,msg,sizeof(_msg),0) == -1){
		perror("send");
	}

	//frees temp message
	free(send_msg);
	
}


/*This function receives the value returned by clipboard_connect and uses it to
retrieve/paste data from the clipboard, using a socket (all reads and writes will be done in
the same file descriptor)
*/
int clipboard_paste(int clipboard_id, int region, void *buf, size_t count){

	//allocates memory for structures
	char* msg = (char*)malloc(sizeof(_msg));
	_msg* send_msg = (_msg*)malloc(sizeof(_msg));
	
	//fills structure for transit
	send_msg->option = READ_REQUEST;
	send_msg->region = region;
	
	memcpy(msg,send_msg, sizeof(_msg));

	
	//writes request to clipboard	
	if(send(clipboard_id,msg,sizeof(_msg)) == -1){
		perror("send");
	}
	
	//reads buf message from clipbord
	if(recv(clipboard_id, buf, sizeof(count), 0) == -1 ){
		perror("receive");
	}

	free(send_msg);
}
