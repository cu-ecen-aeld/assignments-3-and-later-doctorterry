#!/bin/bash

filedir=$1
searchstr=$2

if [ "$#" -ne 2 ]
then
    echo "ERROR: Invalid Number of Arguments"
    echo "Total number of arguments should be 2."
    echo "The order of the arguments should be:"
    echo -e "\t1)File Directory Path."
    echo -e "\t2)String to be searched in the specified directory path."
    exit 1
fi

if [ -d "${filedir}" ] 
then
    echo "Directory ${filedir} exists."
    fcount=$(find ${filedir} -type f | wc -l)
    scount=$(grep ${searchstr} ${filedir}/* | wc -l)
    echo "The number of files are ${fcount} and the number of matching lines are ${scount}"
    exit 0
else
    echo "ERROR: Directory ${filedir} Does Not Exist."
    exit 1
fi
