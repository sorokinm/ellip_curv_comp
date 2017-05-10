#!/bin/bash

times=10
echo "Start testing $times times"
while [ $times -gt 0 ]
do
    ./curg
    let "times--"
done