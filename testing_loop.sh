#!/bin/bash

times=8
echo "Start testing $times times"
while [ $times -gt 0 ]
do
    ./curg
    let "times--"
done