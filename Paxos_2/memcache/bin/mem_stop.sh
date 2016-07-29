#!/bin/sh

args=$#
if [ $args -lt 1 ]
then
echo "USAGE:mem_stop.sh cluster [id]"
exit 1
fi

cluster=$1
conf=_$cluster.conf

while read id host bin root
do
if [ ! $id ]; then break; fi	
if [ `echo $id|cut -c1` = '#' ]; then continue; fi
if [ $args -ge 2 ]; then
	if [ $2 != $id ]; then continue; fi
fi

shutdown=$bin/PaxosMemcacheAdm

echo "ssh -f $host $shutdown $id stop"
ssh -f $host $shutdown $id stop

sleep 1
done < $conf

exit 0
