#ifndef MYHELPERS_H
#define MYHELPERS_H

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
#include "server.h"
#include "sbuf.h"



void SendOK(int connfd)
{
    printf("Testing: sent okay.\n");
    petrV_header responseHeader;  
    responseHeader.msg_type = OK;
    responseHeader.msg_len = 0;
    if (wr_msg(connfd, &responseHeader, NULL) < 0)
    {
        perror("Sending failed\n");
    }
}

void SendError(int connfd, enum msg_types responseEnum)
{
    printf("Testing: sent error.\n");
    petrV_header responseHeader;  
    responseHeader.msg_type = responseEnum;
    responseHeader.msg_len = 0;
    if (wr_msg(connfd, &responseHeader, NULL) < 0)
    {
        perror("Sending failed\n");
    }
}

char* GetMessage(int socket_fd, petrV_header* h)
{
    if(rd_msgheader(socket_fd, h) < 0)
    {
        printf("\tReceiving header failed\n");
        return NULL;
    }
    int msg_len = h->msg_len;

    // Allocate memory for the message body
    char* msgBody = malloc(msg_len + 1);
    if (!msgBody) // If malloc() failed
    {
        perror("malloc");
        return NULL;
    }
    // Read the message body from the socket
    ssize_t bytesRead = read(socket_fd, msgBody, msg_len);
    if (bytesRead < 0) // If read() failed
    {
        perror("read");
        free(msgBody);
        return NULL;
    }

    if (bytesRead < msg_len) // If not all of the message was read
    {
        printf("\tReceiving message body failed\n");
        free(msgBody);
        close(socket_fd);  // Add this line to close the connection if receiving message body fails
        return NULL;
    }

    // Null-terminate the message body
    msgBody[msg_len] = '\0';

    return msgBody;
}

int authorizeUser(petrV_header *receivedHeader, char * msgBody, int connfd, pthread_t tid) 
{

    switch(userExists(msgBody))
    {
        case 0: 
            // CONNECTED
            fprintf(logFile,"CONNECTED %s\n",msgBody);
            printf("CONNECTED %s\n",msgBody);
            insertUser(msgBody,connfd,tid,0);
            updateCurrentStats(1,1,0);
            SendOK(connfd);
            break;

        case 1: 
            // RECONNECTED
            fprintf(logFile,"RECONNECTED %s\n",msgBody);
            printf("RECONNECTED %s\n",msgBody);
            updateUser(msgBody,connfd,tid,getPollVotesVec(msgBody));
            updateCurrentStats(1,1,0);
            SendOK(connfd);
            break;

        case 2: 
            // ERROR
            fprintf(logFile,"REJECT %s\n",msgBody);
            updateCurrentStats(1,0,0);
            SendError(connfd, EUSRLGDIN);
            close(connfd);
            return -1;
        default: 

            break;
    }


    return 0;
}

void printBits(uint32_t num) {
    int size = sizeof(uint32_t) * 8; // number of bits in type
    for(int i = size-1; i >= 0; i--) 
    {
        int bit = (num >> i) & 1;
        printf("%d", bit);
    }
    printf("\n");
}

bool checkVote(int pollIndex, uint32_t pollVoteVector) 
{
    printf("checking to see if poll %d was voted on: ", pollIndex);
    printBits(pollVoteVector);
    return ((1 << pollIndex) & pollVoteVector) != 0;
}

uint32_t markVote(int pollIndex, uint32_t pollVoteVector) 
{
    // Shift a 1 to the left by pollIndex bits, then OR with pollVoteVector.
    // This will set the bit at pollIndex in pollVoteVector to 1.
    printf("Marking poll #%d as voted!\n",pollIndex);

    return pollVoteVector | (1 << pollIndex);
}

int verifyUserVote (const int pollIndex, const int optionIndex, uint32_t pollVoteVector)
{
    if (pollIndex > (numOfPolls - 1) || pollIndex < 0)
        return 1;
    printf("poll index was valid\n");
    
    P(&pollArrayMutex);
    if (pollArray[pollIndex].options[optionIndex].text == NULL || optionIndex > 3)
    {
        V(&pollArrayMutex);
        return 2;
    }
    V(&pollArrayMutex);

    printf("poll option index was valid\n");
    

    if (checkVote(pollIndex,pollVoteVector))
        return 3;
    printf("poll was not voted on before\n");

    return 4;
}



#endif
