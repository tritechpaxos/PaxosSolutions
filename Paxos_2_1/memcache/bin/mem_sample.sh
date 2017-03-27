#!/bin/sh
 
args=$#
if [ $args -lt 2 ]
then
echo "USAGE:mem_test.sh group cell [id]"
exit 1
fi

group=$1
cell=$2
conf=$HOME/_$group.conf

while read id host bin root
do
if [ ! $id ]; then break; fi	
if [ `echo $id|cut -c1` = '#' ]; then continue; fi
if [ $args -ge 3 ]; then
	if [ $3 != $id ]; then continue; fi
fi

#relay memcached
echo "ssh -f $host $bin/PaxosMemcacheAdm $id add localhost:11212 paxos03:11211"
ssh -f $host $bin/PaxosMemcacheAdm $id add localhost:11212 paxos03:11211

echo "ssh -f $host $bin/PaxosMemcacheAdm $id add localhost:11212 paxos03:11211"
ssh -f $host $bin/PaxosMemcacheAdm $id add localhost:11213 paxos04:11211

#ras cell directory
ssh -f $host $bin/PaxosMemcacheAdm $id ras $cell

sleep 1
done < $conf

exit 0
