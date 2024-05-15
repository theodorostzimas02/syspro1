#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include "types.h"

void enqueue(Queue* q, Process* p) {
    QueueNode* newNode = (QueueNode*)malloc(sizeof(QueueNode));
    newNode->proc = *p;
    newNode->next = NULL;
    newNode->prev = q->Tail;
    newNode->proc.queuePosition = q->JobNum+1;

    if (q->Tail != NULL) {
        q->Tail->next = newNode;
    }
    q->Tail = newNode;

    if (q->Head == NULL) {
        q->Head = newNode;
    }

    q->JobNum++;
    q->allJobs++;
    
}

void printQueue(Queue* q) {
    QueueNode* current = q->Head;
    while (current != NULL) {
        printf("JobID: %s, Job: %s, queuePosition: %d\n", current->proc.JobID, current->proc.Job, current->proc.queuePosition);
        current = current->next;
    }
}

Process dequeue(Queue* q) {
    if (q->Head == NULL) {
        Process p;
        p.JobID = NULL;
        return p;
    }

    Process p = q->Head->proc;
    QueueNode* temp = q->Head;
    q->Head = q->Head->next;
    if (q->Head != NULL) {
        q->Head->prev = NULL;
    } else {
        q->Tail = NULL;
    }

    free(temp);
    
    // Decrement the queue position of all other jobs
    QueueNode* current = q->Head;
    while (current != NULL) {
        current->proc.queuePosition--;
        current = current->next;
    }

    q->JobNum--;
    return p;
}


Process* search(Queue* q, char* jobID){
    QueueNode* current = q->Head;
    while (current != NULL){
        if (strcmp(current->proc.JobID, jobID) == 0){
            return &current->proc;
        }
        current = current->next;
    }
    return NULL;
}

Process* searchByPID(Queue* q, pid_t pid){
    QueueNode* current = q->Head;
    while (current != NULL){
        if (current->proc.pid == pid){
            return &current->proc;
        }
        current = current->next;
    }
    return NULL;
}

void deleteNode(Queue* q, Process* node){
    QueueNode* current = q->Head;
    while (current != NULL){
        if (current->proc.JobID == node->JobID){
            if (current->prev != NULL){
                current->prev->next = current->next;
            }
            if (current->next != NULL){
                current->next->prev = current->prev;
            }
            free(current);
            q->JobNum--;
            return;
        }
        current = current->next;
    }
}


void freeQueue(Queue* q) {
    QueueNode* current = q->Head;
    while (current != NULL) {
        QueueNode* temp = current;
        current = current->next;
        free(temp->proc.JobID);
        free(temp);
    }
    q->Head = NULL;
    q->Tail = NULL;
    q->JobNum = 0;
}