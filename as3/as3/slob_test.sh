#!/bin/sh

echo  > error/$1-error.txt

for i in 1 2 3 4 5 6 7 8 9 10
do
	for j in 1 2 3
	do
		echo  $1-$j-$i >> error/$1-error.txt
        echo  $1-$j-$i 
		./test input/test$j.txt >result/$1-$j-$i.txt 2>> error/$1-error.txt
        sleep $2
	done
done
