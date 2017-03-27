fio - <<_EOF_
[READ_REMOTE]
	rw=read
	size=100m
	directory=/home/nw/vv/remote/
_EOF_
echo
echo
fio - <<_EOF_
[READ_LOCAL]
	rw=read
	size=100m
	directory=/home/nw/vv/local/
_EOF_

echo
echo

fio - <<_EOF_
[RANDOM_READ_REMOTE]
	rw=randread
	size=100m
	directory=/home/nw/vv/remote/
_EOF_
echo
echo
fio - <<_EOF_
[RANDOM_READ_LOCAL]
	rw=randread
	size=100m
	directory=/home/nw/vv/local/
_EOF_

echo
echo

fio - <<_EOF_
[WRITE_REMOTE]
	rw=write
	size=100m
	directory=/home/nw/vv/remote/
_EOF_
echo
echo
fio - <<_EOF_
[WRITE_LOCAL]
	rw=write
	size=100m
	directory=/home/nw/vv/local/
_EOF_

echo
echo

fio - <<_EOF_
[RANDOM_WRITE_REMOTE]
	rw=randwrite
	size=100m
	directory=/home/nw/vv/remote/
_EOF_
echo
echo
fio - <<_EOF_
[RANDOM_WRITE_LOCAL]
	rw=randwrite
	size=100m
	directory=/home/nw/vv/local/
_EOF_

fio - <<_EOF_
[RW_REMOTE]
	rw=rw
	size=100m
	directory=/home/nw/vv/remote/
_EOF_
echo
echo
fio - <<_EOF_
[RW_LOCAL]
	rw=rw
	size=100m
	directory=/home/nw/vv/local/
_EOF_

fio - <<_EOF_
[RANDOM_RW_REMOTE]
	rw=randrw
	size=100m
	directory=/home/nw/vv/remote/
_EOF_
echo
echo
fio - <<_EOF_
[RANDOM_RW_LOCAL]
	rw=randrw
	size=100m
	directory=/home/nw/vv/local/
_EOF_
