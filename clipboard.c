#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "clipboard.h"

 
int main(){
	char file_name[100];
	
	sprintf(file_name, "./%s", OUTBOUND_FIFO);
	unlink(file_name);
	if(mkfifo(file_name, 0666)==-1){
		printf("Error creating out fifo\n");
		exit(-1);
	}
	int fifo_out = open(file_name, O_RDWR);
	if(fifo_out == -1){
		printf("Error opening in fifo\n");
		exit(-1);
	}
	
	
	
	sprintf(file_name, "./%s", INBOUND_FIFO);
	unlink(file_name);
	if(mkfifo(file_name, 0666)==-1){
		printf("Error creating in fifo\n");
		exit(-1);
	}
	int fifo_in = open(file_name, O_RDWR);
	if(fifo_in == -1){
		printf("Error opening in fifo\n");
		exit(-1);
	}
	//abrir FIFOS
	char** clipboard = (char*)malloc(10*sizeof(char*));
	char* msg = (char*)malloc(sizeof(_msg));
	_msg* send_msg = (_msg*)malloc(sizeof(_msg));
	while(1){
		printf(".\n");
		read(fifo_in, msg, sizeof(_msg));
		memcpy(send_msg, msg);

		if(send_msg->option == WRITE_REQUEST){
			printf("received %s and writing to region %d\n", send_msg->message, send_msg->region);
			clipboard[send_msg->region] = send_msg->message;
		}
		else{
			
			printf("sending message from region %d", send_msg->region);
			write(fifo_out, clipboard[send_msg->region], sizeof(clipboard[send_msg->region]);
		}

	}
		
	exit(0);
	
}
