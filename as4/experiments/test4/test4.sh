#!/bin/bash

echo -e "\n  Test test4.sh"

echo -e "\nMount the file system"
cd ..

echo -e "\nmake clean"
make clean

echo -e "\nmake"
make
../src/bbfs rootdir/ mountdir/

echo -e "\n  Start the test4.sh"

echo -e "\nAdd the all the files"
for i in {1..13}
do
    cp test4/file$i.txt mountdir/file$i.txt
done

echo -e "\nls in mountdir"
ls -l mountdir

echo -e "\nls in rootdir"
ls -laR rootdir

cd mountdir
rm file6.txt file9.txt file11.txt
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