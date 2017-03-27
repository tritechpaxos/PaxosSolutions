#!/bin/sh

xjDrop $1 B1
echo	"### B1Create $1 B1 10 10000 100000 ==="
B1Create $1 B1 10 10000 100000
if [ $? -ne 0 ] ; then exit 3 ; fi

echo	"=== B1Bench $1 B1 10000 ==="
B1Bench $1 B1 10000
if [ $? -ne 0 ] ; then exit 3 ; fi

echo	"=== B1Bench $1 B1 10000 ==="
B1Bench $1 B1 10000
if [ $? -ne 0 ] ; then exit 3 ; fi
