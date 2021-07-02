#!/usr/bin/php
<?php

$basedir = dirname(dirname(__FILE__));
require_once($basedir . '/lib/Constants.inc');
require_once($basedir . '/lib/myEnv.inc');

$rc = OK;

$gMyNameBase = basename(__FILE__);

$gDefaults = array(
  'user'     => 'root'
, 'password' => ''
, 'host'     => 'localhost'
, 'port'     => '3306'
, 'socket'   => '/var/run/mysql/mysqld.sock'
, 'type'     => 'innodb-bp'
);

// ---------------------------------------------------------------------
function printUsage($pMyNameBase, $pDefaults)
// ---------------------------------------------------------------------
{
	print "
Report MySQL/MariaDB statistics in a vmstat/iostat way.

usage: $pMyNameBase [--user=<user>] [--password=<password>] [--host=<hostname>]
       [--port=<port>] [--socket=<socket>]
       [--type=<type>]
       [--help] <interval> [<count>]

Options:
  user         Database user who should run the command (default = " . $pDefaults['user'] . ").
  password     Password of the database user (default = '') OR it is a
               file where the password is stored in.
  host         Hostname or IP address where database is located (default
               = " . $pDefaults['host'] . ").
  port         Port where database is listening (default = " . $pDefaults['port'] . ").
  socket       Socket where database is listening (default = " . $pDefaults['socket'] . ").
  help         Prints this help.
  type         Type of monitoring target (query-cache, binary-log, innodb-log,
               innodb-bp, innodb-dwb, galera) (default=" . $pDefaults['type'] . ").

Parameter:
  interval     Interval in seconds.
  count        Number of repetitions.

Examples:

  $pMyNameBase --user=" . $pDefaults['user'] . " --password=secret --host=" . $pDefaults['host'] . " --type=query-cache 1 3

  $pMyNameBase --user=" . $pDefaults['user'] . " --password=secret --host=" . $pDefaults['host'] . " --type=innodb-log 0.1

";
}

// ---------------------------------------------------------------------
function printQueryCache($mysqli)
// ---------------------------------------------------------------------
{
	$rc = OK;

	static $sFirstRun = 1;

	$aData = array();

	$sql = sprintf("SHOW GLOBAL VARIABLES WHERE Variable_name IN ('query_cache_size', 'query_cache_type')");

	if ( ! $result = $mysqli->query($sql) ) {
		$rc = 439;
		fprintf(STDERR, "ERROR: Invalid query: $sql, " . $mysqli->error . " (rc=%d).\n", $rc);
		return $rc;
	}

	while ( $record = $result->fetch_array(MYSQLI_ASSOC) ) {
		$aData[$record['Variable_name']] = $record['Value'];
	}

	if ( $aData['query_cache_type'] == 'ON' ) {
		$aData['query_cache_type'] = '1';
	}
	elseif ( $aData['query_cache_type'] == 'OFF' ) {
		$aData['query_cache_type'] = '0';
	}
	elseif ( $aData['query_cache_type'] == 'DEMAND' ) {
		$aData['query_cache_type'] = 'D';
	}
	else {
		$aData['query_cache_type'] = '?';
	}


	$sql = sprintf("SHOW GLOBAL STATUS WHERE Variable_name IN ('Com_select', 'Qcache_free_blocks', 'Qcache_free_memory', 'Qcache_hits'
	, 'Qcache_inserts', 'Qcache_lowmem_prunes', 'Qcache_not_cached', 'Qcache_queries_in_cache', 'Qcache_total_blocks')");

	if ( ! $result = $mysqli->query($sql) ) {
		$rc = 440;
		fprintf(STDERR, "ERROR: Invalid query: $sql, " . $mysqli->error . " (rc=%d).\n", $rc);
		return $rc;
	}

	$aVariables = array();
	while ( $record = $result->fetch_array(MYSQLI_ASSOC) ) {
		$aData[$record['Variable_name']] = $record['Value'];
	}

	// var_dump($aData);

	if ( $sFirstRun == 1 ) {
		printf("%s %-9s %-9s %-9s %-12s %-12s %-4s %s %s %s %s %s %s %s\n", 'T', 'Size', 'Free', 'Used', 'SELECT', 'HIT', 'QCHR', 'INSERT', 'LMP', 'NC', 'IC', 'Blk', 'Free Blk', 'Used Blk');
		// var_dump($aData);
	}
	printf("%s %9d %9d %9d %12d %12d %3d%%\n"
	, $aData['query_cache_type'], $aData['query_cache_size'], $aData['Qcache_free_memory']
	, ($aData['query_cache_size'] - $aData['Qcache_free_memory']), $aData['Com_select'], $aData['Qcache_hits']
	, round($aData['Qcache_hits'] / ($aData['Com_select'] + $aData['Qcache_hits']), 0)
	);

	$sFirstRun = 0;

	return $rc;
}

// ---------------------------------------------------------------------
// Start here
// ---------------------------------------------------------------------

// Check requirements

$cnt = count($argv);

// if ( ($cnt <= 1) || ($cnt >= 4) ) {
// 	$rc = 405;
// 	printUsage($gMyNameBase, $gDefaults);
// 	exit($rc);
// }

// Parse command line
$shortopts  = '';
$longopts  = array(
  'user:'
, 'password:'
, 'host:'
, 'port:'
, 'socket:'
, 'help'
, 'type:'
);


$aOptions = getopt($shortopts, $longopts);

if ( isset($aOptions['help']) ) {
	printUsage($gMyNameBase, $gDefaults);
	exit($rc);
}


// Get parameters which are not parsed with getopt

$aParameters = array();
foreach ( $argv as $key => $value ) {

	// Omit the name of the script
	if ( $key == '0' ) {
		continue;
	}

	// Omit all options (short or long)
	if ( substr($value, 0, 1) == '-' ) {
		continue;
	}

	// Everything else is considered to be a parameter
	array_push($aParameters, $value);
}

$cnt = count($aParameters);
if ( ($cnt < 1) || ($cnt > 2) ) {
	printUsage($gMyNameBase, $gDefaults);
	exit($rc);
}

if ( isset($aParameters[0]) ) {
	$pInterval = $aParameters[0];
}
// If not defined then infinit
$pCount = 1024*1024*1024;
if ( isset($aParameters[1]) ) {
	$pCount = $aParameters[1];
}


// Set defaults

if ( ! isset($aOptions['user']) ) {
	$aOptions['user'] = $gDefaults['user'];
}
if ( ! isset($aOptions['password']) ) {
	$aOptions['password'] = $gDefaults['password'];
}
if ( ! isset($aOptions['host']) ) {
	$aOptions['host'] = $gDefaults['host'];
}
if ( ! isset($aOptions['port']) ) {
	$aOptions['port'] = $gDefaults['port'];
}
if ( ! isset($aOptions['socket']) ) {
	$aOptions['socket'] = $gDefaults['socket'];
}
if ( ! isset($aOptions['type']) ) {
	$aOptions['type'] = $gDefaults['type'];
}

// Check options

if ( ! isset($aOptions['type']) ) {
	$rc = 437;
	fprintf(STDERR, "Please specify type (rc=%d).\n", $rc);
	exit($rc);
}

// ---------------------------------------------------------------------
// MAIN
// ---------------------------------------------------------------------

$mysqli = @new mysqli($aOptions['host'], $aOptions['user'], $aOptions['password'], null, $aOptions['port'], $aOptions['socket']);

if ( $mysqli->connect_error ) {
	$rc = 438;
	fprintf(STDERR, "ERROR: Connect failed: (%d) %s (rc=%d).\n", $mysqli->connect_errno, $mysqli->connect_error, $rc);
	fprintf(STDERR, "       %s@%s:%s or %s\n", $aOptions['user'], $aOptions['host'], $aOptions['port'], $aOptions['socket']);
	exit($rc);
}

// Loop over all log maintenance lines

$cnt = 1;
while ( $cnt <= $pCount ) {

	switch ($aOptions['type']) {
	case 'query-cache':
	case 'qc':
		printQueryCache($mysqli);
	  break;
	default:
		print "Unknown type " . $aOptions['type'] . "\n";
		exit;
	}

	$cnt++;
	if ( $pCount > 1 ) {
		sleep($pInterval);
	}
}

$mysqli->close();

exit($rc);

?>
