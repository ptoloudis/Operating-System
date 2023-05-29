#!/bin/bash

echo -e "\n  Test test4.sh"

echo -e "\nMount the file system"
cd ..
make
../src/bbfs rootdir/ mountdir/

echo -e "\n  Start the test4.sh"

echo -e "\nAdd the all the files"
for i in {1..13}
do
    cp test4/test$i.txt mountdir/test$i.txt
done

echo -e "\nls in mountdir"
ls -l mountdir

echo -e "\nls in rootdir"
ls -laR rootdir

cd mountdir
rm test6.txt test9.txt test11.txt
cd ..

echo -e "\nls in mountdir"
ls -l mountdir

echo -e "\nls in rootdir"
ls -laR rootdir

echo -e "\nUnmount the file system"
fusermount -u mountdir

echo -e "\nls in mountdir"
ls -l mountdir

echo -e "\nls in rootdir"
ls -laR rootdir

echo -e "\nTest test4.sh finished"