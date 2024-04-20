#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

typedef struct QueueNode QueueNode; 

typedef struct{
    int JobID;
    char* Job;
    int queuePosition;
} Process;

typedef struct QueueNode{
    Process proc;
    QueueNode* next;
    QueueNode* prev;
} QueueNode;

typedef struct{
    QueueNode* Head;
    QueueNode* Tail;
    int JobNum;
}Queue;


int main(int argc, char* argv[]) {
    int mode = 0;
    char* job;
    char* N = "1"; // concurrency
    char* jobID;
    char* pollState; 

    if (argc < 2) {
        printf("Usage: <command> [<job>/<N>]\n", argv[0]);
        return 1;
    }
    if (strcmp(argv[1], "issueJob") == 0) {
        if (argc < 3) {
            printf("Usage: issueJob <job>\n", argv[0]);
            return 1;
        }
        mode = 1;
        job = argv[2];
    } else if (strcmp(argv[1], "setConcurrency") == 0) {
        if (argc < 3) {
            printf("Usage: setConcurrency <N>\n", argv[0]);
            return 1;
        }
        mode = 2;
        N = argv[2];
    } else if (strcmp(argv[1], "stop") == 0) {
        if (argc < 3) {
            printf("Usage: stop <jobID>\n", argv[0]);
            return 1;
        }
        mode = 3;
        jobID = argv[2];
    } else if (strcmp(argv[1], "poll") == 0) {
        if ((argc < 3) || ((strcmp(argv[2],"running") != 0) && (strcmp(argv[2],"queued")!=0)))  {
            printf("Usage: %s poll [running|queued]\n", argv[0]);
            return 1;
        }
        mode = 4;
        pollState = argv[2];
    } else if (strcmp(argv[1], "exit") == 0) {
        printf("jobExecutorServer terminated.\n");
        return 0;
    } else {
        printf("Invalid command: %s\n", argv[1]);
        return 1;
    }


    int concurrency = atoi(N);
    bool txtExists = false;

    int fd = open("jobExecutorServer.txt", O_RDONLY);
    if (fd != -1) {
        txtExists = true;
        close(fd);
    } 

    char* fifo = "myfifo";
    if (mkfifo(fifo, 0666) == -1) {
        if (errno != EEXIST) {
            printf("Error creating fifo\n");
            exit(1);
        }
    }
    if (fd = open(fifo, O_RDWR) == -1) {
        
    }

    // Send signal to jobExecutorServer
    kill(getpid(), SIGUSR1);
    // Handle each mode accordingly
    switch (mode) {
        case 1:
            printf("job : %s",job);
            break;
        case 2:
            int intN = atoi(N);
            printf("N: %d",intN);
            break;
        case 3:
            break;
        case 4:
            // poll mode
            // Implement poll logic
            break;
        default:
            // Should not reach here
            break;
    }

    return 0;
}



