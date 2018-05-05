#include "clipboard.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>


int clipboard_connect(char * clipboard_dir){
	//Creating
	int sockUnCl = createSocket(AF_UNIX,SOCK_STREAM);
	//Connect
	UnixClientSocket(sockUnCl,SOCK_LOCAL_ADDR);

	return 0;
}

int clipboard_copy(int clipboard_id, int region, void *buf, size_t count){
	handShake(clipboard_id,&count,1,region);
	printf("Sending data\n");
	sendData(clipboard_id,count,buf);
	return 0;
	
}
int clipboard_paste(int clipboard_id, int region, void *buf, size_t count){
	struct msg * info ;
	handShake(clipboard_id,&count,0,region);
	info = handleHandShake(clipboard_id);
	receiveData(clipboard_id,info.msg_size);
	return 0;
}
