#!/bin/sh

if [ $# -lt 4 ]
then
echo "USAGE:VV-1.sh lvs_cell pool CSS DB"
exit 1
fi

cell=$1
pool=$2

export	"CSS=$3"
export	"DB=$4"

echo "DropAllDB.sh $DB"
DropAllDB.sh $DB
if [ ! $? ]; then exit 1; fi
echo "InitDB.sh $DB"
InitDB.sh $DB
if [ ! $? ]; then exit 1; fi

echo
echo "VVAdmin CreateLV LV-A $cell $pool 500 --- 2G"
VVAdmin CreateLV LV-A $cell $pool 500
if [ ! $? ]; then exit 1; fi

echo
echo "VVAdmin CreateLV LV-B $cell $pool 500 --- +2G = 4G"
VVAdmin CreateLV LV-B $cell $pool 500
if [ ! $? ]; then exit 1; fi

echo
echo "VVAdmin CreateLV LV-C $cell $pool 500 --- +2G = 6G"
VVAdmin CreateLV LV-C $cell $pool 500
if [ ! $? ]; then exit 1; fi

echo
echo "VVAdmin CreateLV LV-D $cell $pool 500 --- +2G = 8G"
VVAdmin CreateLV LV-D $cell $pool 500
if [ ! $? ]; then exit 1; fi

echo
echo "VVAdmin CreateVolume VV-1 target 8000000000(8G) 2 $pool"
VVAdmin CreateVolume VV-1 target 8000000000 2 $pool
if [ ! $? ]; then exit 1; fi
