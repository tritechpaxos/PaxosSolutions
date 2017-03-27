#!/bin/sh

if [ $# -lt 4 ]
then
echo "USAGE:mem_autonomic css cluster [-i id] -r ras_cell"
exit 1
fi

css=$1
shift

cluster=$1
shift
conf=~/bin/$cluster.conf

space=$cluster

while [ "$1" != "" ]
do
	case "$1" in
		"-r") shift; ras_cell=$1; shift ;;
		"-i") shift; ID=$1; shift ;;
		* ) shift;;
	esac
done

#
#	Active check
#
while read id host bin root
do
	if [ ! $id ]; then break; fi	
	if [ `echo $id|cut -c1` = '#' ]; then continue; fi
	if [ "X$ID" != "X" -a "$ID" != "$id" ]; then continue; fi


	env="PAXOS_CELL_CONFIG=$bin/"
	ssh -n $host "$env $bin/PaxosMemcacheAdm $id active > /dev/null 2>&1"
	ret=$?
	if [ $ret -ne 0 ]
	then
		scp $host:$root/SHUTDOWN /tmp/SHUTDOWN > /dev/null 2>&1
		ret=$?
		if [ $ret -eq 0 ]; then continue; fi

		IDs=$IDs"$id "

		eval bin=$bin

		eval bin_$id=\$bin
		eval root_$id=\$root
		eval host_$id=\$host
	fi

done < $conf

#echo cluster[$cluster] IDs[$IDs]

env0="LOG_SIZE=20000"
files=20
#
#	start
#
for id in $IDs
do
	eval host='$host_'$id
	eval bin='$bin_'$id
	eval root='$root_'$id

	env="$env0 PAXOS_CELL_CONFIG=$bin/"

	echo "ssh -f $host \"cd $root;$env $bin/PaxosMemcache $id $css $space 3>&1 1>&2 2>&3 | $bin/LogFiles MEM$id $files\""
	ssh -f $host "cd $root;$env $bin/PaxosMemcache $id $css $space 3>&1 1>&2 2>&3 | $bin/LogFiles MEM$id $files"
done
sleep 1

#
#	Ras regist
#
if [ "X$ras_cell" != "X" ]; then
	for id in $IDs
	do
		eval host='$host_'$id
		eval bin='$bin_'$id
		eval root='$root_'$id

		env="PAXOS_CELL_CONFIG=$bin/"
 		echo "ssh -f $host $env $bin/PaxosMemcacheAdm $id ras $ras_cell"
		ssh -f $host "$env $bin/PaxosMemcacheAdm $id ras $ras_cell"
	done
fi
exit 0
