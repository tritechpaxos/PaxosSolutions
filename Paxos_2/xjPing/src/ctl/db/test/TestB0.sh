#!/bin/sh

echo	"=== Drop A, A_1 ==="
xjDrop $1 A
xjDrop $1 A_1

echo	"=== Create A A_1 100000 ==="
xjCre $1 A 1 100000
if [ $? -ne 0 ] ; then exit 2 ; fi

echo	"=== create view V_A as select * from A, A_1 where A.INT=A_1.INT; ==="
time xjSql $1 <<_EOF_ > z
drop view V_A;
create view V_A as select * from A, A_1 where A.INT=A_1.INT;
quit;
_EOF_
if [ $? -ne 0 ] ; then exit 3 ; fi
#count=$(fgrep "Total=1" z |wc -l)
#if [ $count -ne 1 ]; then exit 4; fi

echo	"1-st === select * from V_A where INT=5; ==="
time xjSql $1 <<_EOF_ > z
select * from V_A where INT=5;
quit;
_EOF_
if [ $? -ne 0 ] ; then exit 3 ; fi
count=$(fgrep "Total=1" z |wc -l)
if [ $count -ne 1 ]; then exit 4; fi

echo	"2-nd === select * from V_A where INT=5; ==="
time xjSql $1 <<_EOF_ > z
select * from V_A where INT=5;
quit;
_EOF_
if [ $? -ne 0 ] ; then exit 3 ; fi
count=$(fgrep "Total=1" z |wc -l)
if [ $count -ne 1 ]; then exit 4; fi
date

echo	"### OK ###"
exit 0

