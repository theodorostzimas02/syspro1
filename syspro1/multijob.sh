#!/bin/bash
echo Number of input arguments: $#

for arg in ${*:1}    
    do
    if [ -f $arg ]; then
        echo $arg is a file
        while read -r line
        do    
        ./jobCommander issueJob ./$line                     #valgrind --track-origins=yes --dsymutil=yes 
        done < $arg
    else
        echo $arg is not a file
    fi
done