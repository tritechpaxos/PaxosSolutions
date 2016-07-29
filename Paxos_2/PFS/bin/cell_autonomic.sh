#!/bin/sh

#
#	Analyze parameters
#
args=$#
if [ $args -lt 6 ]
then
echo "USAGE:css_autonomic load cell -i id -r ras_cell [-s seq] [-m mode] ..."
	exit 1
fi

load=$1
shift

cell=$1
shift

while [ "$1" != "" ]
do
	case "$1" in
		"-i") shift; ID=$1; shift ;;
		"-r") shift; ras_cell=$1; shift ;;
		"-s") shift; seq=$1; shift ;;
		"-m") shift; mode=$1; shift ;;
		"-B") shift; BIN=$1; shift ;;
		* ) shift;;
	esac
done

if [ "X$ID" = "X" -o "X$ras_cell" = "X" ]; then
	exit 1
fi

env0="LOG_SIZE=20000"
files=20

conf=~/bin/$cell.conf

#
#	Check active
#
while read id paxos udp host session bin root
do
	if [ ! $id ]; then break; fi	
	if [ `echo $id|cut -c1` = '#' ]; then continue; fi
	if [ $ID ]; then if [ $ID != $id ]; then continue; fi; fi

	eval bin=$bin
	env="PAXOS_CELL_CONFIG=$bin/"

	ssh -n $host "$env $bin/PaxosAdmin $cell $id is_active > /dev/null 2>&1"
	ret=$?
	if [ $ret != 0 ]
	then
		scp $host:$root/SHUTDOWN /tmp/SHUTDOWN > /dev/null 2>&1
		ret=$?
		if [ $ret != 0 ]
		then
			IDs=$IDs"$id "
			eval host_$id=\$host
			eval bin_$id=\$bin
			eval root_$id=\$root
		fi
	fi
done < $conf

for id in $IDs
do
	eval host='$host_'$id
	eval bin='$bin_'$id
	eval root='$root_'$id

	env="$env0 PAXOS_CELL_CONFIG=$bin/"

	echo "ssh -f $host \"cd $root;$env $bin/$load $cell $id $root FALSE 3>&1 1>&2 2>&3 | $bin/LogFiles $load $files\""

	ssh -f $host "cd $root;$env $bin/$load $cell $id $root FALSE 3>&1 1>&2 2>&3 | $bin/LogFiles $load $files"

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
