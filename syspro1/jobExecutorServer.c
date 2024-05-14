#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>

char* fifo = "myfifo";
char* clientFifo = "clientFifo";
int concurrency = 1;
int activeProcesses = 0;


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

Queue ProcessQueue;
Queue RunningQueue;

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
    //printf("Job is queued with pid %d\n", p->pid);
    
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
        printf("current->proc.JobID %s\n", current->proc.JobID);
        if (strcmp(current->proc.JobID, jobID) == 0){
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

void processExecute(Process* proc){
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Split the job command into command and arguments
        char* argv[32]; // Assuming a maximum of 32 arguments
        int argc = 0;
        char* token = strtok(proc->Job, " ");
        while (token != NULL) {
            argv[argc++] = token;
            token = strtok(NULL, " ");
        }
        argv[argc] = NULL; // Null-terminate the argument list
        //printf("command %s and argv %s\n", argv[0], argv[1]);
        // Execute the job command with the arguments
        if (access(argv[0], X_OK) != 0) {
            argv[0] = argv[0] + 2; // Remove the "./" prefix
        }
        
        execvp(argv[0], argv);
        perror("execvp");
        exit(EXIT_FAILURE);
    }else {
        proc->pid = pid;
        enqueue(&RunningQueue, proc);       
        activeProcesses++;
    }
}



void handleIssueJob(char* job, Queue* q, char* triplet) {
    Process* p;
    p = (Process*)malloc(sizeof(Process));
    p->JobID = (char*)malloc(sizeof(char) * 8);
    sprintf(p->JobID, "job_%d", q->allJobs+1);
    p->Job = strdup(job); // Allocate memory and copy the job command
    p->queuePosition = q->JobNum+1;
    enqueue(q, p);
    sprintf(triplet, "<%s,%s,%d>", p->JobID, job, p->queuePosition);
}

void handleSetConcurrency(char* N) {
    int n = atoi(N);
    if (n < 1) {
        //printf("Invalid concurrency value: %d\n", n);
        return;
    }
    //printf("Setting concurrency to %d\n", n);
    concurrency = n;

    while (ProcessQueue.Head != NULL && activeProcesses < concurrency) {
        Process p = dequeue(&ProcessQueue);
        processExecute(&p);
    }
}


void handleSIGCHLD(int signum) {
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        activeProcesses--;
        dequeue(&RunningQueue);
    }
}



void handleUSR1(int signum) {
    //printf("Received SIGUSR1. Waiting for call, opening PIPE\n");

    int fd = open(fifo, O_RDONLY);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    char buf[1024];
    int bytesRead = read(fd, buf, sizeof(buf));
    if (bytesRead == -1) {
        perror("read");
        exit(EXIT_FAILURE);
    }

    buf[bytesRead] = '\0';
    //printf("Received: %s\n", buf);
    if (strcmp(buf, "exit") == 0) {
        //printf("Received termination signal. Exiting...\n");
        freeQueue(&ProcessQueue);
        freeQueue(&RunningQueue);
        unlink("jobExecutorServer.txt");
        unlink(fifo);
        exit(EXIT_SUCCESS);
    }
    if (strncmp(buf, "issueJob", 8) == 0) {
        char triplet[1024];
        handleIssueJob(buf + 9, &ProcessQueue, triplet);
        int fd1 = open(clientFifo, O_WRONLY);
        if (fd1 == -1) {
            perror("open");
            exit(EXIT_FAILURE);
        }
        write(fd1, triplet, strlen(triplet));
        close(fd1);
    } else if (strncmp(buf, "setConcurrency", 14) == 0) {
        handleSetConcurrency(buf + 15);
    }else if (strncmp(buf, "stop", 4) == 0){
        
        Process* needed = NULL;
        //printf("something %s\n", buf + 5);
        int fd1 = open(clientFifo, O_WRONLY);
        if (fd1 == -1) {
            perror("open");
            exit(EXIT_FAILURE);
        }

        if ((needed = search(&ProcessQueue, buf + 5)) != NULL){
            printf("we are in process queue\n");
            
            char buf[1024];
            sprintf(buf, "Job %s has been removed\n", needed->JobID);
            write(fd1, buf, strlen(buf));
            Process* p = search(&ProcessQueue, buf + 5);
            deleteNode(&ProcessQueue, p);
        }else if((needed = search(&RunningQueue, buf + 5)) != NULL){
                printf("needed->pid %d\n", needed->pid);
                kill(needed->pid, SIGKILL);
                printf("we are in running queue\n");
                char buf[1024];
                sprintf(buf, "%s has been terminated\n", needed->JobID);
                write(fd1, buf, strlen(buf));
                dequeue(&RunningQueue);
                activeProcesses--;
        }
        
        if (needed == NULL){
            char buf[1024];
            sprintf(buf, "Job %s not found\n", buf + 5);
            write(fd1, "Job not found", strlen("Job not found"));
        }
        
        close(fd1);

    }else if (strncmp(buf, "poll", 4) == 0) {
        int fd1 = open(clientFifo, O_WRONLY);
        if (fd1 == -1) {
            perror("open");
            exit(EXIT_FAILURE);
        }

        char response[4096]; // Assuming a maximum size for the response
        memset(response, 0, sizeof(response)); // Initialize response buffer

        if (strcmp(buf + 5, "running") == 0) {
            QueueNode* current = RunningQueue.Head;
            if (current == NULL) {
                strcat(response, "No jobs running");
            }
            char triplet[1024];
            while (current != NULL) {
                sprintf(triplet, "<%s,%s,%d>", current->proc.JobID, current->proc.Job, current->proc.queuePosition);
                strcat(response, triplet); // Concatenate triplet to response
                current = current->next;
            }
        } else if (strcmp(buf + 5, "queued") == 0) {
            QueueNode* current = ProcessQueue.Head;
            if (current == NULL) {
                strcat(response, "No jobs queued");
            }
            char triplet[1024];
            while (current != NULL) {
                sprintf(triplet, "<%s,%s,%d>", current->proc.JobID, current->proc.Job, current->proc.queuePosition);
                strcat(response, triplet); // Concatenate triplet to response
                current = current->next;
            }
        } else {
            strcat(response, "Invalid command");
        }
        // Write the entire response to the FIFO
        write(fd1, response, strlen(response));

        close(fd1); 
    }else {
        printf("Invalid command: %s\n", buf);
    }

    close(fd);
}



int main() {
    //printf("Creating FIFO\n");
    // Set up signal handlers
    struct sigaction signalAction;
    signalAction.sa_handler = handleUSR1;
    sigemptyset(&signalAction.sa_mask);
    signalAction.sa_flags = SA_RESTART;
    sigaction(SIGUSR1, &signalAction, NULL);

    struct sigaction sigchldAction;
    sigchldAction.sa_handler = handleSIGCHLD;
    sigemptyset(&sigchldAction.sa_mask);
    sigchldAction.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sigchldAction, NULL);


    // Write the process ID to the file
    //printf("Writing PID to file\n");
    int pid = getpid();
    int fd = open("jobExecutorServer.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd != -1) {
        char pidStr[16];
        snprintf(pidStr, sizeof(pidStr), "%d", pid);
        int bytesWritten = write(fd, pidStr, strlen(pidStr));
        if (bytesWritten == -1) {
            perror("write");
            return 1;
        }
        close(fd);
    } else {
        perror("open");
        return 1;
    }

    
    // Create FIFO
    if (mkfifo(fifo, 0666) == -1) {
        perror("Error creating fifo");
        exit(1);
    }
    kill(getppid(), SIGUSR1); // Send signal to parent to wake up

    while (1) {
        if (ProcessQueue.Head != NULL &&  activeProcesses < concurrency) {
            printf("ProcessQueue.Head %s\n", ProcessQueue.Head->proc.JobID);
            Process p = dequeue(&ProcessQueue);
            processExecute(&p);
        }
        pause(); // Wait for signals
        
    }

    return 0;
}
