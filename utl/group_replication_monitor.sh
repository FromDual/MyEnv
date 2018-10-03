#!/bin/bash

# Call with watch -d -n 1 ...

sql="SELECT CONCAT(member_host, ':', member_port) AS node, member_state AS state FROM performance_schema.replication_group_members"
mysql --user=root --execute="${sql}"
echo

sql="SELECT CHANNEL_NAME as channel, SERVICE_STATE AS state, LAST_ERROR_NUMBER AS err_no, LAST_ERROR_MESSAGE AS err_msg, LAST_ERROR_TIMESTAMP AS err_ts FROM performance_schema.replication_connection_status"
mysql --user=root --execute="${sql}"
echo

sql="SELECT CHANNEL_NAME AS channel, COUNT_TRANSACTIONS_IN_QUEUE AS cnt_trx_in_q, COUNT_TRANSACTIONS_CHECKED cnt_trx_chkd, COUNT_CONFLICTS_DETECTED cnt_confl_detc, COUNT_TRANSACTIONS_ROWS_VALIDATING cnt_trx_row_val  FROM performance_schema.replication_group_member_stats"
mysql --user=root --execute="${sql}"
echo

sql="SELECT @@read_only AS ro, @@super_read_only AS super_ro, @@group_replication_single_primary_mode AS single_prim, @@group_replication_bootstrap_group AS bootstrap"
mysql --user=root --execute="${sql}"
