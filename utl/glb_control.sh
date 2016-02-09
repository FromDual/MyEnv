#!/bin/bash -eu

#
# /etc/mysql/conf.d/wsrep.cnf
#
# [mysqld]
# wsrep_notify_cmd = /usr/local/bin/glb_control.sh
#

LOG='/tmp/glb_control.log'
LBIP='192.168.150.189'
VIP='192.168.150.189'
PORT='3306'
GLB_MAINT_PORT='4444'
LBUSER='galera'
LBUSER='root'
ETC='/etc/mysql/conf.d/wsrep.cnf'
ETC='/home/mysql/data/mysql-5.5-wsrep-23.7-a/my.cnf'
MYIP=''
WEIGHT='1'
DATE=$(date '+%Y-%m-%d %H:%M:%S')

echo $DATE >>$LOG

regex='^.*=\s*([0-9]+.[0-9]+.[0-9]+.[0-9]+).*'
str=$(grep "^wsrep_node_incoming_address" $ETC 2>>$LOG)

if [[ $str =~ $regex ]] ; then
  MYIP=${BASH_REMATCH[1]}
else
  echo "Cannot find IP address in $str" >>$LOG
  exit 1
fi

while [ $# -gt 0 ] ; do

  case $1 in
  --status)
    STATUS=$2
    shift
    ;;
  --uuid)
    CLUSTER_UUID=$2
    shift
    ;;
  --primary)
    PRIMARY=$2
    shift
    ;;
  --index)
    INDEX=$2
    shift
    ;;
  --members)
    MEMBERS=$2
    shift
    ;;
  esac
  shift
done

# echo $* >> $LOG
echo $STATUS >> $LOG

# Undefined means node is shutting down
# Synced means node is ready again
if [ "$STATUS" != "Synced" ] ; then
  cmd="echo $MYIP:$PORT:0 | nc -q 1 $VIP $GLB_MAINT_PORT"
else
  cmd="echo $MYIP:$PORT:$WEIGHT | nc -q 1 $VIP $GLB_MAINT_PORT"
fi

echo $cmd >>$LOG
eval $cmd >>$LOG 2>&1
echo "ret=$?" >>$LOG

exit 0
