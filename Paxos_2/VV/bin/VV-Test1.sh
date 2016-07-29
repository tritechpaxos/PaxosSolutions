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

sh <<_EOF_ &
echo "=== VVTest write AAAA VV_Fr 0 10 200 A$$ === "
VVTest write AAAA VV_Fr 0 10 200 A$$ 
ret=\$?
#if [ ! \$ret ]; then exit 1; fi
echo [\$ret]
_EOF_
pid0=$!
echo pid0=$pid0

sleep 1

sh <<_EOF_ &
echo "=== VVTest read AAAA VV_Fr 0 10 200 5 ==="
data=`VVTest read AAAA VV_Fr 0 10 200 5`
if [ -z \$data ]; then exit 1; fi
echo "[\$data]"
_EOF_
pid1=$!
echo pid1=$pid1

wait $pid1
wait $pid0

sh <<_EOF_ &
echo "=== VVTest write AAAA VV_Fr 0 10 200 B$$ === "
VVTest write AAAA VV_Fr 0 10 200 B$$ 
ret=\$?
#if [ ! \$ret ]; then exit 1; fi
echo [\$ret]
_EOF_
pid0=$!
echo pid0=$pid0

sleep 1

echo "=== VVAdmin CreateSnap VV_Fr SNAP ==="
start=`date +%s`
VVAdmin CreateSnap VV_Fr SNAP
if [ ! $? ]; then exit 1; fi
end=`date +%s`
dt=`expr $end - $start`
echo Elapsed time[$dt]

wait $pid0
