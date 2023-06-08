#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include "helpers.h"



typedef struct user_t{
    char* username;	// Pointer to dynamically allocated string
    int socket_fd;		// >= 0 if connection active on server, set to -1 if not active
    pthread_t tid;		// Current thread id, only if active on server
    uint32_t pollVotes;	// Bit vector storage for polls that user has voted on
				// Only updated when the user logout/disconnect from server
    struct user_t* next;
} user_t;

user_t* userListHead = NULL;
sem_t userMutex, userMutexWrite;
int userListReadCount;



void insertUser(char* username, int socket_fd, pthread_t tid, uint32_t pollVotes)
{
    P(&userMutexWrite);
    user_t* newUser = (user_t*)malloc(sizeof(user_t));
    newUser->username = strdup(username); // Create a copy of the username
    newUser->socket_fd = socket_fd;
    newUser->tid = tid;
    newUser->pollVotes = pollVotes;
    newUser->next = userListHead;
    userListHead = newUser;
    V(&userMutexWrite);
}

void updateUser(char* username, int socket_fd, pthread_t tid, uint32_t pollVotes) 
{
    P(&userMutexWrite);
    printf("Updating %s's info...\n",username);

    user_t* ptr = userListHead;
    while(ptr != NULL) 
    {
        if(strcmp(ptr->username, username) == 0) 
        {
            if (socket_fd != 1) ptr->socket_fd = socket_fd;
            if (tid != (pthread_t)-1) ptr->tid = tid;
            if (pollVotes != (uint32_t)-1) ptr->pollVotes = pollVotes;
            break;
        }
        ptr = ptr->next;
    }

    V(&userMutexWrite);
}

void removeUser(char* username) {

    if (userListHead == NULL) return;

    P(&userMutexWrite);
    if (strcmp(userListHead->username, username) == 0) {
        user_t* temp = userListHead;
        userListHead = userListHead->next;
        free(temp->username);
        free(temp);
        V(&userMutexWrite);
        return;
    }
    user_t* current = userListHead;
    while (current->next != NULL && strcmp(current->next->username, username) != 0) {
        current = current->next;
    }
    if (current->next != NULL) {
        user_t* temp = current->next;
        current->next = current->next->next;
        free(temp->username);
        free(temp);
    }
    V(&userMutexWrite);
}

// 0 - does not exist
// 1 - exists and is not active -1
// 2 - exists and is active ERROR
int userExists(char* keyName)
{
    int result = 0;

    P(&userMutex);
    userListReadCount++;

    if(userListReadCount == 1)
        P(&userMutexWrite);

    V(&userMutex);

    user_t* ptr = userListHead;
    while (ptr != NULL) 
    {
        if (strcmp(ptr->username, keyName) == 0)
        { // If the user exists
            if(ptr->socket_fd == -1)
            {
                result = 1;
            }
            else
            {
                result = 2;
            }

            P(&userMutex);
            userListReadCount--;

            if(userListReadCount == 0)
                V(&userMutexWrite);

            V(&userMutex);

            return result;
        }
        ptr = ptr->next;
    }

    P(&userMutex);
    userListReadCount--;

    if(userListReadCount == 0)
        V(&userMutexWrite);

    V(&userMutex);

    return 0; // If the user does not exist
}

void printUserList() 
{
    P(&userMutex);
    userListReadCount++;

    if(userListReadCount == 1)
        P(&userMutexWrite);

    V(&userMutex);
    
    user_t* ptr = userListHead;
    while (ptr != NULL) {
        printf("Username: %s, Socket_fd: %d, Thread_id: %ld, PollVotes: %u\n", ptr->username, ptr->socket_fd, (long)ptr->tid, ptr->pollVotes);
        ptr = ptr->next;
    }

    P(&userMutex);
    userListReadCount--;

    if(userListReadCount == 0)
        V(&userMutexWrite);

    V(&userMutex);

}



#endif



