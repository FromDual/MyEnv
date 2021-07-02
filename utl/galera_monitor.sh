#!/bin/bash

USER=root
PASSWORD=''
HOST='localhost'
PORT='3306'

function inject_password()
{
  cat << _EOF
[client]
user     = ${USER}
password = ${PASSWORD}
_EOF
}


sql="SHOW GLOBAL STATUS WHERE Variable_name IN( 'wsrep_cluster_size', 'wsrep_cluster_status', 'wsrep_local_state_comment', 'wsrep_ready', 'wsrep_connected')"

mysql --defaults-extra-file=<(inject_password) --host=${HOST} --port=${PORT} --execute="${sql}" | column -t
