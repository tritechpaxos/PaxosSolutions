#!/bin/sh

if [ $# -lt 1 ]
then
echo "USAGE:DeleDB.sh db_cell table"
exit 1
fi

db_cell=$1
#
#	Create tables
#

F_PAXOSDIR="~/PAXOS/Paxos_2/xjPing/bin"
eval F_PAXOSDIR=$F_PAXOSDIR

$F_PAXOSDIR/xjSqlPaxos $db_cell <<_EOF_
drop table '$2';
commit;
_EOF_

echo xjSqlPaxos=$?

exit 0
