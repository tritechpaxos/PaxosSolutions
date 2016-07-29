echo "DropAllDB.sh xjPing"
DropAllDB.sh xjPing
if [ ! $? ]; then exit 1; fi
echo "InitDB.sh xjPing"
InitDB.sh xjPing
if [ ! $? ]; then exit 1; fi

echo
echo "VVAdmin CreateLV LV-A lvs-0 pool-0 20"
VVAdmin CreateLV LV-A lvs-0 pool-0 20
if [ ! $? ]; then exit 1; fi
echo "VVAdmin CreateLV LV-B lvs-0 pool-0 20"
VVAdmin CreateLV LV-B lvs-0 pool-0 20
if [ ! $? ]; then exit 1; fi

echo
echo "VVAdmin CreateVolume TEST target 10000000 2 pool-0"
VVAdmin CreateVolume TEST target 10000000 2 pool-0
if [ ! $? ]; then exit 1; fi
echo "VVTest w target TEST 8387584 100 8387584 1054"
VVTest w target TEST 8387584 100 8387584 1054
if [ ! $? ]; then exit 1; fi
echo	"VVTest r target TEST 8388598 100 8388598 30"
VVTest r target TEST 8388598 100 8388598 30
if [ ! $? ]; then exit 1; fi

echo
echo 	"VVAdmin CreateSnap TEST TEST0"
VVAdmin CreateSnap TEST TEST0
if [ ! $? ]; then exit 1; fi
echo	"VVTest r target TEST 8388598 100 8388598 30"
VVTest r target TEST 8388598 100 8388598 30
if [ ! $? ]; then exit 1; fi
echo	"VVTest w target TEST 8388603 100 8388603 ABCDEFGHIJ"
VVTest w target TEST 8388603 100 8388603 ABCDEFGHIJKLMN
if [ ! $? ]; then exit 1; fi
echo	"VVTest r target TEST 8388598 100 8388598 30"
VVTest r target TEST 8388598 100 8388598 30
if [ ! $? ]; then exit 1; fi

echo
echo 	"VVAdmin RestoreSnap TEST TEST0"
VVAdmin RestoreSnap TEST TEST0
if [ ! $? ]; then exit 1; fi
echo	"VVTest r target TEST 8388598 100 8388598 30"
VVTest r target TEST 8388598 100 8388598 30
if [ ! $? ]; then exit 1; fi

echo
echo 	"VVAdmin CreateSnap TEST TEST0"
VVAdmin CreateSnap TEST TEST0
if [ ! $? ]; then exit 1; fi
echo	"VVTest w target TEST 8388603 100 8388603 ABCDEFGHIJ"
VVTest w target TEST 8388603 100 8388603 ABCDEFGHIJKLMN
if [ ! $? ]; then exit 1; fi

echo
echo 	"VVAdmin CreateSnap TEST TEST0"
VVAdmin CreateSnap TEST TEST1
if [ ! $? ]; then exit 1; fi
echo	"VVTest w target TEST 8388603 100 8388603 ZZZZZZ"
VVTest w target TEST 8388607 100 8388607 ZZZZZZ
if [ ! $? ]; then exit 1; fi

echo	"VVTest r target TEST 8388598 100 8388598 30"
VVTest r target TEST 8388598 100 8388598 30
if [ ! $? ]; then exit 1; fi

echo
echo 	"VVAdmin DeleteSnap TEST1"
VVAdmin DeleteSnap TEST1
if [ ! $? ]; then exit 1; fi
echo	"VVTest r target TEST 8388598 100 8388598 30"
VVTest r target TEST 8388598 100 8388598 30
if [ ! $? ]; then exit 1; fi

echo
echo 	"VVAdmin DeleteSnap TEST0 (Bottom)"
VVAdmin DeleteSnap TEST0
if [ ! $? ]; then exit 1; fi
echo	"VVTest r target TEST 8388598 100 8388598 30"
VVTest r target TEST 8388598 100 8388598 30
if [ ! $? ]; then exit 1; fi

