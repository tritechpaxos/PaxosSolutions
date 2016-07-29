#!/bin/sh

if [ $# -lt 3 ]
then
echo "USAGE:cell_stop.sh load cell comment [-i id]"
exit 1
fi

load=$1
shift

cell=$1
conf=~/bin/$cell.conf
shift

comment=$1
shift

while [ "$1" != "" ]
do
	case "$1" in
		"-i") shift; ID=$1; shift ;;
		* ) shift;;
	esac
done

while read id paxos udp host session bin root
do
	if [ ! $id ]; then break; fi	
	if [ `echo $id|cut -c1` = '#' ]; then continue; fi
	if [ "X$ID" != "X" -a "$ID" != "$id" ]; then continue; fi

	env="PAXOS_CELL_CONFIG=$bin/"
	ssh -n $host "$env $bin/PaxosAdmin $cell $id is_active > /dev/null 2>&1"
	ret=$?
	if [ $ret = 0 ]; then
		ssh -n $host "$env $bin/PaxosAdmin $cell $id is_leader > /dev/null 2>&1"
		ret=$?
		if [ $ret = 0 -a ! $leader_id ]; then
			leader_id=$id
			leader_host=$host
			leader_bin=$bin
		else
			IDs=$IDs"$id "
			eval bin_$id=\$bin
			eval root_$id=\$root
			eval host_$id=\$host
		fi
	fi
done < $conf

for id in $IDs
do
	eval host='$host_'$id
	eval bin='$bin_'$id
	eval root='$root_'$id

	env="PAXOS_CELL_CONFIG=$bin/"
	shutdown=$bin/PaxosSessionShutdown

	echo "ssh -f $host $env $shutdown $cell $id $comment"
	ssh -f $host "$env $shutdown $cell $id $comment"
done

if [ $leader_id ]; then
	env="PAXOS_CELL_CONFIG=$leader_bin/"
	shutdown=$leader_bin/PaxosSessionShutdown

	echo "LEADER:ssh -f $leader_host $env $shutdown $cell $leader_id $comment"
	ssh -f $leader_host "$env $shutdown $cell $leader_id $comment"
fi

exit 0
