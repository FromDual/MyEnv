#!/usr/bin/php -d variables_order=EGPCS
<?php

// TODO: should not be hard coded but configurable!
ini_set('date.timezone', 'Europe/Zurich');

// Caution $_ENV variables are not be known here yet when variables_order
// does NOT contain E!!!
if ( isset($_ENV['MYENV_BASE']) ) {
	$basedir = strval($_ENV['MYENV_BASE']);
}
// We have to guess:
else {
	$basedir = dirname(dirname(__FILE__));
}

$lMyEnvConfFile = '/etc/myenv/myenv.conf';

require_once($basedir . '/lib/myEnv.inc');
require_once($basedir . '/lib/myEnv.inc');

$rc = OK;

if ( checkMyEnvRequirements() == ERR ) {
	$rc = 540;
	exit($rc);
}

$PHP        = '/usr/bin/php';
$MyBasename = basename(__FILE__, '.php');

debug('basename = ' . $MyBasename . "\n");

if ( '' == $basedir ) {
	output('Basedir is not set.' . "\n");
	$rc = 541;
	exit($rc);
}

debug('MYENV_BASE = ' . $basedir . "\n");

// TODO this is possibly redundant with installMyEnv.php
$LogFile = $basedir . '/log/' . $MyBasename . '.log';
$handle = fopen($LogFile, 'a');
if ( ! $handle ) {
	$rc = 542;
	$msg = 'Cannot log to file ' . $LogFile . " (rc=$rc)";
	output($msg . "\n");
	exit($rc);
}

// Get user for log file, etc.
$lCurrentUser = getCurrentUser();
list($ret, $aConfiguration) = getConfiguration($lMyEnvConfFile);
if ( '' != $aConfiguration['default']['user'] ) {
	$lCurrentUser = $aConfiguration['default']['user'];
}
chown($LogFile, $lCurrentUser);
// TODO: This is bullshit in case grp != user (mysql:adm) or so...
chgrp($LogFile, $lCurrentUser);


switch ( $argv[1] ) {
case 'start':
	// TODO: What about SystemD/JournalD?
	fputs($handle, date('Y-m-d H:i:s') . ' Starting ' . $MyBasename . "\n");
	// find all databases to start
	list($ret, $databases) = printStartingDatabases($lMyEnvConfFile);
	if ( $ret != OK ) {
		exit($ret);
	}
	if ( count($databases) == 0 ) {
		fputs($handle, date('Y-m-d H:i:s') . ' No instances to start.' . "\n");
	}
	else {
		fputs($handle, date('Y-m-d H:i:s') . ' Starting instances: ' . implode(' ', $databases) . "\n");

		foreach ( $databases as $db ) {
			fputs($handle, date('Y-m-d H:i:s') . '   Starting instance: ' . $db . "\n");
			$aStdout = array();
			list($ret, $output, $aStdout, $aStderr) = my_exec($PHP . " -d variables_order=EGPCS -f $basedir/bin/database.php $db " . $argv[1] . ' 2>&1');
			if ( $ret != OK ) {
				$rc++;
			}
			if ( count($aStderr) > 0 ) {
				fputs($handle, date('Y-m-d H:i:s') . ' ' . implode("\n", $aStderr) . "\n");
			}
		}
		// Should be done for all
		fputs($handle, date('Y-m-d H:i:s') . ' Finished ' . $MyBasename . ' ' . $argv[1] . " (rc=$rc)" . "\n");
		fputs($handle, date('Y-m-d H:i:s') . ' ----' . "\n");
	}
break;
case 'stop':

	fputs($handle, date('Y-m-d H:i:s') . ' Stopping ' . $MyBasename . "\n");
	// find all started databases and stop them (better to stop all started db,
	// that are still running or even all running dbs?)
	list($ret, $databases) = printStoppingDatabases($lMyEnvConfFile);
	if ( $ret != OK ) {
		exit($ret);
	}
	if ( count($databases) == 0 ) {
		fputs($handle, date('Y-m-d H:i:s') . ' No instances to stop.' . "\n");
	}
	else {
		fputs($handle, date('Y-m-d H:i:s') . ' Stopping instances: ' . implode(' ' , $databases) . "\n");
		foreach ( $databases as $db ) {
			fputs($handle, date('Y-m-d H:i:s') . '   Stopping instance: ' . $db . "\n");
			$aStdout = array();
			list($ret, $output, $aStdout, $aStderr) = my_exec($PHP . " -d variables_order=EGPCS -f $basedir/bin/database.php $db " . $argv[1] . ' 2>&1');
			if ( $ret != OK ) {
				$rc++;
			}
			if ( count($aStderr) > 0 ) {
				fputs($handle, date('Y-m-d H:i:s') . ' ' . implode("\n", $aStderr) . "\n");
			}
		}
		// Should be done for all
		// TODO: What about SystemD/JournalD?
		fputs($handle, date('Y-m-d H:i:s') . ' Finished ' . $MyBasename . ' ' . $argv[1] . " (rc=$rc)" . "\n");
		fputs($handle, date('Y-m-d H:i:s') . ' ----' . "\n");
	}
break;
default:
	fputs($handle, date('Y-m-d H:i:s') . ' Usage: ' . $MyBasename . ' {start|stop}' . "\n");
}

fclose($handle);
// Always OK otherwise SystemD will tear down all instances again if just one fails starting.
exit(OK);

?>
