#!/bin/sh

F_LOGDIR="~/PAXOS/NWGadget/bin"
F_PFSDIR="~/PAXOS/Paxos_2/PFS/bin"
F_VVDIR="~/PAXOS/Paxos_2/VV/bin"

eval F_LOGDIR=$F_LOGDIR
eval F_PFSDIR=$F_PFSDIR
eval F_VVDIR=$F_VVDIR

CMDS_LOG="$F_LOGDIR/LogFiles $F_LOGDIR/LogPrint"
CMDS_PFS="$F_PFSDIR/PFSRasCluster"
TGTD=tgtdVV
CMDS_VV="$F_VVDIR/$TGTD $F_VVDIR/VVProbe $F_VVDIR/vv_autonomic.sh"

if [ $# -lt 1 ]; then
echo "USAGE:vv_deploy cluster"
exit 1
fi

cluster=$1
conf=_$cluster.conf

echo "cp $conf ~/bin/$cluster.conf"
cp $conf ~/bin/$cluster.conf
cp $CMDS_VV ~/bin/
cp $CMDS_PFS ~/bin/ 
cp $CMDS_LOG ~/bin/

while read id host bin root
do
echo "[$id $host $bin $root]"
if [ ! $id ]; then break; fi	
if [ `echo $id|cut -c1` = '#' ]; then continue; fi

#
#	Deploy bin
#
BIN=$bin

echo "ssh -t $host mkdir -p $BIN"
sh <<EOT
ssh -t -t $host mkdir -p $BIN
EOT

echo "scp $CMDS_VV $host:$BIN"
sh <<EOT
scp $CMDS_VV $host:$BIN
EOT

echo "scp $CMDS_PFS $host:$BIN"
sh <<EOT
scp $CMDS_PFS $host:$BIN
EOT

echo "scp $CMDS_LOG $host:$BIN"
sh <<EOT
scp $CMDS_LOG $host:$BIN
EOT

#
#	Deploy conf
#

PAXOS_CELL_CONFIG=$bin

echo "scp $conf $host:$PAXOS_CELL_CONFIG/$cluster.conf"
sh <<EOT
scp $conf $host:$PAXOS_CELL_CONFIG/$cluster.conf
EOT

echo	"ssh -t $host rm -rf $root"
sh <<EOT
ssh -t -t $host rm -rf $root
EOT

echo	"ssh -t $host mkdir -p $root"
sh <<EOT
ssh -t -t $host mkdir -p $root
EOT

done < $conf

exit 0
