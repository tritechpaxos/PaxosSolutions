#!/bin/bash

function usage() {
    printf "usage: $0 (LVServer|PFSServer|xjPingPaxos) cell_name no cell_dir [init|first]\n" 1>&2
    exit 1
}

function get_nextseq() {
    next_path="${cell_dir}/SHUTDOWN"
    if [ -f $next_path ]; then
        cut --delimiter=" "  -f1 ${next_path}
    else
        echo "-1"
    fi
}

if [ $# != 4 -a $# != 5 ]; then
    usage
fi

case "$1" in
"LVServer"|"PFSServer"|"xjPingPaxos") paxos_cmd=$1 ;;
*) usage;;
esac

cell_name=$2
cell_no=$3
cell_home=$4
cell_dir="${cell_home}/data/${cell_name}/${cell_no}"

if [ $# == 5 ]; then
case "$5" in
"init") args="TRUE 0" ;;
"first") args="TRUE `get_nextseq`" ;;
*) usage ;;
esac
else
if [ "`get_nextseq`" == -1 ]; then
args="FALSE"
else
args="TRUE"
fi
fi

export PAXOS_CELL_CONFIG="${cell_home}/config/${cell_name}.conf"
cmd="`dirname $0`/${paxos_cmd} ${cell_name} ${cell_no} ${cell_dir} ${args}"
exec $cmd
