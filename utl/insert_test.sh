#!/bin/bash

user=app
password=secret
host=127.0.0.1
port=3306

# CREATE TABLE `test` (
#   `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
#   `data` varchar(255) DEFAULT NULL,
#   `ts` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
#   PRIMARY KEY (`id`)
# ) ENGINE=InnoDB;

while [ 1 ]
do

  mysql --user=$user --password=$password --host=$host --port=$port test -e "insert into test values (NULL, CONCAT('Test data insert from ', @@hostname), NULL);" |& grep -v insecure
  echo -n '.'
  # sleep 1
done
