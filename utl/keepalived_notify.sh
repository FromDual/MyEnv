#!/bin/bash

TYPE=${1}
NAME=${2}
STATE=${3}
PRIORITY=${4}

TS=$(date '+%Y-%m-%d_%H:%M:%S')
LOG=/etc/keepalived/keepalived_notify.log

echo $TS $0 $@ >>${LOG}
