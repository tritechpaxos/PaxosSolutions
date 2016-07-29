#!/bin/sh

if [ $# -lt 2 ]
then
echo "mem_adm.sh add cluster [localhost:]port server[:port]"
echo "mem_adm.sh del cluster [localhost:]port server[:port]"
echo "mem_adm.sh ras cluster ras_cell"
echo "mem_adm.sh unras cluster ras_cell"
exit 1
fi

env="LOG_SIZE=20000 "
files=20

SBIN=../../session/bin
cmd=$1
service=$2
conf=$HOME/_$service.conf

#eval SBIN=$SBIN

while read id host bin root
do
if [ ! $id ]; then continue; fi	
if [ `echo $id|cut -c1` = '#' ]; then continue; fi

case $cmd in
	"add")
		if [ $# -lt 4 ]
		then
			echo "mem_adm.sh add conf [localhost:]port server[:port]"
			exit 1
		fi
		if [ `echo $3 | cut -c1-9` != "localhost" ]
		then
			client=$host:$3
		else
			client=$3
		fi	
		echo "ssh $host $bin/PaxosMemcacheAdm $id add $client $4"
		ssh -f $host $bin/PaxosMemcacheAdm $id add $client $4
		;;
	"del")
		if [ $# -lt 4 ]
		then
			echo "mem_adm.sh del conf [localhost:]port server[:port]"
			exit 1
		fi
		if [ `echo $3 | cut -c1-9` != "localhost" ]
		then
			client=$host:$3
		else
			client=$3
		fi	
		echo "ssh $host $bin/PaxosMemcacheAdm $id del $client $4"
		ssh -f $host $bin/PaxosMemcacheAdm $id del $client $4
		;;
	"ras")
		echo "ssh $host $bin/PaxosMemcacheAdm $id ras $3"
		ssh -f $host $bin/PaxosMemcacheAdm $id ras $3
		;;
	"unras")
		echo "ssh $host $bin/PaxosMemcacheAdm $id unras $3"
		ssh -f $host $bin/PaxosMemcacheAdm $id unras $3
		;;
esac

sleep 1
done < $conf

exit 0
