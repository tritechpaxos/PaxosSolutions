#!/bin/sh

for sh in "Test1.sh" "Test2.sh" "Test3.sh" "TestB0.sh" "TestB100k.sh"
do
	echo	"### $sh ###"
	$sh $1
	if [ $? -ne 0 ] ; then exit 1 ; fi
done
echo	"### DONE ###"
