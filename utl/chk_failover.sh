#!/bin/bash

/usr/bin/stat /etc/keepalived/failover 2>/dev/null 1>&2
if [ ${?} -eq 0 ] ; then
  exit 1
else
  exit 0
fi

