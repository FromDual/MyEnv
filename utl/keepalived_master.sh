#!/bin/bash

TS=$(date '+%Y-%m-%d_%H:%M:%S')
LOG=/etc/keepalived/keepalived_master.log
MY_NAME=${0}

echo ${TS} ${MY_NAME} ${@} >>${LOG}

# Defaults

USER=root
PASSWORD=""
VARIABLE=read_only
WAIT=no

# http://www.bahmanm.com/blogs/command-line-options-how-to-parse-in-bash-using-getopt
OPTS=$(getopt -o u:p:v:w: --long user:,password:,variable:,wait: -n '${MY_NAME}' -- "${@}")
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
    -w|--wait )
      WAIT=${2}
      shift 2 ;;
    --)
      shift
      break ;;
    *)
      echo "Internal error! (${1} ${2})" >>${LOG}
      exit 1 ;;
  esac
done


# Wait for catch up

if [ "${WAIT}" == 'yes' ] ; then

  echo "Waiting for catch up." >>${LOG}
  DELAY=100
  until [ ${DELAY} -eq 0 ] ; do
    DELAY=$(mysql --user=${USER} --password="${PASSWORD}" --execute="SHOW SLAVE STATUS\G" | grep 'Seconds_Behind_Master:' | awk '{ print $2 }')
    echo "Delay: ${DELAY}" >>${LOG}
    if [ "${DELAY}" == 'NULL' ] ; then
      echo "Replication seems to be stopped." >>${LOG}
      break
    fi
    sleep 0.5
    # Possibly specify timeout?
  done
fi


# Set read_only to off

echo "Set ${VARIABLE} to Off" >>${LOG}
mysql --user=${USER} --password="${PASSWORD}" --execute="SET GLOBAL ${VARIABLE} = Off"

exit 0
