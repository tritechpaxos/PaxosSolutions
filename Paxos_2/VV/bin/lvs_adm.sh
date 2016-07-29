#!/bin/sh

if [ $# -lt 3 ]
then
echo "lvs_adm.sh shutdown service id comment"
echo "lvs_adm.sh restart service id"
echo "lvs_adm.sh paxos service id {control|paxos}"
echo "lvs_adm.sh session service id {FdEvent|out_of_band|cell}"
echo "lvs_adm.sh ras service id ras_cell path"
echo "lvs_adm.sh change_member service id in_addr port"
echo "lvs_adm.sh cache service id {block|fd|outofband|accept}"
exit 1
fi

SBIN=../../session/bin
cmd=$1
service=$2
conf=_$service.conf

eval SBIN=$SBIN

env="LOG_SIZE=20000 "
files=20

case $cmd in
	"change_member")
		$SBIN/PaxosSessionChangeMember $service $3 $4 $5
		exit 0 ;;
esac

while read id paxos udp host session bin root
do
if [ $id = $3 ]; then break; fi;

done < $conf

if [ $id = $3 ]
then

case $cmd in
	"shutdown")
		ssh -t $host $bin/PaxosSessionShutdown $service $id $4 ;;
	"restart")
		scp $host:$root/SHUTDOWN /tmp/SHUTDOWN > /dev/null 2>&1
		ret=$?
		if [ $ret = 0 ]
		then
			read seq comment < /tmp/SHUTDOWN

echo "ssh -f $host \"cd $root;$env $bin/LVServer $service $id $root TRUE $seq 3>&1 1>&2 2>&3 | $bin/LogFiles $service $files\""

ssh -f $host "cd $root;$env $bin/LVServer $service $id $root TRUE $seq 3>&1 1>&2 2>&3 | $bin/LogFiles $service $files"
		else
echo "ssh -f $host \"cd $root;$env $bin/LVServer $service $id $root FALSE 3>&1 1>&2 2>&3 | $bin/LogFiles $service $files\""

ssh -f $host "cd $root;$env $bin/LVServer $service $id $root FALSE 3>&1 1>&2 2>&3 | $bin/LogFiles $service $files"

		fi
		;;
	"paxos")
		ssh -t $host $bin/PaxosAdmin $service $id $4;;
	"session")
		ssh -t $host $bin/PaxosSessionProbe $service $id $4;;
	"ras")
		ssh -t $host $bin/PFSRasCluster $service $id $4 $5;;
	"cache")
		ssh -t $host $bin/LVProbe $service $id $4;;
esac
fi	

exit 0
