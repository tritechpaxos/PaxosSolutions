#!/bin/sh

echo	"=== Check A ==="
xjSql $1 <<_EOF_ | fgrep "Total=8" > z
select * from A;
quit;
_EOF_
if [ $? -ne 0 ] ; then exit 3 ; fi
count=$(wc -l < z)
if [ $count -ne 1 ]; then exit 4; fi
echo	"=== END ==="

