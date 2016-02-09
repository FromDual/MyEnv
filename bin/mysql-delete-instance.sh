#!/bin/bash

cmddir=/home/mysql/bin
template=/usr/local/mysql/templates
instance=/usr/local/mysql/instances
backup=/usr/local/mysql/backups
log=/usr/local/mysql/logs

###
### Parameters
###

if [ -z "$1" ]
then
  echo -n "Instance name: "
  read instancename
else
  instancename=$1
fi

targetdir="$instance/$instancename"
logdir="$log/$instancename"

if [ ! -d "$targetdir" ]
then
  echo "There is no instance $instancename in $instance"
  exit 1
fi

if [ ! -d "$logdir" ]
then
  echo "There is no instance $instancename in $log."
fi

if [ ! -f "$targetdir/my.cnf" ]
then
  echo "There is no my.cnf in $targetdir."
  exit 1
fi

port=$(awk '/port/ { print $NF; exit }' $targetdir/my.cnf)
if [ -z "$port" ]
then
  echo "I cannot find a valid port number in $targetdir/my.cnf."
  exit 1
fi

echo "The instance is in $targetdir."
echo "The logs are in $logdir."
echo "The port is $port."
echo
echo -n "Is this correct? (Type yes to delete the instance)."
read something

if [ "$something" != "yes" ]
then
  echo "No doing anything."
  exit 2
fi

echo "Stopping the instance."
mysqlstop-$port

echo "Creating a backup of data and logs."
tar czf $backup/$instancename-$(date +%Y-%m-%d).tgz $targetdir $logdir

echo "Deleting instance data and logs."
rm -rf $targetdir $logdir

echo "Removing symlinks."
rm $HOME/bin/*-$port


