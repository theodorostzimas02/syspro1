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
#include "types.h"

char* fifo = "myfifo";
char* clientFifo = "clientFifo";
int concurrency = 1;
int activeProcesses = 0;

Queue ProcessQueue;
Queue RunningQueue;

void processExecute(Process* proc){
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Split the job command into command and arguments
        char* argv[32]; // We set a maximum of 32 arguments
        int argc = 0;
        char* token = strtok(proc->Job, " ");
        while (token != NULL) {
            argv[argc++] = token;
            token = strtok(NULL, " ");
        }
        argv[argc] = NULL; // Null-terminate the argument list

        // Execute the job command with the arguments
        if (access(argv[0], X_OK) != 0) { 
            argv[0] = argv[0] + 2; // Remove the "./" prefix
        }
        execvp(argv[0], argv);
        perror("execvp");
        exit(EXIT_FAILURE);
    }else {
        proc->pid = pid; // Save the PID of the child process after the fork
        enqueue(&RunningQueue, proc);       // Add the process to the running queue
        activeProcesses++;
    }
}



void handleIssueJob(char* job, Queue* q, char* triplet) {
    Process* p;
    p = (Process*)malloc(sizeof(Process));
    p->JobID = (char*)malloc(sizeof(char) * 8);  // Allocate memory for the job ID for 8 characters 
    sprintf(p->JobID, "job_%d", q->allJobs+1);
    p->Job = strdup(job); // Allocate memory and copy the job command
    p->queuePosition = q->JobNum+1;
    enqueue(q, p);      // Add the job to the waiting queue
    sprintf(triplet, "<%s,%s,%d>", p->JobID, job, p->queuePosition);
}

void handleSetConcurrency(char* N) {
    int n = atoi(N);                        //atoi converts string to integer
    if (n < 1) {
        printf("Invalid concurrency value: %d\n", n);       
        return;
    }
    // Set the new concurrency value
    concurrency = n;

    //if the concurrency changes and we have less active processes, we execute the next process from the queue
    while (ProcessQueue.Head != NULL && activeProcesses < concurrency) { 
        Process p = dequeue(&ProcessQueue);
        processExecute(&p);
    }
}


void handleSIGCHLD(int signum) {
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {  //We handle a child process that has terminated and wait for its pid to be returned so we can 
        if (RunningQueue.Head->proc.pid == pid){
            dequeue(&RunningQueue); //delete it from the running queue
        }else {
            Process* p = searchByPID(&RunningQueue, pid);   //delete it from the running queue
            deleteNode(&RunningQueue, p);
        }
        activeProcesses--; 
    }
}

void handleUSR1(int signum) {
    //Received SIGUSR1. Waiting for call, opening PIPE
    int fd = open(fifo, O_RDONLY);      //Open the FIFO for reading
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    char buf[1024];
    int bytesRead = read(fd, buf, sizeof(buf));  //Read the command from the FIFO
    if (bytesRead == -1) {
        perror("read");
        exit(EXIT_FAILURE);
    }

    buf[bytesRead] = '\0';
    if (strncmp(buf, "exit", 4) == 0) {
        int fd1 = open(clientFifo, O_WRONLY);
        if (fd1 == -1) {
            perror("open");
            exit(EXIT_FAILURE);
        }
        write(fd1, "Exiting server...", strlen("Exiting server..."));
        freeQueue(&ProcessQueue);
        freeQueue(&RunningQueue);
        unlink("jobExecutorServer.txt");
        unlink(fifo);
        close(fd1);
        close(fd);
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

        int fd1 = open(clientFifo, O_WRONLY);
        if (fd1 == -1) {
            perror("open");
            exit(EXIT_FAILURE);
        }
        
        if((needed = search(&RunningQueue, buf + 5)) != NULL){
                kill(needed->pid, SIGKILL); //Kill the process using the PID from the process we found in the running queue
                char buf[1024];
                sprintf(buf, "%s has been terminated\n", needed->JobID);
                write(fd1, buf, strlen(buf));
        }else if ((needed = search(&ProcessQueue, buf + 5)) != NULL){
            char buf[1024];
            sprintf(buf, "Job %s has been removed\n", needed->JobID);
            write(fd1, buf, strlen(buf));
            deleteNode(&ProcessQueue, needed); //Delete the process from the waiting queue
        }
        
        if (needed == NULL){
            //If the process is not found in the running or waiting queue, we send the appropriate message accordingly
            char buf1[1024];
            sprintf(buf1, "Job %s not found\n", buf + 5);
            write(fd1, "Job not found", strlen("Job not found"));
        }
        close(fd1);

    }else if (strncmp(buf, "poll", 4) == 0) {
        int fd1 = open(clientFifo, O_WRONLY);
        if (fd1 == -1) {
            perror("open");
            exit(EXIT_FAILURE);
        }

        char response[4096]; 
        memset(response, 0, sizeof(response)); // Initialize response buffer

        if (strcmp(buf + 5, "running") == 0) {
            QueueNode* current = RunningQueue.Head;
            if (current == NULL) {
                strcat(response, "No jobs running"); // If no jobs are running, write this to the response
            }
            char triplet[1024];
            while (current != NULL) {
                sprintf(triplet, "<%s,%s,%d>", current->proc.JobID, current->proc.Job, current->proc.queuePosition);
                //we write each triplet to the response so we can send it to the client later as a whole
                strcat(response, triplet); 
                current = current->next;
            }
        } else if (strcmp(buf + 5, "queued") == 0) {
            QueueNode* current = ProcessQueue.Head;
            if (current == NULL) { // If no jobs are queued, write this to the response
                strcat(response, "No jobs queued");
            }
            char triplet[1024];
            while (current != NULL) {
                sprintf(triplet, "<%s,%s,%d>", current->proc.JobID, current->proc.Job, current->proc.queuePosition);
                //we write each triplet to the response so we can send it to the client later as a whole
                strcat(response, triplet); 
                current = current->next;
            }
        } else {
            // If the command is invalid, write this to the response
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
    // Set up signal handlers
    //code for handling signals lifted from Alex Delis' slides on K22
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


    // Write the process ID of the jobExecutorServer to a file, create it if it doesn't exist
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

    // Create a FIFO for communication with the jobCommander 
    if (mkfifo(fifo, 0666) == -1) {
        perror("Error creating fifo");
        exit(1);
    }
    kill(getppid(), SIGUSR1); // Send signal to jobCommander to indicate that the FIFO has been created and is ready for use

    //Main loop for the server
    while (1) {
        if (ProcessQueue.Head != NULL &&  activeProcesses < concurrency) {
            Process p = dequeue(&ProcessQueue);
            processExecute(&p);
        }
        // Wait for the appropriate signal to wake up, go to the signal handler and handle the command accordingly
        pause(); 
        
    }

    return 0;
}
