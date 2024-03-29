#!/bin/bash

user=app
password=secret
host=127.0.0.1
port=3306

database=test
table=test
create=0

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

type -p mariadb >/dev/null
if [ 0 -eq ${?} ] ; then
  CLI=mariadb
else
  CLI=mysql
fi

if [ ${create} ]
then
    ${CLI} --user=$user --password=$password --host=$host --port=$port <<EOF
        create database if not exists ${database} ;
        create table if not exists ${database}.${table} (
                \`id\` int(10) unsigned NOT NULL AUTO_INCREMENT,
                \`data\` varchar(255) DEFAULT NULL,
                \`ts\` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
                PRIMARY KEY (\`id\`)
            ) ENGINE=InnoDB;
EOF
fi

function inject_password()
{
  cat << _EOF
[client]
user     = ${user}
password = ${password}
_EOF
}


while [ 1 ]
do
    ${CLI} --defaults-extra-file=<(inject_password) --host=$host --port=$port ${database} \
          -e "INSERT INTO ${table} (id, data, ts) VALUES (NULL, CONCAT('Test data insert from ${client} on ', @@hostname), CURRENT_TIMESTAMP());"
    echo -n '.'
    if [ -n "$sleep" ]
    then
        sleep $sleep
    fi
done
