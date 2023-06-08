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


// function to get the message from the client
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
            // VALID SYNTAX AND NEW USER
            insertUser(msgBody,connfd,tid,0);
            fprintf(logFile,"CONNECTED %s\n",msgBody);
            updateCurrentStats(1,1,0);
            receivedHeader->msg_type = 0x00;
            receivedHeader->msg_len = 0; // No body content
            break;

        case 1: 
            // user name exists and is inactive
            updateUser(msgBody,connfd,tid,0);
            fprintf(logFile,"RECONNECTED %s\n",msgBody);
            updateCurrentStats(1,1,0);
            receivedHeader->msg_type = 0x00;
            receivedHeader->msg_len = 0; // No body content
            break;

        case 2: 
            // user name exists and active: ERROR
            fprintf(logFile,"REJECT %s\n",msgBody);
            receivedHeader->msg_type = EUSRLGDIN;
            receivedHeader->msg_len = 0; // No body content
            if (wr_msg(connfd, receivedHeader, NULL) < 0)
            {
                printf("Sending failed\n");
                free(msgBody);;
            }
            close(connfd);
            return -1;
        default: 
            printf("USER Exists error\n");
    }

    if (wr_msg(connfd, receivedHeader, NULL) < 0) // Pass NULL for msgBody
    {
        printf("Sending failed\n");
        free(msgBody);
    }

    return 0;

}



#endif
