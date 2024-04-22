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
int concurrency = 1;




typedef struct {
    char* JobID;
    char* Job;
    int queuePosition;
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
} Queue;

Queue ProcessQueue;

void enqueue(Queue* q, Process* p) {
    QueueNode* newNode = (QueueNode*)malloc(sizeof(QueueNode));
    newNode->proc = *p;
    newNode->next = NULL;
    newNode->prev = q->Tail;
    newNode->proc.queuePosition = q->JobNum;

    if (q->Tail != NULL) {
        q->Tail->next = newNode;
    }
    q->Tail = newNode;

    if (q->Head == NULL) {
        q->Head = newNode;
    }

    q->JobNum++;
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
    q->JobNum--;
    return p;
}



void handleIssueJob(char* job, Queue* q, char* triplet) {
    Process* p;
    p = (Process*)malloc(sizeof(Process));
    p->JobID = (char*)malloc(sizeof(char) * 8);
    sprintf(p->JobID, "job_%02d", q->JobNum);
    p->Job = strdup(job); // Allocate memory and copy the job command
    p->queuePosition = q->JobNum;
    enqueue(q, p);
    sprintf(triplet, "%s %s %d", p->JobID, job, p->queuePosition);
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Split the job command into command and arguments
        char* argv[32]; // Assuming a maximum of 32 arguments
        int argc = 0;
        char* token = strtok(job, " ");
        while (token != NULL) {
            argv[argc++] = token;
            token = strtok(NULL, " ");
        }
        argv[argc] = NULL; // Null-terminate the argument list

        // Execute the job command with the arguments
        execvp(argv[0], argv);
        // If execvp fails, handle it
        perror("execvp");
        exit(EXIT_FAILURE);
    }
}


void handleSetConcurrency(char* N) {
    int n = atoi(N);
    if (n < 1) {
        printf("Invalid concurrency value: %d\n", n);
        return;
    }
    printf("Setting concurrency to %d\n", n);
    concurrency = n;
}






void handleUSR1(int signum) {
    printf("Received SIGUSR1. Waiting for call, opening PIPE\n");

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
    printf("Received: %s\n", buf);
    if (strcmp(buf, "exit") == 0) {
        printf("Received termination signal. Exiting...\n");
        unlink("jobExecutorServer.txt");
        unlink(fifo);
        exit(EXIT_SUCCESS);
    }
    if (strncmp(buf, "issueJob", 8) == 0) {
        char triplet[1024];
        handleIssueJob(buf + 9, &ProcessQueue, triplet);
        printf("Triplet: %s\n", triplet);
    } else if (strncmp(buf, "setConcurrency", 14) == 0) {
        handleSetConcurrency(buf + 15);
    }else if (strncmp(buf, "stop", 4) == 0){\
        int pid = atoi(buf + 5);
        kill(pid, SIGKILL);
    } else {
        printf("Invalid command: %s\n", buf);
    }

    close(fd);
}



int main() {
    printf("Creating FIFO\n");
    // Set up signal handlers
    struct sigaction signalAction;
    signalAction.sa_handler = handleUSR1;
    sigemptyset(&signalAction.sa_mask);
    signalAction.sa_flags = SA_RESTART;
    sigaction(SIGUSR1, &signalAction, NULL);

    // Write the process ID to the file
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


    // Main loop to wait for signals
    while (1) {
        printf("im here\n");
        pause(); // Wait for signals
    }

    return 0;
}
