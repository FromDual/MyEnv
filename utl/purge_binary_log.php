#!/usr/bin/php
<?php

$rc = 0;

$gMyNameBase = basename(__FILE__);

$aDefaults = array (
  'user'     => 'root'
, 'password' => ''
, 'host'     => '127.0.0.1'
, 'port'     => '3306'
);

if ( count($argv) == 1 ) {
  $rc = 350;
  printUsage($gMyNameBase, $aDefaults);
  exit($rc);
}

// Parse command line
$shortopts  = "";
$longopts  = array(
  'user:'
, 'password:'
, 'host:'
, 'port:'
, 'slaves:'
, 'help'
, 'debug'
);

// ---------------------------------------------------------------------
function printUsage($pMyNameBase, $aDefaults)
// ---------------------------------------------------------------------
{
  print "
Purges binary logs of MySQL/MariaDB database.

usage: $pMyNameBase [--user=<user>] [--password=<password>] [--host=<hostname>]
       [--port=<port>] [--slaves=<host:port>,<ip_address:port>]
       [--help] [--debug]

Required privileges:

on Master: GRANT REPLICATION CLIENT, SUPER ON *.* to '" . $aDefaults['user'] . "'@'" . $aDefaults['host'] . "' IDENTIFIED BY 'secret';
on Slave : GRANT REPLICATION CLIENT ON *.* to '" . $aDefaults['user'] . "'@'slave' IDENTIFIED BY 'secret';

Options:
  user        User who does the purge operation (default = " . $aDefaults['user'] . ").
  password    Password of user (default = '').
  host        Host where to purge (default = " . $aDefaults['host'] . ").
  port        Port where MySQL is listening (default = " . $aDefaults['port'] . ").
  slaves      Slaves to consider before purging.
  debug       Debug.

Examples:

  $pMyNameBase --user=" . $aDefaults['user'] . " --password=secret --host=" . $aDefaults['host'] . " --port=" . $aDefaults['port'] . "

  $pMyNameBase --user=" . $aDefaults['user'] . " --password=secret --host=" . $aDefaults['host'] . " --port=" . $aDefaults['port'] . " --slaves=slave1,slave2,slave3:3307

";
}

// -----------------------------------------------------------------------------
// MAIN
// -----------------------------------------------------------------------------

$aOptions = getopt($shortopts, $longopts);

if ( isset($aOptions['help']) ) {
  printUsage($gMyNameBase, $aDefaults);
  exit($rc);
}

$lDebug = 0;

if ( array_key_exists('debug', $aOptions) ) {
	$lDebug = 1;
	printf("DEBUG: Debugging is on.\n");
}

foreach ( $aDefaults as $key => $value ) {
	if ( ! array_key_exists($key, $aOptions) ) {
		$aOptions[$key] = $value;
	}
}

$dbhM = @new mysqli($aOptions['host'], $aOptions['user'], $aOptions['password'], null, $aOptions['port'], null);

if ( mysqli_connect_error() ) {
	$rc = 351;
	fprintf(STDERR, "ERROR: Connect failed: (%d) %s (rc=%d).\n", mysqli_connect_errno(), mysqli_connect_error(), $rc);
	exit($rc);
}
$dbhM->query('SET NAMES utf8');

$sql = 'SHOW MASTER STATUS';
if ( $lDebug > 0 ) {
	printf("DEBUG: $sql\n");
}
$sBinaryLog = '';

if ( ! $result = $dbhM->query($sql) ) {
  $rc = 352;
  fprintf(STDERR, "ERROR: Invalid query: $sql, " . $dbhM->error . " (rc=%d)\n", $rc);
  exit($rc);
}

while ( $row = $result->fetch_assoc() ) {
	$sBinaryLog = $row['File'];
	if ( $lDebug > 0 ) {
		printf("DEBUG: $sBinaryLog\n");
	}
}
$result->close();

if ( $sBinaryLog == '' ) {
  $rc = 353;
  fprintf(STDERR, 'Database (' . $aOptions['host'] . '/' . $aOptions['port'] . ") has binary log NOT enabled (rc=%d)\n", $rc);
  exit($rc);
}


// Checks slaves

if ( array_key_exists('slaves', $aOptions) ) {

	$sql = 'SHOW SLAVE STATUS';
	if ( $lDebug > 0 ) {
		printf("DEBUG: $sql\n");
	}

	foreach ( explode(',', $aOptions['slaves']) as $s ) {

		if ( $lDebug > 0 ) {
			printf("DEBUG: $s\n");
		}

		// slave:3306
		if ( strpos($s, ':') !== false ) {
			list($sHost, $sPort) = explode(':', $s);
		}
		else {
			$sHost = $s;
			$sPort = 3306;
		}

		$dbhS = @new mysqli($sHost, $aOptions['user'], $aOptions['password'], null, $sPort, null);

		if ( mysqli_connect_error() ) {
			$rc = 354;
			fprintf(STDERR, "ERROR: Connect failed: (%d) %s (rc=%d).\n", mysqli_connect_errno(), mysqli_connect_error(), $rc);
			exit($rc);
		}
		$dbhS->query('SET NAMES utf8');


		$sMasterBinaryLog = '';

		if ( ! $result = $dbhS->query($sql) ) {
			$rc = 355;
			fprintf(STDERR, "ERROR: Invalid query: $sql, " . $dbhS->error . " (rc=%d)\n", $rc);
			exit($rc);
		}

		while ( $row = $result->fetch_assoc() ) {
			// Master_Log_File
			$sMasterBinaryLog = $row['Master_Log_File'];
			if ( $lDebug > 0 ) {
			printf("DEBUG: $sMasterBinaryLog\n");
			}
		}

		$result->close();
		$dbhS->close();


		if ( $sMasterBinaryLog == '' ) {

			$rc = 356;
			fprintf(STDERR, "ERROR: %s (port %s) is not a slave (rc=%d).\n", $sHost, $sPort, $rc);
			// no exit here!
		}

		// If Slave is behind only purge until Slaves binary log!
		if ( $sMasterBinaryLog < $sBinaryLog ) {
			$rc = 357;
			fprintf(STDOUT, "WARNING: Slave %s is reading from older binary log (%s) than master is writing to (%s) (rc=%d).\n", $sHost, $sMasterBinaryLog, $sBinaryLog, $rc);
			$sBinaryLog = $sMasterBinaryLog;
			$rc = 0;
		}
	}
}

$sql = sprintf("PURGE BINARY LOGS TO '%s'", $sBinaryLog);
if ( $lDebug > 0 ) {
  printf("DEBUG: $sql\n");
}
$sMasterBinaryLog = '';

if ( ! $result = $dbhM->query($sql) ) {
  $rc = 358;
  fprintf(STDERR, "ERROR: Invalid query: $sql, " . $dbhM->error . " (rc=%d)\n", $rc);
  exit($rc);
}

$dbhM->close();

exit($rc);

?>
