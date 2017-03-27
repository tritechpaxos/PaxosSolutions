#!/bin/sh

if [ $# -lt 1 ]
then
echo "USAGE:TestDelegation.sh CSS"
exit 1
fi

cell=$1
if [ $# -lt 2 ]
then
cnt=$2
fi

echo "1.=== PFSDelegation $cell delegR A ==="
PFSDelegation $cell delegR A $cnt
if [ ! $? ]; then exit 1; fi

echo "2.=== PFSDelegation $cell delegW A ==="
PFSDelegation $cell delegW A $cnt
if [ ! $? ]; then exit 1; fi

echo "3.=== PFSDelegation $cell delegRW A ==="
PFSDelegation $cell delegRW A $cnt
if [ ! $? ]; then exit 1; fi

echo "4.=== PFSDelegation $cell delegWR A ==="
PFSDelegation $cell delegWR A $cnt
if [ ! $? ]; then exit 1; fi

echo "5.=== PFSDelegation $cell delegRRet A ==="
PFSDelegation $cell delegRRet A $cnt
if [ ! $? ]; then exit 1; fi

echo "6.=== PFSDelegation $cell delegWRet A ==="
PFSDelegation $cell delegWRet A $cnt
if [ ! $? ]; then exit 1; fi

echo "7.=== PFSDelegation $cell delegR_W A ==="
PFSDelegation $cell delegR_W A $cnt
if [ ! $? ]; then exit 1; fi

echo "8.=== PFSDelegation $cell delegW_R A ==="
PFSDelegation $cell delegW_R A $cnt
if [ ! $? ]; then exit 1; fi

echo "9.=== PFSDelegation $cell recuR A ==="
PFSDelegation $cell recuR A $cnt
if [ ! $? ]; then exit 1; fi

echo "10.=== PFSDelegation $cell recuW A ==="
PFSDelegation $cell recuW A $cnt
if [ ! $? ]; then exit 1; fi

echo "11.=== R-R  ==="
sh <<_EOF_ &
echo "=== PFSDelegation $cell delegR A ==="
PFSDelegation $cell delegR A $cnt
ret=\$?
echo ret0[\$ret]
if [ ! \$ret ]; then exit 1; fi
exit 0
_EOF_
pid0=$!
echo pid0=$pid0

sleep 1

sh <<_EOF_ &
echo "=== PFSDelegation $cell delegR A ==="
PFSDelegation $cell delegR A $cnt
ret=\$?
echo ret1[\$ret] 
if [ ! \$ret ]; then exit 1; fi
exit 0
_EOF_
pid1=$!
echo pid1=$pid1

wait $pid1
wait $pid0

echo "12.=== R-W  ==="
sh <<_EOF_ &
echo "=== PFSDelegation $cell delegR A ==="
PFSDelegation $cell delegR A $cnt
ret=\$?
echo ret0[\$ret]
if [ ! \$ret ]; then exit 1; fi
exit 0
_EOF_
pid0=$!
echo pid0=$pid0

sleep 1

sh <<_EOF_ &
echo "=== PFSDelegation $cell delegW A ==="
PFSDelegation $cell delegW A $cnt
ret=\$?
echo ret1[\$ret]
if [ ! \$ret ]; then exit 1; fi
exit 0
_EOF_
pid1=$!
echo pid1=$pid1

wait $pid1
wait $pid0

echo "13.=== W-R  ==="
sh <<_EOF_ &
echo "=== PFSDelegation $cell delegW A ==="
PFSDelegation $cell delegW A $cnt
ret=\$?
echo ret0[\$ret]
if [ ! \$ret ]; then exit 1; fi
exit 0
_EOF_
pid0=$!
echo pid0=$pid0

sleep 1

sh <<_EOF_ &
echo "=== PFSDelegation $cell delegR A ==="
PFSDelegation $cell delegR A $cnt
ret=\$?
echo ret1[\$ret]
if [ ! \$ret ]; then exit 1; fi
exit 0
_EOF_
pid1=$!
echo pid1=$pid1

wait $pid1
wait $pid0

echo "13.=== W-W  ==="
sh <<_EOF_ &
echo "=== PFSDelegation $cell delegW A ==="
PFSDelegation $cell delegW A $cnt
ret=\$?
echo ret0[\$ret]
if [ ! \$ret ]; then exit 1; fi
exit 0
_EOF_
pid0=$!
echo pid0=$pid0

sleep 1

sh <<_EOF_ &
echo "=== PFSDelegation $cell delegW A ==="
PFSDelegation $cell delegW A $cnt
ret=\$?
echo ret1[\$ret]
if [ ! \$ret ]; then exit 1; fi
exit 0
_EOF_
pid1=$!
echo pid1=$pid1

wait $pid1
wait $pid0

echo "14.=== nest delegation  ==="
sh <<_EOF_ &
echo "=== PFSDelegation $cell nest A B ==="
PFSDelegation $cell nest A B
ret=\$?
echo ret0[\$ret]
if [ ! \$ret ]; then exit 1; fi
exit 0
_EOF_
pid0=$!
echo pid0=$pid0

sleep 1

sh <<_EOF_ &
echo "=== PFSDelegation $cell nest A B==="
PFSDelegation $cell nest A B
ret=\$?
echo ret1[\$ret]
if [ ! \$ret ]; then exit 1; fi
exit 0
_EOF_
pid1=$!
echo pid1=$pid1

wait $pid1
wait $pid0

echo
echo	"!!! SUCCESS !!!"
echo
exit 0
