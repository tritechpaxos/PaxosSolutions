#!/bin/sh

if [ $# -lt 2 ]
then
echo "USAGE:vv_log.sh cluster id"
exit 1
fi

F_LOGDIR="~/PAXOS/NWGadget/bin"
eval F_LOGDIR=$F_LOGDIR
LOG_PRINT="$F_LOGDIR/LogPrint"

cluster=$1
conf=_$cluster.conf

ID=$2

while read id host bin root tgt
do
if [ ! $id ]; then break; fi	
if [ $id = "#" ]; then continue; fi

if [ $id = $ID ]; then break; fi	

done < $conf

if [  $id = $ID ]
then

	ssh $host "sudo $bin/VVProbe $cluster $id log"
	ssh $host "cat \`ls -Crt $root/LOG-VV-*\`" | $LOG_PRINT
	ssh $host ls -Crt $root/LOG-VV-*

fi

exit 0

