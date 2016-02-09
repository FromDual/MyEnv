#!/usr/bin/php
<?php

$lHost     = '127.0.0.1';
$lUser     = 'app';
$lPassword = 'secret';
$lDatabase = 'test';
$lPort     = '3306';
$lSocket   = '/var/run/mysqld/mysql.sock';

$sql       = 'SELECT USER()';
$sql       = "SELECT SQL_NO_CACHE * FROM customers IGNORE INDEX (name) WHERE name = 'No Clue of MySQL LLC'";
$sql       = "SELECT SQL_NO_CACHE * FROM customers WHERE name = 'No Clue of MySQL LLC'";
$sql       = "SELECT * FROM customers AS c JOIN orders AS o ON c.customer_id = o.customer_id  WHERE c.name = 'No Clue of MySQL LLC'";
$sql       = "select SQL_NO_CACHE customer_id, MAX(amount) FROM orders GROUP BY customer_id ORDER BY customer_id";
$sql       = "SELECT * FROM contacts  WHERE last_name = 'Sennhauser' ORDER BY last_name, first_name";
$sql       = "select customer_id, amount FROM orders IGNORE INDEX (customer_id_2) WHERE customer_id = 59349";
$sql       = "select customer_id, amount FROM orders WHERE customer_id = 59349";

$lUser     = 'root';
$lPassword = '';
$lPort     = '35569';

$rc = 0;

$mysqli = new mysqli($lHost, $lUser, $lPassword, $lDatabase, $lPort, $lSocket);

if ( mysqli_connect_error() ) {
  $rc = 385;
  fprintf(STDERR, "ERROR: Connect failed: (%d) %s\n", mysqli_connect_errno(), mysqli_connect_error());
  exit($rc);
}
$mysqli->query('SET NAMES utf8');

$a = gettimeofday();
$ret = $mysqli->query($sql);
$b = gettimeofday();

if ( $ret === false ) {
  fprintf(STDERR, "\nERROR %d: %s\n", $mysqli->errno, $mysqli->error);
}

$ts1 = $a['sec'] * 1000000 + $a['usec'];
$ts2 = $b['sec'] * 1000000 + $b['usec'];
printf("Elapsed time: %d us\n", ($ts2 - $ts1));

$mysqli->close();
exit($rc);

?>
