#ifndef SBUF_H
#define SBUF_H

#include "helpers.h"


typedef struct {
    int *buf;
    int n;
    int front;
    int rear;
    sem_t mutex;
    sem_t slots;
    sem_t items;

} sbuf_t;

sbuf_t *sbuf;
char buffer[BUFFER_SIZE];


void printsbuf()
{
    P(&sbuf->mutex);
    printf("front: %d______", sbuf->front);
    printf("rear: %d\n", sbuf->rear);
    V(&sbuf->mutex);
}

void sbuf_init(int n)
{
    //printf("sbuf_init\n");
    sbuf->buf = calloc(n, sizeof(int));
    if (!sbuf->buf) 
    {
        fprintf(stderr, "Failed to allocate memory for buffer\n");
        return;
    }
    sbuf->n = n;
    sbuf->front = sbuf->rear = 0;
    Sem_init(&sbuf->mutex,0,1);
    Sem_init(&sbuf->slots,0,n);
    Sem_init(&sbuf->items,0,0);
    //printsbuf();

}



void sbuf_deinit()
{
    free(sbuf->buf);
    sem_destroy(&sbuf->mutex);
    sem_destroy(&sbuf->slots);
    sem_destroy(&sbuf->items);
    free(sbuf);
}


void sbuf_insert(int item)
{
    P(&sbuf->slots);
    P(&sbuf->mutex);
    sbuf->buf[sbuf->rear] = item;
    sbuf->rear = (sbuf->rear + 1) % (sbuf->n);  
    V(&sbuf->mutex);
    V(&sbuf->items);
    //printf("After Insert...");
    //printsbuf();  
}


int sbuf_remove()
{
    int item;
    P(&sbuf->items);
    P(&sbuf->mutex);
    item = sbuf->buf[sbuf->front];
    sbuf->front = (sbuf->front + 1) % (sbuf->n);
    V(&sbuf->mutex);
    V(&sbuf->slots);
    //printf("After Remove...");
    //printsbuf();  
    
    return item;
}




#endif



