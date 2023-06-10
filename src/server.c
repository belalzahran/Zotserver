#include "server.h"
#include "protocol.h"
#include "helpers.h"
#include "pollarray.h"
#include "linkedlist.h"
#include "sbuf.h"
#include "myhelpers.h"


void *workerThread(void *vargp)
{
    // detach thread from main thread
    pthread_detach(pthread_self());
    
    // initialize header to process client requests
    petrV_header *receivedHeader = malloc(sizeof(petrV_header)); 

    // intialize a header to for responding to client
    petrV_header responseHeader;  

    while(1)
    {
        // wait to obtain new client connection
        int connfd = sbuf_remove();

        // Check for SIGINT signal
        if (sigint_received) {close(connfd);pthread_exit(NULL);}
 
        // get LOGIN request from user
        char* msgBody = GetMessage(connfd, receivedHeader);
        if(msgBody == NULL){printf("\tFailed to get message from client\n");break;}

        // authorize user -  If LOGIN fails, close connection
        if (authorizeUser(receivedHeader, msgBody, connfd, pthread_self()) < 0){continue;}


        // initialize copy of clients username
        char userName[25];
        strncpy(userName, msgBody, sizeof(userName) - 1);
        userName[sizeof(userName) - 1] = '\0'; // Ensure null-termination

        // initialize copy of current user's poll voting vector
        uint32_t userPollVotesVec = getPollVotesVec(userName);

        do // loop to handle clients requests
        {
            // free previous MSG and obtain new MSG
            free(msgBody);
            msgBody = GetMessage(connfd, receivedHeader);
            int pollIndex;
            int optionIndex;
            int votingRespnseType;
            char *pollList;

            // switch based on the type of MSG
            switch (receivedHeader->msg_type)
            {
                case OK:
                case LOGIN:
                case PLIST:

                        pollList = returnCombinedPollStrings();
                        responseHeader.msg_type = PLIST;
                        responseHeader.msg_len = strlen(pollList) + 1; // adjust this according to how you calculate length
                        wr_msg(connfd, &responseHeader, pollList);
                        fprintf(logFile,"%s PLIST\n", userName);
                    break;


                case STATS:

                        // fprintf(logFile,"%s STATS %d\n", userName, userPollVotesVec);
                        // responseHeader.msg_type = PLIST;
                        // responseHeader.msg_len = 0;
                        
                        // // Send the OK message back to the client
                        // wr_msg(connfd, &responseHeader, NULL);
                        break;

                case VOTE:
            
                    sscanf(msgBody, "%d %d", &pollIndex, &optionIndex);
                    votingRespnseType = verifyUserVote(pollIndex, optionIndex, userPollVotesVec);
                    printf("voting response type: %d\n", votingRespnseType);

                        switch (votingRespnseType)
                        {
                            case 1: 
                                    SendError(connfd, EPNOTFOUND);
                                    break;
                            case 2: 
                                    SendError(connfd, ECNOTFOUND);
                                    break;
                            case 3:
                                    SendError(connfd, EPDENIED);
                                    break;
                            case 4:
                                    userPollVotesVec = markVote(pollIndex, userPollVotesVec);
                                    updateUser(userName, 1,-1,userPollVotesVec);
                                    updateCurrentStats(0,0,1);
                                    pollArray[pollIndex].options[optionIndex].voteCnt++;
                                    SendOK(connfd);
                                    break;
                        }

                break;

                case LOGOUT:
                        // mark user inactive and update votes
                        updateUser(userName,-1,-1,userPollVotesVec);
                        fprintf(logFile,"%s LOGOUT\n",userName);   
                        SendOK(connfd);
                        break;
                
                default:
                    // Unrecognized message type. You might want to add some error handling here.
                    break;
            }

        } while (receivedHeader->msg_type != 0x2);

        printf("Finished processing current client.\n");

        // free last MSG and close connection file to prepare for new client connection
        free(msgBody);
        close(connfd);
    }

    free(receivedHeader);
    return NULL;
}



int main(int argc, char *argv[]) 
{

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

   
    socklen_t clientlen;
    int connfd;
    struct sockaddr_in clientaddr;
    pthread_t tid;
    mainThreadID = pthread_self();
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
   
    while(1) {

        clientlen = sizeof(struct sockaddr_storage);

        connfd = accept(listenfd, (SA *) &clientaddr, &clientlen);
        if (connfd < 0) {
            if (errno == EINTR && sigint_received) 
            {
                close(listenfd);
                handle_SIGINT(connfd);
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