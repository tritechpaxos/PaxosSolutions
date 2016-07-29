#!/bin/bash

beep='echo -n -e \a'

if [ $# -lt 2 ]
then
echo "USAGE:cmdb_monitor.sh ras_cell service"
exit 1
fi

ras=$1
service=$2
shift

env="LOG_SIZE=20000 "
files=20

conf=$HOME/_$service.conf

hosts=()
roots=()
bins=()
modes=()
comments=()
starts=()

seq=0

Status()
{
	echo "--- Status ---"
	for (( i = 0; i < ${#hosts[@]}; ++i ))
	do
		echo "$i ${hosts[$i]} ${modes[$i]} ${comments[$i]}"
	done
}

#
#	Up Server
#
echo	"### Init Mode ###"

while read id paxos udp host session bin root
do
	if [ ! $id ]; then break; fi	
	if [ $id = "#" ]; then continue; fi

	hosts[$id]=$paxos
	modes[$id]="D"
	bins[$id]=$bin
	roots[$id]=$root
	starts[$id]=`date +"%s"`

	echo "ssh -f $host cd $root;$env $bin/xjPingPaxos $service $id $root TRUE 0 3>&1 1>&2 2>&3 | $bin/LogFiles CMDB $files"

	ssh -f $host "cd $root;$env $bin/xjPingPaxos $service $id $root TRUE 0 3>&1 1>&2 2>&3 | $bin/LogFiles CMDB $files"

done < $conf

Status

echo	""
echo	"### Register into ras_cell ###"

while read id paxos udp host session bin root
do
	if [ ! $id ]; then break; fi	
	if [ $id = "#" ]; then continue; fi

	echo "ssh -f $host $bin/PFSRasCluster $service $id $ras $service"
	ssh -f $host "$bin/PFSRasCluster $service $id $ras $service"

done < $conf

Status
sleep 1

echo	""
echo	"### Event Mode ###"

PFSRasMonitor $ras $service |
while read mode path id comment
do
	modes[$id]=$mode
	comments[$id]=$comment

	echo "=== $seq. Event $mode $path $id $comment ==="
	$beep
	sleep 1

	seq=`expr $seq + 1`

	if [ $mode = "D" ]
	then
		host=${hosts[$id]}
		bin=${bins[$id]}
		root=${roots[$id]}

		echo "scp $host:$root/SHUTDOWN /tmp/SHUTDOWN > /dev/null 2>&1"
		scp $host:$root/SHUTDOWN /tmp/SHUTDOWN > /dev/null 2>&1
		ret=$?
		if [ $ret = 0 ]
		then
			echo "$host $id `cat /tmp/SHUTDOWN` is forced down."
		else
			now=`date +"%s"`
			expire=`expr ${starts[id]} + 120`
			if [ $now -lt $expire ]
			then
				echo "$host $id is dupricated down."
			else
				echo "ssh -f $host cd $root;$env $bin/PFSServer $service $id $root FALSE 3>&1 1>&2 2>&3 | $bin/LogFiles CSS $files"

				ssh -f $host "cd $root;$env $bin/xjPingPaxos $service $id $root FALSE 3>&1 1>&2 2>&3 | $bin/LogFiles CMDB $files"

				sleep 1
				echo "ssh -f $host $bin/PFSRasCluster $service $id $ras $service"
				ssh -f $host "$bin/PFSRasCluster $service $id $ras $service"
			fi
		fi
	fi
	Status
done

exit 0
