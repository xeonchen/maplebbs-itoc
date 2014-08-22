#!/bin/sh
# 清除站上使用者與shared memory
kill `ps -auxwww | grep bbsd | awk '{print $2}'`

# for freebsd only
for i in `ipcs | grep bbs | awk '{print $3}'`
do
  if [ $OSTYPE = "FreeBSD" ]; then
         ipcrm -M $i
  fi
done

# Linux 請用 ipcs 及 ipcrm shm
