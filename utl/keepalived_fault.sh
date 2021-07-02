#!/bin/bash

TS=$(date '+%Y-%m-%d_%H:%M:%S')
LOG=/etc/keepalived/keepalived_fault.log

echo ${TS} ${0} ${@} >>${LOG}
