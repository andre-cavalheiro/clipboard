#include "header.h"


int main(int argc, char** argv) {
    //Global variable initialization
    head = iniLista();
    pthread_mutex_init ( &setup_mutex, NULL);
    pthread_mutex_init ( &list_mutex, NULL);
    for(int i=0;i<REGION_SIZE;i++){
        pthread_mutex_init ( &mutex[i], NULL);
    }

    //Local variables
    pthread_t localHandler;
    int opt,sock;
    struct sigaction shutdown;
    int * client = malloc(sizeof(int));

    //Clipboard setup
    for(int i=0;i<REGION_SIZE;i++){
        clipboard[i].size = 1;              //We'll just have to live with this
        clipboard[i].payload = malloc(clipboard[i].size);   //Don't fill payload with anything!
        clipboard[i].hash[0] = '\0';
    }

    //handle command line arguments  -- should handle errors
    opt = getopt(argc, argv, "c:");
    pthread_t clipboard_hub;
    if(opt == 'c'){
        //If clipboard is launched in connected mode act accordingly
        char * ip = malloc(16);
        int port = atoi(argv[optind]);
        pthread_t parent_connection;
        struct argument * parentArg = malloc(sizeof(struct argument));
        struct node * node = malloc(sizeof(struct node));
        int err =0;

        //create socket for parent clipboard
        int parent_sock_write = createSocket(AF_INET,SOCK_STREAM);
        int parent_sock_read = createSocket(AF_INET,SOCK_STREAM);


        //Get user arguments
        ip = optarg;
        printf("%s   %d\n",ip, port);

        //Bind sockets for parent comunication
        InternetClientSocket(parent_sock_write, ip, port);
        InternetClientSocket(parent_sock_read, ip, port);

        //Add parent to list of clipboard we can talk to
        node->sock = parent_sock_write;
        node->id=0;
        head = criaNovoNoLista(head,node,&err);
        if(err != 0){
            printf("Error creating node\n");
        }

        //Receives current clipboard from parent
        ClipSync(parent_sock_write);

        //Creates clipboard hub to allow connections from other clipboards
        if(pthread_create(&clipboard_hub, NULL, ClipHub,NULL) != 0){
            perror("Creating thread\n");
            exit(-1);
        }

        //Create Thread that handles comunication with the parent
        parentArg->id = 0;
        parentArg->isParent = true;
        parentArg->sock = parent_sock_read;
        pthread_mutex_lock(&passingArgumentsToHandleClip);
        if(pthread_create(&parent_connection, NULL, handleClipboard, parentArg) != 0){
            perror("Creating thread\n");
            exit(-1);
        }
    }
    else{
        //In case clipboard is launched in local mode
        //Creates clipboard hub to allow connections from other clipboards
        if(pthread_create(&clipboard_hub, NULL, ClipHub, NULL) != 0){
            perror("Creating thread\n");
            exit(-1);
        }
    }


    /*Create a thread for each region of the clipboard that will inform other
    clipboards when a change occurs*/
    int region[REGION_SIZE];            //FIXME later - not a very clean way of doing this but works fine
    for(int i=0;i<REGION_SIZE;i++){
        region[i]=i;
        //CHEESY CRITICAL REGION FOR localHandler
        pthread_mutex_lock(&setup_mutex);
        pthread_create(&localHandler,NULL,regionWatch,&region[i]);
    }

    //Create Signal handlers
    shutdown.sa_handler = shutDownClipboard;
    sigaction(SIGINT,&shutdown,NULL);
    sigaction(SIGPIPE,&shutdown,NULL);
    //sigaction(SIGSEGV,&shutdown,NULL);  //FIXME - SHOULD WE?


    //Create Local Socket for local clients
    sock = createSocket(AF_UNIX,SOCK_STREAM);


    //Bind Local socket
    //FIXME criar um nome para o socket, alterar isto só mm no fim! Para podermos testar à vontade
    char * delete_me = malloc(100);
    char * pid = malloc(20);
    snprintf (pid, 40, "%d",getpid());
    strcpy(delete_me, SOCK_LOCAL_ADDR);
    strcat(delete_me,pid);
    printf("[main] socket in %s\n",delete_me);
    UnixServerSocket(sock,delete_me,5);


    // Handle Local clients
    while(1){
        printf("Ready to accept \n");
        if((*client = accept(sock, NULL, NULL)) == -1) {
            perror("accept");
            exit(-1);
        }
        //CHEESY CRITICAL REGION FOR localHandler
        pthread_mutex_lock(&setup_mutex);
        if(pthread_create(&localHandler, NULL, handleLocalClient, client) != 0){
            printf("Creating thread");
            exit(-1);
        }

    }
    return 0;
}


