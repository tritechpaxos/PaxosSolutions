#!/bin/sh

if [ $# -lt 2 ]
then
echo "USAGE:raseye_log.sh cluster id"
exit 1
fi

F_LOGDIR="~/PAXOS/NWGadget/bin"
eval F_LOGDIR=$F_LOGDIR
LOG_PRINT="$F_LOGDIR/LogPrint"

cluster=$1
conf=~/bin/$cluster.conf

while read id host bin root
do
if [ ! $id ]; then continue; fi	
if [ `echo $id|cut -c1` = '#' ]; then continue; fi
if [ $id = $2 ]; then break; fi;

done < $conf

if [  $id = $2 ]
then
eval bin=$bin
RASEYE_ADM="$bin/RasEyeAdm"
env="PAXOS_CELL_CONFIG=$bin/"

echo "ssh $host $env $RASEYE_ADM $cluster $id log"
ssh $host "$env $RASEYE_ADM $cluster $id log"
ssh $host "cat \`ls -Crt $root/RasEye-*\`" | $LOG_PRINT
ssh $host ls -Crt $root/RasEye-*

fi	

exit 0
