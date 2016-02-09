<?php

/*

	debug($s)
	error($s)
	getConfiguration($pConfigurationFile)
	extractVersion($s)
	startDatabase($aInstance, $pOptions = '')
	checkDatabase($aInstance)
	wait_for_pid($lMysqldPid, $pidfile)
	stopDatabase($aInstance)
	getSchemaNames($pDatadir)
	addDirectoryToPath($lPath, $lDir, $lOldDir, $location = 'left')
	deleteDirectoryFromPath($lPath, $lDir)
	getSectionTitles($a)
	printStartingDatabases()
	getCurrentUser()
	checkMyEnvRequirements()
	parseConfigFile($pConfigurationFile, $scanner_mode = INI_SCANNER_NORMAL)
	getMd5sum2($file, $aOptions)
	checkForOldStuff()
	my_exec($pCmd)
	getVersionFromMysqld($pBasedir)
	getOs()
	getDistribution()
	simulateLsbRelease()
	getPacketList($pDistribution)
	noop()
	parseConnectString($pConnectString)

*/

require_once('Constants.inc');

// ----------------------------------------------------------------------
function debug($s)
// ----------------------------------------------------------------------
{
	if ( isset($_ENV['MYENV_DEBUG']) && ($_ENV['MYENV_DEBUG'] != '') ) {
		echo 'MYENV_DEBUG: ' . $s . "\n";
	}
}

// ----------------------------------------------------------------------
function error($s)
// ----------------------------------------------------------------------
{
	fprintf(STDERR, "ERROR: %s\n", $s);
}

// ----------------------------------------------------------------------
function output($s)
// ----------------------------------------------------------------------
{
	print $s;
}

// ----------------------------------------------------------------------
function getConfiguration($pConfigurationFile)
// ----------------------------------------------------------------------
{
	$rc = OK;

	$aConfiguration = array();

	if ( ! file_exists($pConfigurationFile) ) {
		$rc = 521;
		$msg = "Configuration file $pConfigurationFile does not exist (rc=$rc).";
		debug($msg);
		return array($rc, $msg);
	}

	list($ret, $aConfiguration) = parseConfigFile($pConfigurationFile, INI_SCANNER_RAW);
	if ( $ret != OK ) {
		$rc = 502;
		$msg = "In file $pConfigurationFile: $aConfiguration (rc=$rc).";
		// Do not print here, just return
		// because if floods screen when non privileged user does up command
		return array($rc, $msg);
	}
	if ( count($aConfiguration) == 0 ) {
		$rc = 503;
		$msg = 'Empty configuration file: ' . $pConfigurationFile;
		error("$msg (rc=$rc).");
		return array($rc, $msg);
	}

	// Process default section first
	$lInstance = 'default';

	// Define default behaviour here

	if ( ! isset($aConfiguration[$lInstance]['angel']) ) {
		$aConfiguration[$lInstance]['angel'] = 'yes';
	}

	if ( ! isset($aConfiguration[$lInstance]['user']) ) {
		$aConfiguration[$lInstance]['user'] = 'mysql';
	}

	// Process all other sections != default
	foreach ( $aConfiguration as $db => $value ) {

		// add section name to section
		$aConfiguration[$db]['name'] = $db;

		if ( $db != 'default' ) {

			// Inherit from default
			foreach ( $aConfiguration['default'] as $key => $val ) {
				if ( ! isset($aConfiguration[$db][$key]) ) {
					$aConfiguration[$db][$key] = $val;
				}
			}
		}
	}

	return array($rc, $aConfiguration);
}

// ----------------------------------------------------------------------
function extractVersion($s)
// $s = basedir
// ----------------------------------------------------------------------
{
	$version = 'unknown';

	// Expected patterns: see tst/extractVersion.php
	if ( preg_match("/([a-zA-Z]+)\d?\-/", $s, $matches) ) {

		if ( ($matches[1] == 'mysql')
			|| ($matches[1] == 'MySQL')
			|| ($matches[1] == 'percona')
			|| ($matches[1] == 'mariadb')
				) {

			if ( preg_match("/(\d{1,2}\.\d{1,2}\.\d{1,2})/", $s, $matches) ) {
				$version = $matches[1];
			}
			elseif ( preg_match("/(\d{1,2}\.\d{1,2}\.\d{1,2}\-ndb\-\d{1,2}\.\d{1,2}\.\d{1,2}[a-z]?)/", $s, $matches) ) {
				$version = $matches[1];
			}
			elseif ( preg_match("/(\d{1,2}\.\d{1,2})/", $s, $matches) ) {
				$version = $matches[1];
			}
			else {
			}
		}
		elseif ( ($matches[1] == 'Percona') 
					|| ($matches[1] == 'Percona-Server')
					) {
			if ( preg_match("/(\d{1,2}\.\d{1,2}\.\d{1,2})/", $s, $matches) ) {
				$version = $matches[1];
			}
		}
		else {
			// print '1';
		}
	}
	else {
		// Case: 5.6.12
		if ( preg_match("/(\d{1,2}\.\d{1,2}\.\d{1,2})/", $s, $matches) ) {
			$version = $matches[1];
		}
	}

	return $version;
}

// ----------------------------------------------------------------------
function startDatabase($aInstance, $pOptions = '')
// ----------------------------------------------------------------------
{
	$rc = OK;
	global $lDebug;

	// Check if db is started with correct user
	// Also root should be allowed to start DB (Bug #99)

	$lCurrentUser = getCurrentUser();
	if ( ($lCurrentUser != $aInstance['user'])
		&& ($lCurrentUser != 'root')
	   ) {
		$rc = 522;
		error("Database is started with the wrong user ($lCurrentUser) but should should be strated with " . $aInstance['user'] . " (rc=$rc).");
		return $rc;
	}


	// Check if mysql.user table exists

	if ( ! file_exists($aInstance['datadir'] . '/mysql/user.frm') ) {
		$rc = 523;
		error("MySQL user table (user.frm) does not exist. Are you sure you have installed a MySQL instance already? (rc=$rc)");
		error("Create a MySQL instance with the command: mysql_install_db --datadir=" . $aInstance['datadir'] . " --basedir=" . $aInstance['basedir']);
		return $rc;
	}

	list($ret, $aMyCnf) = parseConfigFile($aInstance['my.cnf'], INI_SCANNER_RAW);


	// Check if there is any missmatch between my.cnf and myenv.conf

	foreach ( $aInstance as $key => $value ) {
		if ( isset($aMyCnf['mysqld'][$key]) ) {
			if ( $aMyCnf['mysqld'][$key] != $value ) {
				$rc = 569;
				$cnf = '/etc/myenv/myenv.conf';
				print "WARNING: $key differs between " . $aInstance['my.cnf'] . " and " . $cnf . '(' . $aMyCnf['mysqld'][$key] . ' vs. ' . $value . ") (rc=$rc). Please fix this to avoid troubles...\n";
			}
		}
	}


	// It can happen, that client section is missing in my.cnf

	if ( isset($aMyCnf['client']) ) {

		// Check also missmatches between mysqld and client section in my.cnf
		foreach ( $aMyCnf['client'] as $key => $value ) {
			if ( isset($aMyCnf['mysqld'][$key]) ) {
				if ( $aMyCnf['mysqld'][$key] != $value ) {
					$rc = 570;
					print "WARNING: $key differs between mysqld and client section (" . $aMyCnf['mysqld'][$key] . " vs. " . $value . ") in my.cnf (rc=$rc). Please fix this to avoid troubles...\n";
				}
			}
		}
	}


	// Check if the database is started already

	$ret = checkDatabase($aInstance);
	if ( $ret == 0 ) {
		$rc = 533;
		error("Database is alreay started (rc=$rc).");
		return $rc;
	}


	// Load cgroups if required

	$cgexec = '';
	if ( isset($aInstance['cgroups']) && ($aInstance['cgroups'] == 'yes') ) {

		$lCgroupsMainGroup = 'MyEnv';
		debug('DEBUG: Activating cgroups for main group ' . $lCgroupsMainGroup);

		// Check if main group MyEnv exists
		$aDir = glob('/sys/fs/cgroup/*/' . $lCgroupsMainGroup);
		if ( count($aDir) == 0 ) {
			$rc = 501;
			$msg = "Cgroups main group $lCgroupsMainGroup does not exist. (rc=$rc)";
			error($msg);
			return $rc;
		}
		else {
			$msg = "Cgroups main group $lCgroupsMainGroup exists.";
			debug($msg);
		}

		$lCgroupsFile = $aInstance['datadir'] . '/' . 'cgroups.conf';
		$aCgroupsParameter = array();
		if ( is_file($lCgroupsFile) && (is_readable($lCgroupsFile)) ) {

			$fh = fopen($lCgroupsFile, 'r');
			if ( ! $fh ) {
				$rc = 566;
				error("Cannot open file $lCgroupsFile (rc=$rc).");
				// No exit here
			}
			else {
				while ( ($buffer = fgets($fh, 256)) !== false ) {

					// Skip empty lines
					if ( preg_match('/^\s*$/', $buffer) ) {
						continue;
					}
					// Skip comment lines
					if ( preg_match('/^\s*#/', $buffer) ) {
						continue;
					}
					// Cgroups parameter
					if ( preg_match('/(.+)=(.+)/', $buffer, $matches) ) {
						$key = trim($matches[1]);
						$value = trim($matches[2]);
						$aCgroupsParameter[$key] = $value;
						continue;
					}
					error("Cannot interprete cgroups parameter: $buffer");
				}
				if ( ! feof($fh) ) {
					$rc = 567;
					error("Unexpected fgets() fail (rc=$rc).");
					// No exit here
				}
				fclose($fh);
			}

			foreach ( $aCgroupsParameter as $key => $value ) {
				$cmd = "cgset -r $key=$value $lCgroupsMainGroup" . '/' . $aInstance['name'];
				debug("DEBUG: cmd=$cmd");
				list($rc, $output, $stdout, $stderr) = my_exec($cmd);
			}

			// Check if sub group MyEnv/instance exists

			$aDir = glob('/sys/fs/cgroup/*/' . $lCgroupsMainGroup . '/' . $aInstance['name']);
			if ( count($aDir) == 0 ) {
				$rc = 505;
				$msg = "Cgroups sub group $lCgroupsMainGroup/" . $aInstance['name'] . " does not exist. (rc=$rc)";
				error($msg);
				return $rc;
			}
			else {
				$msg = "Cgroups sub group $lCgroupsMainGroup/" . $aInstance['name'] . " exists.";
				debug($msg);
			}

			// All controllers in control group MyEnv/instance
			$cgexec = 'cgexec -g blkio,cpu,memory:/' . $lCgroupsMainGroup . '/' . $aInstance['name'];
		}
		else {
			$rc = 565;
			error("Local cgroups file $lCgroupsFile does not exist or is not readable (rc=$rc).");
		}
	}


	$cmd = '';
	$cmd .= 'cd ' . $aInstance['basedir'] . " ; ";

	$err = '/tmp' . '/' . posix_getpid() . '.err';

	// Start mysqld with angel process
	if ( $aInstance['angel'] == 'yes' ) {

		$cmd .= "nohup $cgexec bin/mysqld_safe --defaults-file=" . $aInstance['my.cnf'] . " --datadir=" . $aInstance['datadir'] . " --basedir=" . $aInstance['basedir'] . " " . $pOptions;

		if ( $lDebug != '' ) {
			$cmd .= ' & echo $!';
		}
		else {
			$cmd .= ' >' . $err . ' 2>&1 & echo $!';
		}
	}
	// Start mysqld WITHOUT angel process
	else {
		// Should be better redirected to the error log!

		$cmd .= $cgexec . ' ' . $aInstance['basedir'] . '/bin/mysqld --defaults-file=' . $aInstance['my.cnf'] . ' --basedir=' . $aInstance['basedir'] . ' --datadir=' . $aInstance['datadir'] . ' --user=' . $aInstance['user']. ' ' . $pOptions;

		if ( $lDebug != '' ) {
			$cmd .= ' & echo $!';
		}
		else {
			$cmd .= ' >' . $err . ' 2>&1 & echo $!';
		}
	}

	debug($cmd);
	list($rc, $output, $stdout, $stderr) = my_exec($cmd);
	// No return code here!
	// if ( $ret != 0 ) {
	//   error("Command $cmd failed.");
	//   $rc = 572;
	//   // no return here
	// }

	// it takes some time until $err is filled
	sleep(1);

	// Different variants of error we get
	// Filter out start and end line

	// 150130 16:42:01 mysqld_safe Logging to '/home/mysql/data/mariadb-10.0.16/error.log'.
	// mkdir: cannot create directory ‘/var/run/mysqld’: Permission denied
	// Fatal error Can't create database directory '/var/run/mysqld/mysql-10016.sock'

	// 150130 16:44:19 mysqld_safe Logging to '/home/mysql/data/mariadb-10.0.16/error.log'.
	// 150130 16:44:19 mysqld_safe Starting mysqld daemon with databases from /home/mysql/data/mariadb-10.0.16

	foreach ( explode("\n", trim(file_get_contents($err))) as $line ) {

		if ( (strpos($line, 'mysqld_safe Logging to') === false)
		  && (strpos($line, 'mysqld_safe Starting mysqld daemon') === false)
		   ) {
			array_push($stderr, $line . "\n");
		}
	}
	$keep_stderr = $stderr;
	if ( file_exists($err) ) {
		unlink($err);
	}

	$lDebugOrig = $lDebug;
	$lDebug = '';
	$timeout = 60;
	for ( $i = 1; $i <= $timeout ; $i++ ) {

		sleep(1);
		echo ".";
		$ret = checkDatabase($aInstance);
		$rc = $ret;
		if ( $ret == 0 ) {
			break;
		}
	}
	$lDebug = $lDebugOrig;

	if ( $rc == OK ) {
		print " SUCCESS!\n";
		return($rc);
	}
	else {
		$rc = 518;
		error("(rc=$rc).");
		if ( count($keep_stderr) > 0 ) {
			error("Command $cmd failed.");
			foreach ( $keep_stderr as $line ) {
				error("  " . trim($line));
			}
		}
		return($rc);
	}
}

// ----------------------------------------------------------------------
function checkDatabase($aInstance)
// ----------------------------------------------------------------------
{
	$rc = OK;

	$lTimeout = 3;

	// Sometimes it can happen, that mysqladmin is missing (Galera)
	// In some situations (sudo) the origin user (oli->mysql) is used giving some
	// warnings in the mysql error log when log_warnings is set to 2.
	// [Warning] Access denied for user 'oli'@'localhost' (using password: NO)
	// todo: This seems to be not critical but has to be fixed later!

	if ( ! isset($aInstance['basedir']) ) {
		$rc = 735;
		$msg = "basedir is not set. Please check configuration for instance.";
		error($msg);
		return $rc;
	}

	$exe = $aInstance['basedir'] . '/bin/mysqladmin';
	list($ret, $output, $stdout, $stderr) = my_exec("which $exe 2>&1");
	if ( $ret != OK ) {
		$rc = 524;
		$msg = "$exe does no exist (rc=$rc).";
		print "$msg\n";
		return $rc;
	}

	// todo: This strategy is not good because it requires access to
	// defaults-file which is not given to non privileged users
	// but for just ping we do not really need defaults-file?
	// Timeout is for hanging mysqld (happend with Galera Bug #56)
	$cmd = "$exe --socket=" . $aInstance['socket'] . " ping --connect-timeout=$lTimeout";

	debug($cmd);
	list($ret, $output, $stdout, $stderr) = my_exec($cmd);

	if ( $ret != OK ) {
		$rc = 525;
		if ( $ret != 1 ) {
			foreach ( $stderr as $line ) {
			  error($line);
			}
		}
		// no return here
	}

	return $rc;
}

// ----------------------------------------------------------------------
function wait_for_pid($lMysqldPid, $pidfile)
// ----------------------------------------------------------------------
{
	$rc = OK;

	$i = 0;
	$service_shutdown_timeout = 900;   // 15 min

	while ( $i <= $service_shutdown_timeout ) {

		sleep(1);
		print '.';
		// Process seems dead
		if ( posix_kill($lMysqldPid, SIG_IGN) === false ) {
			break;
		}

		$i++;
	}

	if ( file_exists($pidfile) ) {
		$rc = 526;
		print "\nThe server quit without removing PID file ($pidfile) (rc=$rc).\n";
	}

	if ( $rc == OK ) {
		print " SUCCESS!\n";
		return($rc);
	}
	else {
		error(" (rc=$rc).");
		return($rc);
	}
}

// ----------------------------------------------------------------------
function stopDatabase($aInstance)
// ----------------------------------------------------------------------
{
	$rc = OK;

	$lDebug         = isset($_ENV['MYENV_DEBUG']) ? intval($_ENV['MYENV_DEBUG']) : 0;
	$basedir        = $_ENV['MYENV_BASE'];
	$user           = 'root';

	// We need defaults file here to support root password in client
	// section for shutdown service
	// $cmd = $aInstance['basedir'] . "/bin/mysqladmin --defaults-file=" . $aInstance['my.cnf'] . " --user=$user shutdown --socket=" . $aInstance['socket'];

	// debug($cmd);
	// list($ret, $output, $stdout, $stderr) = my_exec($cmd);
	// if ( $ret != 0 ) {
	//   $rc = 573;
	//   error("Command $cmd failed (rc=$rc).");
	//   error("Please use password in ~/.my.cnf.");
	//   // no return here
	// }


	// Find the PID file

	$lMysqldPidFile = '';
	// We do not look into the database because this requires some account
	// which we do not have yet...

	// Check if my.cnf is set. This might be not the case if a dummy instance is
	// defined.
	if ( ! isset($aInstance['my.cnf']) ) {
		$rc = 568;
		error("my.cnf is not defined in myenv.conf. Are you running a dummy instance? (rc=$rc)");
		return $rc;
	}

	list($ret, $aMyCnf) = parseConfigFile($aInstance['my.cnf'], INI_SCANNER_RAW);
	// print_r($aMyCnf);
	// Use PID file from my.cnf
	if ( isset($aMyCnf['mysqld']['pid_file']) && ($aMyCnf['mysqld']['pid_file'] != '') ) {
		$lMysqldPidFile = $aMyCnf['mysqld']['pid_file'];
	}
	elseif ( isset($aMyCnf['mysqld']['pid-file']) && ($aMyCnf['mysqld']['pid-file'] != '') ) {
		$lMysqldPidFile = $aMyCnf['mysqld']['pid-file'];
	}
	// Guess PID file
	// Guessing on /var/run/mysqld/mysqld.pid is NOT allowed because this could kill the wrong Instance!
	else {

		$pf = $aInstance['datadir'] . '/mysqld.pid';
		if ( is_file($pf) ) {
			$lMysqldPidFile = $pf;
		}

		$pf = $aInstance['datadir'] . '/' . php_uname('n') . '.pid';
		if ( is_file($pf) ) {
			$lMysqldPidFile = $pf;
		}
	}

	if ( $lMysqldPidFile == '' ) {
		$rc = 520;
		error("Cannot find nor guess PID file (rc=$rc). Is it possible that the database is already stopped? (rc=$rc)");
		return $rc;
	}


	// Stop daemon. We use a signal here to avoid having to know the
	// root password.

	if ( file_exists($lMysqldPidFile) ) {

		if ( ! is_readable($lMysqldPidFile) ) {
			$rc = 558;
			error("Cannot read PID file $lMysqldPidFile. Wrong privileges? (rc=$rc).");
			return $rc;
		}

		$lMysqldPid = intval(file_get_contents($lMysqldPidFile));
		if ( $lMysqldPid == 0 ) {
			$rc = 559;
			error("PID from $lMysqldPidFile is wrong or empty (rc=$rc).");
			return $rc;
		}

		if ( posix_kill($lMysqldPid, SIG_IGN) === true ) {

			// print 'Shutting down MySQL';
			posix_kill($lMysqldPid, SIGTERM);
			// mysqld should remove the PID file when it exits, so wait for it.
			$rc = wait_for_pid($lMysqldPid, $lMysqldPidFile);
		}
		else {
			print "MySQL server process $lMysqldPid is not running or you have not the right to stop it!\n";
			if ( is_writeable(dirname($lMysqldPidFile)) ) {
				unlink($lMysqldPidFile);
			}
		}

		// Delete lock for RedHat / SuSE
		$lLockDir  = '/var/lock/subsys';
		$lLockFile = $lLockDir . '/mysql';

		if ( is_file($lLockFile) ) {
		
			if ( is_writeable($lLockFile) ) {
				unlink($lLockFile);
			}
			else {
				$aUser = posix_getpwuid(fileowner($lLockFile));
				print "File $lLockFile is NOT deletable for me and belongs to user " . $aUser['name'] . ".\n";
				print 'Please delete file ' . $lLockFile . ' manually as user ' . $aUser['name'] . ".\n";
			}
		}
	}
	else {
		$rc = 531;
		print "MySQL server PID file could not be found at $lMysqldPidFile. Database already\nstopped? (rc=$rc)!\n";
		return $rc;
	}

	return $rc;
}

# ----------------------------------------------------------------------
function getSchemaNames($pDatadir)
# Parameter     : $_[0] data directory
# Return values : hash of schema names
# ----------------------------------------------------------------------
{
	$aSchema = array();

	foreach ( glob("$pDatadir/*", GLOB_ONLYDIR) as $schema ) {
		if ( preg_match(':([^/]+)$:', $schema, $match) ) {
			array_push($aSchema, $match[1]);
		}
	}

	return $aSchema;
}

# ----------------------------------------------------------------------
function addDirectoryToPath($lPath, $lDir, $lOldDir, $location = 'left')
# Parameter     : $_[0] $PATH
#                 $_[1] new dir
# Return values : new $PATH
# ----------------------------------------------------------------------
{
	global $lDebug;

	if ( $lDebug != '' ) {
		print "PATH = " . $lPath . "\n";
		print "DIR  = " . $lDir . "\n";
		print "OLD  = " . $lOldDir . "\n";
	}

	$lPath = str_replace($lDir, '', $lPath);
	$lPath = str_replace($lOldDir, '', $lPath);
	if ( $location == 'left' ) {
		$lPath = $lDir . ':' . $lPath;
	}
	elseif ( $location == 'right' ) {
		$lPath = $lPath . ':' . $lDir;
	}
	else {
		$rc = 532;
		fprint(STDERR, "Invalid location $location (rc=$rc).\n");
	}
	// some clean-up
	$lPath = preg_replace('/:+/', ':', $lPath);
	$lPath = trim($lPath, ':');
	if ( $lDebug != '' ) {
		print "PATH = " . $lPath . "\n";
	}

	return $lPath;
}

# ----------------------------------------------------------------------
function deleteDirectoryFromPath($lPath, $lDir)
# ----------------------------------------------------------------------
{
	// delete
	$lPath = str_replace($lDir, '', $lPath);
	// remove ::
	$lPath = preg_replace('/:+/', ':', $lPath);
	// clean : at left or right side of PATH
	$lPath = trim($lPath, ':');

	return $lPath;
}

// ----------------------------------------------------------------------
function getSectionTitles($a)
// ----------------------------------------------------------------------
{
	$aDbNames = array();
	foreach ( $a as $db => $value ) {

		if ( $db != 'default' ) {
			array_push($aDbNames, $db);
		}
	}
	return $aDbNames;
}

// ----------------------------------------------------------------------
function printStartingDatabases()
// ----------------------------------------------------------------------
{
	$rc = OK;
	$aDbNames = array();

	$lDebug         = isset($_ENV['MYENV_DEBUG']) ? strval($_ENV['MYENV_DEBUG']) : '';
	$basedir        = $_ENV['MYENV_BASE'];
	// todo: hard coded here:
	$lConfFile      = '/etc/myenv/myenv.conf';
	list($ret, $aConfiguration) = getConfiguration($lConfFile);
	if ( $lDebug ) {
		var_dump($aConfiguration);
	}
	if ( $ret != OK ) {
		return array($ret, $aDbNames);
	}

	foreach ( $aConfiguration as $db => $parameter ) {

		if ( $db != 'default' ) {
			foreach ( $parameter as $key => $value ) {

				if ( ($key == 'start') && ((strtoupper($value) == 'YES' ) || ($value == 1)) ) {
					array_push($aDbNames, $db);
				}
			}
		}
	}
	return array($rc, $aDbNames);
}

// ----------------------------------------------------------------------
if ( ! function_exists('readline') ) {

	function readline($lne = '') {
// ----------------------------------------------------------------------

		print $lne;
		$fh = fopen('php://stdin', 'r');
		$input = fgets($fh, 1024);
		fclose($fh);
	return rtrim($input);
	}
}

// ----------------------------------------------------------------------
function getCurrentUser()
// ----------------------------------------------------------------------
{
	list($rc, $output, $stdout, $stderr) = my_exec('whoami');
	return($output);
}

// ----------------------------------------------------------------------
function checkMyEnvRequirements()
// ----------------------------------------------------------------------
{
	$ret = OK;
	$lDistribution = 'unknown';

	$lOs = getOs();
	if ( $lOs == 'Linux' ) {
		$lDistribution = getDistribution();
	}

	// print_r(get_defined_functions());
	// print_r(get_loaded_extensions());

	// Check if environment variables are enabled

	if ( count($_ENV) == 0 ) {
		$vo = ini_get('variables_order');
		# Alternative: shell> sudo echo "variables_order = 'EGPCS'" > /etc/php.d/myenv.ini
		error("Environment variables cannot be used. Please set variables_order = 'EGPCS' in");
		error("PHP cli config file. Current value is: variables_order = " . $vo . "\n");

		switch ($lDistribution) {
		case 'CentOS':
		case 'RedHat':
		case 'Fedora':
			$cmd = "sudo vi /etc/php.ini";
			print "shell> $cmd\n\n";
			break;
		case 'SUSE':
		case 'openSUSE project':
			$cmd = "sudo vi /etc/php5/cli/php.ini";
			print "shell> $cmd\n\n";
			break;
		case 'Ubuntu':
		case 'Debian':
			$cmd = "sudo vi /etc/php5/cli/php.ini";
			print "shell> $cmd\n\n";
			break;
		default:
			$cmd = 'echo "Unknown distribution: ' . $lDistribution . '"';
			print "OS/Distribution $lOs/$lDistribution is not yet supported.\n";
		}
		return ERR;
	}

	// Check if posix functions are installed

	if ( ! function_exists('posix_kill') ) {
		print "WARNING: The function posix_kill is not installed. You cannot stop MySQL\n";
		print "instances without. Please install the package containing posix_kill:\n\n";

		switch ($lDistribution) {
		case 'CentOS':
		case 'RedHat':
		case 'Fedora':
			$cmd = "sudo yum install " . getPacketList($lDistribution);
			print "shell> $cmd\n\n";
			break;
		case 'SUSE':
		case 'openSUSE project':
			$cmd = "sudo zypper install " . getPacketList($lDistribution);
			print "shell> $cmd\n\n";
			break;
		case 'Ubuntu':
		case 'Debian':
			$cmd = "sudo apt-get install " . getPacketList($lDistribution);
			print "shell> $cmd\n\n";
			break;
		default:
			$cmd = 'echo "Unknown distribution: ' . $lDistribution . '"';
			print "OS/Distribution $lOs/$lDistribution is not yet supported.\n";
		}
		$ret = WARN;
	}
	if ( ! function_exists('posix_getpwuid') ) {
		print "WARNING: The function posix_getpwuid is not installed. You cannot continue\n";
		print "installing without. Please install the package containing posix_getpwuid first:\n\n";

		switch ($lDistribution) {
		case 'CentOS':
		case 'RedHat':
		case 'Fedora':
			$cmd = "sudo yum install " . getPacketList($lDistribution);
			print "shell> $cmd\n\n";
			break;
		case 'SUSE':
		case 'openSUSE project':
			$cmd = "sudo zypper install " . getPacketList($lDistribution);
			print "shell> $cmd\n\n";
			break;
		case 'Ubuntu':
		case 'Debian':
			$cmd = "sudo apt-get install " . getPacketList($lDistribution);
			print "shell> $cmd\n\n";
			break;
		default:
			$cmd = 'echo "Unknown distribution: ' . $lDistribution . '"';
			print "OS/Distribution $lOs/$lDistribution is not yet supported.\n";
		}
		return ERR;
	}

	if ( (! defined('SIG_IGN')) || (! defined('SIGTERM')) ) {
		print "WARNING: The constants SIG_IGN and/or SIGTERM are not known. You cannot stop\n";
		print "MySQL instances without. Please install the package containing these constants\n";
		print "first:\n";
		print "Try to install the package php5-pcntl. We will help you in the meanwhile with a\n";
		print "fake...\n\n";

		switch ($lDistribution) {
		case 'CentOS':
		case 'RedHat':
		case 'Fedora':
			$cmd = "sudo yum install " . getPacketList($lDistribution);
			print "shell> $cmd\n\n";
			break;
		case 'SUSE':
		case 'openSUSE project':
			$cmd = "sudo zypper install " . getPacketList($lDistribution);
			print "shell> $cmd\n\n";
			break;
		case 'Ubuntu':
		case 'Debian':
			$cmd = "sudo apt-get install " . getPacketList($lDistribution);
			print "shell> $cmd\n\n";
			break;
		default:
			$cmd = 'echo "Unknown distribution: ' . $lDistribution . '"';
			print "OS/Distribution $lOs/$lDistribution is not yet supported.\n";
		}
		define('SIG_IGN', 0);
		define('SIGTERM', 15);
		$ret = WARN;
	}

	// Check if php-mysqli is installed (for fromdual_bman)

	if ( ! in_array('mysqli', get_loaded_extensions()) ) {
		print "WARNING: The PHP extension mysqli is not installed. You cannot run some tools\n";
		print "without it. Please install the package containing mysqli first:\n\n";

		switch ($lDistribution) {
		case 'CentOS':
		case 'RedHat':
		case 'Fedora':
			$cmd = "sudo yum install " . getPacketList($lDistribution);
			print "shell> $cmd\n\n";
			break;
		case 'SUSE':
		case 'openSUSE project':
			$cmd = "sudo zypper install " . getPacketList($lDistribution);
			print "shell> $cmd\n\n";
			break;
		case 'Ubuntu':
		case 'Debian':
			$cmd = "sudo apt-get install " . getPacketList($lDistribution);
			print "shell> $cmd\n\n";
			break;
		default:
			$cmd = 'echo "Unknown distribution: ' . $lDistribution . '"';
			print "OS/Distribution $lOs/$lDistribution is not yet supported.\n";
		}
		$ret = ERR;
	}

	// Check if lsb_release is installed

	$lOs = getOs();
	if ( $lOs == 'Linux' ) {

		$cmd = 'which lsb_release';
		list($ret, $output, $stdout, $stderr) = my_exec($cmd);

		if ( $ret != OK ) {
			print "WARNING: lsb_release is not installed. This can cause some malfunction in parts\n";
			print "of MyEnv.\n";
			print "Try to install the package lsb_release as follows:\n\n";

		switch ($lDistribution) {
		case 'CentOS':
		case 'CentOS Linux':
		case 'RedHat':
		case 'Fedora':
			$cmd = "sudo yum install " . getPacketList($lDistribution);
			print "shell> $cmd\n\n";
			break;
		case 'SUSE':
		case 'openSUSE project':
			$cmd = "sudo zypper install " . getPacketList($lDistribution);
			print "shell> $cmd\n\n";
			break;
		case 'Ubuntu':
		case 'Debian':
			$cmd = "sudo apt-get install " . getPacketList($lDistribution);
			print "shell> $cmd\n\n";
			break;
		default:
			$cmd = 'echo "Unknown distribution: ' . $lDistribution . '"';
			print "OS/Distribution $lOs/$lDistribution is not yet supported.\n";
		}
			$ret = WARN;
		}
	}

	return $ret;
}

// ----------------------------------------------------------------------
function parseConfigFile($pConfigurationFile, $scanner_mode = INI_SCANNER_NORMAL)
/*

	we have different problems parsing mysql config files:
	parse_ini_file() seems to be not a good idea to use with mysql my.cnf files
	Comments starting with '#' are deprecated in /home/mysql/data/mysql-5.6.15/my.cnf on line 42 in /home/mysql/product/myenv-rev392/lib/myEnv.inc on line 365
	Comments starting with '#' are deprecated in /home/mysql/data/mysql-5.6.15/my.cnf on line 42 in /home/mysql/product/myenv-rev392/lib/myEnv.inc on line 166
	syntax error, unexpected '!' in /etc/mysql/my.cnf on line 127 (rc=164)
	127 !includedir /etc/mysql/conf.d/
	Seems to have problems with !include directive!!!
	Seems to have problems with passwords with strange characaters
	core-file is not shown!
	The my.cnf has this line at the end "!includedir /etc/mysql/conf.d/" which caused the error "syntax error, unexpected '!' in /etc/mysql/my.cnf on line 132 (rc=164)".
	fromdual_bman.php

*/
// ----------------------------------------------------------------------
{
	$rc = OK;

	// Replace all comments like:
	// \w*#$

	if ( ! file_exists($pConfigurationFile) ) {
		$rc = 527;
		$msg = "File $pConfigurationFile does not exist (rc=$rc).";
		return array($rc, $msg);
	}

	if ( ! is_readable($pConfigurationFile) ) {
		$rc = 571;
		$msg = "File $pConfigurationFile is not readable (rc=$rc).";
		return array($rc, $msg);
	}

	if ( ($aRawConf = file($pConfigurationFile, FILE_IGNORE_NEW_LINES | FILE_SKIP_EMPTY_LINES)) === false ) {
		$rc = 528;
		$msg = "Cannot read file $pConfigurationFile (rc=$rc).";
		return array($rc, $msg);
	}

	$tmp = '/tmp' . '/' . posix_getpid() . '.cnf';
	$fh = fopen($tmp, 'w');

	$pattern = array(
		'/^#.*$/'
	, '/\s+#.*$/'
	, '/^!include/'
	, '/\s+$/'
	);
	foreach ( $aRawConf as $line ) {
		$line = preg_replace($pattern, '', $line) . "\n";
		$n = fwrite($fh, $line);
	}
	fclose($fh);

	// Do not hide error outputs!
	if ( ($aConfig = @parse_ini_file($tmp, true, $scanner_mode)) === false ) {
		$rc = 530;
		parse_ini_file($tmp, true, $scanner_mode);
		$msg = "Troubles reading configuration file $pConfigurationFile (rc=$rc).";
		unlink($tmp);
		return array($rc, $msg);
	}
	unlink($tmp);

	// Check if config file was formed correctly:
	foreach ( $aConfig as $section => $values ) {
		if ( ! is_array($values) ) {
			$rc = 500;
			$msg = "Malformed section $section is not an array (rc=$rc).";
			return array($rc, $msg);
		}
	}

	return array($rc, $aConfig);
}

// ----------------------------------------------------------------------
function getMd5sum2($file, $aOptions)
// ----------------------------------------------------------------------
{
	$rc = OK;
	$ret = OK;
	$md5 = '';

	$aUname = posix_uname();
	if ( $aUname['sysname'] == 'Linux' ) {
		$exe = 'md5sum --binary';
	}
	elseif ( $aUname['sysname'] == 'Darwin' ) {
		$exe = 'md5 -r';
	}
	else {
		$rc = 563;
		error('Operating system ' . $aUname['sysname'] . " is not yet supported (rc=$rc).");
		return array($rc, $md5);
	}

	$cmd = "$exe $file";

	if ( ! array_key_exists('simulate', $aOptions) ) {
		list($ret, $output, $stdout, $stderr) = my_exec("$cmd 2>&1");
		$s = implode("\n", $stdout);

		if ( $ret != OK ) {
			$rc = 556;
			error("Command $exe failed for file $file (rc=$rc).");
			return array($rc, $md5);
		}
		if ( preg_match("/^([0-9a-f]{32})\s?.*$/i", $s, $matches) ) {
			$md5 = $matches[1];
		}
		else {
			$rc = 557;
			error("Error building md5 (rc=$rc).");
			return array($rc, $md5);
		}
	}
	return array($ret, $md5);
}

// ----------------------------------------------------------------------
function checkForOldStuff()
// ----------------------------------------------------------------------
{
	$rc = OK;

	// Check that MYSQL_HOME was switched from basedir to datadir

	$file = '/etc/myenv/variables.conf';
	$aLines = explode("\n", file_get_contents($file));
	foreach ( $aLines as $line ) {
		if ( preg_match('/MYSQL_HOME/', $line, $matches) ) {
			// only first one to allow users to overwrite later
			break;
		}
	}

	if ( count($matches) == 0 ) {
		$rc = 560;
		print "Variable MYSQL_HOME is missing in file $file (rc=$rc).\n";
	}

	if ( ! preg_match('/datadir/', $line) ) {
		$rc = 562;
		print "Variable MYSQL_HOME is NOT set correctly in file $file (rc=$rc).\n";
		print $line . "\n";
		print "Please change the MYSQL_HOME line from basedir to datadir e.g. with the following\ncommand:\n\n";
		print "shell> sed --in-place '0,/basedir/{s/basedir/datadir/}' $file\n\n";
	}


	// Check that myenv.server template file is same as /etc/init.d/myenv

	$ini = '/etc/init.d/myenv';
	$tpl = $_ENV['MYENV_BASE'] . '/' . 'bin/myenv.server';

	list($ret, $md5_1) = getMd5sum2($tpl, array());
	list($ret, $md5_2) = getMd5sum2($ini, array());
	if ( $md5_1 != $md5_2 ) {
		$rc = 561;
		print "File $ini is old and possibly not up to date (rc=$rc).\n";
		print "Please fix this with the following command:\n\n";
		print "shell> sudo cp $tpl $ini\n\n";
	}

	return $rc;
}

// ----------------------------------------------------------------------
function my_exec($pCmd)
// We have seen some cases, where return values are not return correctly
// To be really sure also check the stderr!
// ----------------------------------------------------------------------
{
	$rc = OK;

	$file = '/tmp/my_exec.stderr.' . getmypid();
	// http://unix.stackexchange.com/questions/87745/what-does-lc-all-c-do
	unset($stdout);
	$output = exec("export LC_ALL=C ; $pCmd 2>$file", $stdout, $rc);

	$stderr = array();
	$fh = fopen($file, 'r');
	while ( $line = fgets($fh) ) {
		$line = trim($line);
	
		// Skip this nasty warning:
		if ( $line == 'Warning: Using a password on the command line interface can be insecure.' ) {
			continue;
		}
		array_push($stderr, $line);
	}
	fclose($fh);
	unlink($file);

	return array($rc, $output, $stdout, $stderr);
}

// ----------------------------------------------------------------------
function getVersionFromMysqld($pBasedir)
// ----------------------------------------------------------------------
{
	$version = 'unknown';

	$aExecutables = array('/sbin/mysqld', '/libexec/mysqld', '/bin/mysqld');
	foreach ( $aExecutables as $e ) {
	
		// /usr/sbin/mysqld
		$exe = $pBasedir . $e;
		if ( is_executable($exe) ) {
		
			// /usr/sbin/mysqld --version  Ver 5.5.38-0ubuntu0.14.04.1 for debian-linux-gnu on x86_64 ((Ubuntu))
			$cmd = $exe . ' --version';
			debug($cmd);
			list($rc, $output, $stderr, $stdout) = my_exec($cmd);
			// var_dump($rc, $output, $stderr, $stdout);
			if ( preg_match("/(\d{1,2}\.\d{1,2}\.\d{1,2})/", $output, $matches) ) {
				$version = $matches[1];
				break;
			}
		}
	}

	return $version;
}

// ---------------------------------------------------------------------
function getOs()
/*
	$lOs == 'Linux'
	$lOs == 'Darwin'
*/
// ---------------------------------------------------------------------
{
	// possibly better use php_uname(...) for Windows compatibility!
	$aUname = posix_uname();
	return $aUname['sysname'];
}

// ---------------------------------------------------------------------
function getDistribution()
/*
	$distribution == 'CentOS'
	$distribution == 'RedHat'
	$distribution == 'Fedora'
	$distribution == 'Ubuntu'
	$distribution == 'Debian'
	$distribution == 'SUSE'
	$distribution == 'openSUSE project'
*/
// ---------------------------------------------------------------------
{
	$distribution = 'unknown';
	$lOs = getOs();
	if ( $lOs == 'Linux' ) {

		$exe = 'lsb_release';
		list($ret, $output, $stdout, $stderr) = my_exec("which $exe 2>&1");
		if ( $ret == OK ) {
			$cmd = $exe . ' -s -i';
			list($ret, $distribution, $stdout, $stderr) = my_exec($cmd);
		}
		else {
			$distribution = simulateLsbRelease();
		}
	}
	return $distribution;
}

// ---------------------------------------------------------------------
function simulateLsbRelease()
// Simulates lsb_release -s -i if this does not exist
// ---------------------------------------------------------------------
{
	$distribution = 'unknown';

	// Ubuntu  : NAME="Ubuntu"
	// OpenSuSE: NAME=openSUSE
	// Debian  : NAME="Debian GNU/Linux"
	$lOsRelease = '/etc/os-release';
	if ( file_exists($lOsRelease) && is_readable($lOsRelease) ) {

		if ( $fh = fopen($lOsRelease, 'r') ) {

			while ( ($buffer = fgets($fh, 256)) === true ) {
				list($key, $value) = explode('=', $buffer);
				if ( $key == 'NAME' ) {
					$distribution = trim($value, '"');
					break;
				}
			}
			// This seems to be wrong when NAME is reached!!!
			/* if ( feof($fh) === false ) {
				$rc = 517;
				error("Unexpected fgets() fail on file $lOsRelease (rc=$rc).");
				return $distribution;
			} */
			fclose($fh);
		}
		else {
			$rc = 516;
			error("Cannot open file $lOsRelease (rc=$rc).");
			return $distribution;
		}
	}
	// CentOS  : cpe:/o:centos:centos:7:GA
	//           cpe:/o:centos:linux:6:GA
	else {

		$lSystemReleaseCpe = '/etc/system-release-cpe';
		if ( file_exists($lSystemReleaseCpe) && is_readable($lSystemReleaseCpe) ) {

			if ( $fh = fopen($lSystemReleaseCpe, 'r') ) {

				while ( ($buffer = fgets($fh, 256)) === true ) {
					$a = explode(':', $buffer);
					$distribution = $a[2];
				}
				// This seems to be wrong when NAME is reached!!!
				/* if ( ! feof($fh) ) {
					$rc = 574;
					error("Unexpected fgets() fail (rc=$rc).");
					return $distribution;
				} */
				fclose($fh);
			}
			else {
				$rc = 575;
				error("Cannot open file $lSystemReleaseCpe (rc=$rc).");
				return $distribution;
			}

		}
	}

	// Normalize data
	switch ($distribution) {
	case 'openSUSE':
		$distribution .= ' project';
		break;
	case 'Debian GNU/Linux':
		$distribution = 'Debian';
		break;
	case 'centos':
		$distribution = 'CentOS';
		break;
	default:
		null;
	}
	return $distribution;
}

// ---------------------------------------------------------------------
function getPacketList($pDistribution)
// ---------------------------------------------------------------------
{
	$lList = 'unknown distribution ' . $pDistribution;

	switch ($pDistribution) {
	case 'CentOS':
	case 'RedHat':
	case 'Fedora':
		$lList = 'php-cli php-posix php-pcntl php-mysqli redhat-lsb-core';
		break;
	case 'SUSE':
	case 'openSUSE project':
		$lList = 'php5 php5-cli php5-posix php5-pcntl php5-mysqli lsb-release';
		break;
	case 'Ubuntu':
	case 'Debian':
		$lList = 'php5 php5-posix php5-pcntl php5-mysql lsb-release';
		break;
	default:
		$lList = 'unknown distribution ' . $pDistribution;
	}

	return $lList;
}

// ---------------------------------------------------------------------
function noop()
// ---------------------------------------------------------------------
{
}

// ----------------------------------------------------------------------
function parseConnectString($pConnectString)
// ----------------------------------------------------------------------
{
	// Connect string style:
	// user/password@host:port

	// http://en.wikipedia.org/wiki/URI_scheme
	// foo://username:password@example.com:8042

	// Use the last @ for splitting host and user
	// Use the first / or : for splitting user and password
	// Characters like : @ and / are allowed in password!
	// Allowed combinations for user are:
	// user/password
	// user:password
	// user
	// Allowed combinations for host are:
	// host:port
	// host

	$aTarget = array();
	$ret = OK;


	// Split first into user and host part: @ is mandatory

	$pos = strrpos($pConnectString, '@');

	if ( $pos == 0 ) {
		$ret = 173;
		// This is/was a security issue! Removed: $pConnectString
		error("Connect string does not match pattern: user/password@host:port (ret=$ret).");
		return array($ret, $aTarget);
	}

	$user_part = substr($pConnectString, 0, $pos);
	$host_part = substr($pConnectString, $pos+1);


	// User part

	$u = preg_split("/[\/:]/", $user_part, 2);
	$aTarget['user']     = $u[0];
	$aTarget['password'] = isset($u[1]) ? $u[1] : '';

	// Host part

	$h = preg_split("/:/", $host_part);
	$aTarget['host']   = $h[0];

	$aTarget['port']   = null;
	$aTarget['socket'] = null;

	if ( isset($h[1]) ) {

		if ( $h[1] == '' ) {
			$aTarget['port']   = 3306;
			$aTarget['socket'] = '/var/run/mysqld/mysql.sock';
		}
		else {

			if ( intval($h[1]) == 0 ) {
				$aTarget['socket'] = $h[1];
				$aTarget['port']   = 3306;
			}
			else {
				$aTarget['port']   = intval($h[1]);
				$aTarget['socket'] = '/var/run/mysqld/mysql.sock';
			}
		}

		if ( $aTarget['port'] == 0 ) {
			$ret = 172;
			error("Port " . $h[1] . " is zero or not an integer (ret=$ret).");
			$aTarget['port'] = '';
			return array($ret, $aTarget);
		}
	}

	return array($ret, $aTarget);
}

// -------------------------------------------------------------------
function getMachineReadableVersion($pVersion)
// todo: stolen from focmm/lib/Tuning.inc, make unique
// -------------------------------------------------------------------
{
	$rc = OK;
	$mr_version = '';

	$pattern='/(\d+)\.(\d+).(\d+)/';
	preg_match($pattern, $pVersion, $matches);
	$mr_version = sprintf("%02d%02d%02d", $matches[1], $matches[2], $matches[3]);

	return array($rc, $mr_version);
}

?>
