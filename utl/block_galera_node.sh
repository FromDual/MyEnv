#!/bin/bash -eu

#
# /etc/mysql/conf.d/wsrep.cnf
#
# [mysqld]
# wsrep_notify_cmd = /usr/local/bin/block_galera_node.sh
#
#
# /etc/sudoers
#
#includedir /etc/sudoers.d
#
# /etc/sudoers.d/mysql (chmod 0440)
#
# mysql ALL = (root) NOPASSWD: /sbin/iptables
#

LOG='/tmp/block_galera_node.log'
LB_IP="192.168.1.99 192.168.1.98"
DATE=$(date '+%Y-%m-%d %H:%M:%S')
STATUS=''
MYSQL_PORT=3306

rc=0

# ----------------------------------------------------------------------
function rule_exists()
# ----------------------------------------------------------------------
{
	# REJECT     tcp  --  anywhere             anywhere             tcp dpt:$MYSQL_PORT reject-with icmp-port-unreachable

	cmd="sudo /sbin/iptables --list -n | grep -c 'REJECT .* tcp dpt:$MYSQL_PORT reject-with icmp-port-unreachable'"
	echo "$cmd" >>$LOG
	eval "$cmd"
}

# ----------------------------------------------------------------------
function printUsage()
# ----------------------------------------------------------------------
{
	cat << _EOF

Rises firewall rules to block traffic from load balancers to galera cluster node on port 3306

usage: $0 [--status <status>] [--uuid <uuid>] [--primary] [--index <n>]
      [--members <member>]
      [--help]

Options:
  status      New node status (Synced, ...)
  uuid        
  primary     
  index       
  members     
  help        Prints this help.

Examples:

  $0 --help

  $0 --status Synced

  $0 --status SyncedNo

_EOF
}

# ----------------------------------------------------------------------
# MAIN
# ----------------------------------------------------------------------

if [ $# -eq 0 ] ; then
	printUsage
	exit $rc
fi

echo $DATE >>$LOG

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
	--help)
		printUsage
		exit $rc
		;;
	esac
	shift
done

# echo $* >> $LOG
echo 'status: '$STATUS >> $LOG
if [ "$STATUS" == '' ] ; then
	echo "No status defined." >> $LOG
	exit 1
fi

# Undefined means node is shutting down
# Synced means node is ready again
if [ "$STATUS" == "Synced" ] ; then

	# Can have more than 1 row matching
	if [ `rule_exists` -gt 1 ] ; then
		for src in $LB_IP ; do
			cmd="sudo /sbin/iptables --delete INPUT --source=$src --protocol tcp --dport $MYSQL_PORT -j REJECT"
			echo $cmd >>$LOG
			eval $cmd >>$LOG 2>&1
			rc=$?
		done
	fi
else
	# Check if rule exists
	# iptables exits with exit_group(1) which also terminates this script
	# which is possibly a bit overkill. So we do not use the --check but
	# a grep as suggested otherwise...
	# cmd="sudo /sbin/iptables --check INPUT --protocol tcp --dport $MYSQL_PORT -j REJECT"
	if [ `rule_exists` -lt 1 ] ; then
		for src in $LB_IP ; do
			cmd="sudo /sbin/iptables --insert INPUT --source=$src --protocol tcp --dport $MYSQL_PORT -j REJECT"
			echo $cmd >>$LOG
			eval $cmd >>$LOG 2>&1
			rc=$?
		done
	else
		echo "Rule already exists." >> $LOG
	fi
fi

echo "ret=$rc" >>$LOG
exit $rc
