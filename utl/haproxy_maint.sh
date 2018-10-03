#!/bin/bash

MYPORT=${1}
if [ "${MYPORT}" == '' ] ; then
  echo "No port specified."
  exit 1
fi

if [ "${MYPORT}" == '3312' ] ; then
  NODE=mygr57-a
elif [ "${MYPORT}" == '3313' ] ; then
  NODE=mygr57-b
elif [ "${MYPORT}" == '3314' ] ; then
  NODE=mygr57-c
else
  echo "Wrong port $MYPORT."
  exit 2
fi

sql="SELECT member_state AS state FROM performance_schema.replication_group_members WHERE member_port = $MYPORT"
out=$(mysql --user=root --execute="${sql}")
ret=$?
state=$(echo $out | awk '{ print $2 }')

if [ $ret -ne 0 ] ; then
  STATE=maint
else
  if [ "$state" == 'ONLINE' ] ; then
    STATE=ready
  else
    STATE=maint
  fi
fi
echo $STATE

echo "set server gr-cluster/${NODE} state $STATE" | socat stdio /home/mysql/product/haproxy/tmp/stats.sock

