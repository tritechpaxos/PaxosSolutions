#!/bin/sh

#
#	Analyze parameters
#
args=$#
if [ $args -lt 1 ]
then
echo "USAGE:ras_autonomic.sh cluster [-i id] [-r ras_cell]"
exit 1
fi

cluster=$1
shift

while [ "$1" != "" ]
do
	case "$1" in
		"-r") shift; ras_cell=$1; shift ;;
		"-i") shift; ID=$1; shift ;;
		* ) shift;;
	esac
done

env0="LOG_SIZE=20000"
files=20

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

	ssh -n $host "$env $bin/RasMail $cluster $id active > /dev/null 2>&1"
	ret=$?
	if [ $ret -ne 0 ]
	then
		MailIDs=$MailIDs"$id "
		eval Mailbin_$id=\$bin
		eval Mailroot_$id=\$root
		eval Mailhost_$id=\$host
	fi

	env="PAXOS_CELL_CONFIG=$bin/"

	ssh -n $host "$env $bin/RasEyeAdm $cluster $id active > /dev/null 2>&1"
	ret=$?
	if [ $ret -ne 0 ]
	then
		EyeIDs=$EyeIDs"$id "
		eval Eyebin_$id=\$bin
		eval Eyeroot_$id=\$root
		eval Eyehost_$id=\$host
	fi
done < $conf

#echo cluster[$cluster] MailIDs[$MailIDs] EyeIDs[$EyeIDs]
#
#	RasMail
#
for id in $MailIDs
do
	eval host='$Mailhost_'$id
	eval bin='$Mailbin_'$id
	eval root='$Mailroot_'$id

	env="$env0 PAXOS_CELL_CONFIG=$bin/"
	echo "ssh -f $host cd $root;$env $bin/RasMail $cluster $id 3>&1 1>&2 2>&3 | $bin/LogFiles RasMail $files"
	ssh -f $host "cd $root;$env $bin/RasMail $cluster $id 3>&1 1>&2 2>&3 | $bin/LogFiles RasMail $files"

	sleep 1

#. $bin/ras_mail.data

done

echo "$bin/ras_mail_data.sh $cluster add"
$bin/ras_mail_data.sh $cluster add

#
#	RasEye
#
for id in $EyeIDs
do
	eval host='$Eyehost_'$id
	eval bin='$Eyebin_'$id
	eval root='$Eyeroot_'$id

	env="$env0 PAXOS_CELL_CONFIG=$bin/"
	echo "ssh -f $host cd $root;$env $bin/RasEye $cluster $id 3>&1 1>&2 2>&3 | $bin/LogFiles RasEye $files"
	ssh -f $host "cd $root;$env $bin/RasEye $cluster $id 3>&1 1>&2 2>&3 | $bin/LogFiles RasEye $files"

	sleep 1

#data="start"
#. $bin/ras_eye.data

done

if [ "X$ras_cell" != "X" ]; then

	$bin/ras_eye_data.sh $cluster set -r $ras_cell
	$bin/ras_eye_data.sh $cluster reg -r $ras_cell
fi

exit 0
