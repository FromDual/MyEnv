#!/usr/bin/php
<?php

$rc = 0;

$gMyNameBase = basename(__FILE__);

if ( count($argv) == 1 ) {
  $rc = 333;
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
, 'help'
, 'debug'
, 'dryrun'
);

$aOptions = getopt($shortopts, $longopts);

if ( isset($aOptions['help']) ) {
  printUsage();
  exit($rc);
}

if ( (count($argv) - 1) != count($aOptions) ) {

	$rc = 445;

	fwrite(STDERR, "ERROR: Options were not entered correctly. Please fix it (rc=$rc).\n");

	// Check and show which variables are not correct

	// Remove 1st option which is the filename
	unset($argv[0]);
	// Remove all options which were detected corretly from argv
	foreach ( $aOptions as $option => $v ) {
		foreach ( $argv as $key => $value ) {
		  $pattern = "/^\-\-$option/";
			if ( preg_match($pattern, $value) ) {
				unset($argv[$key]);
			}
		}
	}

	fwrite(STDERR, "       I could not interprete the following options:\n");
	fwrite(STDERR, print_r($argv, true));
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
// if ( ! isset($aOptions['socket']) ) {
//   $aOptions['socket'] = '/run/mysqld/mysqld.sock';
// }

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
// no checks defined yet

// ---------------------------------------------------------------------
function printUsage()
// ---------------------------------------------------------------------
{
  global $gMyNameBase;

  print "

Split the latest partition into this weeks partition and next weeks partition:

usage: $gMyNameBase [--user=<user>] [--password=<password>] [--host=<hostname>]
       [--port=3306] [--socket=<socket>]
       --schema=<schema_name> --table=<table_name>
       [--debug] [--dryrun] [--help]

Options:
  user        Database user who should run the command (default = root).
  password    Password of the database user (default = '') OR it is a
              file where the password is stored in.
  host        Hostname or IP address where database is located (default =
              localhost).
  port        Port where database is listening (default = 3306).
  socket      Socket where database is listening (defaul =
              /run/mysqld/mysqld.sock).
  schema      Schema where partitoned table is located.
  table       Table to split the newest partiton.
  help        Prints this help.
  debug       Prints all debugging information.
  dryrun      Only prints SQL statement but do NOT execute.

Examples:

  $gMyNameBase --user=root --password=secret --host=192.168.1.42 --schema=dwh --table=sales

";
}

// ---------------------------------------------------------------------
// MAIN
// ---------------------------------------------------------------------

$mysqli = @new mysqli($aOptions['host'], $aOptions['user'], $aOptions['password'], null, $aOptions['port'], $aOptions['socket']);

if ( mysqli_connect_error() ) {
  $rc = 337;
  fprintf(STDERR, "ERROR: Connect failed: (%d) %s (rc=%d).\n", mysqli_connect_errno(), mysqli_connect_error(), $rc);
  exit($rc);
}
$sql = 'SET NAMES utf8';
if ( isset($aOptions['debug']) ) {
	print "$sql\n";
}
$mysqli->query($sql);

// select table_name, partition_name, PARTITION_ORDINAL_POSITION, PARTITION_DESCRIPTION, from_unixtime(PARTITION_DESCRIPTION) from partitions where TABLE_NAME = 'history';

$sql = sprintf("SELECT partition_name AS 'partition_name', MAX(partition_ordinal_position) AS 'ptn'
  FROM information_schema.partitions
 WHERE table_schema = '%s'
   AND table_name = '%s'
   AND partition_name IS NOT NULL
 GROUP BY partition_name
 ORDER BY ptn DESC
LIMIT 1", $aOptions['schema'], $aOptions['table']);

if ( isset($aOptions['debug']) ) {
	print $sql . "\n";
}

if ( ! $result = $mysqli->query($sql) ) {
  $rc = 338;
  fprintf(STDERR, "ERROR: Invalid query: $sql, " . $mysqli->error . " (rc=%d)\n", $rc);
  exit($rc);
}
if ( ! $record = $result->fetch_array(MYSQLI_ASSOC) ) {
  $rc = 339;
  fprintf(STDERR, "ERROR: No record found (rc=%d).\n", $rc);
  exit($rc);
}

if ( isset($aOptions['debug']) ) {
	print_r($record);
}

# Do some calculations
$ts  = strtotime(date('Y-m-d', strtotime('+14 day')) . ' 00:00:00');
// $ts = strtotime('2019-01-14 00:00:00');
$dt  = date('Y-m-d H:i:s', $ts);
$ptn = 'p' . date('Y', $ts) . '_kw' . sprintf("%02d", date('W', $ts));

$sql = sprintf("ALTER TABLE %s.%s
REORGANIZE PARTITION %s
INTO (
  PARTITION %s VALUES LESS THAN (UNIX_TIMESTAMP('%s'))
, PARTITION %s VALUES LESS THAN (MAXVALUE)
)"
, $aOptions['schema'], $aOptions['table'], $record['partition_name']
, $ptn, $dt, $record['partition_name']);

print date('Y-m-d H:i:s') . "\n";
print $sql . "\n";

if ( ! isset($aOptions['dryrun']) ) {
	if ( ! $mysqli->query($sql) ) {
		$rc = 340;
		fprintf(STDERR, "ERROR: %s %s : %s (rc=%d).\n", $mysqli->sqlstate, $mysqli->errno, $mysqli->error, $rc);
	}
}

$mysqli->close();
exit($rc);

?>
