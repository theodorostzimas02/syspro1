#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>

char *fifo = "myfifo"; // FIFO File that the client writes to
char *clientFifo = "clientFifo"; //FIFO FIle that the client reads from

void handleUSR1(int sig) {  
    // Exists purely for the purpose of waking up the client when the server is created
}

int main(int argc, char* argv[]) {
    int mode = 0;   // 1: issueJob, 2: setConcurrency, 3: stop, 4: poll, 5: exit
    char* job;      
    char* N = "1"; // concurrency
    char* jobID;    
    char* pollState;    

    struct sigaction signalAction;              //code for handling signals lifted from Alex Delis' slides on K22
    signalAction.sa_handler = handleUSR1;       
    sigemptyset(&signalAction.sa_mask);
    signalAction.sa_flags = SA_RESTART;
    sigaction(SIGUSR1, &signalAction, NULL);        

    if (argc < 2) {
        printf("Usage: <command> [<job>/<N>]\n");           //error handling for invalid number of arguments
        return 1;
    }
    if (strcmp(argv[1], "issueJob") == 0) {                 
        if (argc < 3) {                                     
            printf("Usage: issueJob <job>\n");
            return 1;
        }

        // Construct the entire command string
        char jobCommand[1024] = "";
        for (int i = 2; i < argc; ++i) {
            strcat(jobCommand, argv[i]);
            strcat(jobCommand, " ");
        }

        // Trim any trailing whitespace
        //jobCommand[strcspn(jobCommand, "\n")] = 0;

        mode = 1;
        job = strdup(jobCommand);
    } else if (strcmp(argv[1], "setConcurrency") == 0) {
        if (argc < 3) {
            printf("Usage: setConcurrency <N>\n");
            return 1;
        }
        mode = 2;
        N = argv[2];
    } else if (strcmp(argv[1], "stop") == 0) {
        if (argc < 3) {
            printf("Usage: stop <jobID>\n");
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
        mode = 5;
    } else {
        printf("Invalid command: %s\n", argv[1]);
        return 1;
    }

    bool txtExists = false;
    
    //if jobExecutorServer.txt exists, read the PID of the server
    int fd = open("jobExecutorServer.txt", O_RDONLY);
    if (fd != -1) {
        txtExists = true;
        close(fd);
    } 

    //if jobExecutorServer.txt does not exist, create a new server
    if (!txtExists) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(1);
        } else if (pid == 0) {
            execlp("./jobExecutorServer", "./jobExecutorServer", NULL);
        } else{
            //wait for usr1 signal to wake up the client
            pause();
        }
    }


    fd = open("jobExecutorServer.txt", O_RDONLY);
    char buf[1024];
    int bytesRead = read(fd, buf, sizeof(buf));
    
    if (bytesRead == -1) {
        perror("read");
        return 1;
    }

    // Extract the PID of the server
    pid_t server_pid = atoi(buf);

    // Create a FIFO for the client
    if (mkfifo(clientFifo, 0666) == -1) {
        if (errno != EEXIST) {
            perror("mkfifo");
            exit(EXIT_FAILURE);
        }
    }

    kill(server_pid, SIGUSR1); // Send signal to server to wake up

    // Open the FIFO for writing
    int fifo_fd = open(fifo, O_WRONLY);
    if (fifo_fd == -1) {
        perror("open FIFO");
        exit(EXIT_FAILURE);
    }

    // Write the command to the FIFO based on the mode
    switch (mode) {
        case 1: {
            char buffer[512];
            sprintf(buffer, "issueJob %s", job);
            write(fifo_fd, buffer, strlen(buffer));
            int fd1 = open(clientFifo, O_RDONLY);
            if (fd1 == -1) {
                perror("open");
                exit(EXIT_FAILURE);
            }
            char buf1[1024];
            int bytesRead1 = read(fd1, buf1, sizeof(buf1));
            if (bytesRead1 == -1) {
                perror("read");
                exit(EXIT_FAILURE);
            }

            printf("%s\n", buf1);
            close (fd1);
            free(job);
            break;
        }
        case 2: {
            char buffer[strlen("setConcurrency") + 16 + 1]; // Assuming N is an integer
            sprintf(buffer, "setConcurrency %s", N);
            write(fifo_fd, buffer, strlen(buffer));
            break;
        }
        case 3: {
            char buffer[strlen("stop") + strlen(jobID) + 1];
            sprintf(buffer, "stop %s", jobID);
            write(fifo_fd, buffer, strlen(buffer));
            int fd1 = open(clientFifo, O_RDONLY);
            if (fd1 == -1) {
                perror("open");
                exit(EXIT_FAILURE);
            }
            char buf1[1024];
            int bytesRead1 = read(fd1, buf1, sizeof(buf1));
            if (bytesRead1 == -1) {
                perror("read");
                exit(EXIT_FAILURE);
            }
            printf("%s\n", buf1);
            close (fd1);
            break;
        }
        case 4: {
            char buffer[strlen("poll") + strlen(pollState) + 1];
            sprintf(buffer, "poll %s", pollState);
            write(fifo_fd, buffer, strlen(buffer));
            int fd1 = open(clientFifo, O_RDONLY);
            if (fd1 == -1) {
                perror("open");
                exit(EXIT_FAILURE);
            }
            char buf1[1024];
            int bytesRead1 = read(fd1, buf1, sizeof(buf1));
            if (bytesRead1 == -1) {
                perror("read");
                exit(EXIT_FAILURE);
            }
            printf("%s\n", buf1);
            close (fd1);
            break;
        }
        case 5:
            write(fifo_fd, "exit", strlen("exit"));
            int fd1 = open(clientFifo, O_RDONLY);
            if (fd1 == -1) {
                perror("open");
                exit(EXIT_FAILURE);
            }
            char buf1[1024];
            int bytesRead1 = read(fd1, buf1, sizeof(buf1));
            if (bytesRead1 == -1) {
                perror("read");
                exit(EXIT_FAILURE);
            }
            printf("%s\n", buf1);
            close (fd1);
            break;
        default:
            // Should not reach here
            break;
    }

    
    close(fifo_fd); // Close the FIFO
    unlink(clientFifo); // Remove the FIFO

    return 0;
}
