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

char *fifo = "myfifo";

int main(int argc, char* argv[]) {
    int mode = 0;
    char* job;
    char* N = "1"; // concurrency
    char* jobID;
    char* pollState; 

    if (argc < 2) {
        printf("Usage: <command> [<job>/<N>]\n");
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
        jobCommand[strcspn(jobCommand, "\n")] = 0;

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
        //printf("jobExecutorServer terminated.\n");
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

    if (!txtExists) {
        pid_t pid = fork();
        if (pid == -1) {
            printf("Error forking\n");
            exit(1);
        } else if (pid == 0) {
            execlp("./jobExecutorServer", "./jobExecutorServer", NULL);
        }else{
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
    printf("Txt: %s\n", buf);
    pid_t server_pid = atoi(buf);

    kill(server_pid, SIGUSR1); // Send signal to server to wake up
    printf("Server PID: %d\n", server_pid);
    
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
            printf("Buffer: %s\n", buffer);
            write(fifo_fd, buffer, strlen(buffer));
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
            break;
        }
        case 4: {
            char buffer[strlen("poll") + strlen(pollState) + 1];
            sprintf(buffer, "poll %s", pollState);
            write(fifo_fd, buffer, strlen(buffer));
            break;
        }
        case 5:
            printf("jobExecutorServer terminated.\n");
            printf("Sending exit signal to server...\n");
            write(fifo_fd, "exit", strlen("exit"));
            break;
        default:
            // Should not reach here
            break;
    }

    close(fifo_fd); // Close the FIFO

    return 0;
}
