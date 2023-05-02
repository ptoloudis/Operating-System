#!/bin/bash

echo "Clean the old kernel...."
cd /usr/src/
sudo rm -rf linux-5.4.86*
cd $1
echo "Clean the old kernel."

echo "Copy to install the new kernel...."
sudo tar -C /usr/src -xvf linux-5.4.86.tar.gz
cd /usr/src/
sudo mv linux-5.4.86 linux-5.4.86-orig
sudo cp -r linux-5.4.86-orig linux-5.4.86-dev

echo "Is ok:"
read  x
if [ $x == "no" ]
then 
	echo "Try your self"
	exit 1
else 
	echo "Start to install the new kernel...."
fi

cd /usr/src/linux-5.4.86-dev
sudo nano Makefile
sudo make localmodconfig
sudo make menuconfig
sudo make -j4
sudo make modules_install install
echo "Install the new kernel."

ls /boot
echo "Reboot:"
read  x
if [ $x ==  "yes" ]
then
	sudo shutdown -r now
else 
	echo "Try your self"
fi

