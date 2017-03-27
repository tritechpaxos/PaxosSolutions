#!/bin/sh

#
#	Analyze parameters
#
args=$#
if [ $args -lt 2 ]
then
echo "USAGE:ras_mail_data.sh cluster {add|del}"
exit 1
fi

cluster=$1
cmd=$2

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
	ssh -n $host "$bin/RasMail $cluster $id active > /dev/null 2>&1"
	ret=$?
	if [ $ret -eq 0 ]
	then
		MailIDs=$MailIDs"$id "
		eval Mailbin_$id=\$bin
		eval Mailroot_$id=\$root
		eval Mailhost_$id=\$host
	fi
done < $conf

#
#	RasMail
#
for id in $MailIDs
do
	eval host='$Mailhost_'$id
	eval bin='$Mailbin_'$id
	eval root='$Mailroot_'$id

	echo "ssh -f $host $bin/RasMail $cluster $id $cmd nw@tritech.co.jp"
	ssh -f $host "$bin/RasMail $cluster $id $cmd nw@tritech.co.jp"
done

exit 0
