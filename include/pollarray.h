#ifndef POLLARRAY_H
#define POLLARRAY_H

#include "server.h"
#include "helpers.h"

sem_t pollArrayMutex;
int numOfPolls;


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


void print_poll_array() {
    P(&pollArrayMutex);
    for (int i = 0; i < 32; i++) {
        if (pollArray[i].question != NULL) {
            printf("%s;", pollArray[i].question);
            for (int j = 0; j < 4; j++) {
                if (pollArray[i].options[j].text != NULL) {
                    printf("%d;%s", pollArray[i].options[j].voteCnt, pollArray[i].options[j].text);
                    if (j != 3 && pollArray[i].options[j+1].text != NULL) {
                        printf(",");
                    }
                }
            }
            printf("\n");
        }
    }
    V(&pollArrayMutex);
}
char* str_split(char* a_str, const char a_delim){
    static char* input_string;
    if(a_str != NULL)
        input_string = a_str;
        
    if(input_string == NULL)
        return NULL;
        
    char* part = strchr(input_string, a_delim);
    char* ret_str = input_string;
    
    if(part != NULL){
        *part = '\0';
        input_string = part+1;
    }else{
        input_string = NULL;
    }
    
    return ret_str;
}

void readInPollInfo(FILE *pollFile){
    char buffer[1024];
    int pollArrayIndex = 0;

    while(fgets(buffer, sizeof(buffer), pollFile)){
        buffer[strcspn(buffer, "\n")] = 0;  // remove the newline

        // Allocate memory for the question
        char* question = (char*) malloc(100 * sizeof(char));

        // Get the question and the number of choices
        char* temp = str_split(buffer, ';');
        strcpy(question, temp);

        int numOfChoices = atoi(str_split(NULL, ';'));

        pollArray[pollArrayIndex].question = question;

        // Load the choices
        for(int i = 0; i < numOfChoices; i++){
            // Allocate memory for the choice text
            char* choiceText = (char*) malloc(50 * sizeof(char));

            temp = str_split(NULL, ';');
            char* voteCountStr = strchr(temp, ',');

            // Split the choice text and the vote count
            if(voteCountStr != NULL){
                *voteCountStr = '\0';
                voteCountStr++;
            }

            // Copy the choice text and vote count
            strcpy(choiceText, temp);
            int voteCount = atoi(voteCountStr);

            pollArray[pollArrayIndex].options[i].text = choiceText;
            pollArray[pollArrayIndex].options[i].voteCnt = voteCount;
        }

        // Mark unused choices
        for(int i = numOfChoices; i < 4; i++){
            pollArray[pollArrayIndex].options[i].text = NULL;
        }

        pollArrayIndex++;
    }

    numOfPolls = pollArrayIndex;
}



char* getPollString(int index){
    if(index >= numOfPolls)
        return NULL;

    char *poll = (char *) malloc(2000 * sizeof(char)); 
    if(poll == NULL){
        //Handle error
    }
    
    sprintf(poll, "Poll %d - %s - ", index, pollArray[index].question);
    
    int optionNum = 0;
    while(pollArray[index].options[optionNum].text != NULL && optionNum < 4){
        char temp[100]; // temporary string for each option, adjust size as needed
        sprintf(temp, "%d:%s, ", optionNum,pollArray[index].options[optionNum].text);
        strcat(poll, temp);
        optionNum++;
    }   

    // Replace last ";" with "\0" to mark end of string
    poll[strlen(poll)-2] = '\0';
    
    return poll;
}

char* returnCombinedPollStrings() 
{
    P(&pollArrayMutex);
    char *combinedPolls = (char *) malloc(10000 * sizeof(char));
    if(combinedPolls == NULL) {
        // Handle error
    }
    combinedPolls[0] = '\0'; // make it an empty string

    int index = 0;
    while(index < numOfPolls && index < 32)
    {
        char *pollString = getPollString(index);
        if (pollString == NULL) {
            continue;  // Skip if pollString is NULL
        }

        // Check if adding the new pollString would overflow the buffer
        if (strlen(combinedPolls) + strlen(pollString) + 2 > 10000) {  // +2 for newline and null terminator
            // Handle overflow
            free(pollString);
            break;
        }
        strcat(combinedPolls, pollString);
        strcat(combinedPolls, "\n");  // Add newline between polls
        free(pollString);
        index++;
    }
    combinedPolls[strlen(combinedPolls)-1] = '\0';  // Replace last newline with null terminator

    V(&pollArrayMutex);

    return combinedPolls;
}

#endif



