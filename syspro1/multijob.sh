#!/bin/bash

echo Number of input arguments: $#

for arg in "$@" ; do
    echo $arg
done