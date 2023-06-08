#ifndef SERVER_H
#define SERVER_H

#include <arpa/inet.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include "linkedlist.h"
#include "helpers.h"
#include "protocol.h"
#include "pollarray.h"
#include <stdbool.h>



#define USAGE_MSG "./bin/zoteVote_server [-h] PORT_NUMBER POLL_FILENAME LOG_FILENAME"\
                  "\n  -h                 Displays this help menu and returns EXIT_SUCCESS."\
                  "\n  PORT_NUMBER        Port number to listen on."\
                  "\n  POLL_FILENAME      File to read poll information from at the start of the server"\
                  "\n  LOG_FILENAME       File to output server actions into. Create/overwrite, if exists\n"

typedef struct {
    int clientCnt;  // # of attempted logins (successful and unsuccessful) 
    int threadCnt;  // # of threads created (successful login)
    int totalVotes; // # of votes placed on any poll - updated by all client threads
} stats_t;   // Stats collected since server start-up


stats_t curStats;  // Global variable
sem_t curStatsMutex;  // Global Variable

void updateCurrentStats(int addclientCnt, int addthreadCnt, int addtotalVotes)
{
    P(&curStatsMutex);
    curStats.clientCnt += addclientCnt;
    curStats.threadCnt += addthreadCnt;
    curStats.totalVotes += addtotalVotes;
    V(&curStatsMutex);
}





FILE *logFile;
sem_t votingLogMutex;
volatile sig_atomic_t sigint_received = 0;

int maxNumOfThreads = 3;





// INSERT FUNCTIONS HERE
void handle_SIGINT() {
   

    // Lock the mutex before accessing the user list
    P(&userMutex);
    userListReadCount++;

    if(userListReadCount == 1)
        P(&userMutexWrite);

    V(&userMutex);

    // Iterate over each client
    user_t* ptr = userListHead;
    while (ptr != NULL) {
        // Send SIGINT to the thread handling this client
        pthread_kill(ptr->tid, SIGINT);
        
        // Close the client socket
        close(ptr->socket_fd);

        // Wait for this thread to terminate
        pthread_join(ptr->tid, NULL);

        // Move to the next client
        ptr = ptr->next;
    }

    // Unlock the mutex
    P(&userMutex);
    userListReadCount--;

    if(userListReadCount == 0)
        V(&userMutexWrite);

    V(&userMutex);

    // ADD CODE TO OUTPUT THE STATES OF THE POLLS TO FILE

    // OUTPUT the user linked list info to STDERR

    // output curSTTATS values to STDERRR 

    // Exit the process
    printf("Signal has been handled. Terminating...\n");
    exit(0);
}




#endif
