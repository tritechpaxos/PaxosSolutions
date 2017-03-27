#!/bin/sh -x

if [ $# -lt 1 ]
then
echo "USAGE:iscsi-login.sh host"
exit 1
fi
host=$1

TARGET=$(iscsiadm -m discovery -t sendtargets -p $host)
TARGET=$(echo $TARGET | cut -d " " -f 2)

iscsiadm -m node -T $TARGET -l

exit 0
