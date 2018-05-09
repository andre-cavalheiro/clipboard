#include "clipboard.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>


// gcc -Wall -o lib.o -c library.c  && gcc -o app_test.o app_teste.c socketlib.o library.c

/**
 *
 * @param clipboard_dir
 * @return
 */
int clipboard_connect(char * clipboard_dir){			//Must use clipboard_dir
	//Creating
	int sock ;
	if((sock = createSocket(AF_UNIX,SOCK_STREAM)) == -1){
		return -1;
	}
	//Connect
	if(UnixClientSocket(sock,SOCK_LOCAL_ADDR)==-1){
		return -1;
	}
	printf("\t[Clipboard Connect] connected successfully\n");
	return sock;
}

/**
 *
 * @param clipboard_id
 * @param region
 * @param buf
 * @param count
 * @return
 */
int clipboard_copy(int clipboard_id, int region, void *buf, size_t count){
    //printf("\t[Clipboard copy] buf = %s\n",buf);
	char * bytestream = malloc(sizeof(struct metaData));
	struct metaData info;
	info.region=region;
	info.action=0;
	info.msg_size=count;
	memcpy(bytestream,&info,sizeof(struct metaData));
	handShake(clipboard_id,bytestream,sizeof(struct metaData));
	if(sendData(clipboard_id,count,buf)==-1){
		return -1;
	}
	printf("\t[Clipboard copy] Copy successful\n");
	return 0;
}

/**
 *
 * @param clipboard_id
 * @param region
 * @param buf
 * @param count
 * @return
 */
int clipboard_paste(int clipboard_id, int region, void *buf, size_t count){
	char * bytestream = malloc(sizeof(struct metaData));
	struct metaData info;
	info.region=region;
	info.action=1;
	info.msg_size=count;
	memcpy(bytestream,&info,sizeof(struct metaData));
	handShake(clipboard_id,bytestream,sizeof(struct metaData));
	if((buf = receiveData(clipboard_id,count))==NULL){
		return -1;
	}
	printf("\t[clipboard_paste] Paste successful\n");
	return 0;

}

/**
 *
 * @param clipboard_id
 * @param region
 * @param buf
 * @param count
 * @return
 */
int clipboard_wait(int clipboard_id, int region, void *buf, size_t count){

}

/**
 *
 * @param clipboard_id
 */
void clipboard_close(int clipboard_id){

}
