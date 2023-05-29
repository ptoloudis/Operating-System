#!/bin/bash

echo -e "\n  Test test3.sh"

echo -e "\nMount the file system"
cd ..
make
../src/bbfs rootdir/ mountdir/

echo -e "\n  Start the test3.sh"

echo -e "\nAdd the first file"
cp test3/output.txt mountdir/output.txt

echo -e "\nls in mountdir"
ls -l mountdir

echo -e "\nls in rootdir"
ls -laR rootdir

echo -e "\nAdd the second file"
cp test3/output3.txt mountdir/output3.txt

echo -e "\nls in mountdir"
ls -l mountdir

echo -e "\nls in rootdir"
ls -laR rootdir

echo -e "\nRemove the first file"
rm mountdir/output.txt

echo -e "\nls in mountdir"
ls -l mountdir

echo -e "\nls in rootdir"
ls -laR rootdir

echo -e "\nRemove the first file"
rm mountdir/output3.txt

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

echo -e "\nTest test3.sh finished"