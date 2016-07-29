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
CMDS_PFS="$F_PFSDIR/PFS* $F_PFSDIR/cell_autonomic.sh"

if [ $# -lt 1 ]; then
echo "USAGE:css_deploy service"
exit 1
fi

service=$1
conf=$HOME/_$service.conf

while read id paxos udp host session bin root
do
#echo "[$id $host $session $paxos $udp $bin $root]"
if [ ! $id ]; then break; fi	
if [ $id = "#" ]; then continue; fi

#
#	Deploy bin
#
T_PFS=$bin

PAXOS_CELL_CONFIG=$bin

echo "ssh -t $host mkdir -p $T_PFS"
sh <<EOT
ssh -t -t $host mkdir -p $T_PFS
EOT

echo "cp $CMDS_PFS ~/bin/"
cp $CMDS_PFS ~/bin/
echo "scp $CMDS_PFS $host:$T_PFS"
sh <<EOT
scp $CMDS_PFS $host:$T_PFS
EOT

echo "cp $CMDS_PAXOS ~/bin/"
cp $CMDS_PAXOS ~/bin/
echo "scp $CMDS_PAXOS $host:$T_PFS"
sh <<EOT
scp $CMDS_PAXOS $host:$T_PFS
EOT

echo "cp $CMDS_SESSION ~/bin/"
cp $CMDS_SESSION ~/bin/
echo "scp $CMDS_SESSION $host:$T_PFS"
sh <<EOT
scp $CMDS_SESSION $host:$T_PFS
EOT

echo "cp $CMDS_LOG ~/bin/"
cp $CMDS_LOG ~/bin/
echo "scp $CMDS_LOG $host:$T_PFS"
sh <<EOT
scp $CMDS_LOG $host:$T_PFS
EOT

#
#	Deploy conf
#
#echo "scp $conf $host:~"
#sh <<EOT
#scp $conf $host:~
#EOT

echo "cp $conf ~/bin/$service.conf"
cp $conf ~/bin/$service.conf

echo "scp $conf $host:$PAXOS_CELL_CONFIG/$service.conf"
sh <<EOT
scp $conf $host:$PAXOS_CELL_CONFIG/$service.conf
EOT

echo	"ssh -t $host rm -rf $root"
sh <<EOT
ssh -t -t $host rm -rf $root
EOT

echo	"ssh -t $host mkdir -p $root/PFS"
sh <<EOT
ssh -t -t $host mkdir -p $root/PFS
EOT

echo	"ssh -t $host mkdir -p $root/DATA"
sh <<EOT
ssh -t -t $host mkdir -p $root/DATA
EOT

done < $conf

exit 0
