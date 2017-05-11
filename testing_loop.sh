#!/bin/bash

times=25
echo "Start testing $times times"
while [ $times -gt 0 ]
do
    ./curg
    ./curgp
    let "times--"
done