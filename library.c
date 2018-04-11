#include "clipboard.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


int clipboard_connect(char * clipboard_dir){
	struct sockaddr_un local_addr;		//Isto torna só valido para AF_UNIX

	/*Concat clipboard_dir to SOCKET_NAME to variable path 	-----  NOT IMPLEMENTES
	char *path = malloc(strlen(clipboard_dir)+strlen(SOCKET_NAME)+1);//+1 for the null-terminator
	strcpy(path, clipboard_dir);
	strcat(path, SOCKET_NAME);*/
	//Create Socket
	int sock_fd= socket(AF_UNIX,SOCK_STREAM , 0);
	if (sock_fd == -1){
		perror("socket: ");
		return -1;
	}
	//Bind socket to local address
	local_addr.sun_family = AF_UNIX;
	strcpy(local_addr.sun_path, SOCK_ADDRESS);
	if(bind(sock_fd, (struct sockaddr *)&local_addr, sizeof(local_addr)) == -1) {
		perror("bind");
		return -1;
	}
	printf(" socket created and binded \n");
	//Enabeling connections
	if(listen(sock_fd,SOCKET_QUEUE_LENGTH) == -1){
		perror("listen");
		return -1;
	}
	return sock_fd;

	/*Agora devemos lançar um processo que esteja constantemente a ouvir pedidos para o socket
	e guardar a info algures.
	 Depois o clip_board copy/paste só acedem a essa info guardadam e tratam dela right?
	 */

}

int clipboard_copy(int clipboard_id, int region, void *buf, size_t count){

	
}
int clipboard_paste(int clipboard_id, int region, void *buf, size_t count){

	
}
