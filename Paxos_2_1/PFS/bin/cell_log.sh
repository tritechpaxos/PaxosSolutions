#!/bin/sh

if [ $# -lt 3 ]
then
echo "USAGE:cell_log.sh load service id"
exit 1
fi

F_LOGDIR="~/PAXOS/NWGadget/bin"
eval F_LOGDIR=$F_LOGDIR
LOG_PRINT="$F_LOGDIR/LogPrint"

F_PAXOSDIR="~/PAXOS/Paxos_2/paxos/bin"
eval F_PAXOSDIR=$F_PAXOSDIR
PAXOS_ADMIN="$F_PAXOSDIR/PaxosAdmin"

load=$1
service=$2
ID=$3

conf=~/bin/$service.conf

while read id paxos udp host session bin root
do
if [ $id = $ID ]; then break; fi;

done < $conf

if [  $id = $ID ]
then
	PAXOS_ADMIN="$bin/PaxosAdmin"
	eval PAXOS_ADMIN=$PAXOS_ADMIN

	env="PAXOS_CELL_CONFIG=$bin/"

	ssh $host "$env $PAXOS_ADMIN $service $id log"
	ssh $host "cat \`ls -Crt $root/$load-*\`" | $LOG_PRINT
	ssh $host ls -Crt $root/$load-*

fi	

exit 0
