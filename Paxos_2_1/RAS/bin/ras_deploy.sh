#!/bin/bash

F_LOGDIR="~/PAXOS/NWGadget/bin"
F_PFSDIR="~/PAXOS/Paxos_2/PFS/bin"
F_RASDIR="~/PAXOS/Paxos_2/RAS/bin"

eval F_LOGDIR=$F_LOGDIR
eval F_RASDIR=$F_RASDIR

CMDS_LOG="$F_LOGDIR/LogFiles $F_LOGDIR/LogPrint"
CMDS_PFS="$F_PFSDIR/PFSRasCluster"

CMDS_RAS="$F_RASDIR/RasEye $F_RASDIR/RasEyeAdm $F_RASDIR/RasMail $F_RASDIR/ras_autonomic.sh $F_RASDIR/ras_eye_data.sh $F_RASDIR/ras_mail_data.sh"

if [ $# -lt 1 ]; then
echo "USAGE:ras_deploy cluster"
exit 1
fi

cluster=$1
conf=_$cluster.conf

while read id host bin root
do
if [ ! $id ]; then continue; fi	
if [ `echo $id|cut -c1` = '#' ]; then continue; fi

#
#	Deploy bin
#
BIN=$bin
PAXOS_CELL_CONFIG=$bin

echo "ssh -t $host mkdir -p $BIN"
sh <<EOT
ssh -t -t $host mkdir -p $BIN
EOT

echo "cp $CMDS_RAS ~/bin/"
cp $CMDS_RAS ~/bin/
echo "scp $CMDS_RAS $host:$BIN"
sh <<EOT
scp $CMDS_RAS $host:$BIN
EOT

echo "cp $CMDS_LOG ~/bin/"
cp $CMDS_RAS ~/bin/
echo "scp $CMDS_LOG $host:$BIN"
sh <<EOT
scp $CMDS_LOG $host:$BIN
EOT

#
#	Deploy conf
#
#echo "scp $conf $host:~"
#sh <<EOT
#scp $conf $host:~
#EOT
echo "cp $conf ~/bin/$cluster.conf"
cp $conf ~/bin/$cluster.conf
echo "scp $conf $host:$PAXOS_CELL_CONFIG/$cluster.conf"
sh <<EOT
scp $conf $host:$PAXOS_CELL_CONFIG/$cluster.conf
EOT


echo root[$root]
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
