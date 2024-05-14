#!/bin/bash

if [ -f jobExecutorServer.txt ]; then
    echo "jobExecutorServer.txt is a file"

    # Send 'poll running' command to jobCommander and extract job IDs
    running_jobs=$(./jobCommander poll running | grep -o '<[^>]*>' | cut -d',' -f1 | tr -d '<>' )

    # Send 'poll queued' command to jobCommander and extract job IDs
    queued_jobs=$(./jobCommander poll queued | grep -o '<[^>]*>' | cut -d',' -f1 | tr -d '<>')

    # Stop each running job
    for job_id in $running_jobs; do
        echo "Stopping job $job_id"
        ./jobCommander stop "$job_id"
    done

    # Stop each queued job
    for job_id in $queued_jobs; do
        echo "Stopping job $job_id"
        ./jobCommander stop "$job_id"
    done

    ./jobCommander exit

    echo "All processes stopped."
else
    echo "jobExecutorServer.txt is not a file"
fi
