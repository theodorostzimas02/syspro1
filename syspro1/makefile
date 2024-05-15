# Compiler
CC = gcc

# Compile Options
CFLAGS = -g -Wall -Werror -std=c99 -D_POSIX_C_SOURCE=200809L
ARGSV = issueJob ./processes/progDelay 5

# Compile rules
jobCommander: jobCommander.c queue.c
	$(CC) $(CFLAGS) $^ -o $@

jobExecutorServer: jobExecutorServer.c queue.c
	$(CC) $(CFLAGS) $^ -o $@

progDelay: processes/progDelay.c
	$(CC) $(CFLAGS) $^ -o $@

all: jobCommander jobExecutorServer progDelay

clean:
	rm -rf jobCommander jobExecutorServer jobExecutorServer.txt myfifo clientFifo progDelay
