#ifndef FIFO_H 

struct node
{
    char *gender;
    int *duration;
    struct node *next;
}

typedef struct node fifoItem;

typedef struct 
{
    fifoItem *start;
    fifoItem *end;
}fifo;

