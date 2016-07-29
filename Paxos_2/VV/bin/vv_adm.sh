#!/bin/sh -x

USAGE="vv_adm.sh cluster id show"
USAGE=${USAGE}"\nvv_adm.sh cluster id add target volume"
USAGE=${USAGE}"\nvv_adm.sh cluster id del"
USAGE=${USAGE}"\nvv_adm.sh cluster id ras ras_cell path"
USAGE=${USAGE}"\nvv_adm.sh cluster id {cache|VL|VV|VS|RC|MC}"

usage()
{
	echo $USAGE
	exit 1
}

if [ $# -lt 3 ]
then
	usage
fi

service=$1
conf=_$service.conf

while read id host bin root tgt
do
if [ ! $id ]; then break; fi	
if [ $id = "#" ]; then continue; fi

eval bin=$bin
eval root=$root

if [ $id = $2 ]; then break; fi	

done < $conf

root=$root

ADM=tgtadm
eval ADM=$ADM

case "$3" in
 "show")
	ssh -t $host "sudo $ADM --lld iscsi --op show --mode target"
	;;
 "add")
	if [ $# -lt 5 ]
	then
		usage
	fi
		ssh -t $host "sudo bash<<-_EOF_
$ADM --lld iscsi -o new -m target -t 1 -T $4
$ADM --lld iscsi -o new -m logicalunit -t 1 --lun 1 --bstype VV -b $5
$ADM --lld iscsi -o bind -m target -t 1 -I ALL
_EOF_"
	;;
 "del")
		ssh -t $host "sudo bash<<-_DEL_
$ADM --lld iscsi -o unbind -m target -t 1 -I ALL
$ADM --lld iscsi -o delete -m logicalunit -t 1 --lun 1
$ADM --lld iscsi -o delete -m target -t 1
_DEL_"
	;;
 "ras")
	if [ $# -lt 5 ]
	then
		usage
	fi
	RAS=$bin/VVProbe
	ssh -t $host "sudo $RAS $service $id $4 $5"
	;;
 "active|cache"|"VL"|"VV"|"VS"|"RC"|"MC")
	PROBE=$bin/VVProbe
	ssh -t $host "sudo $PROBE $service $id $3"
	;;
 *)
	usage	
esac

exit 0
