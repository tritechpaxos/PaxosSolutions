#!/bin/sh

if [ $# -lt 3 ]
then
echo "USAGE:vv_start.sh cluster CSS DB [-i id] [-r ras_cell]"
exit 1
fi

cluster=$1
shift
conf=_$cluster.conf

CSS=$1
shift
DB=$1
shift

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


	ssh -n $host "$bin/VVProbe $cluster $id active > /dev/null 2>&1"
	ret=$?
	if [ $ret -ne 0 ]
	then
		IDs=$IDs"$id "

		eval bin=$bin

		eval bin_$id=\$bin
		eval root_$id=\$root
		eval host_$id=\$host
	fi

done < $conf

#echo cluster[$cluster] IDs[$IDs]

#
#	start
#
for id in $IDs
do
	eval host='$host_'$id
	eval bin='$bin_'$id
	eval root='$root_'$id

ROOT=$root

LOG_PIPE=/tmp/VV_LOG_PIPE
LOG_SIZE=100000
LOG_FILES=$bin/LogFiles

TGTD=$bin/tgtdVV

ssh -t $host "sudo nohup bash<<-_EOF_

if [ -e $LOG_PIPE ]; then rm -f $LOG_PIPE; fi

mkfifo $LOG_PIPE
rm -f $ROOT/LOG-VV*

nohup $LOG_FILES $ROOT/LOG-VV 100 < $LOG_PIPE &

export CSS=$CSS
export DB=$DB
export LOG_PIPE=$LOG_PIPE
export LOG_SIZE=$LOG_SIZE
export VV_NAME=$cluster
export VV_ID=$id
export PAXOS_CELL_CONFIG=$bin/
cd $ROOT
nohup $TGTD -f -d 1 > $ROOT/OUTtgt 2>&1 &

_EOF_"

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
		echo "sudo ssh -f $host $env $bin/VVProbe $cluster $id reg $ras_cell"
		ssh -f $host "sudo $env $bin/VVProbe $cluster $id reg $ras_cell"
	done
fi


exit 0
