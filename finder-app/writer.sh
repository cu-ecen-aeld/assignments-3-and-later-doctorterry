#!/bin/bash

writefile=$1
writestr=$2

if [ "$#" -ne 2 ]
then
    echo "ERROR: Invalid Number of Arguments"
    echo "Total number of arguments should be 2."
    echo "The order of the arguments should be:"
    echo -e "\t1)Full Directory Path and Filename."
    echo -e "\t2)String to be written to the new file."
    exit 1
fi

path=$(dirname $writefile)
echo ${path}
mkdir -p ${path}
echo ${writestr} > $writefile

status=$?
if [ $status -eq 1 ]
then
    echo "ERROR: File Not Created "
    exit 1
else
    echo "File Created."
    exit 0
fi