#!/bin/bash

user=app
password=secret
host=127.0.0.1
port=3306

database=test
table=test
create=0
range=50

sleep=''

if [ -n "$MYSQL_TCP_PORT" ]
then
    port=$MYSQL_TCP_PORT
fi

# CREATE TABLE `test` (
#   `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
#   `data` varchar(255) DEFAULT NULL,
#   `ts` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
#   PRIMARY KEY (`id`)
# ) ENGINE=InnoDB;

#
# All arguments are optional, see defaults at top of file
#

while [ -n "$1" ]
do
    case "$1" in
    --user )      user=$2 ;     shift ;;
    --password )  password=$2 ; shift ;;
    --host )      host=$2 ;     shift ;;
    --port )      port=$2 ;     shift ;;
    --database )  database=$2 ; shift ;;
    --table )     table=$2 ;    shift ;;
    --sleep )     sleep=$2 ;    shift ;;
    --create )    create=1 ;;
    * )           echo "Ignoring unknown argument '$1'" ;;
    esac
    shift
done
client=$(hostname -s)

if [ ${create} ]
then
    mysql --user=$user --password=$password --host=$host --port=$port <<EOF
        create database if not exists ${database} ;
        create table if not exists ${database}.${table} (
                \`id\` int(10) unsigned NOT NULL AUTO_INCREMENT,
                \`data\` varchar(255) DEFAULT NULL,
                \`ts\` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
                PRIMARY KEY (\`id\`)
            ) ENGINE=InnoDB;
EOF
fi

while [ 1 ]
do
    mysql --user=$user --password=$password --host=$host --port=$port ${database} \
          -e "UPDATE ${table} SET ts = CURRENT_TIMESTAMP() WHERE id = $(($RANDOM % $range))" |& grep -v insecure
    echo -n '.'
    if [ -n "$sleep" ]
    then
        sleep $sleep
    fi
done
