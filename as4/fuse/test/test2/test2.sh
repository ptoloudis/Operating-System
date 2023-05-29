#!/bin/bash

echo -e "\n  Test test2.sh"

echo -e "\nMount the file system"
cd ..
make
../src/bbfs rootdir/ mountdir/

echo -e "\n  Start the test2.sh"

echo -e "\nAdd the first file"
cp test2/test1.txt mountdir/test1.txt

echo -e "\nls in mountdir"
ls -l mountdir

echo -e "\nls in rootdir"
ls -laR rootdir

echo -e "\nAdd the second file"
cp test2/test2.txt mountdir/test2.txt

echo -e "\nls in mountdir"
ls -l mountdir

echo -e "\nls in rootdir"
ls -laR rootdir
echo -e "\nRemove the first file"
rm mountdir/test1.txt

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

echo -e "\nMount the file system"
../src/bbfs rootdir mountdir

echo -e "\nls in mountdir"
ls -l mountdir

echo -e "\nls in rootdir"
ls -laR rootdir

echo -e "\nRemove the second file"
rm mountdir/test2.txt

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

echo -e "\nTest test2.sh finished"