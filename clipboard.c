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
	struct sockaddr_in server_addr;
	int sockIn_id = socket(AF_INET,SOCK_STREAM,0);
	if (sockIn_id == -1){
		perror("Internet socket : ");
		return -1;
	}
	printf(" socket created\n");
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(3000);
	inet_aton("127.0.0.1", &server_addr.sin_addr);

	if(connect(sockIn_id,(struct sockaddr *)&server_addr,sizeof(struct sockaddr))==-1){
		perror("connect: ");
		return -1;
	}

	//Get initial info that's in the backup
	char* aux =  (char*)malloc(100*sizeof(char));
	size_t sizeAux = 0;
	for(int i = 0; i<10; i++){
		clipboard_paste(sockIn_id,i,aux, &sizeAux);
		clipboard[i]=(char*)malloc(sizeof(char)*sizeAux);
		strcpy(clipboard[i],aux);
	}

	/***
	 * Local socket--------------------------------------------------------
	 */
	//Create Socket
	struct sockaddr_un local_addr ;
	int sock_id= socket(AF_UNIX,SOCK_STREAM , 0);
	if (sock_id == -1){
		perror("socket: ");
		return -1;
	}
	//Bind socket to local address
	local_addr.sun_family = AF_UNIX;
	strcpy(local_addr.sun_path, SOCK_ADDRESS);
	unlink(SOCK_ADDRESS);
	if(bind(sock_id,(struct sockaddr *)&local_addr, sizeof(struct sockaddr)) == -1) {
		perror("bind");
		return -1;
	}
	printf(" socket created and binded \n");
	//Enabeling connections1
	if(listen(sock_id,SOCKET_QUEUE_LENGTH) == -1) {
		perror("listen");
		return -1;
	}
    printf("Listening...\n");

    socklen_t addrLen = sizeof(local_addr);
	int fdClient;
	//Clipboard awaits orders
	while(1){
        printf("Ready to accept\n");
		addrLen = sizeof(local_addr);
		fdClient = accept(sock_id,(struct sockaddr *)&local_addr,&addrLen);

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
