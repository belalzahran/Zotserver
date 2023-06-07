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
        return NULL;
    }

    // Null-terminate the message body
    msgBody[msg_len] = '\0';

    return msgBody;
}

#endif
