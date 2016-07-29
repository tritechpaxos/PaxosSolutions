#!/bin/sh

if [ $# -lt 4 ]
then
echo "USAGE:VV-200M lvs_cell pool CSS DB"
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
echo "VVAdmin CreateLV LV-A $cell $pool 10 --- 40M"
VVAdmin CreateLV LV-A $cell $pool 10
if [ ! $? ]; then exit 1; fi

echo
echo "VVAdmin CreateLV LV-B $cell $pool 10 --- +40M = 80M"
VVAdmin CreateLV LV-B $cell $pool 10
if [ ! $? ]; then exit 1; fi

echo
echo "VVAdmin CreateLV LV-C $cell $pool 10 --- +40M = 120M"
VVAdmin CreateLV LV-C $cell $pool 10
if [ ! $? ]; then exit 1; fi

echo
echo "VVAdmin CreateLV LV-D $cell $pool 10 --- +40M = 160M"
VVAdmin CreateLV LV-D $cell $pool 10
if [ ! $? ]; then exit 1; fi

echo
echo "VVAdmin CreateVolume VV-1 target 200000000(200M) 2 $pool"
VVAdmin CreateVolume VV-1 target 200000000 2 $pool
if [ ! $? ]; then exit 1; fi
