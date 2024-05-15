#include <sys/types.h>

typedef struct {
    char* JobID;
    char* Job;
    int queuePosition;
    pid_t pid ;
} Process;

typedef struct QueueNode {
    Process proc;
    struct QueueNode* next;
    struct QueueNode* prev;
} QueueNode;

typedef struct {
    QueueNode* Head;
    QueueNode* Tail;
    int JobNum;
    int allJobs;
} Queue;

void enqueue(Queue* q, Process* p);
Process dequeue(Queue* q);
Process* search(Queue* q, char* jobID);
Process* searchByPID(Queue* q, pid_t pid);
void printQueue(Queue* q);
void deleteNode(Queue* q, Process* p);
void freeQueue(Queue* q);

