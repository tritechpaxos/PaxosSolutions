#!/bin/sh

echo	"1. === Drop A, A_1, A_2 ==="
xjDrop $1 A
xjDrop $1 A_1
xjDrop $1 A_2

echo	"2. === Create A A_1 A_2 ==="
xjCre $1 A 2 10
if [ $? -ne 0 ] ; then exit 2 ; fi

echo	"3. === create view V_A as select * from A; ==="
xjSql $1 <<_EOF_ | fgrep "Total=8" > z
drop view V_A;
create view V_A as select * from A;
select * from V_A;
quit;
_EOF_
if [ $? -ne 0 ] ; then exit 3 ; fi
count=$(wc -l < z)
if [ $count -ne 1 ]; then exit 4; fi

echo	"4. === create view V_A as select * from A where NAME='5'; ==="
xjSql $1 <<_EOF_ | fgrep "Total=1" > z
drop view V_A;
create view V_A as select * from A where NAME='5';
select * from V_A;
quit;
_EOF_
if [ $? -ne 0 ] ; then exit 5 ; fi
count=$(wc -l < z)
if [ $count -ne 1 ]; then exit 6; fi

echo	"5. === create view V_A as "
echo	"	select * from A"
echo	"	 UNION ALL select * from A_1; ==="
echo	"	 UNION ALL select * from A_2; ==="
xjSql $1 <<_EOF_ | fgrep "Total=24" > z
drop view V_A;
create view V_A as select * from A union all select * from A_1 union all select * from A_2;
select * from V_A;
quit;
_EOF_
if [ $? -ne 0 ] ; then exit 7 ; fi
count=$(wc -l < z)
if [ $count -ne 1 ]; then exit 8; fi

echo"6. === update A set NAME='UPDATE' where NAME='1'; select * from V_A; ==="
echo	"	DEPENDENT check"
xjSql $1 <<_EOF_ | fgrep "[UPDATE]" > z
update A set NAME='UPDATE' where NAME='1';
select * from V_A;
quit;
_EOF_
if [ $? -ne 0 ] ; then exit 9 ; fi
count=$(wc -l < z)
if [ $count -ne 1 ]; then exit 10; fi

echo	"7. === select * from V_A where NAME != '[UPDATE]'; ==="
echo	"	ROLLBACK check"
xjSql $1 <<_EOF_ | fgrep "Total=24" > z
select * from V_A where NAME != '[UPDATE]';
quit;
_EOF_
if [ $? -ne 0 ] ; then exit 11 ; fi
count=$(wc -l < z)
if [ $count -ne 1 ]; then exit 12; fi

echo	"8. === select A_1.NAME NAME1, A_2.NAME NAME2"
echo	"	 from A_1, A_2 where A_1.INT=A_2.INT; ==="
echo	"	REPRESENTATION"
xjSql $1 <<_EOF_ | fgrep "[NAME1(" > z
select A_1.NAME NAME1, A_2.NAME NAME2 from A_1, A_2 where A_1.INT=A_2.INT;
quit;
_EOF_
if [ $? -ne 0 ] ; then exit 13 ; fi
count=$(wc -l < z)
if [ $count -ne 1 ]; then exit 14; fi

echo	"9. === select *"
echo	"	 from A_1 a, A_2 b where a.INT=b.INT; ==="
echo	"	ALIAS"
xjSql $1 <<_EOF_ | fgrep "Total=8" > z
select * from A_1 a, A_2 b where a.INT=b.INT;
quit;
_EOF_
if [ $? -ne 0 ] ; then exit 15 ; fi
count=$(wc -l < z)
if [ $count -ne 1 ]; then exit 16; fi

echo	"10. === select a.NAME b.NAME"
echo	"	 from A_1 a, A_2 b where a.INT=b.INT; ==="
echo	"	ALIAS"
xjSql $1 <<_EOF_ | fgrep "Total=8" > z
select a.NAME, b.NAME from A_1 a, A_2 b where a.INT=b.INT;
quit;
_EOF_
if [ $? -ne 0 ] ; then exit 17 ; fi
count=$(wc -l < z)
if [ $count -ne 1 ]; then exit 18; fi

echo	"11. === select a.NAME NAME1, b.NAME NAME2"
echo	"	 from A_1 a, A_2 b where a.INT=b.INT; ==="
echo	"	REPRESENTATION and ALIAS"
xjSql $1 <<_EOF_ > z
select a.NAME NAME1, b.NAME NAME2 from A_1 a, A_2 b where a.INT=b.INT;
quit;
_EOF_
if [ $? -ne 0 ] ; then exit 19 ; fi
count=$(fgrep "Total=8" z |wc -l )
if [ $count -ne 1 ]; then exit 20; fi

echo	"12. === select A_2.NAME from A, A_1, A_2 "
echo	"	where A.INT=A_1.INT and A_1.HEX=A_2.HEX; ==="
xjSql $1 <<_EOF_ > z
select A_2.NAME from A, A_1, A_2 where A.INT=A_1.INT and A_1.HEX=A_2.HEX;
quit;
_EOF_
if [ $? -ne 0 ] ; then exit 21 ; fi
count=$(fgrep "Total=8" z |wc -l )
if [ $count -ne 1 ]; then exit 22; fi

echo	"13. === Drop A, A_1, A_2 ==="
xjDrop $1 A
xjDrop $1 A_1
xjDrop $1 A_2

echo	"14. === Create A A_1 A_2 ==="
xjCre $1 A 2 10
if [ $? -ne 0 ] ; then exit 2 ; fi

echo	"15. === create view V_A as select * from A, A_1 where A.INT=A_1.INT; ==="
xjSql $1 <<_EOF_ | fgrep "Total=8" > z
drop view V_A;
create view V_A as select * from A, A_1 where A.INT = A_1.INT;
select * from V_A;
quit;
_EOF_
if [ $? -ne 0 ] ; then exit 5 ; fi
count=$(wc -l < z)
if [ $count -ne 1 ]; then exit 6; fi

echo	"16. === select * from V_A ==="
echo	"	DEPENDENCY after drop A"
xjSql $1 <<_EOF_  > z
drop table A;
select * from V_A;
quit;
_EOF_
#if [ $? -ne 0 ] ; then exit 5 ; fi
count=$( fgrep "no entry" z |wc -l )
if [ $count -ne 1 ]; then exit 6; fi

echo	"17. === select * from V_A ==="
echo	"	RECOVER after create A"
xjCre $1 A
xjSql $1 <<_EOF_  > z
select * from V_A;
quit;
_EOF_
if [ $? -ne 0 ] ; then exit 5 ; fi
count=$( fgrep "Total=8" z |wc -l )
if [ $count -ne 1 ]; then exit 6; fi

xjDrop $1 A
xjDrop $1 A_1
xjDrop $1 A_2
xjCre $1 A 2 10

echo	"18. === select * from A  where INT=1 or INT=2; ==="
xjSql $1 <<_EOF_  > z
select * from A  where INT=1 or INT=2;
quit;
_EOF_
if [ $? -ne 0 ] ; then exit 5 ; fi
count=$( fgrep "Total=2" z |wc -l )
if [ $count -ne 1 ]; then exit 6; fi

echo	"19. === select * from A  where INT=1 and INT=2; ==="
xjSql $1 <<_EOF_  > z
select * from A  where INT=1 and INT=2;
quit;
_EOF_
if [ $? -ne 0 ] ; then exit 5 ; fi
count=$( fgrep "Total=0" z |wc -l )
if [ $count -ne 1 ]; then exit 6; fi

echo	"20. === select * from A  where 1=INT or 1=LONG; ==="
xjSql $1 <<_EOF_  > z
select * from A  where 1=INT or 1=LONG;
quit;
_EOF_
if [ $? -ne 0 ] ; then exit 5 ; fi
count=$( fgrep "Total=1" z |wc -l )
if [ $count -ne 1 ]; then exit 6; fi

echo	"21. === select * from A  where 1=INT or 2=LONG; ==="
xjSql $1 <<_EOF_  > z
select * from A  where 1=INT or 2=LONG;
quit;
_EOF_
if [ $? -ne 0 ] ; then exit 5 ; fi
count=$( fgrep "Total=2" z |wc -l )
if [ $count -ne 1 ]; then exit 6; fi

echo	"22. === select * from A, A_1; ==="
xjSql $1 <<_EOF_  > z
select * from A, A_1;
quit;
_EOF_
if [ $? -ne 0 ] ; then exit 5 ; fi
count=$( fgrep "Total=64" z |wc -l )
if [ $count -ne 1 ]; then exit 6; fi

echo	"23. === select * from A, A_1 where A.INT=1; ==="
xjSql $1 <<_EOF_  > z
select * from A, A_1 where A.INT=1;
quit;
_EOF_
if [ $? -ne 0 ] ; then exit 5 ; fi
count=$( fgrep "Total=8" z |wc -l )
if [ $count -ne 1 ]; then exit 6; fi

echo	"24. === select * from A, A_1 where A.INT=1 and 2=A_1.LONG; ==="
xjSql $1 <<_EOF_  > z
select * from A, A_1 where A.INT=1 and 2=A_1.LONG;
quit;
_EOF_
if [ $? -ne 0 ] ; then exit 5 ; fi
count=$( fgrep "Total=1" z |wc -l )
if [ $count -ne 1 ]; then exit 6; fi

echo	"25. === select * from A, A_1 where A.INT=1 or 2=A_1.LONG; ==="
xjSql $1 <<_EOF_  > z
select * from A, A_1 where A.INT=1 or 2=A_1.LONG;
quit;
_EOF_
if [ $? -ne 0 ] ; then exit 5 ; fi
count=$( fgrep "Total=1" z |wc -l )
if [ $count -ne 1 ]; then exit 6; fi

echo	"26. === select * from A, A_1 where (A.INT=1 or A.INT=2) and (3=A_1.LONG or 4=A_1.LONG); ==="
xjSql $1 <<_EOF_  > z
select * from A, A_1 where (A.INT=1 or A.INT=2) and (3=A_1.LONG or 4=A_1.LONG);
quit;
_EOF_
if [ $? -ne 0 ] ; then exit 5 ; fi
count=$( fgrep "Total=4" z |wc -l )
if [ $count -ne 1 ]; then exit 6; fi

echo "#### OK ####"
exit 0
