#!/bin/bash

USER=root
PASS=''

sql="SHOW GLOBAL STATUS WHERE Variable_name IN( 'wsrep_cluster_size', 'wsrep_cluster_status', 'wsrep_local_state_comment', 'wsrep_ready', 'wsrep_connected')"

mysql --user="${USER}" --password="${PASS}" --execute="$sql" 2>&1 | grep -v 'Using a password on the command line interface can be insecure' | column -t
