#include "clipboard.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>


int clipboard_connect(char * clipboard_dir){
	char fifo_name[100];
	
	sprintf(fifo_name, "%s%s", clipboard_dir, INBOUND_FIFO);
	int fifo_send = open(fifo_name, O_WRONLY);
	sprintf(fifo_name, "%s%s", clipboard_dir, OUTBOUND_FIFO);
	int fifo_recv = open(fifo_name, O_RDONLY);
	if(fifo_recv < 0)
		return fifo_recv;
	else
		return fifo_send;
}

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
	free(send_msg);
	
	//writes message to FIFO	
	write(clipboard_id,msg,sizeof(_msg));
	
	
}
int clipboard_paste(int clipboard_id, int region, void *buf, size_t count){
	
	//allocates memory for structures
	char* msg = (char*)malloc(sizeof(_msg));
	_msg* send_msg = (_msg*)malloc(sizeof(_msg));
	
	//fills structure for transit
	send_msg->option = READ_REQUEST;
	send_msg->region = region;
	
	memcpy(msg,send_msg, sizeof(_msg));
	free(send_msg);
	
	//writes request to FIFO	
	write(clipboard_id,msg,sizeof(_msg));
	
	//reads answer message from FIFO
	read(clipboard_id + 1, buf, sizeof(count));
	
}
