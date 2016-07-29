#!/bin/sh

if [ $# -lt 3 ]
then
echo "USAGE:ras_adm.sh set cluster ras_cell path mail \"autonomic...\""
echo "USAGE:ras_adm.sh unset cluster ras_cell path"
echo "USAGE:ras_adm.sh add cluster MailAddr"
echo "USAGE:ras_adm.sh del cluster MailAddr"
echo "USAGE:ras_adm.sh regist_me cluster cell"
echo "USAGE:ras_adm.sh unregist_me cluster cell"
echo "USAGE:ras_adm.sh ras_dump cluster id"
echo "USAGE:ras_adm.sh mail_dump cluster id"
echo "USAGE:ras_adm.sh regist_dump cluster id"
echo "USAGE:ras_adm.sh session_dump cluster id"
exit 1
fi

cmd=$1
cluster=$2
conf=~/bin/$cluster.conf

#
while read id host bin root
do
if [ ! $id ]; then continue; fi	
if [ `echo $id|cut -c1` = '#' ]; then continue; fi

eval bin=$bin

case $cmd in
	"set")
		if [ $# -lt 5 ]; then exit 1; fi
		echo "ssh -f $host $bin/RasEyeAdm $cluster $id $cmd $3 $4 $5 $6"
		ssh -f $host "$bin/RasEyeAdm $cluster $id $cmd $3 $4 $5 \"$6\"" ;;
	"unset")
		if [ $# -lt 3 ]; then exit 1; fi
		echo "ssh -f $host $bin/RasEyeAdm $cluster $id $cmd $3 $4"
		ssh -f $host "$bin/RasEyeAdm $cluster $id $cmd $3 $4" ;;
	"regist_me")
		if [ $# -lt 3 ]; then exit 1; fi
		echo "ssh -f $host $bin/RasEyeAdm $cluster $id $cmd $3"
		ssh -f $host "$bin/RasEyeAdm $cluster $id $cmd $3" ;;
	"unregist_me")
		if [ $# -lt 3 ]; then exit 1; fi
		echo "ssh -f $host $bin/RasEyeAdm $cluster $id $cmd $3"
		ssh -f $host "$bin/RasEyeAdm $cluster $id $cmd $3" ;;
	"add")
		if [ $# -lt 5 ]; then exit 1; fi
		echo "ssh -f $host $bin/RasMail $cluster $id $cmd $3 $4 $5"
		ssh -f $host "$bin/RasMail $cluster $id $cmd $3 $4 $5" ;;
	"del")
		if [ $# -lt 3 ]; then exit 1; fi
		echo "ssh -f $host $bin/RasMail $cluster $id $cmd $3"
		ssh -f $host "$bin/RasMail $cluster $id $cmd $3" ;;
	"mail_dump")
		echo "ssh $host $bin/RasMail $cluster $id dump"
		ssh $host $bin/RasMail $cluster $id dump ;;
	"ras_dump"|"regist_dump"|"session_dump")
		echo "ssh $host $bin/RasEyeAdm $cluster $id $cmd"
		ssh $host $bin/RasEyeAdm $cluster $id $cmd ;;
esac

sleep 1
done < $conf

exit 0
