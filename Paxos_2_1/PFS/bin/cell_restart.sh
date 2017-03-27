#!/bin/sh

#
#	Analyze parameters
#
args=$#
if [ $args -lt 2 ]
then
echo "USAGE:cell_restart.sh load cell [-i id] [-r ras_cell]"
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
#	Check active
#
max=$((-1))
while read id paxos udp host session bin root
do
	if [ ! $id ]; then break; fi	
	if [ `echo $id|cut -c1` = '#' ]; then continue; fi
	if [ "X$ID" != "X" -a "$ID" != "$id" ]; then continue; fi

	ssh -n $host "$bin/PaxosAdmin $cell $id is_active > /dev/null 2>&1"
	ret=$?
	if [ $ret != 0 ]
	then
		IDs=$IDs"$id "
		eval bin_$id=\$bin
		eval root_$id=\$root
		eval host_$id=\$host

		scp $host:$root/SHUTDOWN /tmp/SHUTDOWN > /dev/null 2>&1
		ret=$?
		if [ $ret = 0 ]
		then
			read seq comment < /tmp/SHUTDOWN
			num=$((seq))
			if [ $max -eq -1 ]	# first
			then 
				max=$num; young=$id
			elif [ $max -lt $num ]
			then 
				diff=$((num-max))
				if [ $diff -lt 2147483647 ] 
				then 
					max=$num; young=$id
				fi;
			else
				diff=$((max-num))
				if [ $diff -ge 2147483647 ] 
				then 
					max=$num; young=$id
				fi;
			fi
			eval seq_$id=$seq
			eval comment_$id=$comment
		fi
	fi
done < $conf

#
#	Start youngest	
#
if [ $young ]
then

	eval bin='$bin_'$young
	eval root='$root_'$young
	eval host='$host_'$young

	env="$env0 PAXOS_CELL_CONFIG=$bin/"
	echo "ssh -f $host \"cd $root;$env $bin/$load $cell $young $root TRUE $max 3>&1 1>&2 2>&3 | $bin/LogFiles $load $files\""

	ssh -f $host "cd $root;$env $bin/$load $cell $young $root TRUE $max 3>&1 1>&2 2>&3 | $bin/LogFiles $load $files"

	sleep 1
fi

for id in $IDs
do
	if [ $young ]; then
		if [ $young = $id ]; then continue; fi
	fi

	eval host='$host_'$id
	eval bin='$bin_'$id
	eval root='$root_'$id
	eval seq='$seq_'$id

	env="$env0 PAXOS_CELL_CONFIG=$bin/"
	if [ $seq ]
	then
		echo "ssh -f $host \"cd $root;$env $bin/$load $cell $id $root TRUE $seq 3>&1 1>&2 2>&3 | $bin/LogFiles $load $files\""

		ssh -f $host "cd $root;$env $bin/$load $cell $id $root TRUE $seq 3>&1 1>&2 2>&3 | $bin/LogFiles $load $files"

	else
		if [ $young ]
		then
			echo "ssh -f $host \"cd $root;$env $bin/$load $cell $id $root TRUE 3>&1 1>&2 2>&3 | $bin/LogFiles $load $files\""

			ssh -f $host "cd $root;$env $bin/$load $cell $id $root TRUE 3>&1 1>&2 2>&3 | $bin/LogFiles $load $files"
		else
			echo "ssh -f $host \"cd $root;$env $bin/$load $cell $id $root FALSE 3>&1 1>&2 2>&3 | $bin/LogFiles $load $files\""

			ssh -f $host "cd $root;$env $bin/$load $cell $id $root FALSE 3>&1 1>&2 2>&3 | $bin/LogFiles $load $files"
		fi

	fi

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
