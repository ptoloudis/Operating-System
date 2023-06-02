#!/bin/bash

echo -e "\n  Test test5.sh"

make clean
make

echo -e "\nMount the file system"
cd ..
echo -e "\nmake clean"
make clean

echo -e "\nmake"
make
../src/bbfs rootdir/ mountdir/


echo -e "\n  Start the test5.sh"


echo -e "\nAdd the first file"
./test5/test1

echo -e "\nls in mountdir"
ls -l mountdir

echo -e "\nls in rootdir"
ls -laR rootdir

echo -e "\nAdd the second file"
./test5/test2

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

echo -e "\nTest test5.sh finished"