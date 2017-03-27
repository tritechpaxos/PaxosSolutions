#!/bin/sh

#
#	Analyze parameters
#
args=$#
while [ "$1" != "" ]
do
	case "$1" in
		"-c") shift; cluster=$1; shift ;;
		"-r") shift; ras_cell=$1; shift ;;
		"-I") shift; ID=$1; shift ;;
		"-s") shift; seq=$1; shift ;;
		"-m") shift; mode=$1; shift ;;
		"-B") shift; BIN=$1; shift ;;
		* ) shift;;
	esac
done

if [ ! $cluster ]; then
	echo "USAGE:ras_restart.sh -c cluster/cell -r ras_cell ..."
	echo "	-I	id" 
	echo "	-s	event sequence" 
	echo "	-m	event mode" 
	echo "	-e	event element" 
	echo "	-B	BIN directory" 
	exit 1
fi

#echo "cluster[$cluster] ras_cel[$ras_cell] ID[$ID] seq[$seq] mode[$mode] BIN[$BIN]"

env="LOG_SIZE=20000 "
files=20

conf=$HOME/_$cluster.conf

#
#	Check active
#
while read id host bin root
do
	if [ ! $id ]; then break; fi	
	if [ `echo $id|cut -c1` = '#' ]; then continue; fi
	if [ $ID ]; then if [ $ID != $id ]; then continue; fi; fi

	eval bin=$bin

	ssh -n $host "$bin/RasMail $cluster $id active > /dev/null 2>&1"
	ret=$?
	if [ $ret -ne 0 ]
	then
		MailIDs=$MailIDs"$id "
		eval Mailbin_$id=\$bin
		eval Mailroot_$id=\$root
		eval Mailhost_$id=\$host
	fi

	ssh -n $host "$bin/RasEyeAdm $cluster $id active > /dev/null 2>&1"
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

	echo "ssh -f $host cd $root;$env $bin/RasMail $cluster $id 3>&1 1>&2 2>&3 | $bin/LogFiles RasMail $files"
	ssh -f $host "cd $root;$env $bin/RasMail $cluster $id 3>&1 1>&2 2>&3 | $bin/LogFiles RasMail $files"

	sleep 1

	if [ ! $BIN ]; then BIN=$bin; fi
	. $BIN/ras_mail.data

done

#
#	RasEye
#
for id in $EyeIDs
do
	eval host='$Eyehost_'$id
	eval bin='$Eyebin_'$id
	eval root='$Eyeroot_'$id

	echo "ssh -f $host cd $root;$env $bin/RasEye $cluster $id 3>&1 1>&2 2>&3 | $bin/LogFiles RasEye $files"
	ssh -f $host "cd $root;$env $bin/RasEye $cluster $id 3>&1 1>&2 2>&3 | $bin/LogFiles RasEye $files"

	sleep 1

	if [ ! $BIN ]; then BIN=$bin; fi
	data="start"
	. $BIN/ras_eye.data

done

exit 0
