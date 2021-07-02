#!/usr/bin/php
<?php

$lHost     = '127.0.0.1';
$lUser     = 'app';
$lPassword = 'secret';
$lDatabase = '';
$lPort     = '3306';
$lSocket   = '/var/run/mysqld/mysql.sock';

$lPort     = '3320';

/*

  Needs test table twice. One in each Schema.
  Schema aaa_fromdual and zzz_fromdual:

  CREATE SCHEMA IF NOT EXISTS aaa_fromdual;
  use aaa_fromdual;
  source /home/mysql/product/myenv/sql/test_table.sql
  CREATE SCHEMA IF NOT EXISTS zzz_fromdual;
  use zzz_fromdual;
  source /home/mysql/product/myenv/sql/test_table.sql
  SET GLOBAL validate_password_policy = 0;
  SET GLOBAL validate_password_length = 6;
  CREATE USER 'app'@'127.0.0.1' IDENTIFIED BY 'secret';
  GRANT ALL ON aaa_fromdual.* TO 'app'@'127.0.0.1';
  GRANT ALL ON zzz_fromdual.* TO 'app'@'127.0.0.1';

  SELECT COUNT(*), MAX(id) FROM aaa_fromdual.test UNION ALL SELECT COUNT(*), MAX(id) FROM zzz_fromdual.test;

*/

$rc = 0;
$lUSleep = 1;

$mysqli = new mysqli($lHost, $lUser, $lPassword, $lDatabase, $lPort, $lSocket);

if ( mysqli_connect_error() ) {
  $rc = 442;
  fprintf(STDERR, "ERROR: Connect failed: (%d) %s\n", mysqli_connect_errno(), mysqli_connect_error());
  exit($rc);
}
$mysqli->query('SET NAMES utf8mb4');

	// Catch kill

$lLastId = 0;
try {

	while ( true ) {
		$sql = 'START TRANSACTION';
		$mysqli->query($sql);

		$s = 'aaa_fromdual';
		$sql = 'INSERT INTO `' . $s . '`.`test` VALUES (NULL, "Test data on first table.", NULL)';
		if ( $mysqli->query($sql) !== false ) {
			$id = $mysqli->insert_id;
		}
		else {
			$msg = 'INSERT on ' . $s . '.test failed.';
			throw new Exception($msg);
		}

		$s = 'zzz_fromdual';
		$sql = sprintf('INSERT INTO `zzz_fromdual`.`test` VALUES (%d, "Test data on second table.", NULL)', $id);
		if ( $mysqli->query($sql) !== false ) {
			null;
		}
		else {
			$msg = 'INSERT on ' . $s . '.test failed.';
			throw new Exception($msg);
		}

		$sql = 'COMMIT';
		if ( $mysqli->query($sql) !== false ) {
			$lLastId = $id;
		}
		else {
			$msg = 'INSERT on ' . $s . '.test failed.';
			throw new Exception($msg);
		}

		print '.';
		usleep($lUSleep);
	}
}
catch ( Exception $e ) {
	fprintf(STDERR, "\n" . 'Last id was: ' . $lLastId . "\n");
	fprintf(STDERR, $e->getMessage() . "\n");
}

$mysqli->close();
exit($rc);

?>
