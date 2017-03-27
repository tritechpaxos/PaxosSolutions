#!/bin/sh

echo	"=== Drop A ==="
xjDrop $1 A

echo	"=== Create A ==="
xjCre $1 A
if [ $? -ne 0 ] ; then exit 2 ; fi

echo	"=== Check A ==="
xjSql $1 <<_EOF_ | fgrep "Total=8" > z
select * from A;
quit;
_EOF_
if [ $? -ne 0 ] ; then exit 3 ; fi
count=$(wc -l < z)
if [ $count -ne 1 ]; then exit 4; fi

echo	"=== Insert into A ==="
xjSql $1 <<_EOF_ | fgrep "Total=9" > z
insert into A values('8',0x8,8,8,8.0,8,8);
select * from A;
quit;
_EOF_
if [ $? -ne 0 ] ; then exit 5 ; fi
count=$(wc -l < z)
if [ $count -ne 1 ]; then exit 6; fi

echo	"=== Update  nowhere ==="
xjSql $1 <<_EOF_ | fgrep "[UPDATE]" > z
update A set NAME='UPDATE',HEX=0xa,INT=10,UINT=10,FLOAT=10.0,LONG=10,ULONG=10;
select * from A;
quit;
_EOF_
if [ $? -ne 0 ] ; then exit 7 ; fi
count=$(wc -l < z)
if [ $count -ne 8 ]; then exit 8; fi

echo	"=== Update  where ==="
xjSql $1 <<_EOF_ | fgrep "[UPDATE]" > z
insert into A values('8',0x8,8,8,8.0,8,8);
update A set NAME='UPDATE',HEX=0xa,INT=10,UINT=10,FLOAT=10.0,LONG=10,ULONG=10 where NAME='8';
select * from A;
quit;
_EOF_
if [ $? -ne 0 ] ; then exit 9 ; fi
count=$(wc -l < z)
if [ $count -ne 1 ]; then exit 10; fi

echo	"=== Delete  nowhere ==="
xjSql $1 <<_EOF_ | fgrep "Total=0" > z
delete from A;
select * from A;
quit;
_EOF_
if [ $? -ne 0 ] ; then exit 11 ; fi
count=$(wc -l < z)
if [ $count -ne 1 ]; then exit 12; fi

echo	"=== Update  where ==="
xjSql $1 <<_EOF_ | fgrep "Total=7" > z
delete from A where NAME='5';
select * from A;
quit;
_EOF_
if [ $? -ne 0 ] ; then exit 13 ; fi
count=$(wc -l < z)
if [ $count -ne 1 ]; then exit 14; fi

echo	"=== Delete  nowhere and Rollback ==="
xjSql $1 <<_EOF_ | fgrep "Total=8" > z
delete from A;
rollback;
select * from A;
quit;
_EOF_
if [ $? -ne 0 ] ; then exit 12 ; fi
count=$(wc -l < z)
if [ $count -ne 1 ]; then exit 13; fi

echo	"=== Delete  nowhere and Commit ==="
xjSql $1 <<_EOF_ | fgrep "Total=0" > z
delete from A;
commit;
select * from A;
quit;
_EOF_
if [ $? -ne 0 ] ; then exit 12 ; fi
count=$(wc -l < z)
if [ $count -ne 1 ]; then exit 13; fi

echo "#### OK ####"
exit 0
