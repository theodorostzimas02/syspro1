
# Compiler
CC = gcc

# Compile Options
CFLAGS = -Wall -Werror -std=c99 -D_POSIX_C_SOURCE=200809L
ARGSV = issueJob ./processes/progDelay 5

jobCommander: jobCommander.c
	$(CC) $(FLAGS) $< -o $@

jobExecutorServer: jobExecutorServer.c
	$(CC) $(FLAGS) $< -o $@

all: jobCommander jobExecutorServer

run_cmp: jobCommander jobExecutorServer
	./jobCommander $(ARGSV)

valgrind_cmp: $(COMPARE)
	valgrind --track-origins=yes --leak-check=full --show-leak-kinds=all -s ./$(COMPARE) $(ARGSV)

clean:
	rm -rf jobCommander jobExecutorServer jobExecutorServer.txt myfifo clientFifo