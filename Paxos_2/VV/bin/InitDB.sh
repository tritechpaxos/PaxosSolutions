#!/bin/sh

if [ $# -lt 1 ]
then
echo "USAGE:Init.sh db_cell"
exit 1
fi

db_cell=$1
#
#	Create tables
#

F_PAXOSDIR="~/PAXOS/Paxos_2/xjPing/bin"
eval F_PAXOSDIR=$F_PAXOSDIR

$F_PAXOSDIR/xjSqlPaxos $db_cell <<_EOF_
#
#	Lock table
#
create table LOCK_LIST( Name char(64), Cnt int );
#
#	Sequencer
#
create table SEQ_LIST( Name char(64), Seq ulong );
#	Volume Name list
#
create table VN_LIST( VV_NAME char(64), Target char(64), Pool char(64), 
		Size ulong, Stripe int, Ver int, Ref int, Time uint, 
		Type char(16), Status int );
#
#	Virtual Volume list
#
create table VV_LIST(VV_NAME char(64),
				WR_TBL char(64), COW_TBL char(64), RD_VV char(64) );
#
#	Logical Volume list
#
create table LV_LIST(LV char(64), Flag uint, Total uint, Usable uint,
					Pool char(64), Cell char(64));
#
#	Copy Log	
#
create table COPY_LOG( Seq ulong, Fr char(64), FrSnap char(64), 
	To Char(64), ToSnap char(64), FrLSs ulong, 
	ToLSs ulong, ToCopied ulong, Start uint, End uint, Stage int, Status uint);
#
insert into SEQ_LIST values('COPY_LOG', 0 );
#
commit;
#
_EOF_

echo xjSqlPaxos=$?

exit 0
