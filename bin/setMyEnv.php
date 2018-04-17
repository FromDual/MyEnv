#!/usr/bin/php -d variables_order=EGPCS
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

if ( ! array_key_exists("instancedir", $aDatabaseParameter) ) {
	$aDatabaseParameter['instancedir'] = $aDatabaseParameter['basedir'];
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
fwrite($fh, "export MYENV_VERSION=" . '2.0.0' . "\n");
fwrite($fh, "export MYENV_STAGE=" . (isset($aDatabaseParameter['stage']) ? $aDatabaseParameter['stage'] : 'none') . "\n");
fwrite($fh, "time_off\n");


// Search/Replace patterns for variables.conf, my_variables.conf, aliases.conf and my_aliases.conf

$aVarSearch = array('/%MYENV_BASEDIR%/', '/%INSTANCEDIR%/', '/%DATADIR%/', '/%MYSQL_BASEDIR%/', '/%INSTANCE_NAME%/');
$aVarReplace = array($basedir, $aDatabaseParameter['instancedir'], $aDatabaseParameter['datadir'], $aDatabaseParameter['basedir'], $lDbName);


// Add user export variables here

$conf = '/etc/myenv/variables.conf';
if ( is_readable($conf) ) {

	$aLines = file($conf, FILE_IGNORE_NEW_LINES | FILE_SKIP_EMPTY_LINES);

	foreach ( $aLines as $line) {

		$line = trim($line);
		// debug($line);

		// Skip commented lines
		if ( preg_match('/^\s*#/', $line) ) {
			continue;
		}

		// Check for old style variables.conf
		if ( preg_match("/addDirectoryToPath/", $line) == 1 ) {
			$rc = 510;
			$msg = 'Old style variables.conf is used. Please upgrade from tpl/variables.conf.template as follows: ';
			error($msg);
			$msg = 'shell> sudo cp ' . $basedir . '/tpl/variables.conf.template /etc/myenv/variables.conf' . "\n";
			error($msg);
		}

		$line = preg_replace($aVarSearch, $aVarReplace, $line);
		debug($line);
		fwrite($fh, $line . "\n");
	}
}
else {
	// We do not care if file is not there
}


$conf = '/etc/myenv/my_variables.conf';
if ( is_readable($conf) ) {

	$aLines = file($conf, FILE_IGNORE_NEW_LINES | FILE_SKIP_EMPTY_LINES);

	foreach ( $aLines as $line) {

		$line = trim($line);
		// debug($line);

		// Skip commented lines
		if ( preg_match('/^\s*#/', $line) ) {
			continue;
		}

		// Check for old style my_variables.conf
		if ( preg_match("/addDirectoryToPath/", $line) == 1 ) {
			$rc = 511;
			$msg = 'Old style my_variables.conf is used. Please remove old style PHP stuff in there (for example addDirectoryToPath).';
			error($msg);
		}

		$line = preg_replace($aVarSearch, $aVarReplace, $line);
		debug($line);
		fwrite($fh, $line . "\n");
	}
}
else {
	// We do not care if file is not there
}


// Generic aliases

foreach ( $aDbNames as $dbname ) {

	$alias = "alias $dbname='setMyEnv $dbname'";
	debug('alias: ' . $alias);
	fwrite($fh, "$alias\n");
}


// Default myenv aliases

fwrite($fh, "alias v='echo \$MYENV_VERSION - alias v is deprecated! Please use V instead!'" . "\n");
fwrite($fh, "alias V='" . $basedir . '/bin/showMyEnvVersion.php' .  "'" . "\n");

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
		// debug($line);

		// Skip commented lines
		if ( preg_match('/^\s*#/', $line) ) {
			continue;
		}

		// Check for old style aliases.conf
		if ( preg_match("/DatabaseParameter/", $line) == 1 ) {
			$rc = 509;
			$msg = 'Old style aliases.conf is used. Please upgrade from tpl/aliases.conf.template as follows:';
			error($msg);
			$msg = 'shell> sudo cp ' . $basedir . '/tpl/aliases.conf.template /etc/myenv/aliases.conf' . "\n";
			error($msg);
		}

		$line = preg_replace($aVarSearch, $aVarReplace, $line);
		debug($line);
		fwrite($fh, $line . "\n");
	}
}
else {
	// We do not care if file is not there
}


$conf = '/etc/myenv/my_aliases.conf';
if ( is_readable($conf) ) {

	$aLines = file($conf, FILE_IGNORE_NEW_LINES | FILE_SKIP_EMPTY_LINES);

	foreach ( $aLines as $line) {

		$line = trim($line);
		// debug($line);

		// Skip commented lines
		if ( preg_match('/^\s*#/', $line) ) {
			continue;
		}

		// Check for old style my_aliases.conf
		if ( preg_match("/DatabaseParameter/", $line) == 1 ) {
			$rc = 512;
			$msg = 'Old style my_aliases.conf is used. Please remove old style PHP stuff in there (for example DatabaseParameter).';
			error($msg);
		}

		$line = preg_replace($aVarSearch, $aVarReplace, $line);
		debug($line);
		fwrite($fh, $line . "\n");
	}
}
else {
	// We do not care if file is not there
}

fclose($fh);

// This is to make rc clear because bash truncates to 1st byte.
if ( $rc != OK ) {
	$msg = "rc=$rc";
	error($msg);
}
exit($rc);

?>
