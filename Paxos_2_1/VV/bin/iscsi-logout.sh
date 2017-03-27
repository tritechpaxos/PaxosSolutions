#!/bin/sh -x

TARGET=$(iscsiadm -m session)
echo $!
TARGET=$(echo $TARGET | cut -d " " -f 4 )

echo "iscsiadm -m node -T $TARGET -u"
iscsiadm -m node -T $TARGET -u
