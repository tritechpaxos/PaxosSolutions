#!/bin/sh

if [ $# -lt 3 ]
then
echo "css_adm.sh paxos service id {control|paxos}"
echo "css_adm.sh session service id {FdEvent|out_of_band|cell}"
echo "css_adm.sh css service id {accept|path|lock|queue|outofband|meta|fd|block|ras}"
echo "css_adm.sh ras service id ras_cell path"
echo "css_adm.sh change_member service id in_addr port"
exit 1
fi

env0="LOG_SIZE=20000 "
files=20

SBIN=../../session/bin
cmd=$1
service=$2
conf=~/bin/$service.conf

eval SBIN=$SBIN

case $cmd in
	"change_member")
		$env $SBIN/PaxosSessionChangeMember $service $3 $4 $5
		exit 0 ;;
esac

while read id paxos udp host session bin root
do
if [ $id = $3 ]; then break; fi;

done < $conf

if [  $id = $3 ]
then

env="PAXOS_CELL_CONFIG=$bin/"

case $cmd in
	"paxos")
		ssh -t $host "$env $bin/PaxosAdmin $service $id $4";;
	"session")
		ssh -t $host "$env $bin/PaxosSessionProbe $service $id $4";;
	"css")
		ssh -t $host "$env $bin/PFSProbe $service $id $4";;
	"ras")
		ssh -t $host "$env $bin/PFSRasCluster $service $id $4 $5";;
esac
fi	

exit 0
