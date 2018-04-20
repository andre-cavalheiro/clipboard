#include "clipboard.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
 
int main(){

	//Creates message handlers(assuming right now that clipboard only deals with strings)
	char** clipboard = (char**)malloc(10*sizeof(char*));
	char* msg = (char*)malloc(sizeof(struct _msg));
	struct _msg* send_msg = (struct _msg*)malloc(sizeof(struct _msg));

	/**
	 * Internet Socket ---------------------------------------------------
	 *
	 */
	//Create Internet Socket
	struct sockaddr_in backup_addr;
	int sockIn_id = socket(AF_INET,SOCK_STREAM,0);
	if (sockIn_id == -1){
		perror("Internet socket : ");
		return -1;
	}
	//Bind Internet Socket to backup address
	backup_addr.sin_family = AF_INET;
	backup_addr.sin_port= htons(3000);
	inet_aton("127.0.0.1", &backup_addr.sin_addr);

	int err = bind(sockIn_id, (struct sockaddr *)&backup_addr, sizeof(struct sockaddr));
	if(err == -1) {
		perror("Internet bind");
		exit(-1);
	}
	printf(" socket created and binded \n");

	listen(sockIn_id, 5);


    socklen_t addrLen = sizeof(backup_addr);
	int fdClient;
	//Clipboard awaits orders
	while(1){
        printf("Ready to accept\n");
		addrLen = sizeof(backup_addr);
		fdClient = accept(sockIn_id,(struct sockaddr *)&backup_addr,&addrLen);
		printf("Accepted connection \n");

		while(recv(fdClient, msg, sizeof(struct _msg),0) > 0){
            printf(">>> %s \n",(char*)send_msg->message );
            //de-codes message
			memcpy(send_msg, msg, sizeof(struct _msg));
			if(send_msg->option == WRITE_REQUEST){
				printf("received and writing to region %d\n",send_msg->region);
				clipboard[send_msg->region] = send_msg->message;
			}
			//READ_REQUEST
			else{
				printf("sending message from region %d", send_msg->region);
				if(send(fdClient, clipboard[send_msg->region], sizeof(clipboard[send_msg->region]),0) == -1){
					perror("send");
				}
			}
		}
		close(fdClient);
	}
}
