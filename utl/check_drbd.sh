#!/bin/bash

PATH=$PATH:/sbin
LOG="/tmp/check_drbd.log"
RECEIVER="remote-dba@fromdual.com contact@fromdual.com"

# test if drbdadmin exists
DRBDADM=$(which drbdadm)
if [ $? -ne 0 ] ; then
  echo "$ts drbdadm tool not found." | tee -a $LOG
  exit 2
fi

if [ "$USER" != 'root' ] ; then

  cmd="cat /proc/drbd | grep 'cs:'"
  cnt=$(eval "$cmd | wc -l")
  if [ $cnt -ne 1 ] ; then
    echo "$ts /proc/drbd returns $cnt rows. Only 1 row is expected" | tee -a $LOG
    exit 3
  fi

  #0: cs:Connected ro:Secondary/Primary ds:Diskless/UpToDate C r----
  cnt=$(eval "$cmd | grep -c 'ro:Primary/Secondary ds:UpToDate/UpToDate' 2>&1")
  if [ $cnt -ne 1 ] ; then
    echo "$ts DRBD resource $drbd_resource seems to be NOT ok." | tee -a $LOG

    # alert
    echo "$ts Sending email to $RECEIVER" | tee -a $LOG
    mailx -s "DRBD Alert" $RECEIVER << _EOM
DRBD resource $drbd_resource on host $(hostname) seems to have a problem.
Please investigate!
_EOM

    exit 3
  fi
  exit 0
fi

# test if DRBD resource is in the right state

drbd_resource=$1
if [ -z "$drbd_resource" ] || [ "$drbd_resource" = '' ] ; then
  echo "$ts DRBD resource not specified." | tee -a $LOG
  echo "$ts Please use something like $0 drbd_r1" | tee -a $LOG
  exit 1
fi

# Secondary/Primary
cmd="$DRBDADM role $drbd_resource"
role=`$cmd 2>&1`

ts=`date '+%Y-%m-%d %H:%M:%S'`

if [ "$role" = 'Secondary/Primary' ] || [ "$role" = 'Primary/Secondary' ] ; then
  echo "$ts DRBD resource $drbd_resource seems to be OK with role: $role" | tee -a $LOG

  # UpToDate/UpToDate
  cmd="$DRBDADM dstate $drbd_resource"
  dstate=`$cmd 2>&1`

  if [ "$dstate" = 'UpToDate/UpToDate' ] ; then
    echo "$ts DRBD resource $drbd_resource seems to be OK with dstate: $dstate" | tee -a $LOG
    ret=0
  else
    echo "$ts DRBD resource $drbd_resource seems to be NOT ok with dstate: $dstate" | tee -a $LOG

    # alert
    echo "$ts Sending email to $RECEIVER" | tee -a $LOG
    mailx -s "DRBD Alert" $RECEIVER << _EOM
DRBD resource $drbd_resource on host $(hostname) seems to have a problem.
Please investigate!
_EOM

    ret=3
  fi
else
  echo "$ts DRBD resource $drbd_resource seems to be NOT ok with role: $role" | tee -a $LOG

  # alert
  echo "$ts Sending email to $RECEIVER" | tee -a $LOG
  mailx -s "DRBD Alert" $RECEIVER << _EOM
DRBD resource $drbd_resource on host $(hostname) seems to have a problem.
Please investigate!
_EOM

  ret=2
fi

exit $ret
