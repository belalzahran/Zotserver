#include "server.h"
#include "protocol.h"
#include "helpers.h"
#include "pollarray.h"
#include "linkedlist.h"
#include "sbuf.h"
#include "myhelpers.h"



void *workerThread(void *vargp)
{
    pthread_detach(pthread_self());
    petrV_header *receivedHeader = malloc(sizeof(petrV_header)); 
    //petrV_header *sendingHeader = malloc(sizeof(petrV_header)); 

    while(1)
    {
        int connfd = sbuf_remove();
        //printf("\tconnfd from the buff remove: %d\n", connfd);
        //printf("\tWorker thread began processing pending client connection from the buffer...\n");
        char* msgBody = GetMessage(connfd, receivedHeader);

        if(msgBody == NULL)
        {
            printf("\tFailed to get message from client\n");
            break;
        }


    //    1 - check to see if user exists -> add to linked list
        if (!userExists(msgBody))
        {
            insertUser(msgBody,connfd,getpid(),0);
            fprintf(logFile,"CONNECTED %s\n",msgBody);
            updateCurrentStats(1,1,0);
            receivedHeader->msg_type = 0x00;
            memset(msgBody, 0, receivedHeader->msg_len);
        }
        else if(userExists(msgBody))
        {
            updateUser(msgBody,connfd,getpid(),0);
            fprintf(logFile,"RECONNECTED %s\n",msgBody);
            updateCurrentStats(1,1,0);
        }
        else
        {
            //unsuccesful log in
            fprintf(logFile,"REJECT %s\n",msgBody);
            updateCurrentStats(1,0,0);
            close(connfd);
            exit(1);
        }

        printUserList();


        if (wr_msg(connfd, receivedHeader, msgBody) < 0)
        {
            printf("Sending failed\n");
            free(msgBody);
            break;
        }
        sleep(10);
        printf("end of loop\n");

        free(msgBody);
        close(connfd);
    }

    free(receivedHeader);
    return NULL;
}



int main(int argc, char *argv[]) {

    int opt, listenfd;
    while ((opt = getopt(argc, argv, "h")) != -1) {
        switch (opt) {
            case 'h':
                fprintf(stderr, USAGE_MSG);
                exit(EXIT_FAILURE);
        }
    }

    // 3 positional arguments necessary
    if (argc != 4) {
        fprintf(stderr, USAGE_MSG);
        exit(EXIT_FAILURE);
    }
    unsigned int port_number = atoi(argv[1]);
    char * poll_filename = argv[2];
    char * log_filename = argv[3];


    // Creating the listening Socket for the Server //////////////////////////
    listenfd = open_listenfd(port_number);

    // Initializing the Poll statistics struct //////////////////////////
    curStats.clientCnt = 0;
    curStats.threadCnt = 0;
    curStats.totalVotes = 0;

    // reading in the Polls data  //////////////////////////
    FILE *pollFile;
    pollFile = fopen(poll_filename, "r");  
    if (pollFile == NULL) 
    {
        printf("Error opening the file.\n");
        exit(2); // Exit the program with an error code
    }
    readInPollInfo(pollFile);

    // opening LOG_FILENAME //////////////////////////

    
    logFile = fopen(log_filename, "w");
    if (logFile == NULL) 
    {
        printf("Error opening the file.\n");
        exit(2); // Exit the program with an error code
    }

    // Initializing the linked list for the clients/users //////////////////////////

    userListHead = NULL;

    // Create and initialize synchronization locks

    Sem_init(&curStatsMutex,0, 1);

    Sem_init(&userMutexRead,0,1);
    Sem_init(&userMutexWrite,0,1);
    userListReadCount = 0;

    Sem_init(&pollArrayMutex,0,1);

    Sem_init(&votingLogMutex,0,1);

    // INSTALLING THE SIGNAL AHNDER FOR SIGINT

    struct sigaction myaction = {0};
    myaction.sa_handler = sigint_handler;
    if (sigaction(SIGINT, &myaction, NULL) == -1) 
    {
        printf("Signal handler failed to install.\n");
    }

    printf("Server initialized with %d polls.\n", numOfPolls);
    printf("Currently listening on port %d.\n\n",port_number);
// *************************************************************************************************************************************************
// FINISHED SERVER INITIALIZATION
// *************************************************************************************************************************************************
   
   
    socklen_t clientlen;
    int connfd;
    struct sockaddr_in clientaddr;
    pthread_t tid;
    sbuf = malloc(sizeof(sbuf_t));
    if (!sbuf) 
    {
        fprintf(stderr, "Failed to allocate memory for sbuf\n");
        exit(1);
    }
    sbuf_init(4);
    int i;
    for(i = 0; i < maxNumOfThreads; i++)
        Pthread_create(&tid,NULL,workerThread,NULL);
    //printf("%d worker threads have been created.\n", maxNumOfThreads);

    //printf("Begin looping for new connections...\n");
    while(1)
    {
        clientlen = sizeof(struct sockaddr_storage);
        
        connfd = accept(listenfd, (SA *) &clientaddr, &clientlen);
        if (connfd < 0) {
            printf("server acccept failed.\n");
            exit(EXIT_FAILURE);
        }
        printf("\naccepted a client connection.\n");
        sbuf_insert(connfd);

    }
 
    return 0;
}
