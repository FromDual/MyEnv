#!/bin/bash

log="./ping.log"
sleep=5
target="$1"

while [ 1 ] ; do
  echo -n $(date '+%Y-%m-%d %H:%M:%S')' ' >> $log 2>&1
  ping -c 1 $target | grep "bytes from" >> $log 2>&1
  sleep $sleep
done

