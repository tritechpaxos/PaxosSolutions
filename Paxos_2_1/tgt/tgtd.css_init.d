#!/bin/bash

# This is an example init.d script for stopping/starting/reconfiguring tgtd.css.

TGTD_CONFIG=/etc/tgt/targets.conf

TASK=$1
SERVICE_NAME=`basename $0`
SV_dot_Index=`expr index $SERVICE_NAME "." `
export PAXOS_CELL_NAME=`expr substr $SERVICE_NAME  \( $SV_dot_Index + 1 \) \( length $SERVICE_NAME - $SV_dot_Index \)`

start()
{
	echo "Starting target framework daemon"
	# Start tgtd.css first.
	LOG_SIZE=0 tgtd.css -d 1 2>&1 | logger -t tgtd
	RETVAL=$?
	if [ "$RETVAL" -ne 0 ] ; then
	    echo "Could not start tgtd.css (is tgtd.css already running?)"
	    exit 1
	fi
	# Put tgtd.css into "offline" state until all the targets are configured.
	# We don't want initiators to (re)connect and fail the connection
	# if it's not ready.
	echo tgtadm --op update --mode sys --name State -v offline
	tgtadm --op update --mode sys --name State -v offline
	# Configure the targets.
	echo tgt-admin -e -c $TGTD_CONFIG
	tgt-admin -e -c $TGTD_CONFIG
	# Put tgtd.css into "ready" state.
	echo tgtadm --op update --mode sys --name State -v ready
	tgtadm --op update --mode sys --name State -v ready
}

stop()
{
	if [ -n "$RUNLEVEL" ]
	then
	if [ "$RUNLEVEL" == 0 -o "$RUNLEVEL" == 6 ] ; then
	    forcedstop
	fi
	fi
	echo "Stopping target framework daemon"
	# Remove all targets. It only removes targets which are not in use.
	echo "tgt-admin --update ALL -c /dev/null &>/dev/null"
	tgt-admin --update ALL -c /dev/null &>/dev/null
	# tgtd.css will exit if all targets were removed
	echo "tgtadm --op delete --mode system &>/dev/null"
	sudo tgtadm --op delete --mode system &>/dev/null
	RETVAL=$?
	if [ "$RETVAL" -eq 107 ] ; then
	    echo "tgtd.css is not running"
	    [ "$TASK" != "restart" ] && exit 1
	elif [ "$RETVAL" -ne 0 ] ; then
	    echo "Some initiators are still connected - could not stop tgtd.css"
	    exit 2
	fi
	echo -n
}

forcedstop()
{
	# NOTE: Forced shutdown of the iscsi target may cause data corruption
	# for initiators that are connected.
	echo "Force-stopping target framework daemon"
	# Offline everything first. May be needed if we're rebooting, but
	# expect the initiators to reconnect cleanly when we boot again
	# (i.e. we don't want them to reconnect to a tgtd.css which is still
	# working, but the target is gone).
	tgtadm --op update --mode sys --name State -v offline &>/dev/null
	RETVAL=$?
	if [ "$RETVAL" -eq 107 ] ; then
	    echo "tgtd.css is not running"
	    [ "$TASK" != "restart" ] && exit 1
	else
	    tgt-admin --offline ALL
	    # Remove all targets, even if they are still in use.
	    tgt-admin --update ALL -c /dev/null -f
	    # It will shut down tgtd.css only after all targets were removed.
	    tgtadm --op delete --mode system
	    RETVAL=$?
	    if [ "$RETVAL" -ne 0 ] ; then
		echo "Failed to shutdown tgtd.css"
		exit 1
	    fi
	fi
	echo -n
}

reload()
{
	echo "Updating target framework daemon configuration"
	# Update configuration for targets. Only targets which
	# are not in use will be updated.
	tgt-admin --update ALL -c $TGTD_CONFIG &>/dev/null
	RETVAL=$?
	if [ "$RETVAL" -eq 107 ] ; then
	    echo "tgtd.css is not running"
	    exit 1
	fi
}

forcedreload()
{
	echo "Force-updating target framework daemon configuration"
	# Update configuration for targets, even those in use.
	tgt-admin --update ALL -f -c $TGTD_CONFIG &>/dev/null
	RETVAL=$?
	if [ "$RETVAL" -eq 107 ] ; then
	    echo "tgtd.css is not running"
	    exit 1
	fi
}

status()
{
	# Don't name this script "tgtd.css"...
	TGTD_PROC=$(ps -C tgtd.css | grep -c tgtd.css)
	if [ "$TGTD_PROC" -eq 2 ] ; then
	    echo "tgtd.css is running. Run 'tgt-admin -s' to see detailed target info."
	else
	    echo "tgtd.css is NOT running."
	fi
}

case $1 in
	start)
		start
		;;
	stop)
		stop
		;;
	forcedstop)
		forcedstop
		;;
	restart)
		TASK=restart
		stop && start
		;;
	forcedrestart)
		TASK=restart
		forcedstop && start
		;;
	reload)
		reload
		;;
	forcedreload)
		forcedreload
		;;
	status)
		status
		;;
	*)
		echo "Usage: $0 {start|stop|forcedstop|restart|forcedrestart|reload|forcedreload|status}"
		exit 2
		;;
esac

