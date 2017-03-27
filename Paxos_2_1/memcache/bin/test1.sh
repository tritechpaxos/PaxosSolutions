#!/bin/sh

if [ $# -lt 2 ]
then
echo "USAGE:test1.sh service memcached"
exit 1
fi

service=$1
ID=0
conf=$HOME/_$service.conf
client=/pss/libmemcached-1.0.18/clients
memcached=$2
local=127.0.0.1

while read id host bin root
do
if [ $id = $ID ]; then break; fi;

done < $conf

if [  $id = $ID ]
then

echo "=== memcapable ==="
	ssh -t $host "cd $client;time ./memcapable -h $memcached"
	ssh -t $host "cd $client;time ./memcapable -h $local"

echo "=== memcp ==="
	ssh -t $host "cd $client;time ./memcp --servers=$memcached memcp.cc"
	ssh -t $host "cd $client;time ./memcp --servers=$local memcp.cc"

echo "=== memcat ==="
	ssh -t $host \
		"cd $client;time ./memcat --servers=$memcached memcp.cc --file=z"
	ssh -t $host \
		"cd $client;time ./memcat --servers=$local memcp.cc --file=z"

echo "=== ascii->binary ==="
	ssh -t $host "bash<<-_EOF_
		cd $client
		./memcp --servers=$local memcp.cc
		./memcat --servers=$local memcp.cc --file=z --binary
		diff z memcp.cc
_EOF_"

echo "=== binary->ascii ==="
	ssh -t $host "bash<<-_EOF_
		cd $client
		./memcp --servers=$local memcat.cc --binary
		./memcat --servers=$local memcat.cc --file=z
		diff z memcat.cc
_EOF_"

while read id host bin root
do
if [ $id = "0" ]
then 
	host0=$host 
	bin0=$bin
	root0=$root
fi
if [ $id = "1" ]
then 
	host1=$host 
	bin1=$bin
	root1=$root
fi

done < $conf

echo "### delegation check ###"
	ssh -t $host0 "cd $client;time ./memcp --servers=$memcached memcp.cc"
	ssh -t $host0 "cd $bin0;./PaxosMemcacheAdm 0 item"
	ssh -t $host1 "cd $client;time ./memcp --servers=$memcached memcp.cc"
	ssh -t $host0 "cd $bin0;./PaxosMemcacheAdm 0 item"
fi

exit 0
