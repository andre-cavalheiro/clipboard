#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "clipboard.h"

 
int main(){
	int clipboard_id;


	if((clipboard_id = clipboard_connect("doesnt matter right now"))==-1){
		exit(-1);
	}

	exit(0);
	
}
