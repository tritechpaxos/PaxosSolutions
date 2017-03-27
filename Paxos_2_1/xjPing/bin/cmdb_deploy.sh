#!/bin/sh

F_LOGDIR="~/PAXOS/NWGadget/bin"
F_PAXOSDIR="~/PAXOS/Paxos_2/paxos/bin"
F_SESSIONDIR="~/PAXOS/Paxos_2/session/bin"
F_PFSDIR="~/PAXOS/Paxos_2/PFS/bin"

eval F_LOGDIR=$F_LOGDIR
eval F_SDIR=$F_SDIR
eval F_PAXOSDIR=$F_PAXOSDIR
eval F_SESSIONDIR=$F_SESSIONDIR
eval F_PFSDIR=$F_PFSDIR

CMDS_LOG="$F_LOGDIR/LogFiles $F_LOGDIR/LogPrint"
CMDS_PAXOS="$F_PAXOSDIR/PaxosAdmin"
CMDS_SESSION="$F_SESSIONDIR/PaxosSession*"
CMDS_PFS="$F_PFSDIR/PFSProbe $F_PFSDIR/PFSRasCluster"

if [ $# -lt 1 ]
then
echo "cmdb_deploy.sh service"
exit 1
fi

service=$1
conf=_$service.conf

F_BIN="~/PAXOS/Paxos_2/xjPing/bin"
eval F_BIN=$F_BIN
CMDS="$F_BIN/xjPingPaxos $F_BIN/*Admin $F_BIN/cell_autonomic.sh"

while read id paxos udp host session bin root
do
if [ ! $id ]; then break; fi	
if [ $id = "#" ]; then continue; fi

eval bin=$bin
eval root=$root

PAXOS_CELL_CONFIG=$bin
#
#	Deploy conf and Init
#
#echo "scp $conf $host:~"
#scp $conf $host:~
#echo "cp $conf ~/bin/$service.conf"
cp $conf ~/bin/$service.conf

echo "scp $conf $host:$PAXOS_CELL_CONFIG/$service.conf"
sh <<EOT
scp $conf $host:$PAXOS_CELL_CONFIG/$service.conf
EOT

echo	"ssh -t $host rm -rf $root"
sh <<EOT
ssh -t -t $host rm -rf $root
EOT

echo	"ssh -t $host mkdir -p $root/DB"
sh <<EOT
ssh -t -t $host mkdir -p $root/DB
EOT

echo	"ssh -t $host mkdir -p $root/DATA"
sh <<EOT
ssh -t -t $host mkdir -p $root/DATA
EOT

#
#	Deploy commands
#

echo "ssh -t $host mkdir -p $bin"
sh <<EOT
ssh -t -t $host mkdir -p $bin
EOT

echo "cp $CMDS ~/bin/"
cp $CMDS ~/bin/
echo "scp $CMDS $host:$bin"
scp $CMDS $host:$bin

echo "cp $CMDS_PFS ~/bin/"
cp $CMDS_PFS ~/bin/
echo "scp $CMDS_PFS $host:$bin"
scp $CMDS_PFS $host:$bin

echo "cp $CMDS_SESSION ~/bin/"
cp $CMDS_SESSION ~/bin/
echo "scp $CMDS_SESSION $host:$bin"
scp $CMDS_SESSION $host:$bin

echo "cp $CMDS_PAXOS ~/bin/"
cp $CMDS_PAXOS ~/bin/
echo "scp $CMDS_PAXOS $host:$bin"
scp $CMDS_PAXOS $host:$bin

echo "cp $CMDS_LOG ~/bin/"
cp $CMDS_LOG ~/bin/
echo "scp $CMDS_LOG $host:$bin"
scp $CMDS_LOG $host:$bin

done < $conf

exit 0
