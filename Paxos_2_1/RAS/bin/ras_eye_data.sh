#!/bin/sh

#
#	Analyze parameters
#
args=$#
if [ $args -lt 4 ]
then
echo "USAGE:ras_eye_data.sh cluster {set|unset|reg|unreg} -r ras_cell"
exit 1
fi

cluster=$1
shift

cmd=$1
shift

while [ "$1" != "" ]
do
	case "$1" in
		"-r") shift; ras_cell=$1; shift ;;
		"-i") shift; ID=$1; shift ;;
		* ) shift;;
	esac
done

conf=~/bin/$cluster.conf

#
#	Active check
#
while read id host bin root
do
	if [ ! $id ]; then break; fi	
	if [ `echo $id|cut -c1` = '#' ]; then continue; fi
	if [ "X$ID" != "X" -a "$ID" != "$id" ]; then continue; fi

	eval bin=$bin

	env="PAXOS_CELL_CONFIG=$bin/"
	ssh -n $host "$bin/RasEyeAdm $cluster $id active > /dev/null 2>&1"
	ret=$?
	if [ $ret -eq 0 ]
	then
		EyeIDs=$EyeIDs"$id "
		eval Eyebin_$id=\$bin
		eval Eyeroot_$id=\$root
		eval Eyehost_$id=\$host
	fi
done < $conf

#echo EyeIDs[$EyeIDs]

for id in $EyeIDs
do
	eval host='$Eyehost_'$id
	eval bin='$Eyebin_'$id
	eval root='$Eyeroot_'$id

	env="PAXOS_CELL_CONFIG=$bin/"

	case $cmd in
	"set")
		#self set
		echo "ssh -f $host $env $bin/RasEyeAdm $cluster $id set $ras_cell RasEye/$cluster $bin/RasMail $bin/ras_autonomic.sh"
		ssh -f $host "$env $bin/RasEyeAdm $cluster $id set $ras_cell RasEye/$cluster $bin/RasMail $bin/ras_autonomic.sh"

		#css set
		echo "ssh -f $host $env $bin/RasEyeAdm $cluster $id set $ras_cell PFSServer/css $bin/RasMail $bin/cell_autonomic.sh PFSServer"
		ssh -f $host "$env $bin/RasEyeAdm $cluster $id set $ras_cell PFSServer/css $bin/RasMail \"$bin/cell_autonomic.sh PFSServer\""

		#xjPing set
		echo "ssh -f $host $env $bin/RasEyeAdm $cluster $id set $ras_cell xjPingPaxos/xjPing $bin/RasMail $bin/cell_autonomic.sh xjPingPaxos"
		ssh -f $host "$env $bin/RasEyeAdm $cluster $id set $ras_cell xjPingPaxos/xjPing $bin/RasMail \"$bin/cell_autonomic.sh xjPingPaxos\""

		#lvs_0 set
		echo "ssh -f $host $env $bin/RasEyeAdm $cluster $id set $ras_cell LVServer/lvs_0 $bin/RasMail $bin/cell_autonomic.sh LVServer"
		ssh -f $host "$env $bin/RasEyeAdm $cluster $id set $ras_cell LVServer/lvs_0 $bin/RasMail \"$bin/cell_autonomic.sh LVServer\""

		#tgtdVV set
		echo "ssh -f $host $env $bin/RasEyeAdm $cluster $id set $ras_cell VV/vv $bin/RasMail $bin/vv_autonomic.sh vv css xjPing"
		ssh -f $host "$env $bin/RasEyeAdm $cluster $id set $ras_cell VV/vv $bin/RasMail \"$bin/vv_autonomic.sh vv css xjPing\""

		#Memcache set
		echo "ssh -f $host $env $bin/RasEyeAdm $cluster $id set $ras_cell PaxosMemcache/Memcache $bin/RasMail $bin/mem_autonomic.sh css"
		ssh -f $host "$env $bin/RasEyeAdm $cluster $id set $ras_cell PaxosMemcache/Memcache $bin/RasMail \"$bin/mem_autonomic.sh css\""
		;;

	"unset")
		echo "ssh -f $host $env $bin/RasEyeAdm $cluster $id unset $ras_cell RasEye/$cluster"
		ssh -f $host "$env $bin/RasEyeAdm $cluster $id unset $ras_cell RasEye/$cluster"
		#css set
		echo "ssh -f $host $env $bin/RasEyeAdm $cluster $id unset $ras_cell PFSServer/css"
		ssh -f $host "$env $bin/RasEyeAdm $cluster $id unset $ras_cell PFSServer/css"

		#xjPing set
		echo "ssh -f $host $env $bin/RasEyeAdm $cluster $id unset $ras_cell xjPingPaxos/xjPing"
		ssh -f $host "$env $bin/RasEyeAdm $cluster $id unset $ras_cell xjPingPaxos/xjPing"

		#lvs_0 set
		echo "ssh -f $host $env $bin/RasEyeAdm $cluster $id unset $ras_cell LVServer/lvs_0"
		ssh -f $host "$env $bin/RasEyeAdm $cluster $id unset $ras_cell LVServer/lvs_0"

		#tgtdVV set
		echo "ssh -f $host $env $bin/RasEyeAdm $cluster $id unset $ras_cell VV/vv"
		ssh -f $host "$env $bin/RasEyeAdm $cluster $id unset $ras_cell VV/vv"

		#Memcache set
		echo "ssh -f $host $env $bin/RasEyeAdm $cluster $id unset $ras_cell PaxosMemcache/Memcache"
		ssh -f $host "$env $bin/RasEyeAdm $cluster $id unset $ras_cell PaxosMemcache/Memcache"
		;;
	"reg")
		#self reg
		echo "ssh -f $host $env $bin/RasEyeAdm $cluster $id regist_me $ras_cell"
		ssh -f $host "$env $bin/RasEyeAdm $cluster $id regist_me $ras_cell"
		;;

	"unreg")
		echo "ssh -f $host $env $bin/RasEyeAdm $cluster $id unregist_me $ras_cell"
		ssh -f $host "$env $bin/RasEyeAdm $cluster $id unregist_me $ras_cell"
		;;
	esac
done

exit 0
