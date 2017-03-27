#!/bin/sh

if [ $# -lt 5 ]
then
echo "USAGE:VV-Test lvs_cell pool_fr pool_to CSS DB"
exit 1
fi

cell=$1
pool_fr=$2
pool_to=$3

export	"CSS=$4"
export	"DB=$5"

echo "DropAllDB.sh $DB"
DropAllDB.sh $DB
if [ ! $? ]; then exit 1; fi
echo "InitDB.sh $DB"
InitDB.sh $DB
if [ ! $? ]; then exit 1; fi

echo
echo "VVAdmin CreateLV LV_A $cell $pool_fr 10 --- 40M"
start=`date +%s`
VVAdmin CreateLV LV_A $cell $pool_fr 10
if [ ! $? ]; then exit 1; fi
end=`date +%s`
dt=`expr $end - $start`
echo Elapsed time[$dt]

echo
echo "VVAdmin CreateLV LV_B $cell $pool_fr 10 --- +40M = 80M"
VVAdmin CreateLV LV_B $cell $pool_fr 10
if [ ! $? ]; then exit 1; fi

echo
echo "VVAdmin CreateVolume VV_Fr target 80000000(80M) 2 $pool_fr"
start=`date +%s`
VVAdmin CreateVolume VV_Fr FROM 80000000 2 $pool_fr
if [ ! $? ]; then exit 1; fi
end=`date +%s`
dt=`expr $end - $start`
echo Elapsed time[$dt]
echo	"=== CreateVolume passed ==="
echo

echo
echo "VVTest write AAAA VV_Fr 0 100 10 ABCD"
start=`date +%s`
VVTest write AAAA VV_Fr 0 100 10 ABCD
if [ ! $? ]; then exit 1; fi
end=`date +%s`
dt=`expr $end - $start`
echo Elapsed time[$dt]
echo "VVTest read AAAA VV_Fr 0 100 10 4"
data=`VVTest read AAAA VV_Fr 0 100 10 4`
if [ -z $data ]; then exit 1; fi
echo "[$data]"
if [ $data != "ABCD" ] ; then exit 1; fi
echo	"=== write ABCD passed ==="
echo

echo
echo "VVAdmin CreateLV LV_a $cell $pool_fr 10 --- 40M"
VVAdmin CreateLV LV_a $cell $pool_to 10
if [ ! $? ]; then exit 1; fi

echo
echo "VVAdmin CreateLV LV_b $cell $pool_fr 10 --- +40M = 80M"
VVAdmin CreateLV LV_b $cell $pool_to 10
if [ ! $? ]; then exit 1; fi

echo
echo "VVAdmin CreateVolume VV_To target 80000000(80M) 2 $pool_to"
VVAdmin CreateVolume VV_To TO 80000000 2 $pool_to
if [ ! $? ]; then exit 1; fi

echo
echo "VVAdmin CopyVolume VV_Fr VV_To"
start=`date +%s`
VVAdmin CopyVolume VV_Fr VV_To
if [ ! $? ]; then exit 1; fi
end=`date +%s`
dt=`expr $end - $start`
echo Elapsed time[$dt]

echo "VVTest read AAAA VV_To 0 100 10 4"
data=`VVTest read AAAA VV_To 0 100 10 4`
echo "[$data]"
if [ $data != "ABCD" ] ; then exit 1; fi
echo	"=== CopyVolume passed ==="
echo

echo "VVAdmin CreateSnap VV_Fr SNAP"
start=`date +%s`
VVAdmin CreateSnap VV_Fr SNAP
if [ ! $? ]; then exit 1; fi
end=`date +%s`
dt=`expr $end - $start`
echo Elapsed time[$dt]
echo "VVTest write AAAA VV_Fr 0 100 10 XY"
VVTest write AAAA VV_Fr 0 100 10 XY
if [ ! $? ]; then exit 1; fi
echo "VVTest read AAAA VV_Fr 0 100 10 4"
data=`VVTest read AAAA VV_Fr 0 100 10 4`
echo "[$data]"
if [ $data != "XYCD" ] ; then exit 1; fi
echo	"=== SnapWriteRead VV_Fr passed ==="
echo

echo "VVTest write AAAA VV_Fr 8388606 100 8388606 abcde"
VVTest write AAAA VV_Fr 8388606 100 8388606 abcde
if [ ! $? ]; then exit 1; fi
echo "VVTest read AAAA VV_Fr 8388606 100 8388606 5"
eval data=`VVTest read AAAA VV_Fr 8388606 100 8388606 5`
if [ -z $data ]; then exit 1; fi
echo "[$data]"
if [ $data != "abcde" ] ; then exit 1; fi
echo	"=== SnapWriteRead VV_Fr over VS(0,1) passed ==="
echo

echo "VVAdmin CreateSnap VV_Fr SNAP1"
start=`date +%s`
VVAdmin CreateSnap VV_Fr SNAP1
end=`date +%s`
dt=`expr $end - $start`
echo Elapsed time[$dt]
if [ ! $? ]; then exit 1; fi
echo "VVTest write AAAA VV_Fr 0 100 10 ST"
VVTest write AAAA VV_Fr 0 100 10 ST
if [ ! $? ]; then exit 1; fi
echo "VVTest read AAAA VV_Fr 0 100 10 4"
data=`VVTest read AAAA VV_Fr 0 100 10 4`
if [ -z $data ]; then exit 1; fi
echo "[$data]"
if [ $data != "STCD" ] ; then exit 1; fi
echo	"=== SnapWriteRead VV_Fr/SNAP1 passed ==="
echo

echo
echo "VVAdmin CopyVolume VV_Fr VV_To"
start=`date +%s`
VVAdmin CopyVolume VV_Fr VV_To
if [ ! $? ]; then exit 1; fi
end=`date +%s`
dt=`expr $end - $start`
echo Elapsed time[$dt]
echo "VVTest read AAAA VV_To 0 100 10 4"
data=`VVTest read AAAA VV_To 0 100 10 4`
if [ -z $data ]; then exit 1; fi
echo "[$data]"
if [ $data != "STCD" ] ; then exit 1; fi
echo "VVTest read AAAA VV_To 8388606 100 8388606 5"
eval data=`VVTest read AAAA VV_To 8388606 100 8388606 5`
if [ -z $data ]; then exit 1; fi
if [ $data != "abcde" ] ; then exit 1; fi
echo	"=== CopyVolume VV_Fr/SNAP1/SNAP VV_To passed ==="
echo

echo "VVAdmin RestoreSnap VV_Fr SNAP1"
start=`date +%s`
VVAdmin RestoreSnap VV_Fr SNAP1
if [ ! $? ]; then exit 1; fi
end=`date +%s`
dt=`expr $end - $start`
echo Elapsed time[$dt]
echo "VVTest read AAAA VV_Fr 0 100 10 4"
data=`VVTest read AAAA VV_Fr 0 100 10 4`
if [ -z $data ]; then exit 1; fi
echo "[$data]"
if [ $data != "XYCD" ] ; then exit 1; fi
echo	"=== RestoreSnap VV_Fr SNAP1 passed ==="
echo

echo "VVAdmin RenameVolume VV_Fr RENAME_Fr"
VVAdmin RenameVolume VV_Fr RENAME_Fr
if [ ! $? ]; then exit 1; fi
echo "VVTest read AAAA RENAME_Fr 0 100 10 4"
data=`VVTest read AAAA RENAME_Fr 0 100 10 4`
if [ -z $data ]; then exit 1; fi
echo "[$data]"
if [ $data != "XYCD" ] ; then exit 1; fi
echo	"=== RenameVolume VV_Fr RENAME_Fr passed ==="
echo

echo
echo	"!!! SUCCESS !!!"
echo
exit 0
