#include "clipboard.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
 
int main(){

	//Creates message handlers (assuming right now that clipboard only deals with strings)
	char** clipboard = (char**)malloc(10*sizeof(char*));
	char* msg = (char*)malloc(sizeof(struct msg));
	struct msg* send_msg = (struct msg*)malloc(sizeof(struct msg));

	printf("Starting\n");

	//Create Socket
	struct sockaddr_un local_addr;			//Isto torna só valido para AF_UNIX
	int sock_id= socket(AF_UNIX,SOCK_STREAM , 0);
	if (sock_id == -1){
		perror("socket: ");
		return -1;
	}
	printf("Creating Socket\n");
	//Bind socket to local address
	local_addr.sun_family = AF_UNIX;
	strcpy(local_addr.sun_path, SOCK_ADDRESS);
	if(unlink(SOCK_ADDRESS) == -1){
		perror("unlinking");
	}
	if(bind(sock_id,(struct sockaddr *)&local_addr, sizeof(struct sockaddr)) == -1) {
		perror("bind");
		return -1;
	}
	printf("Socket binded \n");

	//Enabling connections
	if(listen(sock_id,SOCKET_QUEUE_LENGTH) == -1){
		perror("listen");
		return -1;
	}
    printf("Listening...\n");
    socklen_t addrLen = sizeof(local_addr);
	int fdClient;
	//Clipboard awaits orders
	while(1){
		addrLen = sizeof(local_addr);
		fdClient = accept(sock_id,(struct sockaddr *)&local_addr,&addrLen);
		printf("Waiting...\n");
		if(recv(fdClient, msg, sizeof(struct msg),0) > 0){
			//de-codes message
			memcpy(send_msg, msg, sizeof(struct msg));
			printf(">Received option: %d region: %d mst_size: %d \n",send_msg->option,send_msg->region,(int)send_msg->msg_size);
			//Receives actual message
			char * actualMessage = (char*)malloc(sizeof(char)*send_msg->msg_size);
			if(recv(fdClient,actualMessage ,send_msg->msg_size ,0) < 0){
				perror("Receive");
			}
			printf(">Actual message: %s\n",actualMessage);
			//Handle Write request
			if(send_msg->option == WRITE_REQUEST){
				printf(">Received and writing to region %d\n",send_msg->region);
				clipboard[send_msg->region] = actualMessage ;
			}
				//Handle Read request
			else{
				printf("sending message from region %d\n", send_msg->region);
				if(send(fdClient, clipboard[send_msg->region], sizeof(clipboard[send_msg->region]),0) == -1){
					perror("send");
				}
			}
			printf(">Finished processing Message\n");
		}
	}
	close(fdClient);
}
