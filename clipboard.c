#include "clipboard.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
 
int main(){
	int sockt_id;

	if((sockt_id = clipboard_connect("doesnt matter right now"))==-1){
		exit(-1);
	}

	//Creates message handlers(assuming right now that clipboard only deals with strings)
	char** clipboard = (char*)malloc(10*sizeof(char*));
	char* msg = (char*)malloc(sizeof(_msg));
	_msg* send_msg = (_msg*)malloc(sizeof(_msg));

	//Clipboard awaits orders
	while(1){

		if(recv(sockt_id, msg, sizeof(_msg),0) == -1){
			perror("receive");
		}
		//de-codes message
		memcpy(send_msg, msg);

		if(send_msg->option == WRITE_REQUEST){
			printf("received and writing to region %d\n",send_msg->region);
			clipboard[send_msg->region] = send_msg->message;
		}
		//READ_REQUEST
		else{
			printf("sending message from region %d", send_msg->region);
			if(send(sockt_id, clipboard[send_msg->region], sizeof(clipboard[send_msg->region],0) == -1)){
				perror("send");
			}
		}
	}
	exit(0);
	
}
