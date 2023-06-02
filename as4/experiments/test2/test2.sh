#!/bin/bash

echo -e "\n  Test test2.sh"

echo -e "\nMount the file system"
cd ..
echo -e "\nmake clean"
make clean

echo -e "\nmake"
make
../src/bbfs rootdir/ mountdir/

echo -e "\n  Start the test2.sh"

echo -e "\nAdd the first file"
cp test2/file1.txt mountdir/file1.txt

echo -e "\nls in mountdir"
ls -l mountdir

echo -e "\nls in rootdir"
ls -laR rootdir

echo -e "\nAdd the second file"
cp test2/file2.txt mountdir/file2.txt

echo -e "\nls in mountdir"
ls -l mountdir

echo -e "\nls in rootdir"
ls -laR rootdir
echo -e "\nRemove the first file"
rm mountdir/file1.txt

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
rm mountdir/file2.txt

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