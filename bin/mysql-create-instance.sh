#!/bin/bash

cmddir=/home/mysql/bin
template=/usr/local/mysql/templates
instance=/usr/local/mysql/instances
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

if [ -d "$targetdir" ]
then
  echo "Existing directory: $targetdir"
else
  echo "Creating directory: $targetdir"
  mkdir $targetdir
fi

if [ -d "$logdir" ]
then
  echo "Existing directory: $logdir"
else
  echo "Creating directory: $logdir"
  mkdir $logdir
fi

if [ -z "$2" ]
then
  echo -n "Version to use: "
  read targetversion
else
  targetversion=$2
fi


sourceversion=$( echo $template/*$targetversion* )

if [ -z "$3" ]
then
  echo -n "Port to use: "
  read targetport
else
  targetport=$3
fi

###
### Check validity
###

if [ ! -d $sourceversion ]
then
  echo "Unknown version $sourceversion."
  exit 1
fi

if [ ! -f $sourceversion/my.cnf ]
then
  echo "No my.cnf in $sourceversion"
  exit 2
fi

if [ ! -d $sourceversion/data/mysql ]
then
  echo "No data/mysql in $sourceversion"
  exit 3
fi

echo "Creating $targetversion instance 
	from $sourceversion
	in $targetdir, 
	listening on port $targetport."

###
### Do the copy
###
( cd $sourceversion; tar -cf - my.cnf data/mysql )|
( cd $targetdir;     tar -xf - )

###
### Create start/stop scripts and wrappers
###

cat > $targetdir/start <<EOF
#! /bin/bash --

MYSQL_CMD_DIR=$sourceversion
MYSQL_DIR=$targetdir
MYSQL_UNIX_PORT=\$MYSQL_DIR/mysql.sock
MYSQL_TCP_PORT=$targetport
export MYSQL_UNIX_PORT MYSQL_TCP_PORT MYSQL_DIR

cd \$MYSQL_CMD_DIR
\$MYSQL_CMD_DIR/bin/mysqld_safe --defaults-file=\$MYSQL_DIR/my.cnf --datadir=\$MYSQL_DIR/data &

EOF

ln -s $targetdir/start $cmddir/mysqlstart-$targetport

cat > $targetdir/stop <<EOF
#! /bin/bash --

MYSQL_CMD_DIR=$sourceversion
MYSQL_DIR=$targetdir
cd \$MYSQL_DIR

HOSTNAME=\$(hostname)
PIDFILE="\$MYSQL_DIR/data/\$HOSTNAME.pid"

if [ -s \$PIDFILE ]
then
  kill \$(cat \$PIDFILE)
else
  echo "There is no \$PIDFILE or it is empty. Can't kill."
  exit 1
fi

echo -n "Waiting for server shutdown: "
while [ -s \$PIDFILE ]
do
  echo -n "."
  sleep 1
done
echo "gone."
exit 0
EOF
ln -s $targetdir/stop $cmddir/mysqlstop-$targetport

chmod 755 $targetdir/start $targetdir/stop

for i in mysql mysqladmin mysqldump mysqldumpslow mysqlbinlog
do
  cat > $targetdir/$i-$targetport << EOF
#! /bin/bash --

MYSQL_CMD_DIR=$sourceversion
MYSQL_DIR=$targetdir

\$MYSQL_CMD_DIR/bin/$i --defaults-file=\$MYSQL_DIR/my.cnf \$@
EOF
  chmod 755 $targetdir/$i-$targetport
  ln -s $targetdir/$i-$targetport $cmddir
done

###
### patch the my.cnf
###

sed -e "s!^server-id.*\$!server-id = $targetport!" \
    -e "s!^log.bin.*\$!log_bin = $logdir/binlog!" \
    -e "s!^log.slow.queries.*\$!log_slow_queries = $logdir/slow.log!" \
    -e "s!^port.*\$!port = $targetport!" \
    -e "s!^socket.*\$!socket = $targetdir/mysql.sock!" < $targetdir/my.cnf > $targetdir/my.cnf.new
mv $targetdir/my.cnf.new $targetdir/my.cnf

chown -R mysql.mysql $targetdir

