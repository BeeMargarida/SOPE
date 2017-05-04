#include <stdlib.h>
#include <string.h>
#include "fifo.h"


fifo *newFifo()
{
    fifo *aux = (fifo *) malloc(sizeof(fifo));

    if(aux == NULL)
    {
        return NULL;
    }
    aux->start = aux->end = NULL;
    
    return aux;
}

void clearFifo(fifo *aux)
{

    fifoItem *cur, *next;

    if(aux->start != NULL)
    {
        cur = f->start;

        while(cur->next!=NULL)
        {
            free(cur->gender);
            free(cur->duration);
            next = cur->next;
            free(cur);
            cur = next;
        }
    }
    free(aux);
}


int sizeFifo(fifo *aux)
{
    int i;
    fifoItem *cur;

    cur= aux->start;

    for(i=0;cur != NULL;i++)
    {
        cur = cur->next;
    }
    return i;
}

void pushFifo(fifo *aux, const char *gender,  const int *duration)
{
    fifoItem *newfifoItem; *lastfifoItem = aux->end;

    newfifoItem = (fifoItem *) malloc(sizeof(fifoItem));

    newfifoItem->gender = (char *) calloc(strlen(gender)+1, sizeof(char));

    strcpy(newfifoItem->gender, gender);
    newfifoItem->next=NULL;

    if(lastfifoItem == NULL)
    {
        aux->start = aux->end = newfifoItem;
    }
    else
    {
        aux->end->next = newfifoItem;
        aux->end = newfifoItem;
    }
    
}

void popFifo(fifo *aux)
{
    fifoItem *cur;

    cur = aux->start;
    aux->start = cur->next;

    free(cur->gender);
    free(cur->duration);
    free(cur);

}