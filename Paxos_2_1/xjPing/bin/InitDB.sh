#!/bin/sh

if [ $# -ne 1 ] 
then
	echo "InitDB id"
	exit 1
fi

rm -rf $1
mkdir $1
mkdir $1/DB
mkdir $1/DATA

exit 0
