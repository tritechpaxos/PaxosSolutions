#!/bin/sh

if [ $# -lt 2 ]
then
echo "USAGE:mem_log.sh cluster id"
exit 1
fi

F_LOGDIR="~/PAXOS/NWGadget/bin"
eval F_LOGDIR=$F_LOGDIR
LOG_PRINT="$F_LOGDIR/LogPrint"

cluster=$1
conf=~/bin/$cluster.conf

while read id host bin root
do
if [ $id = $2 ]; then break; fi;

done < $conf

if [  $id = $2 ]
then
ADMIN="$bin/PaxosMemcacheAdm"
eval ADMIN=$ADMIN
ssh $host $ADMIN $id log
ssh $host "cat \`ls -Crt $root/MEM$id-*\`" | $LOG_PRINT
#ssh $host ls -Crt $root/CSS-*

fi	

exit 0
