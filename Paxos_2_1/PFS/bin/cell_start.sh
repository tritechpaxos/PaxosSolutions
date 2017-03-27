#!/bin/sh

#
#	Analyze parameters
#
args=$#
if [ $args -lt 2 ]
then
echo "USAGE:cell_start.sh load cell [-i id] [-r ras_cell]"
exit 1
fi

load=$1
shift

cell=$1
shift

while [ "$1" != "" ]
do
	case "$1" in
		"-r") shift; ras_cell=$1; shift ;;
		"-i") shift; ID=$1; shift ;;
		* ) shift;;
	esac
done

env0="LOG_SIZE=20000"
files=20

conf=~/bin/$cell.conf

#
#	Active check
#
while read id paxos udp host session bin root
do
	if [ ! $id ]; then break; fi	
	if [ `echo $id|cut -c1` = '#' ]; then continue; fi
	if [ "X$ID" != "X" -a "$ID" != "$id" ]; then continue; fi

	ssh -n $host "$bin/PaxosAdmin $cell $id is_active > /dev/null 2>&1"
	ret=$?
	if [ $ret != 0 ]; then
		IDs=$IDs"$id "
		eval bin_$id=\$bin
		eval root_$id=\$root
		eval host_$id=\$host
	fi
done < $conf

#
#	Start
#
for id in $IDs
do
	eval host='$host_'$id
	eval bin='$bin_'$id
	eval root='$root_'$id

	env="$env0 PAXOS_CELL_CONFIG=$bin/"
	echo "ssh -f $host \"cd $root;$env $bin/$load $cell $id $root TRUE 0 3>&1 1>&2 2>&3 | $bin/LogFiles $load $files\""

	ssh -f $host "cd $root;$env $bin/$load $cell $id $root TRUE 0 3>&1 1>&2 2>&3 | $bin/LogFiles $load $files"
	sleep 1

done

#
#	Register
#
if [ "X$ras_cell" != "X" ]; then
	for id in $IDs
	do
		eval host='$host_'$id
		eval bin='$bin_'$id
		eval root='$root_'$id

		env="PAXOS_CELL_CONFIG=$bin/"

		echo "ssh -f $host $env $bin/PFSRasCluster $cell $id $ras_cell RAS/$load/$cell"
		ssh -f $host "$env $bin/PFSRasCluster $cell $id $ras_cell RAS/$load/$cell"
	done
fi
exit 0
