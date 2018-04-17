#!/usr/bin/php
<?php

// start, stop, restart a mysql instance

// Caution $_ENV variables are not be known here yet when variables_order
// does NOT contain E!!!
if ( isset($_ENV['MYENV_BASE']) ) {
  $basedir = strval($_ENV['MYENV_BASE']);
}
// We have to guess:
else {
  $basedir = dirname(dirname(__FILE__));
}

require_once($basedir . '/lib/Constants.inc');
require_once($basedir . '/lib/myEnv.inc');

$rc = OK;

if ( checkMyEnvRequirements() == ERR ) {
  $rc = 551;
  exit($rc);
}

$_ENV['MYENV_BASE'] = $basedir;
if ( count($argv) >= 5 ) {
  $rc = 543;
  error("Wrong command. Command must be database <instance_name> {start|stop|restart|status} (rc=$rc).");
  exit($rc);
}

$lInstance = $argv[1];
$lCommand  = $argv[2];
$lOptions  = '';
if ( isset($argv[3]) ) {
  $lOptions  = $argv[3];
}

$lHomeDir  = $_ENV['HOME'];
$lBaseDir  = $_ENV['MYENV_BASE'];
// The next line is possibly a bug!!!
// so let us remove this line and see what happens:
// $lBaseDir  = $lHomeDir . '/myenv';
// $lConfFile = $lBaseDir . '/' . 'etc/' . 'myenv.conf';
$lConfFile = '/etc/myenv/myenv.conf';

list($ret, $aConfiguration) = getConfiguration($lConfFile);

if ( count($aConfiguration) == 0 ) {
	$rc = 529;
	$msg = "Configuration $lConfFile does not exist or is not readable (rc=$rc).";
  error($msg);
  exit($rc);
}

if ( ! array_key_exists($lInstance, $aConfiguration) ) {
  $rc = 544;
  error("Instance $lInstance does NOT exist in your conf (rc=$rc).");
  exit($rc);
}

switch ( $lCommand ) {
case 'start':
  $ret = startDatabase($aConfiguration[$lInstance], $lOptions);
  if ( $ret != 0 ) {
    $rc = 545;
    error("Starting database $lInstance failed (ret=$ret/rc=$rc).");
    error("Please have a look in the MySQL error log or");
    error("try again with export MYENV_DEBUG=1 if you cannot find any reason...");
  }
  break;
case 'bootstrap':
  $lOptions .= ' --wsrep-new-cluster';
  $ret = startDatabase($aConfiguration[$lInstance], $lOptions);
  if ( $ret != 0 ) {
    $rc = 506;
    error("Bootstrapping galera node $lInstance failed (ret=$ret/rc=$rc).");
    error("Please have a look in the MySQL error log or");
    error("try again with export MYENV_DEBUG=1 if you cannot find any reason...");
  }
  break;
case 'stop':
  $ret = stopDatabase($aConfiguration[$lInstance]);
  if ( $ret != 0 ) {
    $rc = 546;
    error("Stopping database $lInstance failed (rc=$rc).");
  }
  break;
case 'status':
  $ret = checkDatabase($aConfiguration[$lInstance]);
  if ( $ret != 0 ) {
    $rc = 547;
    error("Check on database $lInstance failed (rc=$rc).");
  }
  break;
case 'restart':
  $ret = stopDatabase($aConfiguration[$lInstance]);
  if ( $ret != 0 ) {
    $rc = 548;
    error("Stopping database $lInstance failed (rc=$rc).");
  }
  $ret = startDatabase($aConfiguration[$lInstance]);
  if ( $ret != 0 ) {
    $rc = 549;
    error("Starting database $lInstance failed (rc=$rc).");
  }
  break;
default:
  $rc = 550;
  error("Unknown command $lCommand (rc=$rc).");
}

exit($rc);

?>
