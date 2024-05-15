#!/bin/bash

# Check if jobExecutorServer.txt is a file so we know that the server is active
if [ -f jobExecutorServer.txt ]; then                           
    echo "jobExecutorServer has been initialized."

    # grep with -o and pattern so we can take anything enclosed in '<' and '>'
    # cut breaks the line into fields and -d specifies the delimiter while f1 specifies the field
    # tr deletes the '<' and '>' characters

    # Send 'poll running' command to jobCommander so we can use the job IDs for terminating the jobs currently running
    running_jobs=$(./jobCommander poll running | grep -o '<[^>]*>' | cut -d',' -f1 | tr -d '<>' )

    # Send 'poll queued' command to jobCommander so we can use the job IDs for removing the jobs currently queued
    queued_jobs=$(./jobCommander poll queued | grep -o '<[^>]*>' | cut -d',' -f1 | tr -d '<>')

    # Stop each running job
    for job_id in $running_jobs; do
        echo "Stopping $job_id"
        ./jobCommander stop "$job_id"
    done

    # Stop each queued job
    for job_id in $queued_jobs; do
        echo "Stopping $job_id"
        ./jobCommander stop "$job_id"
    done
    ./jobCommander exit
    echo "All processes stopped."
else
    echo "jobExecutorServer hasn't been initialized yet."
fi
