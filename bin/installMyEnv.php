#!/usr/bin/php -d variables_order=EGPCS
<?php

date_default_timezone_set('Europe/Zurich');

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
require_once($basedir . '/lib/Constants.inc');
require_once($basedir . '/lib/installMyEnv.inc');
require_once($basedir . '/lib/installer.inc');

$rc = OK;

if ( checkMyEnvRequirements() == ERR ) {
	$rc = 700;
	exit($rc);
}

$_ENV['MYENV_BASE'] = $basedir;
$lHomeDir = $_ENV['HOME'];

$lConfFile         = '/etc/myenv/myenv.conf';
$lConfFileTemplate = $basedir . '/' . 'tpl/' . 'myenv.conf' . '.template';

// ---------------------------------------------------------------------
// Start here
// ---------------------------------------------------------------------

// var_dump($argv);

$shortopts  = "";

$longopts  = array(
  'help'
, 'operation:'
, 'instance:'
, 'purge-data'
, 'port:'
, 'socket:'
, 'basedir:'
, 'datadir:'
, 'instancedir:'
, 'user:'
, 'my.cnf:'
, 'angel:'
, 'cgroups:'
, 'stage:'
);

$aOptions = getopt($shortopts, $longopts);

if ( isset($aOptions['help']) ) {
	printUsage();
	exit($rc);
}

// Automatized
if ( count($argv) > 1 ) {
	$rc = checkOptions($argv, $aOptions, $lConfFile);
	if ( $rc == OK ) {
		switch ( $aOptions['operation'] ) {
		case 'delete':
			$rc = deleteInstanceAutomatized($aOptions, $lConfFile);
			break;
		case 'add':
			$rc = addInstanceAutomatized($aOptions, $lConfFile);
			break;
		default:
			$rc = 729;
		}
	}
	else {
		printUsage();
	}
	exit($rc);
}
// Interactive
else {
	// Just continue
	null;
}

// Initial empty line for better readability
output("\n");

debug("basedir=$basedir");
debug("ConfFile: $lConfFile\n");


// Check if installer is started as root user
$lCurrentUser = getCurrentUser();
$lMyenvUser = checkForRootUser($lCurrentUser);

// Check if MyEnv user exists
checkOsUser($lMyenvUser);


// Check if /etc/myenv exists and if it belongs to user mysql

$lEtcMyenv = '/etc/myenv';
checkMyEnvDir($lEtcMyenv, $lMyenvUser);
checkHomeDirOwner($lEtcMyenv, $lMyenvUser);


// Check if myenv.conf exists
// Use this? Overwrite with template? Abort?
if ( file_exists($lConfFile) ) {
	useOrCopyConfFile($lConfFile, $lConfFileTemplate);
}
// myenv.conf does not exits
// Copy from template?
else {
	copyConfFileFromTemplate($lConfFile, $lConfFileTemplate);
}

// Get configuration and display for selection
list($ret, $aConfiguration) = getConfiguration($lConfFile);

// Loop as long as customer wants some changes
$lConfigurationChanged = false;
do {

	$aInstances = getInstances($aConfiguration);

	if ( count($aInstances) > 0 ) {
		output("The following instances are available:\n\n");
		// TODO: Do output nicer here!
		foreach ($aInstances as $instance) {
			output($instance . ' ');
		}
		output("\n");
	}
	else {
		output("\nNo instance exists yet.\n");
	}

	output("\n" . 'An instance is the same as a mariadbd/mysqld process.' . "\n\n");

	$question = "What do you want to do next?
o Add a new instance,
o change an existing instance,
o delete an existing instance,
o save configuration and exit or
o quit without saving";

	if ( $lConfigurationChanged === false ) {
		$question .= "\n\n(A/c/d/s/q)? ";
		$default = 'a';
	}
	else {
		$question .= "\n\n(a/c/d/S/q)? ";
		$default = 's';
	}

	$key = answerQuestion($question, array('a', 'c', 'd', 's', 'q'), $default);
	output("\n");

	switch ( $key ) {
	case 'a':
		$rc = addInstance();
		if ( $rc != OK ) {
			$q = 'Have you seen the error message? [Y] ';
			$k = answerQuestion($q, array('y'), 'y');
			// $default = 'q';
			output("\n");
		}
		$lConfigurationChanged = true;
		break;
	case 'c':
		$rc = changeInstance2($aInstances);
		if ( $rc != OK ) {
			$q = 'Have you seen the error message? [Y] ';
			$k = answerQuestion($q, array('y'), 'y');
			// $default = 'q';
			output("\n");
		}
		$lConfigurationChanged = true;
		break;
	case 'd':
		deleteInstance2($aInstances);
		$lConfigurationChanged = true;
		break;
	case 's':
	case 'q':
		// do nothing
		break;
	case 'i':
		// not implemented yet
		$rc = 719;
		error("Not implemented yet (rc=$rc, key=$key)!");
		break;
	default:
		$rc = 713;
		error("Fatal error. Please report this (rc=$rc, key=$key)!");
		exit($rc);
	}
} while ( ($key != 'q') && ($key != 's') );

// Abort without safing...
if ( $key == 'q' ) {
	// ask if we are really sure!
	// but only if config has chnaged...
	$rc = 720;
	output("Aborting... (rc=$rc)\n");
	exit($rc);
}
// Abort with saving
elseif  ( $key == 's' ) {
	writeConfigurationFile($aConfiguration, $lConfFile);
}
else {
	$rc = 715;
	error("Fatal error. Please report this (rc=$rc, key=$key)!");
	exit($rc);
}


// Check if user has a HOME directory at all
checkHomeDir($lHomeDir, $lMyenvUser);


// Check if we are allowed to write to HOME directory
createHomeDir($lHomeDir);


// Adding myenv.profile to interactive shell startup file ~/.bash_profile
addMyEnvProfile($lHomeDir);


// Add MyEnv hook
$MYENV_HOOK = '/etc/myenv/MYENV_BASE';
addMyEnvHook($MYENV_HOOK);


// Add MyEnv init script
// TODO: PHP 8 is not supported yet in Debian 10 and 11
// addMyEnvInitScript(pKey: '', pMyenvUser: $lMyenvUser);
addMyEnvInitScript('', $lMyenvUser);

// Make sure log file belongs to user otherwise it will belong to root
// after first restart and then we cannot log any more...

$lLogFile = $basedir . '/log/myenv_start_stop.log';
touch($lLogFile);


// Make sure /etc/init.d/{mysql|mysqld|mysqld_multi} is replaced

replaceInitScripts();


// Check if systemd services are enabled which would cause conflicts

require_once($basedir . '/lib/myEnv.inc');


$cmd = 'systemctl is-enabled mariadb mysql mysqld';
list($rc, $output, $aStdout, $aStderr) = my_exec($cmd);

if ( in_array('enabled', $aStdout) ) {
	$rc = 784;
	$msg = 'One of the SystemD services mariadb, mysql or mysqld is enabled. This can cause conflicts with myenv.' . " (rc=$rc)";
	warn($msg);
}


// Final comment

output("\n");
output("Now source your profile as follows:" . "\n");
output("shell> source ~/.bash_profile" . "\n");
output("\n");
output("The README gives some hints how to continue..." . "\n");
output("\n");

exit($rc);

?>
