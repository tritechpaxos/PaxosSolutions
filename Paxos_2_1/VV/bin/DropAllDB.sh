#!/bin/sh

if [ $# -lt 1 ]
then
echo "USAGE:Drop db_cell"
exit 1
fi

db_cell=$1

#
#	Drop all tables
#

F_PAXOSDIR="~/PAXOS/Paxos_2/xjPing/bin"
eval F_PAXOSDIR=$F_PAXOSDIR

$F_PAXOSDIR/xjListPaxos $db_cell \
| grep -v "\.bytes"	| grep -o "[\[][0-9a-zA-Z_.-]\+\]" \
| sed -e "s/\[/drop table '/" -e "s/\]/';/" \
| $F_PAXOSDIR/xjSqlPaxos $db_cell

echo xjSqlPaxos=$?

exit 0
