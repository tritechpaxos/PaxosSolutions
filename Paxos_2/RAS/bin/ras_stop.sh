#!/bin/sh

args=$#
if [ $args -lt 1 ]
then
echo "USAGE:ras_stop.sh cluster [id]"
exit 1
fi

cluster=$1
conf=~/bin/$cluster.conf

#
#	Check inactive
#
while read id host bin root
do
	if [ ! $id ]; then break; fi	
	if [ `echo $id|cut -c1` = '#' ]; then continue; fi
	if [ $args -ge 2 ]; then if [ $2 -ne $id ]; then continue; fi; fi

	eval bin=$bin
	env="PAXOS_CELL_CONFIG=$bin/"

	ssh -n $host "$env $bin/RasMail $cluster $id active > /dev/null 2>&1"
	ret=$?
	if [ $ret -eq 0 ]
	then
		MailIDs=$MailIDs"$id "
		eval Mailbin_$id=\$bin
		eval Mailroot_$id=\$root
		eval Mailhost_$id=\$host
	fi

	ssh -n $host "$env $bin/RasEyeAdm $cluster $id active > /dev/null 2>&1"
	ret=$?
	if [ $ret -eq 0 ]
	then
		EyeIDs=$EyeIDs"$id "
		eval Eyebin_$id=\$bin
		eval Eyeroot_$id=\$root
		eval Eyehost_$id=\$host
	fi

done < $conf

#
#	unset_all	
#
for id in $EyeIDs
do
	eval host='$Eyehost_'$id
	eval bin='$Eyebin_'$id
	eval root='$Eyeroot_'$id

	env="PAXOS_CELL_CONFIG=$bin/"

	echo "ssh -f $host $env $bin/RasEyeAdm $cluster $id unset_all"
	ssh -f $host "$env $bin/RasEyeAdm $cluster $id unset_all"

done

sleep 1
#
#	stop RasMail
#
for id in $MailIDs
do
	eval host='$Mailhost_'$id
	eval bin='$Mailbin_'$id
	eval root='$Mailroot_'$id

	echo "ssh -f $host $bin/RasMail $cluster $id stop"
	ssh -f $host "$bin/RasMail $cluster $id stop"
done

sleep 1
#
#	stop RasEye
#
for id in $EyeIDs
do
	eval host='$Eyehost_'$id
	eval bin='$Eyebin_'$id
	eval root='$Eyeroot_'$id

	echo "ssh -f $host $bin/RasEyeAdm $cluster $id stop"
	ssh -f $host "$bin/RasEyeAdm $cluster $id stop"

done

exit 0
