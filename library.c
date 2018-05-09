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
    char * received;
    //Request Data
	info.region=region;
	info.action=1;
	info.msg_size=-1;
	memcpy(bytestream,&info,sizeof(struct metaData));
	handShake(clipboard_id,bytestream,sizeof(struct metaData));
    //Get size of data
    bytestream = handleHandShake(clipboard_id,sizeof(struct metaData));
    memcpy(&info,bytestream,sizeof(struct metaData));
    received = malloc(info.msg_size);
    //Get data
    if(info.msg_size > count){
        printf("[clipboard_paste] Given buffer is not big enough\n");
    }
	if((received = receiveData(clipboard_id,info.msg_size))==NULL){
		return -1;
	}
    memcpy(buf,received,info.msg_size);     //Only way actually to change buf pointer
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
