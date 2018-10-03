#!/bin/bash

sql="SHOW GLOBAL STATUS WHERE Variable_name IN( 'wsrep_cluster_size', 'wsrep_cluster_status', 'wsrep_local_state_comment', 'wsrep_ready', 'wsrep_connected')"
mysql --user=root --execute="$sql"
