#!/bin/sh

if [ $# -lt 2 ]
then
echo "db_adm.sh list db_cell"
echo "db_adm.sh find db_cell table [search]"
exit 1
fi

F_PAXOSDIR="~/PAXOS/Paxos_2"
xjPing=$F_PAXOSDIR/xjPing/bin
eval xjPing=$xjPing

cmd=$1
cell=$2

case $cmd in
	"list")
		$xjPing/xjListPaxos $cell \
		| grep -v "\.bytes"	| grep -o "[\[][0-9a-zA-Z_.-]\+\]"
		;;
	"find")
		table=$3
		if [ $# -ge 4 ] 
		then 
			search=$4 
		fi
		$xjPing/findPaxos $cell $table $search
		;;
esac

exit 0
