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
#include "helpers.h"



#define USAGE_MSG "./bin/zoteVote_server [-h] PORT_NUMBER POLL_FILENAME LOG_FILENAME"\
                  "\n  -h                 Displays this help menu and returns EXIT_SUCCESS."\
                  "\n  PORT_NUMBER        Port number to listen on."\
                  "\n  POLL_FILENAME      File to read poll information from at the start of the server"\
                  "\n  LOG_FILENAME       File to output server actions into. Create/overwrite, if exists\n"

typedef struct {
    char* username;	// Pointer to dynamically allocated string
    int socket_fd;		// >= 0 if connection active on server, set to -1 if not active
    pthread_t tid;		// Current thread id, only if active on server
    uint32_t pollVotes;	// Bit vector storage for polls that user has voted on
				// Only updated when the user logout/disconnect from server
} user_t;

typedef struct {
    int clientCnt;  // # of attempted logins (successful and unsuccessful) 
    int threadCnt;  // # of threads created (successful login)
    int totalVotes; // # of votes placed on any poll - updated by all client threads
} stats_t;   // Stats collected since server start-up

typedef struct {
    char* text;	// Pointer to dynamically allocated string
    int voteCnt;  // Count for the choice
} choice_t;

typedef struct {
    char* question;         // Pointer to dynamically allocated string
    choice_t options[4];    // At most 4 options per poll. Stored low to high index.
                            // choice_t text pointer set to NULL if the option is not used. 
} poll_t; 

poll_t pollArray[32]; // Global variable
                      // One poll per index, stored lowest to highest index.  
                      // Set question pointer to NULL if poll does not exist
                      // Maximum of 32 polls on the server.

stats_t curStats;  // Global variable
stats_t* serverStatsPtr; // Global Variables
sem_t statsMutex;  // Global Variable



// INSERT FUNCTIONS HERE







// void handler(int sig)
// {
//     pid_t pid;
//     int stat;
//     while ((pid = waitpid(-1, &stat, WNOHANG)) > 0);

//     return;
//  }

// // funciton that will run when working thread is called with file descriptor
// void *thread(void *vargp)
// {
//     int connfd = *((int *)vargp);
    
//     Pthread_detach(pthread_self());
//     free(vargp);
    
//     //echo_r(connfd); /* reentrant version of echo() */
//     Close(connfd);
//     return NULL;
// }



void readInPollInfo(char* poll_filename)
{
     FILE *pollFile;
    pollFile = fopen(poll_filename, "r");

    if (pollFile == NULL) 
    {
        printf("Error opening the file.\n");
        exit(1); // Exit the program with an error code
    }

    char buffer[1024];
    int pollArrayIndex = 0;
    int numOfChoices,choiceNum,voteCount = 0;
    

    while((fgets(buffer, 1024, pollFile)))
    {
        //printf("string: %s\n",buffer);

        buffer[strcspn(buffer, "\n")] = '\0';
        char *question = malloc(sizeof(char) * 100);
        getQuestion(buffer, question, &numOfChoices);
        pollArray[pollArrayIndex].question = question;

        printf("The question: %s\n",question);
        printf("The num Of choices: %d\n",numOfChoices);

        for(int i = 0; i < 4; i++) 
        {
            if (i >= numOfChoices) {
                pollArray[pollArrayIndex].options[i].text = NULL;
                continue;
            }
            else
            {
                char *choiceString = malloc(sizeof(char) * 50);
                int voteCount;
                getChoiceStringAndVoteCnt(buffer, numOfChoices, i, choiceString, &voteCount);
                printf("choice string is: %s, and voteCount is: %d\n", choiceString,voteCount);

                pollArray[pollArrayIndex].options[i].text = choiceString;
                pollArray[pollArrayIndex].options[i].voteCnt = voteCount;
            }

            
        }

        pollArrayIndex++;
    }
}

void getQuestion(char *buffer, char* question, int *numOfChoices)
{
    int i;

    for(i = 0; i < strlen(buffer);i++)
    {
        if(buffer[i] == ';')
            break;
    }
    
    strncpy(question, buffer, i);
    question[i] = '\0';
    *numOfChoices = buffer[i+1] - '0';
     
}



void getChoiceStringAndVoteCnt(char *buffer, int numOfChoices, int choiceNum, char* choiceString, int *voteCount) {
    // strtok modifies the input string, so we copy buffer to a temporary string
    char temp[strlen(buffer) + 1];
    strcpy(temp, buffer);

    // skip over the question
    strtok(temp, ";");
    // skip over the number of choices
    strtok(NULL, ";");

    char *token = NULL;
    for (int i = 0; i <= choiceNum; i++) {
        token = strtok(NULL, ";");
        if (token == NULL) {
            // Not enough choices in the string
            printf("Error: Not enough choices in the string.\n");
            return;
        }
    }

    // split the choice into choiceString and voteCount
    char *commaPos = strchr(token, ',');
    if (commaPos == NULL) {
        // Malformed choice
        printf("Error: Malformed choice.\n");
        return;
    }

    // copy the part before the comma to choiceString
    strncpy(choiceString, token, commaPos - token);
    // add the null terminator manually
    choiceString[commaPos - token] = '\0';

    // convert the part after the comma to an integer
    *voteCount = atoi(commaPos + 1);
}




#endif
