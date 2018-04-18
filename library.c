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
	//Create Socket
	int sock_fd= socket(AF_UNIX,SOCK_STREAM , 0);
	if (sock_fd == -1){
		perror("socket: ");
		return -1;
	}
	//Bind socket to local address
	local_addr.sun_family = AF_UNIX;
	strcpy(local_addr.sun_path, SOCK_ADDRESS);
    if(connect(sock_fd,(struct sockaddr *)&local_addr,sizeof(struct sockaddr))==-1){
        perror("connect: ");
        return -1;
    }
	//Return socket file descriptor
	return sock_fd;

}

/* This function receives the value returned by clipboard_connect and uses it to send/copy
data to clipboard, using a socket (all reads and writes will be done in the same file
descriptor)
*/
int clipboard_copy(int clipboard_id, int region, void *buf, size_t count){
	struct msg * msgInit = (struct msg*)malloc(sizeof(struct msg));

	//fills structure for transit
	msgInit->option = WRITE_REQUEST;
	msgInit->region = region;
	msgInit->msg_size = count;

	//Prepares message to send
	char * msgInitInString = (char*)malloc(sizeof(char)* sizeof(msgInit));
	if(memcpy(msgInitInString,msgInit, (int)sizeof(msgInit)) == NULL){
		perror("Allocating msgInitInString");
		return -1;
	};

	//sends message to clipboard
	if (send(clipboard_id,msgInitInString,sizeof(struct msg),0) == -1){
		perror("send");
		return -1;
	} else{
		int bytesSent=0;
		//Send the actual message
		//FIXME while(send ?
		bytesSent = send(clipboard_id,buf,count,0);
		printf("[clipboard_copy] Sent %d bytes -- Message was %d bytes\n",bytesSent,(int)count);
	}

	//frees temp message
	free(msgInit);
	return 0;
}


/*This function receives the value returned by clipboard_connect and uses it to
retrieve/paste data from the clipboard, using a socket (all reads and writes will be done in
the same file descriptor)
*/

/*
int clipboard_paste(int clipboard_id, int region, void *buf, size_t count){

	//allocates memory for structures
	char* msg = (char*)malloc(sizeof(struct msg));
	struct msg* send_msg = (struct msg*)malloc(sizeof(struct msg));

	//fills structure for transit
	send_msg->option = READ_REQUEST;
	send_msg->region = region;

	memcpy(msg,send_msg, sizeof(struct msg));


	//writes request to clipboard
	if(send(clipboard_id,msg,sizeof(struct msg),0) == -1){
		perror("send");
	}

	//reads buf message from clipbord
	if(recv(clipboard_id, buf, sizeof(count), 0) == -1 ){
		perror("receive");
	}

	free(send_msg);
}
*/