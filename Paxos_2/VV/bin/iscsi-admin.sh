#!/bin/sh

if [ $# -lt 1 ]
then
echo "USAGE:iscsi-adm.sh {scsi}"
exit 1
fi

iscsiadm -m session -P 3

exit 0
