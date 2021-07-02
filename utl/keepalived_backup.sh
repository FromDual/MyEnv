#!/bin/bash

TS=$(date '+%Y-%m-%d_%H:%M:%S')
LOG=/etc/keepalived/keepalived_backup.log
MY_NAME=${0}

echo ${TS} ${MY_NAME} ${@} >>${LOG}

# Defaults

USER=root
PASSWORD=""
VARIABLE=read_only
KILL=no

# http://www.bahmanm.com/blogs/command-line-options-how-to-parse-in-bash-using-getopt
OPTS=$(getopt -o u:p:v:k: --long user:,password:,variable:,kill: -n '${MY_NAME}' -- "${@}")
eval set -- "${OPTS}"

while true ; do

  case "${1}" in
    -u|--user )
      USER=${2}
      shift 2 ;;
    -p|--password )
      PASSWORD=$2
      shift 2 ;;
    -v|--variable )
      VARIABLE=${2}
      shift 2 ;;
    -k|--kill )
      KILL=${2}
      shift 2 ;;
    --)
      shift
      break ;;
    *)
      echo "Internal error! (${1} ${2})" >>${LOG}
      exit 1 ;;
  esac
done


# Set read_only to on

echo "Set ${VARIABLE} to On" >>${LOG}
mysql --user=${USER} --password="${PASSWORD}" --execute="SET GLOBAL ${VARIABLE} = On"


# Kill open connections

if [ "${KILL}" == 'yes' ] ; then

  echo "Kill open connections." >>${LOG}
  for ID in $(mysql --user=${USER} --password="${PASSWORD}" --execute="SELECT id AS 'id'
  FROM information_schema.processlist
 WHERE user NOT IN ('system user', 'event_scheduler')
   AND id != CONNECTION_ID()
   AND command != 'Binlog Dump'\G" | grep 'id:' | awk '{ print $2 }') ; do
    echo "Killing connection ${ID}" >>${LOG}
    mysql --user=${USER} --password="${PASSWORD}" --execute="KILL CONNECTION ${ID}"
  done
fi

exit 0
