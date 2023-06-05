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
sem_t userMutexRead, userMutexWrite;
int userListReadCount;

user_t* createUser(char* username, int socket_fd, pthread_t tid, uint32_t pollVotes) {
    user_t* newUser = (user_t*)malloc(sizeof(user_t));
    newUser->username = strdup(username); // Create a copy of the username
    newUser->socket_fd = socket_fd;
    newUser->tid = tid;
    newUser->pollVotes = pollVotes;
    newUser->next = NULL;
    return newUser;
}

void insertUser(user_t** head, char* username, int socket_fd, pthread_t tid, uint32_t pollVotes)
{
    P(&userMutexWrite);
    user_t* newUser = createUser(username, socket_fd, tid, pollVotes);
    newUser->next = *head;
    *head = newUser;
    V(&userMutexWrite);
}

void printUserList(user_t* head) 
{
    P(&userMutexRead);
    userListReadCount++;

    if(userListReadCount == 1)
        P(&userMutexWrite);

    V(&userMutexRead);
    
    user_t* ptr = head;
    while (ptr != NULL) {
        printf("Username: %s, Socket_fd: %d, Thread_id: %ld, PollVotes: %u\n", ptr->username, ptr->socket_fd, (long)ptr->tid, ptr->pollVotes);
        ptr = ptr->next;
    }

    P(&userMutexRead);
    userListReadCount--;

    if(userListReadCount == 0)
        V(&userMutexWrite);

    V(&userMutexRead);

}

void removeUser(user_t** head, char* username) {

    if (*head == NULL) return;

    P(&userMutexWrite);
    if (strcmp((*head)->username, username) == 0) {
        user_t* temp = *head;
        *head = (*head)->next;
        free(temp->username);
        free(temp);
        return;
    }
    user_t* current = *head;
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





#endif



