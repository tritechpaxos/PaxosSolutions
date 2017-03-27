#LOG_SIZE=0 gdb --args PFSServer paxos $1 $1 -F 2 -B 5 -S 16 -N 2 -U 1000000000 
gdb --args PFSServer paxos $1 $1 -F 2 -B 3 -S 16 -N 2 -U 1000000000 -M 150000 
