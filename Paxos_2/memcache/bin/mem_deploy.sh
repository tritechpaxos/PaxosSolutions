#!/bin/sh

F_LOGDIR="~/PAXOS/NWGadget/bin"
F_MEM_DIR="~/PAXOS/Paxos_2/memcache/bin"

eval F_LOGDIR=$F_LOGDIR
eval F_MEM_DIR=$F_MEM_DIR

CMDS_LOG="$F_LOGDIR/LogFiles $F_LOGDIR/LogPrint"
CMDS_MEM="$F_MEM_DIR/PaxosMemcache* $F_MEM_DIR/mem_autonomic.sh"

if [ $# -lt 1 ]; then
echo "USAGE:mem_deploy cluster"
exit 1
fi

cluster=$1
conf=_$cluster.conf

cp $conf ~/bin/$cluster.conf
cp $CMDS_MEM ~/bin/
cp $CMDS_LOG ~/bin/

while read id host bin root
do
#echo "[$id $host $bin $root]"
if [ ! $id ]; then continue; fi	
if [ `echo $id|cut -c1` = '#' ]; then continue; fi
#
#	Deploy bin
#
T_BIN=$bin
T_ROOT=$root

PAXOS_CELL_CONFIG=$bin

echo "ssh -t $host mkdir -p $T_BIN"
sh <<EOT
ssh -t -t $host mkdir -p $T_BIN
EOT

echo "ssh -t $host rm -rf $T_ROOT"
sh <<EOT
ssh -t -t $host rm -rf $T_ROOT
EOT

echo "ssh -t $host mkdir -p $T_ROOT"
sh <<EOT
ssh -t -t $host mkdir -p $T_ROOT
EOT

echo "scp $CMDS_MEM $host:$T_BIN"
sh <<EOT
scp $CMDS_MEM $host:$T_BIN
EOT

echo "scp $CMDS_LOG $host:$T_BIN"
sh <<EOT
scp $CMDS_LOG $host:$T_BIN
EOT

#
#	Deploy conf
#

echo "scp $conf $host:$PAXOS_CELL_CONFIG/$cluster.conf"
sh <<EOT
scp $conf $host:$PAXOS_CELL_CONFIG/$cluster.conf
EOT

done < $conf

exit 0
