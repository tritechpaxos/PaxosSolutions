#!/bin/sh

if [ $# -lt 3 ]
then
echo "cmdb_adm.sh shutdown service id comment"
echo "cmdb_adm.sh restart service id"

echo "cmdb_adm.sh paxos service id {control|paxos}"
echo "cmdb_adm.sh session service id {FdEvent|out_of_band|cell}"
echo "cmdb_adm.sh ras service id ras_cell path"
echo "cmdb_adm.sh change_member service id in_addr port"

echo "cmdb_adm.sh info service id"
echo "cmdb_adm.sh list service id"
echo "cmdb_adm.sh table service id table_name"

echo "cmdb_adm.sh cache service id {block|meta|fd}"

exit 1
fi

SBIN=../../session/bin
cmd=$1
service=$2
conf=$HOME/_$service.conf

eval SBIN=$SBIN

case $cmd in
	"change_member")
		$SBIN/PaxosSessionChangeMember $service $3 $4 $5
		exit 0 ;;
esac

while read id paxos udp host session bin root
do
if [ $id = $3 ]; then break; fi;

done < $conf

env="LOG_SIZE=20000 "
files=20

if [  $id = $3 ]
then

case $cmd in
	"paxos")
		ssh -t $host $bin/PaxosAdmin $service $id $4;;
	"session")
		ssh -t $host $bin/PaxosSessionProbe $service $id $4;;
	"ras")
		ssh -t $host $bin/PFSRasCluster $service $id $4 $5;;

	"shutdown")
		ssh -t $host $bin/PaxosSessionShutdown $service $id $3;;
	"restart")
		scp $host:$root/SHUTDOWN /tmp/SHUTDOWN > /dev/null 2>&1
		ret=$?
		if [ $ret = 0 ]
		then
			read seq comment < /tmp/SHUTDOWN

echo "ssh -f $host \"cd $root;$env $bin/xjPingPaxos $service $id $root TRUE $seq 3>&1 1>&2 2>&3 | $bin/LogFiles CSS $files\""

ssh -f $host "cd $root;$env $bin/xjPingPaxos $service $id $root TRUE $seq 3>&1 1>&2 2>&3 | $bin/LogFiles CSS $files"
		else
echo "ssh -f $host \"cd $root;$env $bin/xjPingPaxos $service $id $root FALSE 3>&1 1>&2 2>&3 | $bin/LogFiles CSS $files\""

ssh -f $host "cd $root;$env $bin/xjPingPaxos $service $id $root FALSE 3>&1 1>&2 2>&3 | $bin/LogFiles CSS $files"
		fi

#ssh -f $host "cd $root;$env $bin/xjPingPaxos  $service $id $root -A $4 3>&1 1>&2 2>&3 | $bin/LogFiles CMDB $files";;
		;;
	"info")
		ssh -t $host $bin/xjInfoAdmin $service:$id;;
	"list")
		ssh -t $host $bin/xjListAdmin $service:$id;;
	"table")
		ssh -t $host $bin/xjTableAdmin $service:$id $4;;
	"cache")
		ssh -t $host $bin/FCProbeAdmin $service:$id $4;;
esac
fi	

exit 0
