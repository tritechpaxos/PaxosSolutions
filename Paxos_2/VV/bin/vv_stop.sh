#!/bin/sh

if [ $# -lt 1 ]
then
echo "USAGE:vv_stop.sh cluster [-i id]"
exit 1
fi

cluster=$1
shift
conf=_$cluster.conf

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

	eval bin=$bin

	ssh -n $host "sudo $bin/VVProbe $cluster $id active > /dev/null 2>&1"
	ret=$?
	if [ $ret -eq 0 ]
	then
		IDs=$IDs"$id "
		eval bin_$id=\$bin
		eval root_$id=\$root
		eval host_$id=\$host
	fi
done < $conf

#echo bin_0=$bin_0

ADM=tgtadm

eval ADM=$ADM

for id in $IDs
do
	eval host='$host_'$id
	eval bin='$bin_'$id
	eval root='$root_'$id

	env="PAXOS_CELL_CONFIG=$bin/"
	echo "ssh -n $host sudo $env $bin/VVProbe $cluster $id stop"
	ssh -n $host "sudo $bin/VVProbe $cluster $id stop"

	echo "ssh -t $host sudo $ADM --op delete --mode system"
	ssh -t $host "sudo $ADM --op delete --mode system"
done

exit 0
