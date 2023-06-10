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
    char *username;
    //petrV_header *sendingHeader = malloc(sizeof(petrV_header)); 

    while(1)
    {
        char userName[25];
        int connfd = sbuf_remove();
        if (sigint_received) {
            // Handle SIGINT: clean up and exit
            close(connfd);
            pthread_exit(NULL);
        }
 
        char* msgBody = GetMessage(connfd, receivedHeader);

        if(msgBody == NULL)
        {
            printf("\tFailed to get message from client\n");
            break;
        }

        if (authorizeUser(receivedHeader, msgBody, connfd, pthread_self()) < 0)
        {
            continue;
        }

        printUserList();
        strncpy(userName, msgBody, sizeof(userName) - 1);
        userName[sizeof(userName) - 1] = '\0'; // Ensure null-termination

        uint32_t userPollVotesVec = getPollVotesVec(userName);

        do 
        {

            free(msgBody);
            msgBody = GetMessage(connfd, receivedHeader);
            petrV_header responseHeader;

            switch (receivedHeader->msg_type)
            {
                case OK:
                case LOGIN:
                case PLIST:

                        fprintf(logFile,"%s PLIST\n", userName);
                        responseHeader.msg_type = PLIST;
                        char *pollList = returnCombinedPollStrings();
                        responseHeader.msg_len = strlen(pollList) + 1; // adjust this according to how you calculate length
                        wr_msg(connfd, &responseHeader, pollList);
                        break;


                case STATS:

                        // fprintf(logFile,"%s STATS %d\n", userName, userPollVotesVec);
                        // responseHeader.msg_type = PLIST;
                        // responseHeader.msg_len = 0;
                        
                        // // Send the OK message back to the client
                        // wr_msg(connfd, &responseHeader, NULL);
                        break;

                case VOTE:
                    //receivedHeader->
                    
                    //userPollVotesVec
                    // fprintf(logFile,"%s VOTE %d %d %d\n", userName, pollIndex, choiceNum, pollVotes);
                    // responseHeader.msg_type = PLIST;
                    // char *pollList = returnCombinedPollStrings();
                    // responseHeader.msg_len = strlen(pollList) + 1; // adjust this according to how you calculate length
                    // wr_msg(connfd, &responseHeader, pollList);
                    break;

                case LOGOUT:
                    // Assume updateUser is a function that marks the user as inactive.
                        
                        fprintf(logFile,"%s LOGOUT\n",userName);

                        // include update for poll votes vector
                        updateUser(userName,-1,-1,-1);
                        
                        // Create an OK message header
                        petrV_header responseHeader;
                        responseHeader.msg_len = 0;
                        responseHeader.msg_type = OK;
                        // Send the OK message back to the client
                        wr_msg(connfd, &responseHeader, NULL);
                            
                    
                        break;
                
                default:
                    // Unrecognized message type. You might want to add some error handling here.
                    break;
            }
        } while (receivedHeader->msg_type != 0x2);

        printf("no more server code to handle client\n");

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

    Sem_init(&userMutex,0,1);
    Sem_init(&userMutexWrite,0,1);
    userListReadCount = 0;

    Sem_init(&pollArrayMutex,0,1);

    Sem_init(&votingLogMutex,0,1);

    // INSTALLING THE SIGNAL AHNDER FOR SIGINT

    struct sigaction myaction = {0};
    myaction.sa_handler = handle_SIGINT;
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
    while(1) {

        clientlen = sizeof(struct sockaddr_storage);

        connfd = accept(listenfd, (SA *) &clientaddr, &clientlen);
        if (connfd < 0) {
            if (errno == EINTR && sigint_received) 
            {
                close(listenfd);
                handle_SIGINT();
                break;
            }
            else
            {
                perror("accept");
                continue;
            }
        }
        printf("\naccepted a client connection.\n");
        sbuf_insert(connfd);
    }


    // ...
    return 0;
}