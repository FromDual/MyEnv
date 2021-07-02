#!/bin/bash

TS=$(date '+%Y-%m-%d_%H:%M:%S')
LOG=/etc/keepalived/keepalived_stop.log

echo ${TS} ${0} ${@} >>${LOG}

