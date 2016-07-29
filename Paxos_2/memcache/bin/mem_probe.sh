#!/bin/sh

if [ $# -lt 2 ]
then
echo "mem_adm.sh event conf id"
echo "mem_adm.sh item conf id"
echo "mem_adm.sh pair conf id"
exit 1
fi

env="LOG_SIZE=20000 "
files=20

SBIN=../../session/bin
cmd=$1
service=$2
conf=$HOME/_$service.conf

eval SBIN=$SBIN

while read id host bin root
do
if [ $id = $3 ]; then break; fi;

done < $conf

if [  $id = $3 ]
then

case $cmd in
	"event")
		ssh -t $host $bin/PaxosMemcacheAdm $id event ;;
	"item")
		ssh -t $host $bin/PaxosMemcacheAdm $id item ;;
	"pair")
		ssh -t $host $bin/PaxosMemcacheAdm $id pair ;;
esac
fi	

exit 0
