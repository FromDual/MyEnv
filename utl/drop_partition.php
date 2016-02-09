#!/usr/bin/php
<?php

$rc = 0;

$gMyNameBase = basename(__FILE__);

if ( count($argv) == 1 ) {
  $rc = 322;
  printUsage();
  exit($rc);
}

// Parse command line
$shortopts  = "";
$longopts  = array(
  'user:'
, 'password:'
, 'host:'
, 'port:'
, 'socket:'
, 'schema:'
, 'table:'
, 'number:'
, 'help'
, 'debug'
, 'dryrun'
);

$aOptions = getopt($shortopts, $longopts);

if ( isset($aOptions['help']) ) {
  printUsage();
  exit($rc);
}

if ( isset($aOptions['debug']) ) {
	print_r($argv);
}

// Set defaults
if ( ! isset($aOptions['user']) ) {
  $aOptions['user'] = 'root';
}
if ( ! isset($aOptions['password']) ) {
  $aOptions['password'] = '';
}
if ( ! isset($aOptions['host']) ) {
  $aOptions['host'] = 'localhost';
}
if ( ! isset($aOptions['port']) ) {
  $aOptions['port'] = 3306;
}
if ( ! isset($aOptions['socket']) ) {
  $aOptions['socket'] = '/var/run/mysqld/mysql.sock';
}

// If password is a file extrac password from file
if ( file_exists($aOptions['password']) ) {
  
	$handle = @fopen($aOptions['password'], 'r');
	if ( $handle ) {
	
		while ( ($buffer = fgets($handle, 4096)) !== false ) {

      if ( preg_match('/password\s*=\s*(.*)$/', $buffer, $matches) ) {
        $aOptions['password'] = trim($matches[1]);
      }
		}
		if ( ! feof($handle) ) {
			fprintf(STDERR, "Error: unexpected fgets() fail.\n");
    }
		fclose($handle);
	}
}

if ( isset($aOptions['debug']) ) {
	print_r($aOptions);
}

// Check options
if ( ! isset($aOptions['number']) ) {
  $rc = 323;
  fprintf(STDERR, "Please specify number --number (rc=%d)\n", $rc);
  exit($rc);
}
if ( intval($aOptions['number']) == 0 ) {
  $rc = 324;
  fprintf(STDERR, "Number must be at least 1 (rc=%d)\n", $rc);
  exit($rc);
}

// ---------------------------------------------------------------------
function printUsage()
// ---------------------------------------------------------------------
{
  global $gMyNameBase;

  print "

Drop the oldest partitions (but max 10) so that --number of partitons remain:

usage: $gMyNameBase [--user=<user>] [--password=<password>] [--host=<hostname>]
       [--port=3306] [--socket=<socket>]
       --schema=<schema_name> --table=<table_name> --number=<n>
       [--debug] [--dryrun] [--help]

Options:
  user        Database user who should run the command (default = root).
  password    Password of the database user (default = '') OR it is a
              file where the password is stored in.
  host        Hostname or IP address where database is located (default
              = localhost).
  port        Port where database is listening (default = 3306).
  socket      Socket where database is listening (defaul =
              /var/run/mysqld/mysql.sock).
  schema      Schema where partitoned table is located.
  table       Table to split the newest partiton.
  number      Number of partitions to remain.
  help        Prints this help.
  debug       Prints all debugging information.
  dryrun      Only prints SQL statement but do NOT execute.

Examples:

  $gMyNameBase --user=root --password=secret --host=192.168.1.42 --schema=dwh --table=sales --number=26

";
}

// ---------------------------------------------------------------------
// MAIN
// ---------------------------------------------------------------------

$mysqli = @new mysqli($aOptions['host'], $aOptions['user'], $aOptions['password'], null, $aOptions['port'], $aOptions['socket']);

if ( mysqli_connect_error() ) {
	$rc = 325;
	fprintf(STDERR, "ERROR: Connect failed: (%d) %s (rc=%d).\n", mysqli_connect_errno(), mysqli_connect_error(), $rc);
	exit($rc);
}
$sql = 'SET NAMES utf8';
if ( isset($aOptions['debug']) ) {
	print "$sql\n";
}
$mysqli->query($sql);

$sql = sprintf("SELECT partition_name
  FROM information_schema.partitions
 WHERE table_schema = '%s'
   AND table_name = '%s'
   AND partition_name IS NOT NULL
 ORDER BY partition_ordinal_position DESC
 LIMIT %d, 10", $aOptions['schema'], $aOptions['table'], $aOptions['number']);

if ( isset($aOptions['debug']) ) {
	print $sql . "\n";
}

if ( ! $result = $mysqli->query($sql) ) {
	$rc = 328;
	fprintf(STDERR, "ERROR: Invalid query: $sql, " . $mysqli->error . " (rc=%d)\n", $rc);
	exit($rc);
}

$cnt = 0;
while ( $record = $result->fetch_array(MYSQLI_ASSOC) ) {

	if ( isset($aOptions['debug']) ) {
		print_r($record);
	}

	$sql = sprintf("ALTER TABLE %s.%s DROP PARTITION %s", $aOptions['schema'], $aOptions['table'], $record['partition_name']);

	print date('Y-m-d H:i:s') . "\n";
	print $sql . "\n";

	if ( ! isset($aOptions['dryrun']) ) {
		if ( ! $mysqli->query($sql) ) {
			$rc = 329;
			fprintf(STDERR, "ERROR: %s %s : %s (rc=$rc).\n", $mysqli->sqlstate, $mysqli->errno, $mysqli->error);
		}
	}
	$cnt++;
}

if ( $cnt == 0 ) {
	print "No partition found to drop.\n";
}

$mysqli->close();
exit($rc);

?>
