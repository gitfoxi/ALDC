#!/bin/bash

for f in `find /usr/ -type f`; do
    diff $f <( cat $f | ./sample -c | ./sample -d)
    if [ $? ]
    then
        echo $f GOOD
    else
        echo $f MISMATCHED
        exit 1
    fi
done
