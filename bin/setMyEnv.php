#!/usr/bin/php
<?php

ini_set('date.timezone', 'Europe/Zurich');
// ini_set('error_reporting', E_ALL & ~E_DEPRECATED);
// We want to be more radical:
ini_set('error_reporting', E_ALL);

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

$rc = OK;

if ( checkMyEnvRequirements() == ERR ) {
  $rc = 536;
  exit($rc);
}

$_ENV['MYENV_BASE'] = $basedir;
// This is needed for a proper path substitution
if ( isset($_ENV['MYENV_DATABASE']) ) {
  $lOldDatabase = $_ENV['MYENV_DATABASE'];
}
else {
  $lOldDatabase = '';
}

$tmp           = strval($argv[1]);
$pDatabaseName = isset($argv[2]) ? strval($argv[2]) : '';

debug("tmp file: $tmp");
debug("database: $pDatabaseName");

foreach ( array('/etc/my.cnf', '/etc/mysql/my.cnf', '/usr/local/mysql/etc/my.cnf', "~/.my.cnf") as $file ) {
  if ( file_exists($file) ) {
    output("Warning: $file exists. This can screw up myenv. Please remove the file.\n");
  }
}

$lConfigurationFile = '/etc/myenv/myenv.conf';

if ( ! is_file($lConfigurationFile) ) {
  $rc = 537;
  output("Warning: Configuration file $lConfigurationFile does not exist!\n");
  output("         Please create or copy from $lConfigurationFile.template (rc=$rc).\n");
  exit($rc);
}

list($ret, $aConfigurationFile) = getConfiguration($lConfigurationFile);
$aDbNames = array();
foreach ( $aConfigurationFile as $dbname => $paramter ) {
  if ( $dbname != 'default' ) {
    array_push($aDbNames, $dbname);
  }
}
if ( count($aDbNames) == 0 ) {
	$rc = 564;
	error("Configuration file $lConfigurationFile does NOT contain any section be-\n       side [default]. Please configure one section first.\n");
	exit($rc);
}

$lDbName = '';
if ( $pDatabaseName != '' ) {
  $lDbName = $pDatabaseName;
}
elseif ( isset($_ENV['MYENV_DATABASE']) ) {
  $lDbName = $_ENV['MYENV_DATABASE'];
}
// default database name
elseif ( isset($aConfigurationFile['default']['default']) && ($aConfigurationFile['default']['default'] != '') ) {
  $lDbName = $aConfigurationFile['default']['default'];
}
else {
  $lDbName = $aDbNames[0];
}

debug("database: $lDbName");

$_ENV['INFODIR']         = isset($_ENV['INFODIR']) ? $_ENV['INFODIR'] : '';
$_ENV['INFOPATH']        = isset($_ENV['INFOPATH']) ? $_ENV['INFOPATH'] : '';
# $_ENV['MANPATH']         = isset($_ENV['MANPATH']) ? $_ENV['MANPATH'] : '';
$_ENV['LD_LIBRARY_PATH'] = isset($_ENV['LD_LIBRARY_PATH']) ? $_ENV['LD_LIBRARY_PATH'] : '';

// getEnv
if ( $lOldDatabase == '' ) {
  $aOldDatabaseParameter = array('basedir' => '');
}
else {
  // If database was just deleted this array is empty!
  // And would cause some nasty error messages (Bug #104)
  if ( isset($aConfigurationFile[$lOldDatabase]) ) {
    $aOldDatabaseParameter = $aConfigurationFile[$lOldDatabase];
  }
  else {
    // Thus skip and set some default to prevent noise further down!
    $aOldDatabaseParameter['basedir'] = '';
  }
}
// If database was just deleted this array is empty!
// And would cause some nasty error messages (Bug #104)
if ( isset($aConfigurationFile[$lDbName]) ) {
  $aDatabaseParameter    = $aConfigurationFile[$lDbName];
}
// If DB is not set just set the first one
else {
  $aDatabaseParameter    = $aConfigurationFile[$aDbNames[0]];
}

$file = $aDatabaseParameter['basedir'] . '/my.cnf';
if ( file_exists($file) ) {
  output("Warning: $file exists. This can screw up myenv.\n");
}

// Check if my.cnf file is readable for group or others for security reasons
// output("%o\n", fileperms($file));
$lConfFile = $aDatabaseParameter['datadir'] . '/my.cnf';

if ( file_exists($lConfFile) && ((fileperms($lConfFile) & 0x001c) > 0) ) {
  output("Warning: File $lConfFile is writeable for group or readable for others. Please fix with: chmod g-w,o-rw $lConfFile\n");
}

// to avoid a complete mess:
$path = $_ENV['PATH'];
$old = '';
if ( $aDatabaseParameter['basedir'] != '' ) {

	foreach ( array('scripts', 'libexec', 'bin') as $dir ) {

		if ( $aOldDatabaseParameter['basedir'] != '' ) {
			$old = $aOldDatabaseParameter['basedir'] . '/' . $dir;
			$path = deleteDirectoryFromPath($path, $old);
		}

		$new = $aDatabaseParameter['basedir'] . '/'. $dir;
		if ( is_dir($new) ) {
			debug("substitute PATH $old by $new");
			// Clean-up to be sure
			$path = deleteDirectoryFromPath($path, $new);
			$path = addDirectoryToPath($path, $new, '', 'left');
		}
	}
}

foreach ( array('scripts', 'libexec', 'bin') as $dir ) {

	$new = $basedir . '/' . $dir;
	if ( is_dir($new) ) {
		debug("substitute PATH $old by $new");
		$path = addDirectoryToPath($path, $new, '', 'left');
	}
}

// and last myenv/utl path
$path = addDirectoryToPath($path, "$basedir/utl", '', 'right');

$fh = fopen($tmp, 'w');
if ( $fh === false ) {
  $rc = 538;
  error("Cannot open file $tmp");
  exit($rc);
}

// Add generic variables here
// todo: is redundant with variables.conf.template check why and if needed?

fwrite($fh, "export PATH=$path\n");
fwrite($fh, "export INFODIR=" . addDirectoryToPath($_ENV['INFODIR'], $aDatabaseParameter['basedir'] . "/docs", $aDatabaseParameter['basedir'] . "/docs") . "\n");
fwrite($fh, "export INFOPATH=" . addDirectoryToPath($_ENV['INFOPATH'], $aDatabaseParameter['basedir'] . "/docs", $aDatabaseParameter['basedir'] . "/docs") . "\n");
fwrite($fh, "export LD_LIBRARY_PATH=" . addDirectoryToPath($_ENV['LD_LIBRARY_PATH'], $aDatabaseParameter['basedir'] . "/lib", $aDatabaseParameter['basedir'] . "/lib") . "\n");
fwrite($fh, "export LD_LIBRARY_PATH=" . addDirectoryToPath($_ENV['LD_LIBRARY_PATH'], $aDatabaseParameter['basedir'] . "/lib/mysql", $aDatabaseParameter['basedir'] . "/lib/mysql") . "\n");
fwrite($fh, "export MYSQL_HOME=" . $aDatabaseParameter['datadir'] . "\n");
fwrite($fh, "export MYSQL_TCP_PORT=" . $aDatabaseParameter['port'] . "\n");
fwrite($fh, "export MYSQL_UNIX_PORT=" . $aDatabaseParameter['socket'] . "\n");
fwrite($fh, "export MYSQL_PS1='\u@" . $lDbName . " [\d] SQL> '\n");
fwrite($fh, "export MYENV_DATABASE=" . $lDbName . "\n");
fwrite($fh, "export MYENV_DATADIR=" . $aDatabaseParameter['datadir'] . "\n");
fwrite($fh, "export MYENV_VERSION=" . '1.3.0' . "\n");
fwrite($fh, "export MYENV_STAGE=" . (isset($aDatabaseParameter['stage']) ? $aDatabaseParameter['stage'] : 'none') . "\n");
fwrite($fh, "time_off\n");

// Add user export variables here

$conf = '/etc/myenv/variables.conf';
if ( is_readable($conf) ) {

	$aLines = file($conf, FILE_IGNORE_NEW_LINES | FILE_SKIP_EMPTY_LINES);

	foreach ( $aLines as $line) {

		$line = trim($line);
		// output($line . "\n");

		// Skip commented lines
		if ( preg_match('/^\s*#/', $line) ) {
			continue;
		}

		$code = 'fwrite($fh, "' . $line . '\n");';
		debug("code: $code");

		eval($code);
	}
}
else {
	// We do not care if file is not there
}

// Generic alias

foreach ( $aDbNames as $dbname ) {

	$alias = "alias $dbname='setMyEnv $dbname'\n";
	debug("alias: $alias");
	fwrite($fh, $alias);
}


// Default myenv aliases

fwrite($fh, "alias cdh='cd " . $aDatabaseParameter['basedir'] . "'\n");
fwrite($fh, "alias cdb='cd " . $aDatabaseParameter['basedir'] . "'\n");
fwrite($fh, "alias cdd='cd " . $aDatabaseParameter['datadir'] . "'\n");
fwrite($fh, "alias cde='cd " . $basedir . "'\n");
fwrite($fh, "alias v='echo \$MYENV_VERSION - alias v is deprecated! Please use V instead!'\n");
fwrite($fh, "alias V='echo \$MYENV_VERSION'\n");
fwrite($fh, "alias cdl='cd " . $basedir . '/log' . "'\n");

fwrite($fh, "alias ll='ls -l'\n");
fwrite($fh, "alias la='ls -la'\n");

// RedHat style is not a bad idea!
fwrite($fh, "alias rm='rm -i'\n");
fwrite($fh, "alias mv='mv -i'\n");

// Add User alias here

$conf = '/etc/myenv/aliases.conf';
if ( is_readable($conf) ) {

	$aLines = file($conf, FILE_IGNORE_NEW_LINES | FILE_SKIP_EMPTY_LINES);

	foreach ( $aLines as $line) {

		$line = trim($line);
		// output($line . "\n");

		// Skip commented lines
		if ( preg_match('/^\s*#/', $line) ) {
			continue;
		}

		$code = 'fwrite($fh, "' . $line . '\n");';
		debug("code: $code");

		eval($code);
	}
}
else {
	// We do not care if file is not there
}

fclose($fh);

exit($rc);

?>
