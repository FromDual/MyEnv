#!/bin/bash

parameter="$@"

USER='root'
PASSSWORD=''
HOST='127.0.0.1'
PORT=3306
MYSQL='mysql'
SQL="FLUSH QUERY CACHE"

if [ "$parameter" == '' ] ; then
  parameter="--user=$USER --password=$PASSWORD --host=$HOST --port=$PORT"
fi

cmd="$MYSQL $parameter --execute='$SQL'"
# echo $cmd
eval $cmd
exit $?

