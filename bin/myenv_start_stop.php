#!/usr/bin/php
<?php

// todo: should not be hard coded but configurable!
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

require_once($basedir . '/lib/myEnv.inc');
require_once($basedir . '/lib/myEnv.inc');

$rc = OK;

if ( checkMyEnvRequirements() == ERR ) {
  $rc = 540;
  exit($rc);
}

$PHP        = "/usr/bin/php";
$MyBasename = basename(__FILE__, '.php');

debug("basename = $MyBasename\n");

if ( $basedir == '' ) {
  output("Basedir is not set.\n");
  $rc = 541;
  exit($rc);
}

debug("MYENV_BASE = $basedir\n");

$LogFile    = "$basedir/log/$MyBasename.log";
$handle = fopen($LogFile, 'a');
if ( ! $handle ) {
  output("Cannot log to file $LogFile\n");
  $rc = 542;
  exit($rc);
}

switch ( $argv[1] ) {
case 'start':
  fputs($handle, date("Y-m-d H:i:s") . " Starting $MyBasename\n");
  // find all databases
  list($ret, $databases) = printStartingDatabases();
  if ( $ret != OK ) {
		exit($ret);
  }
  if ( count($databases) == 0 ) {
    fputs($handle, date("Y-m-d H:i:s") . " No databases to start.\n");
  }
  else {
    fputs($handle, date("Y-m-d H:i:s") . " Starting databases: " . implode(' ', $databases) . "\n");

    foreach ( $databases as $db ) {
      fputs($handle, date("Y-m-d H:i:s") . " Starting database: $db\n");
      $stdout = array();
      list($rc, $output, $stdout, $errout) = my_exec("$PHP -f $basedir/bin/database.php $db " . $argv[1] . " 2>&1");
      fputs($handle, date("Y-m-d H:i:s") . ' ' . implode("\n", $stdout) . "\n");
    }
    // Should be done for all
    fputs($handle, date("Y-m-d H:i:s") . " Finished $MyBasename (rc=$rc).\n");
  }
break;
case 'stop':

  fputs($handle, date("Y-m-d H:i:s") . " Stopping $MyBasename\n");
  // find all started databases and stop them (better to stop all started db,
  // that are still running or even all running dbs?)
  list($ret, $databases) = printStartingDatabases();
  if ( $ret != OK ) {
		exit($ret);
  }
  if ( count($databases) == 0 ) {
    fputs($handle, date("Y-m-d H:i:s") . " No databases to start.\n");
  }
  else {
    fputs($handle, date("Y-m-d H:i:s") . " Stopping databases: " . implode(' ' , $databases) . "\n");
    foreach ( $databases as $db ) {
      fputs($handle, date("Y-m-d H:i:s") . " Stopping database: $db\n");
      $stdout = array();
      list($rc, $output, $stdout, $stderr) = my_exec("$PHP -f $basedir/bin/database.php $db " . $argv[1] . " 2>&1");
      fputs($handle, date("Y-m-d H:i:s") . ' ' . implode("\n", $stdout) . "\n");
    }
    // Should be done for all
    fputs($handle, date("Y-m-d H:i:s") . " Finished $MyBasename (rc=$rc).\n");
  }
break;
default:
  fputs($handle, date("Y-m-d H:i:s") . " Usage: $MyBasename {start|stop}\n");
}

fclose($handle);
exit($rc);

?>
