#!/bin/bash

USER=monitor
PASSWORD=''
HOST='127.0.0.1'
PORT='3306'

type -p mariadb >/dev/null
if [ 0 -eq ${?} ] ; then
  CLIENT=mariadb
else
  CLIENT=mysql
fi

function inject_password()
{
  cat << _EOF
[client]
user     = ${USER}
password = ${PASSWORD}
_EOF
}


sql="SHOW GLOBAL STATUS WHERE Variable_name IN( 'wsrep_cluster_size', 'wsrep_cluster_status', 'wsrep_local_state_comment', 'wsrep_ready', 'wsrep_connected', 'wsrep_last_committed')"

${CLIENT} --defaults-extra-file=<(inject_password) --host=${HOST} --port=${PORT} --execute="${sql}" | column -t
