#!/bin/bash
echo Number of files: $#                  

for arg in ${*:1}                                       # For all arguments given from 1 to the last one
    do
    if [ -f $arg ]; then                                # If the argument is a file
        echo Issuing jobs from $arg
        while read -r line                              # Read each line of the file
        do    
        ./jobCommander issueJob ./$line                   # valgrind --track-origins=yes --leak-check=full --show-leak-kinds=all --trace-children=yes .
        done < $arg
    else
        echo $arg is not a file
    fi
done