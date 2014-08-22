#!/bin/sh
#セ祘Αノㄓだ猂硈钡 bbs  smtp 硈钡ㄓ方计秖
cat /dev/null | awk 'BEGIN {printf("%10s    %-20s\n", "硈絬Ω计", "硈絬ㄓ方")} {} END{}'

cat /home/bbs/run/bmta.log.* | grep CONN | sort -k 3 -r | awk '{print $3}'| awk 'BEGIN {} {Number[$1]++} END {
  for(course in Number)
     printf("%10d    %-20s\n", Number[course], course)
}' | sort -n -r
