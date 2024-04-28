#!/bin/bash

# Define the paths of the files to delete
jobExecutorServerFile="jobExecutorServer.txt"
myfifoFile="myfifo"
clientFifoFile="clientFifo"

# Check if the files exist and delete them if they do
if [ -f "$jobExecutorServerFile" ]; then
    rm "$jobExecutorServerFile"
    echo "Deleted $jobExecutorServerFile"
fi

if [ -p "$myfifoFile" ]; then
    rm "$myfifoFile"
    echo "Deleted $myfifoFile"
fi

if [ -p "$clientFifoFile" ]; then
    rm "$clientFifoFile"
    echo "Deleted $clientFifoFile"
fi

# Optionally, you can add some error handling or confirmation prompts here
