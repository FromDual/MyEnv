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
);

// ---------------------------------------------------------------------
function printUsage($pMyNameBase, $pDefaults)
// ---------------------------------------------------------------------
{
	print "
Do log (and other files) maintenance. User needs database RELOAD privilege and
O/S read and write access to the file and destination.

usage: $pMyNameBase [--user=<user>] [--password=<password>] [--host=<hostname>]
       [--port=<port>] [--socket=<socket>]
       --operation=<delete | archive | truncate> --type=<general | error | slow | file>
       [--lines=<n>] [--destination=<path>]
       [--config=<configuration file>]
       [--help] [--debug]

Options:
  user         Database user who should run the command (default = " . $pDefaults['user'] . ").
  password     Password of the database user (default = '') OR it is a
               file where the password is stored in.
  host         Hostname or IP address where database is located (default
               = " . $pDefaults['host'] . ").
  port         Port where database is listening (default = " . $pDefaults['port'] . ").
  socket       Socket where database is listening (default = " . $pDefaults['socket'] . ").
  help         Prints this help.
  debug        Print debug information.
  type         Type of log file (general, error, slow, file)
  operation    What to do (delete, archive, truncate)
  destination  Directory where to move the data to (no indications means: same directory).
  lines        Number of lines to keep.
  config       Configuration file (see template).

Operation:
  delete       Deletes file and does FLUSH LOGS.
  archive      Moves file to archive destination and does FLUSH LOGS.
  truncate     Moves file to tmp, cuts last n lines into original file, deletes
               tmp file and does FLUSH LOGS.

Examples:

  $pMyNameBase --user=" . $pDefaults['user'] . " --password=secret --host=" . $pDefaults['host'] . " --type=slow --operation=truncate --lines=1000

  $pMyNameBase --user=" . $pDefaults['user'] . " --password=secret --host=" . '127.0.0.1' . " --port=" . $pDefaults['port'] . " --type=general --operation=delete

  $pMyNameBase --user=" . $pDefaults['user'] . " --password=secret --host=" . 'localhost' . " --socket=" . $pDefaults['socket'] . " --type=slow --operation=archive --destination=/tmp/arch --lines=1000

  $pMyNameBase --user=" . $pDefaults['user'] . " --password=secret --host=" . 'localhost' . " --socket=" . $pDefaults['socket'] . " --type=error --operation=truncate --lines=100

  $pMyNameBase --user=" . $pDefaults['user'] . " --host=" . 'localhost' . " --type=file --operation=truncate --lines=100

  $pMyNameBase --config=/etc/" . basename($pMyNameBase, '.php') . ".conf

";
}

// ---------------------------------------------------------------------
function deleteFile()
// ---------------------------------------------------------------------
{
	$rc = OK;
	return $rc;
}

// ---------------------------------------------------------------------
function archiveFile()
// ---------------------------------------------------------------------
{
	$rc = OK;
	return $rc;
}

// ---------------------------------------------------------------------
function truncateFile($pOld, $pNew, $pLines = 1000)
// http://stackoverflow.com/questions/15025875/what-is-the-best-way-in-php-to-read-last-lines-from-a-file/15025877#15025877
// ---------------------------------------------------------------------
{
	$rc = OK;

	$buffer = 4096;

	if ( ($fh = @fopen($pOld, "rb")) === false ) {
		$rc = 403;
		$error = error_get_last();
		fprintf(STDERR, "ERROR: %s (rc=%d).\n", $error['message'], $rc);
		return $rc;
	}

	// Jump to last character
	fseek($fh, -1, SEEK_END);

	// Read it and adjust line number if necessary
	// (Otherwise the result would be wrong if file does not end with a blank line)
	if ( fread($fh, 1) != "\n" ) {
		$pLines -= 1;
	}

	// Start reading
	$output = '';
	$chunk = '';

	if ( ($fhn = @fopen($pNew, "wb")) === false ) {
		$rc = 404;
		$error = error_get_last();
		fprintf(STDERR, "ERROR: %s (rc=%d).\n", $error['message'], $rc);
		return $rc;
	}

	while ( ftell($fh) > 0 && $pLines >= 0 ) {

		// Figure out how far back we should jump
		$seek = min(ftell($fh), $buffer);

		// Do the jump (backwards, relative to where we are)
		fseek($fh, -$seek, SEEK_CUR);

		// Read a chunk and prepend it to our output
		$output = ($chunk = fread($fh, $seek)) . $output;

		// Jump back to where we started reading
		fseek($fh, -mb_strlen($chunk, '8bit'), SEEK_CUR);

		// Decrease our line counter
		$pLines -= substr_count($chunk, "\n");
	}

	// While we have too many lines
	// (Because of buffer size we might have read too many)
	while ( $pLines++ < 0 ) {

		// Find first newline and remove all text before that
		$output = substr($output, strpos($output, "\n") + 1);
	}

	fclose($fh);

	fwrite($fhn, $output);
	fclose($fhn);

	return $rc;
}

// ---------------------------------------------------------------------
function readOurConfigFile($pConfigurationFile)
// ---------------------------------------------------------------------
{
	$rc = OK;
	$aLines = array();

	// Replace all comments like:
	// \w*#$

	if ( ! file_exists($pConfigurationFile) ) {
		$rc = 427;
		fprintf(STDERR, "File %s does not exist (rc=%d).\n", $pConfigurationFile, $rc);
		return array($rc, $aLines);
	}

	if ( ! is_readable($pConfigurationFile) ) {
		$rc = 428;
		fprintf(STDERR, "File %s is not readable (rc=%d).\n", $pConfigurationFile, $rc);
		return array($rc, $aLines);
	}

	if ( ($aRawConf = file($pConfigurationFile, FILE_IGNORE_NEW_LINES | FILE_SKIP_EMPTY_LINES)) === false ) {
		$rc = 429;
		fprintf(STDERR, "Cannot read file %s (rc=%d).\n", $pConfigurationFile, $rc);
		return array($rc, $aLines);
	}

	$pattern = array(
		'/^#.*$/'
	, '/\s+#.*$/'
	, '/^!include/'
	, '/\s+$/'
	);
	foreach ( $aRawConf as $line ) {
		$line = trim(preg_replace($pattern, '', $line));
		if ( $line != '' ) {

			if ( preg_match('/^(\S+)\s+(\S+)\s+(\S+)\s*(\S+)?\s*(\S+)?$/', $line, $matches) ) {

				// var_dump($matches);
				list($ret, $aTarget) = parseConnectString($matches[1]);
				// var_dump($aTarget);

				$aLine = array(
				  'user'        => $aTarget['user']
				, 'password'    => $aTarget['password']
				, 'host'        => $aTarget['host']
				, 'port'        => $aTarget['port']
				, 'socket'      => $aTarget['socket']
				, 'type'        => $matches[2]
				, 'operation'   => $matches[3]
				, 'destination' => isset($matches[4]) ? strval($matches[4]) : ''
				, 'lines'       => isset($matches[5]) ? intval($matches[5]) : 0
				);

				array_push($aLines, $aLine);
			}
			else {
				$rc = 431;
				fprintf(STDERR, "Line is not properly formed (rc=%d):\n%s\n", $rc, $line);
				// fprintf(STDERR, print_r($matches, true));
				// Do not exit here to catch all malformed lines...
			}
		}
	}

	return array($rc, $aLines);
}

// ---------------------------------------------------------------------
// Start here
// ---------------------------------------------------------------------

// Check requirements

$f = 'mb_strlen';
if ( ! function_exists($f) ) {
	$rc = 434;
	$msg = "PHP function $f is missing. Please install it first.";
	fprintf(STDERR, "%s (rc=%d).\n", $msg, $rc);
	exit($rc);
}


if ( count($argv) == 1 ) {
	$rc = 405;
	printUsage($gMyNameBase, $gDefaults);
	exit($rc);
}

// Parse command line
$shortopts  = '';
$longopts  = array(
  'user:'
, 'password:'
, 'host:'
, 'port:'
, 'socket:'
, 'help'
, 'operation:'
, 'type:'
, 'lines:'
, 'destination:'
, 'config:'
, 'debug'
);


$aOptions = getopt($shortopts, $longopts);

if ( isset($aOptions['help']) ) {
	printUsage($gMyNameBase, $gDefaults);
	exit($rc);
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

// Read config files

$aMyCnfFiles = array(
	'/etc/my.cnf'
, '/etc/mysql/my.cnf'
, $_ENV['HOME'] . '/.my.cnf'
);

if ( isset($aOptions['debug']) ) {
	print_r($aMyCnfFiles);
}

$aMyCnf = array();
foreach ( $aMyCnfFiles as $filename ) {

	if ( is_readable($filename) ) {
		if ( isset($aOptions['debug']) ) {
			print "\nReading configuation from $filename\n";
		}
		list($ret, $aConfig) = parseConfigFile($filename);
		// An error on parsing!
		if ( $aConfig === false ) {
			$rc = 425;
			$err = error_get_last(); 
			print trim($err['message']) . " (rc=$rc).\n";
			exit($rc);
		}
		
		// sections [client]
		if ( array_key_exists('client', $aConfig) ) {
			if ( array_key_exists('user', $aConfig['client']) ) {
				$aMyCnf['user'] = $aConfig['client']['user'];
			}
			if ( array_key_exists('password', $aConfig['client']) ) {
				$aMyCnf['password'] = $aConfig['client']['password'];
			}
		}
	}
	else {
		if ( isset($aOptions['debug']) ) {
			print "No config file $filename found.\n";
		}
	}
}

foreach ( $aMyCnf as $key => $value ) {
  $aOptions[$key] = $value;
}


if ( isset($aOptions['debug']) ) {
  print "Resulting options:\n";
	print_r($aOptions);
}

// Check options

if ( isset($aOptions['config']) ) {

	if ( ! is_readable($aOptions['config']) ) {
		$rc = 426;
		fprintf(STDERR, "Cannot read configuration file " . $aOptions['config'] . " (rc=%d).\n", $rc);
		exit($rc);
	}
}
// We have no config, do all the checks for the options
else {

	if ( ! isset($aOptions['type']) ) {
		$rc = 406;
		fprintf(STDERR, "Please specify type (rc=%d).\n", $rc);
		exit($rc);
	}

	if ( ! isset($aOptions['operation']) ) {
		$rc = 407;
		fprintf(STDERR, "Please specify operation (rc=%d).\n", $rc);
		exit($rc);
	}

	if ( ($aOptions['operation'] == 'archive') && (! isset($aOptions['destination'])) ) {
		$rc = 408;
		fprintf(STDERR, "Destination must be set for operation " . $aOptions['operation'] . " (rc=%d).\n", $rc);
		exit($rc);
	}

	if ( isset($aOptions['destination']) ) {
		if ( ! is_dir($aOptions['destination']) ) {
			$rc = 409;
			fprintf(STDERR, "Directory %s does not exist (rc=%d).\n", $aOptions['destination'], $rc);
			exit($rc);
		}
	}

	if ( $aOptions['operation'] == 'truncate' ) {
		if ( ! isset($aOptions['lines']) ) {
			$rc = 433;
			fprintf(STDERR, "Operation truncate whithout lines does not make sense (rc=%d).\n", $rc);
			exit($rc);
		}
		if ( $aOptions['lines'] > 10000 ) {
			$rc = 435;
			fprintf(STDERR, "Operation truncate whit too many lines (> 10000) is ineffective. Please use archive instead (rc=%d).\n", $rc);
			exit($rc);
		}
	}
}

// ---------------------------------------------------------------------
// MAIN
// ---------------------------------------------------------------------

// Read config file

if ( isset($aOptions['config']) ) {

	list($ret, $aLines) = readOurConfigFile($aOptions['config']);
	if ( isset($aOptions['debug']) ) {
		print_r($aLines);
	}

	if ( $ret != OK ) {
		$rc = 432;
		fprintf(STDERR, "Config file %s could not be parsed (rc=$rc).\n", $aOptions['config'], $rc);
		exit($rc);
	}
	if ( ($ret == OK) && (count($aLines) == 0) ) {
		$rc = 430;
		fprintf(STDERR, "Config file %s does not contain any valid configuration (rc=$rc).\n", $aOptions['config'], $rc);
		exit($rc);
	}
}
// No config, only command line
else {
	$aLines = array($aOptions);
}

// Loop over all log maintenance lines
foreach ( $aLines as $line ) {

	foreach ( $line as $key => $value ) {
		$aOptions[$key] = $value;
	}

	// todo: Check options possibly should better be done here...

	$mysqli = @new mysqli($aOptions['host'], $aOptions['user'], $aOptions['password'], null, $aOptions['port'], $aOptions['socket']);

	if ( $mysqli->connect_error ) {
		$rc = 410;
		fprintf(STDERR, "ERROR: Connect failed: (%d) %s (rc=%d).\n", $mysqli->connect_errno, $mysqli->connect_error, $rc);
		fprintf(STDERR, "       %s@%s:%s or %s\n", $aOptions['user'], $aOptions['host'], $aOptions['port'], $aOptions['socket']);
		continue;
	}
	if ( isset($aOptions['debug']) ) {
		print $mysqli->host_info . "\n";
	}
	$sql = 'SET NAMES utf8';
	if ( isset($aOptions['debug']) ) {
		print "$sql\n";
	}
	$mysqli->query($sql);


	// Get all necessary variables

	// general_log_file    | general.log
	// slow_query_log_file | slow.log
	// log_error           | /home/mysql/data/mysql-5.6.22/error.log
	// datadir             | /home/mysql/data/mysql-5.6.22/

	$sql = sprintf("SHOW GLOBAL VARIABLES WHERE Variable_name IN ('general_log_file', 'slow_query_log_file', 'log_error', 'datadir')");

	if ( isset($aOptions['debug']) ) {
		print $sql . "\n";
	}

	if ( ! $result = $mysqli->query($sql) ) {
		$rc = 411;
		fprintf(STDERR, "ERROR: Invalid query: $sql, " . $mysqli->error . " (rc=%d).\n", $rc);
		continue;
	}

	$aVariables = array();
	while ( $record = $result->fetch_array(MYSQLI_ASSOC) ) {

		if ( $record['Variable_name'] == 'datadir' ) {
			$aVariables[$record['Variable_name']] = rtrim($record['Value'], '/');
		}
		else {
			$aVariables[$record['Variable_name']] = $record['Value'];
		}
	}
	if ( isset($aOptions['debug']) ) {
		print_r($aVariables);
	}

	$lFile = '';
	switch ( $aOptions['type'] ) {
	case 'general':
		if ( is_file($aVariables['general_log_file'])) {
			$lFile = $aVariables['general_log_file'];
		}
		else {
			$lFile = $aVariables['datadir'] . '/' . ltrim($aVariables['general_log_file'], '/');
			if ( ! is_file($lFile) ) {
				$rc = 412;
				fprintf(STDERR, "ERROR: neigther file " . $aVariables['general_log_file'] . ' nor file '. $lFile . " exists (rc=%d).\n", $rc);
				continue 2;
			}
		}
		break;
	case 'error':
		if ( is_file($aVariables['log_error'])) {
			$lFile = $aVariables['log_error'];
		}
		else {
			$rc = 413;
			fprintf(STDERR, "ERROR: file " . $aVariables['log_error'] . " does not exist (rc=%d).\n", $rc);
			continue 2;
		}
		break;
	case 'slow':
		if ( is_file($aVariables['slow_query_log_file'])) {
			$lFile = $aVariables['slow_query_log_file'];
		}
		else {
			$lFile = $aVariables['datadir'] . '/' . ltrim($aVariables['slow_query_log_file'], '/');
			if ( ! is_file($lFile) ) {
				$rc = 414;
				fprintf(STDERR, "ERROR: neigther file " . $aVariables['slow_query_log_file'] . ' nor file '. $lFile . " exists (rc=%d).\n", $rc);
				continue 2;
			}
		}
		break;
	case 'file':
		$rc = 415;
		fprintf(STDERR, "ERROR: Type %s ist not implemented yet (rc=%d).\n", $aOptions['type'], $rc);
		continue 2;
		break;
	default:
		$rc = 416;
		fprintf(STDERR, "ERROR: Type %s does not exist (rc=%d).\n", $aOptions['type'], $rc);
		continue 2;
		break;
	}

	if ( isset($aOptions['debug']) ) {
		print $lFile . "\n";
	}

	// Do operation

	switch ( $aOptions['operation'] ) {
	case 'delete':
		if ( isset($aOptions['debug']) ) {
			print "Unlink $lFile\n";
		}
		if ( @unlink($lFile) === false ) {
			$rc = 417;
			$error = error_get_last();
			fprintf(STDERR, "ERROR: %s (rc=%d).\n", $error['message'], $rc);
			continue 2;
		}
		// This works only with MySQL >= v5.5.3
		$sql = sprintf("FLUSH %s LOGS", $aOptions['type']);

		if ( isset($aOptions['debug']) ) {
			print $sql . "\n";
		}

		if ( ! $result = $mysqli->query($sql) ) {
			$rc = 418;
			fprintf(STDERR, "ERROR: Invalid query: $sql, " . $mysqli->error . " (rc=%d).\n", $rc);
			continue 2;
		}
		break;
	case 'archive':
		$name = basename($lFile);
		$dst  = $aOptions['destination'] . '/' . $name . '.' . date('Y-m-d_H-i-s');
		if ( isset($aOptions['debug']) ) {
			print "Move $lFile to $dst\n";
		}
		if ( @rename($lFile, $dst) === false ) {
			$rc = 419;
			$error = error_get_last();
			fprintf(STDERR, "ERROR: %s (rc=%d).\n", $error['message'], $rc);
			continue 2;
		}
		// This works only with MySQL >= v5.5.3
		$sql = sprintf("FLUSH %s LOGS", $aOptions['type']);

		if ( isset($aOptions['debug']) ) {
			print $sql . "\n";
		}

		if ( ! $result = $mysqli->query($sql) ) {
			$rc = 420;
			fprintf(STDERR, "ERROR: Invalid query: $sql, " . $mysqli->error . " (rc=%d).\n", $rc);
			continue 2;
		}
		break;
	case 'truncate':

		// todo: archive as well???

		$tmp = $lFile . '.tmp';
		if ( isset($aOptions['debug']) ) {
			print "Rename $lFile to $tmp\n";
		}
		// move
		if ( @rename($lFile, $tmp) === false ) {
			$rc = 421;
			$error = error_get_last();
			fprintf(STDERR, "ERROR: %s (rc=%d).\n", $error['message'], $rc);
			continue 2;
		}

		if ( isset($aOptions['debug']) ) {
			print "truncateFile($tmp, $lFile, " .  $aOptions['lines'] . ").\n";  
		}
		$ret = truncateFile($tmp, $lFile, $aOptions['lines']);
		if ( $ret != OK ) {
			continue 2;
		}

		if ( isset($aOptions['debug']) ) {
			print "Delete $tmp\n";
		}
		// unlink old file
		if ( @unlink($tmp) === false ) {
			$rc = 422;
			$error = error_get_last();
			fprintf(STDERR, "ERROR: %s (rc=%d).\n", $error['message'], $rc);
			continue 2;
		}

		// This works only with MySQL >= v5.5.3
		$sql = sprintf("FLUSH %s LOGS", $aOptions['type']);

		if ( isset($aOptions['debug']) ) {
			print $sql . "\n";
		}

		if ( ! $result = $mysqli->query($sql) ) {
			$rc = 423;
			fprintf(STDERR, "ERROR: Invalid query: $sql, " . $mysqli->error . " (rc=%d).\n", $rc);
			continue 2;
		}
		break;
	default:
		$rc = 424;
		fprintf(STDERR, "ERROR: Operation %s is not supported (rc=%d).\n", $aOptions['operation'], $rc);
		continue 2;
		break;
	}

	$mysqli->close();
}

exit($rc);

?>
