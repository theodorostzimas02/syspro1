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
char* fifo = "myfifo";

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

void handleUSR1(int signum) {
    printf("Waiting for call, open PIPE\n");
}

int main(){
    // Write the process ID to the file
    struct sigaction signalAction;  
    signalAction.sa_handler = handleUSR1; 
    sigemptyset(&signalAction.sa_mask);   
    signalAction.sa_flags = SA_RESTART;
    sigaction(SIGUSR1, &signalAction, NULL); 

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

    

    if (mkfifo(fifo, 0666) == -1) {
        perror("Error creating fifo");
        exit(1);
    }

    // int fd1 = open(fifo, O_RDONLY);
    // if (fd1 == -1) {
    //     perror("open");
    //     return 1;
    // }
    // char buf[1024];
    // int bytesRead = read(fd1, buf, sizeof(buf));
    // if (bytesRead == -1) {
    //     perror("read");
    //     return 1;
    // }
    // buf[bytesRead] = '\0';
    // printf("Received: %s\n", buf);

    // close(fd1);
    sleep(3);
    unlink("jobExecutorServer.txt");
    printf("unlink done\n");

    return 0;
}