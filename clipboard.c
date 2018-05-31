#include "header.h"


int main(int argc, char** argv) {
    //Global variable initialization
    head = iniLista();
    pthread_mutex_init ( &setup_mutex, NULL);
    pthread_mutex_init ( &list_mutex, NULL);
    for(int i=0;i<REGION_SIZE;i++){
        pthread_mutex_init ( &mutex[i], NULL);
        pthread_mutex_init(&waitingList_mutex[i],NULL);
    }

    //Local variables
    pthread_t localHandler;
    int opt,sock;
    struct sigaction shutdown;
    int * client = malloc(sizeof(int));

    //Clipboard setup
    for(int i=0;i<REGION_SIZE;i++){
        clipboard[i].size =1 ;              //We'll just have to live with this
        clipboard[i].payload = calloc(1,clipboard[i].size);   //Don't fill payload with anything!
        clipboard[i].hash[0] = '\0';
        waitingLists[i]=iniLista();
    }

    //handle command line arguments  //FIXME should handle errors
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
            printf("[main] Error creating node\n");
        }

        //Receives current clipboard from parent
        ClipSync(parent_sock_write);

        //Creates clipboard hub to allow connections from other clipboards
        if(pthread_create(&clipboard_hub, NULL, ClipHub,NULL) != 0){
            perror("[main]Creating thread\n");
            exit(-1);
        }

        //Create Thread that handles comunication with the parent
        parentArg->id = 0;
        parentArg->isParent = true;
        parentArg->sock = parent_sock_read;
        pthread_mutex_lock(&passingArgumentsToHandleClip);
        if(pthread_create(&parent_connection, NULL, handleClipboard, parentArg) != 0){
            perror("[main] Creating thread\n");
            exit(-1);
        }
    }
    else{
        //In case clipboard is launched in local mode
        //Creates clipboard hub to allow connections from other clipboards
        if(pthread_create(&clipboard_hub, NULL, ClipHub, NULL) != 0){
            perror("[main] Creating thread\n");
            exit(-1);
        }
    }

    //Randomize for hash generation
    srand(getpid());


    /*Create a thread for each region of the clipboard that will inform other
    clipboards when a change occurs*/
    int region[REGION_SIZE];            //FIXME - not a very clean way of doing this but works fine
    for(int i=0;i<REGION_SIZE;i++){
        region[i]=i;
        //CHEESY CRITICAL REGION FOR localHandler
        pthread_mutex_lock(&setup_mutex);
        pthread_create(&localHandler,NULL,regionWatch,&region[i]);
    }

    //Create Signal handlers
    shutdown.sa_handler = shutDownClipboard;
    sigemptyset(&shutdown.sa_mask);
    sigaction(SIGINT,&shutdown,NULL);   //FIXME check if it worked
    signal(SIGPIPE,SIG_IGN);
    //sigaction(SIGSEGV,&shutdown,NULL);  //FIXME - SHOULD WE?


    //Create Local Socket for local clients
    sock = createSocket(AF_UNIX,SOCK_STREAM);


    //Bind Local socket
    UnixServerSocket(sock,SOCK_LOCAL_ADDR,5);


    // Handle Local clients
    while(1){
        printf("[main] Ready to accept \n");
        if((*client = accept(sock, NULL, NULL)) == -1) {
            perror("[main] accept");
            exit(-1);
        }
        //CHEESY CRITICAL REGION FOR localHandler
        pthread_mutex_lock(&setup_mutex);
        if(pthread_create(&localHandler, NULL, handleLocalClient, client) != 0){
            printf("[main] Creating thread");
            exit(-1);
        }

    }
    return 0;
}


