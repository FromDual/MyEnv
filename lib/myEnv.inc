<?php

/*

	debug($s)
	error($s)
	warn($s)
	output($s)
	getConfiguration($pConfigurationFile)
	extractVersion($pBasedir)
	startDatabase($aInstance, $pOptions = '')
	startInstance($aInstance, $pOptions = '')
	checkDatabase($aInstance)
	checkInstance($aInstance)
	wait_for_pid($lMysqldPid, $pidfile)
	stopDatabase($aInstance)
	stopInstance($aInstance)
	getSchemaNames($pDatadir)
	addDirectoryToPath($lPath, $lDir, $lOldDir, $location = 'left')
	deleteDirectoryFromPath($lPath, $lDir)
	getSectionTitles($a)
	printStartingDatabases($pConfFile)
	printStoppingDatabases($pConfFile)
	getCurrentUser()
	checkMyEnvRequirements()
	parseConfigFile($pConfigurationFile, $scanner_mode = INI_SCANNER_NORMAL)
	getMd5sum2($file, $aOptions)
	checkForOldStuff($pBasedir)
	my_exec($pCmd)
	getVersionAndBranchFromDaemon($pBasedir, $pTestString = '')
	getOs()
	getDistribution(&$rDistributionInfo = array(), $pTestFile = null)
	getPacketList($pDistribution)
	noop()
	parseConnectString($pConnectString)
	getMachineReadableVersion($pVersion)
	extractBranch($pVersionComment, $pVersion = '')
	recursivelyRemoveDirectory($dir)
	getInstanceInfo($pVersion, $pVersionComment)
	searchExecutable($pBasedir, $aExecutables)
	formatTime($pTimeInSeconds)

*/

/*

Do NOT use: MOD(256)

$rc = 256;
$rc = 512;
$rc = 768;

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
	// light red
	fprintf(STDERR, "\e[91mERROR\e[39m: %s\n", $s);
}

// ----------------------------------------------------------------------
function warn($s)
// ----------------------------------------------------------------------
{
	// yellow
	output("\e[93m" . 'WARNING' . "\e[39m" . ': ' . $s . "\n");
}

// ----------------------------------------------------------------------
function output($s)
// ----------------------------------------------------------------------
{
	fprintf(STDOUT, "%s", $s);
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
		error($msg . " (rc=$rc)");
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

				// default is NOT wanted in instance sections!
				if ( $key == 'default' ) {
					continue;
				}

				if ( ! isset($aConfiguration[$db][$key]) ) {
					$aConfiguration[$db][$key] = $val;
				}
			}
		}
	}

	return array($rc, $aConfiguration);
}

// ----------------------------------------------------------------------
function extractVersion($pBasedir)
// Extracts MariaDB/MySQL version from basedir (/home/mysql/product/mariadb-10.3.5)
// ----------------------------------------------------------------------
{
	$version = 'unknown';

	// Expected patterns: see tst/extractVersion.php
	if ( preg_match("/([a-zA-Z]+)\d?\-/", $pBasedir, $matches) ) {

		if ( ($matches[1] == 'mysql')
			|| ($matches[1] == 'MySQL')
			|| ($matches[1] == 'percona')
			|| ($matches[1] == 'mariadb')
				) {

			if ( preg_match("/(\d{1,2}\.\d{1,2}\.\d{1,2})/", $pBasedir, $matches) ) {
				$version = $matches[1];
			}
			elseif ( preg_match("/(\d{1,2}\.\d{1,2}\.\d{1,2}\-ndb\-\d{1,2}\.\d{1,2}\.\d{1,2}[a-z]?)/", $pBasedir, $matches) ) {
				$version = $matches[1];
			}
			elseif ( preg_match("/(\d{1,2}\.\d{1,2})/", $pBasedir, $matches) ) {
				$version = $matches[1];
			}
			else {
			}
		}
		elseif ( ($matches[1] == 'Percona')
					|| ($matches[1] == 'Percona-Server')
					) {
			if ( preg_match("/(\d{1,2}\.\d{1,2}\.\d{1,2})/", $pBasedir, $matches) ) {
				$version = $matches[1];
			}
		}
		else {
			// output('1');
		}
	}
	else {
		// Case: 5.6.12
		if ( preg_match("/(\d{1,2}\.\d{1,2}\.\d{1,2})/", $pBasedir, $matches) ) {
			$version = $matches[1];
		}
	}

	return $version;
}

// ----------------------------------------------------------------------
function startDatabase($aInstance, $pOptions = '')
// TODO: Do not use this function call any more!
// ----------------------------------------------------------------------
{
	return startInstance($aInstance, $pOptions);
}

// ----------------------------------------------------------------------
function startInstance($aInstance, $pOptions = '')
// ----------------------------------------------------------------------
{
	$rc = OK;
	global $lDebug;

	debug('startInstance: ' . $aInstance['name'] . ', options:');
	debug($pOptions);

	// Do option check and correction

	$aOptions = explode(' ', $pOptions);
	$key = array_search('--new-cluster', $aOptions);
	if ( $key !== FALSE ) {
		unset($aOptions[$key]);
		array_push($aOptions, '--wsrep-new-cluster');
	}
	$key = array_search('--bootstrap', $aOptions);
	if ( $key !== FALSE ) {
		unset($aOptions[$key]);
		array_push($aOptions, '--wsrep-new-cluster');
	}
	$pOptions = implode(' ', $aOptions);

	debug('options after check and correction:');
	debug($pOptions);

	// Get version and branch
	list($lVersion, $lBranch) = getVersionAndBranchFromDaemon($aInstance['basedir']);

	// Check if db is started with correct user
	// Also root should be allowed to start DB (Bug #99)

	$lCurrentUser = getCurrentUser();
	if ( ($lCurrentUser != $aInstance['user'])
		&& ($lCurrentUser != 'root')
	   ) {
		$rc = 522;
		error("Database is started with the wrong user ($lCurrentUser) but should should be started with " . $aInstance['user'] . " (rc=$rc).");
		return $rc;
	}


	// Check if mysql.user table or mysql.mysql.ibd tablespace in MySQL 8.0 exists

	if ( 'MySQL' == $lBranch ) {

		if ( ! (file_exists($aInstance['datadir'] . '/mysql/user.frm')
				|| file_exists($aInstance['datadir'] . '/mysql/user.ibd')
				|| file_exists($aInstance['datadir'] . '/mysql.ibd')
					) ) {
			$rc = 523;
			error('User table (' . $aInstance['datadir'] . '/mysql/user.{frm|ibd}) or mysql tablespace (' . $aInstance['datadir'] . '/mysql.ibd) does not exist.');
			error("Are you sure you have installed a MySQL instance already?" . " (rc=$rc)");
			error("Create a database instance with the command:");
			error("shell> mysqld --user=mysql --initialize-insecure --datadir=" . $aInstance['datadir'] . " --basedir=" . $aInstance['basedir']);
			return $rc;
		}
	}
	elseif ( 'MariaDB' == $lBranch ) {

		if ( ! (file_exists($aInstance['datadir'] . '/mysql/user.frm')
				|| file_exists($aInstance['datadir'] . '/mysql/user.ibd')
					) ) {
			$rc = 586;
			error('User table (' . $aInstance['datadir'] . '/mysql/user.{frm|ibd}) does not exist.');
			error("Are you sure you have installed a MariaDB instance already? (rc=$rc)");
			error("Create a database instance with the command:");
			error("shell> mariadb-install-db --user=mysql --datadir=" . $aInstance['datadir'] . " --basedir=" . $aInstance['basedir']);
			return $rc;
		}
	}
	else {
		$rc = 573;
		$msg = 'Unknown branch: ' . $lBranch . " (rc=$rc)";
		error($msg);
		return $rc;
	}

	if ( ! array_key_exists('my.cnf', $aInstance) ) {
		$rc = 552;
		$msg = 'Key my.cnf is not defined in MyEnv configuration file for instance ' . $aInstance['name'] . '.';
		error($msg);
		return $rc;
	}

	list($ret, $aMyCnf) = parseConfigFile($aInstance['my.cnf'], INI_SCANNER_RAW);
	if ( $ret != OK ) {
		$rc = 553;
		$msg = 'Error parsing configuration file ' . "'" . $aInstance['my.cnf'] . "'" . '.' . " (rc=$rc)";
		error($msg);
		error($aMyCnf);
		return $rc;
	}

	// Check if there is any mismatch between my.cnf and myenv.conf

	foreach ( $aInstance as $key => $value ) {

		if ( isset($aMyCnf['mysqld'][$key]) ) {
			if ( $aMyCnf['mysqld'][$key] != $value ) {
				$rc = 569;
				$cnf = '/etc/myenv/myenv.conf';
				warn($key . " differs between " . $aInstance['my.cnf'] . " and " . $cnf . ' (' . $aMyCnf['mysqld'][$key] . ' vs. ' . $value . ") (rc=$rc). Please fix this to avoid troubles...");
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
					warn($key . " differs between [mysqld] and [client] section (" . $aMyCnf['mysqld'][$key] . " vs. " . $value . ") in my.cnf (rc=$rc). Please fix this to avoid troubles...");
				}
			}
		}
	}


	// Check if the instance is started already

	$ret = checkInstance($aInstance);
	if ( OK == $ret ) {
		$rc = 533;
		$msg = 'Instance is already started.' . " (rc=$rc)";
		error($msg);
		return $rc;
	}


	// Check if user (mariadb/mysql) can write to /run/{mariadb|mysqld} directory
	// If database packages are installed from repositories, runtime directory is set to mysql
	// which can be wrong in some cases.
	$runtime_directory = '/run/mysqld';
	if ( 'mariadb' == $aInstance['user'] ) {
		$runtime_directory = '/run/mariadb';
	}

	$aUserInfo = posix_getpwuid(fileowner($runtime_directory));
	if ( $aInstance['user'] != $aUserInfo['name'] ) {
		$rc = 587;
		$msg = 'Runtime directory ' . $runtime_directory . ' does NOT belong to user ' . $aInstance['user'] . ' but to user: ' . $aUserInfo['name'] . ' which is possibly wrong.' . " (rc=$rc)";
		warn($msg);
		// Do not error() and exit here because we are not 100% sure yet...
	}


	// Check if port is already in use
	// This can happen on DEB systems when database is installed from repos
	// then the database is started already by the package

	$ip = '127.0.0.1';
	$socket = socket_create(AF_INET, SOCK_STREAM, getprotobyname('tcp'));   // SOL_TCP

	if ( $socket === false ) {
		$rc = 588;
		$err = socket_last_error();
		$msg = 'Cannot create socket [' . $err . ']: ' . socket_strerror($err) . " (rc=$rc)";
		error($msg);
		return $rc;
	}

	$ret = @socket_bind($socket, $ip, $aInstance['port']);
	if ( $ret === false ) {
		$rc = 589;
		$err = socket_last_error();
		$msg = 'Unable to bind address [' . $err . ']: ' . socket_strerror($err) . 'Port ' . $ip . ':' . $aInstance['port'] . ' is already in use. Possibly another database instance is using this port yet or you did a missconfiguration in your myenv.conf.' . " (rc=$rc)";
		error($msg);
		return $rc;
	}
	$void = socket_close($socket);


	// Check if socket file is already there

	if ( file_exists($aInstance['socket']) ) {
		$rc = 590;
		$msg = 'Socket file ' . $aInstance['socket'] . ' already exists. Possibly another database instance is using this socket file yet or you did a missconfiguration in your myenv.conf.' . " (rc=$rc)";
		error($msg);
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

		// Check old cgroups file location
		$old = $aInstance['datadir'] . '/' . 'cgroups.conf';
		if ( is_file($old) ) {
			$msg = "Old location of cgroups configuration $old is used.\n";
			warn($msg);
			$msg = "Please move to new location " . $aInstance['instancedir'] . '/etc/' . ".\n";
			warn($msg);
		}

		$lCgroupsFile = $aInstance['instancedir'] . '/etc/' . 'cgroups.conf';
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
					error("Cannot interpret cgroups parameter: $buffer");
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
				list($rc, $output, $aStdout, $aStderr) = my_exec($cmd);
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


	$cmd  = '';
	$cmd .= 'cd ' . $aInstance['basedir'] . ' ; ';

	$err = '/var/tmp' . '/' . posix_getpid() . '.err';

	// Start mysqld with angel process
	if ( $aInstance['angel'] == 'yes' ) {

		$exe = 'bin/mysqld_safe';
		if ( 'MariaDB' == $lBranch ) {
			$exe = 'bin/mariadbd-safe';
		}

		$cmd .= "nohup $cgexec ' . $exe . ' --defaults-file=" . $aInstance['my.cnf'] . " --datadir=" . $aInstance['datadir'] . " --basedir=" . $aInstance['basedir'] . " " . $pOptions;

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

		$aExecutables = array('bin/mariadbd', 'sbin/mariadbd', 'libexec/mariadbd', 'bin/mysqld', 'sbin/mysqld', 'libexec/mysqld');
		$exe = searchExecutable($aInstance['basedir'], $aExecutables);
		debug($exe);
		if ( $exe == '' ) {
			$rc = 585;
			$msg = 'Executable mariadbd/mysqld does not exist.' . " (rc=$rc)";
			error($msg);
			return $rc;
		}
		// strip basedir again
		$exe = substr($exe, strlen($aInstance['basedir'] . '/'));

		$cmd .= $cgexec . ' ' . $exe . ' --defaults-file=' . $aInstance['my.cnf'] . ' --basedir=' . $aInstance['basedir'] . ' --datadir=' . $aInstance['datadir'] . ' --user=' . $aInstance['user']. ' ' . $pOptions;

		if ( $lDebug != '' ) {
			$cmd .= ' & echo $!';
		}
		else {
			$cmd .= ' >' . $err . ' 2>&1 & echo $!';
		}
	}

	debug($cmd);
	list($rc, $output, $aStdout, $aStderr) = my_exec($cmd);
	// No return code here!
	// if ( $ret != 0 ) {
	//   error("Command $cmd failed.");
	//   $rc = 572;
	//   // no return here
	// }


	$lStartTimeout = 60;
	if ( isset($aInstance['start-timeout']) ) {
		$lStartTimeout = intval($aInstance['start-timeout']);
	}
	output('Timeout is ' . $lStartTimeout . ' seconds: ');

	// it takes some time until $err is filled
	sleep(1);

	// Different variants of error we get
	// Filter out start and end line

	// 150130 16:42:01 mysqld_safe Logging to '/home/mysql/data/mariadb-10.0.16/error.log'.
	// mkdir: cannot create directory ‘/run/mysqld’: Permission denied
	// Fatal error Can't create database directory '/run/mysqld/mysql-10016.sock'

	// 150130 16:44:19 mysqld_safe Logging to '/home/mysql/data/mariadb-10.0.16/error.log'.
	// 150130 16:44:19 mysqld_safe Starting mysqld daemon with databases from /home/mysql/data/mariadb-10.0.16

	$daemon = 'mysqld';
	if ( 'MariaDB' == $lBranch ) {
		$daemon = 'mariadbd';
	}

	foreach ( explode("\n", trim(file_get_contents($err))) as $line ) {

		// mysqld_safe is still true up to MariaDB 11.2
		if ( (strpos($line, 'mysqld_safe Logging to') === false)
		  && (strpos($line, 'mysqld_safe Starting ' . $daemon . ' daemon') === false)
		   ) {
			array_push($aStderr, $line . "\n");
		}
	}
	$aKeepStderr = $aStderr;
	if ( file_exists($err) ) {
		unlink($err);
	}

	$lDebugOrig = $lDebug;
	$lDebug = '';
	for ( $i = 1; $i <= $lStartTimeout ; $i++ ) {

		sleep(1);
		output('.');
		$ret = checkInstance($aInstance);
		$rc = $ret;
		if ( $ret == 0 ) {
			break;
		}
	}
	$lDebug = $lDebugOrig;

	if ( OK == $rc ) {
		output(" SUCCESS!" . "\n");
		return $rc;
	}
	else {
		$rc = 518;
		error("(rc=$rc)");
		if ( count($aKeepStderr) > 0 ) {
			error("Command $cmd failed.");
			foreach ( $aKeepStderr as $line ) {
				error("  " . trim($line));
			}
		}
		return $rc;
	}
}

// ---------------------------------------------------------------------
function checkDatabase($aInstance)
// TODO: Do not use this function call any more! Deprecated!
// TODO: Can be removed later when ALL projects have changed...
// ---------------------------------------------------------------------
{
	return checkInstance($aInstance);
}

// ---------------------------------------------------------------------
function checkInstance($aInstance)
// ---------------------------------------------------------------------
{

	$rc = OK;

	$lTimeout = 3;
	$msg = '';

	try {

		// Create a UNIX file socket, protocol must be IP
		$socket = socket_create(AF_UNIX, SOCK_STREAM, 0);

		if ( $socket === false ) {
			$rc = 577;
			$msg = 'socket_create() failed: ' . socket_strerror(socket_last_error()) . " (rc=$rc)";
			throw new Exception($msg);
		}

		// Do a non-blocking connect
		socket_set_nonblock($socket);

		// Timeout
		$time = time();

		while ( ! @socket_connect($socket, $aInstance['socket']) ) {

			$err = socket_last_error($socket);
			// Socket file does not exist, so we have to assume instance does not run
			if ( $err = SOCKET_ENOENT ) {
				$rc = 581;
				break;
			}
			// 115, 114
			elseif ( $err == SOCKET_EINPROGRESS || $err == SOCKET_EALREADY ) {

				if ( (time() - $time) >= $lTimeout ) {

					socket_close($socket);
					$rc = 579;
					$msg = 'Connection timed out.' . " (rc=$rc)";
					throw new Exception($msg);
				}
				sleep(0.1);
				continue;
			}

			$rc = 580;
			$msg = socket_strerror($err) . " (err=$err / rc=$rc)";
			throw new Exception($msg);
		}

		socket_close($socket);
	}
	catch ( Exception $e ) {
		error($e->getMessage());
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
		output('.');
		// Process seems dead
		if ( posix_kill($lMysqldPid, SIG_IGN) === false ) {
			break;
		}

		$i++;
	}

	if ( file_exists($pidfile) ) {
		$rc = 526;
		output("\n" . "The server quit without removing PID file ($pidfile) (rc=$rc)." . "\n");
	}

	if ( $rc == OK ) {
		output(" SUCCESS!\n");
		return $rc;
	}
	else {
		error(" (rc=$rc).");
		return $rc;
	}
}

// ----------------------------------------------------------------------
function stopDatabase($aInstance)
// TODO: Do not use this function call any more!
// ----------------------------------------------------------------------
{
	return stopInstance($aInstance);
}

// ----------------------------------------------------------------------
function stopInstance($aInstance)
// ----------------------------------------------------------------------
{
	$rc = OK;

	// Find the PID file

	$lMysqldPidFile = '';
	// We do not look into the database because this requires some account
	// which we do not have yet...

	// Check if my.cnf is set. This might be not the case if a dummy instance is
	// defined.
	if ( ! array_key_exists('my.cnf', $aInstance) ) {
		$rc = 568;
		$msg = 'my.cnf is not defined in myenv.conf. Are you running a dummy instance?' . " (rc=$rc)";
		error($msg);
		return $rc;
	}

	list($ret, $aMyCnf) = parseConfigFile($aInstance['my.cnf'], INI_SCANNER_RAW);
	// var_dump($ret, $aMyCnf);
	if ( $ret != OK ) {
		$rc = 525;
		// If $ret != OK then $aMyCnf is a string with the error message!
		$msg = 'Error parsing configuration file: ' . "'" . $aInstance['my.cnf'] . "'" . '. ' . $aMyCnf . " (rc=$rc/ret=$ret)";
		error($msg);
		return $rc;
	}

	// Use PID file from my.cnf
	if ( array_key_exists('pid_file', $aMyCnf['mysqld']) && ($aMyCnf['mysqld']['pid_file'] != '') ) {
		$lMysqldPidFile = $aMyCnf['mysqld']['pid_file'];
	}
	elseif ( array_key_exists('pid-file', $aMyCnf['mysqld']) && ($aMyCnf['mysqld']['pid-file'] != '') ) {
		$lMysqldPidFile = $aMyCnf['mysqld']['pid-file'];
	}
	// Guess PID file
	// Guessing on /run/mysqld/mysqld.pid is NOT allowed because this could kill the wrong Instance!
	else {

		// I have no any idea any more in which case this can happen...
		// /run/mariadb/mariadb.pid

		$pf = $aInstance['datadir'] . '/mysqld.pid';
		if ( is_file($pf) ) {
			$lMysqldPidFile = $pf;
		}
		else {

			# centos7.rebenweg
			$lHostnameShort = explode('.', php_uname('n'))[0];
			$pf = $aInstance['datadir'] . '/' . $lHostnameShort . '.pid';
			if ( is_file($pf) ) {
				$lMysqldPidFile = $pf;
			}
		}
	}

	if ( '' == $lMysqldPidFile) {
		$rc = 520;
		$msg = 'Cannot find nor guess PID file. Is it possible that the database is already stopped?' . " (rc=$rc)";
		error($msg);
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

			// output('Shutting down MariaDB/MySQL instance');
			posix_kill($lMysqldPid, SIGTERM);
			// mysqld should remove the PID file when it exits, so wait for it.
			$rc = wait_for_pid($lMysqldPid, $lMysqldPidFile);
		}
		else {
			output("MariaDB/MySQL server process $lMysqldPid is not running or you have not the right to stop it!\n");
			if ( is_writeable(dirname($lMysqldPidFile)) ) {
				unlink($lMysqldPidFile);
			}
		}

		// Delete lock for RedHat / SUSE
		$lLockDir  = '/var/lock/subsys';
		$lLockFile = $lLockDir . '/mysql';

		if ( is_file($lLockFile) ) {

			if ( is_writeable($lLockFile) ) {
				unlink($lLockFile);
			}
			else {
				$aUser = posix_getpwuid(fileowner($lLockFile));
				output("File $lLockFile is NOT deletable for me and belongs to user " . $aUser['name'] . ".\n");
				output('Please delete file ' . $lLockFile . ' manually as user ' . $aUser['name'] . ".\n");
			}
		}
	}
	else {
		$rc = 531;
		output("MariaDB/MySQL server PID file could not be found at $lMysqldPidFile. Database already\nstopped? (rc=$rc)!\n");
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
		debug('PATH = ' . $lPath . "\n");
		debug('DIR  = ' . $lDir . "\n");
		debug('OLD  = ' . $lOldDir . "\n");
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
		fprint(STDERR, 'Invalid location ' . $location . '.' . " (rc=$rc)" . "\n");
	}
	// some clean-up
	$lPath = preg_replace('/:+/', ':', $lPath);
	$lPath = trim($lPath, ':');
	if ( $lDebug != '' ) {
		debug('PATH = ' . $lPath . "\n");
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
function printStartingDatabases($pConfFile)
// Returns an array of all databases which should be started:
// start = yes
// Keep in sync with printStoppingDatabases
// ----------------------------------------------------------------------
{
	$rc = OK;
	$aDbNames = array();

	$lDebug         = isset($_ENV['MYENV_DEBUG']) ? strval($_ENV['MYENV_DEBUG']) : '';

	list($ret, $aConfiguration) = getConfiguration($pConfFile);
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
function printStoppingDatabases($pConfFile)
// Returns an array of all databases which should be started:
// stop = yes
// Keep in sync with printStartingDatabases
// ----------------------------------------------------------------------
{
	$rc = OK;
	$aDbNames = array();

	$lDebug         = isset($_ENV['MYENV_DEBUG']) ? strval($_ENV['MYENV_DEBUG']) : '';

	list($ret, $aConfiguration) = getConfiguration($pConfFile);
	if ( $lDebug ) {
		var_dump($aConfiguration);
	}
	if ( $ret != OK ) {
		return array($ret, $aDbNames);
	}

	foreach ( $aConfiguration as $db => $parameter ) {

		if ( $db != 'default' ) {
			foreach ( $parameter as $key => $value ) {

				if ( ($key == 'stop') && ((strtoupper($value) == 'YES' ) || ($value == 1)) ) {
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

		output($lne);
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
	list($rc, $output, $aStdout, $aStderr) = my_exec('whoami');
	return $output;
}

// ----------------------------------------------------------------------
function checkMyEnvRequirements()
// ----------------------------------------------------------------------
{
	$ret = OK;
	$lDistribution = 'unknown';

	// Must be done before getOs
	// Happens only on CentOS, not on Ubuntu
	if ( function_exists('posix_uname') === false ) {
		error("Function posix_uname() does not exist. Please install with:");
		error("  shell> sudo dnf install php-process\n");
		$ret = ERR;
	}

	$lOs = getOs();
	if ( $lOs == 'Linux' ) {
		$aDistributionInfo = array();
		$ret = getDistribution($aDistributionInfo);
	}

	// print_r(get_defined_functions());
	// print_r(get_loaded_extensions());

	// Check if environment variables are enabled

	if ( count($_ENV) == 0 ) {
		$vo = ini_get('variables_order');
		# Alternative: shell> sudo echo "variables_order = 'EGPCS'" > /etc/php.d/myenv.ini
		error("Environment variables cannot be used. Please set variables_order = 'EGPCS' in");
		error("PHP cli config file. Current value is: variables_order = " . $vo . "\n");

		switch ($aDistributionInfo['distribution_family']) {
		case 'Redhat':
			$cmd = "sudo vi /etc/php.ini";
			error("shell> " . $cmd . "\n\n");
			break;
		case 'SUSE':
			$cmd = "sudo vi /etc/php7/cli/php.ini";
			error("shell> " . $cmd . "\n\n");
			break;
		case 'Ubuntu':
		case 'Debian':
			$cmd = "sudo vi /etc/php7/cli/php.ini /etc/php/7.?/cli/php.ini";
			error("shell> $cmd\n\n");
			break;
		default:
			error('Unknown distribution: ' . $aDistributionInfo['distribution_family'] . '.');
			error("OS/Distribution " . $lOs . '/' . $aDistributionInfo['distribution_family'] .' is not supported yet.' . "\n");
		}
		return ERR;
	}

	// Check if posix functions are installed

	if ( ! function_exists('posix_kill') ) {
		warn('The function posix_kill is not installed. You cannot stop MySQL/MariaDB');
		output("instances without. Please install the package containing posix_kill:\n\n");

		switch ($aDistributionInfo['distribution_family']) {
		case 'Redhat':
			$cmd = "sudo dnf install " . getPacketList($aDistributionInfo['distribution_family']);
			output("shell> " . $cmd . "\n\n");
			break;
		case 'SUSE':
			$cmd = "sudo zypper install " . getPacketList($aDistributionInfo['distribution_family']);
			output("shell> " . $cmd . "\n\n");
			break;
		case 'Ubuntu':
		case 'Debian':
		case 'Debian GNU/Linux':
			$cmd = "sudo apt install " . getPacketList($aDistributionInfo['distribution_family']);
			output("shell> $cmd\n\n");
			break;
		default:
			$cmd = 'echo "Unknown distribution: ' . $aDistributionInfo['distribution_family'] . '"';
			output("OS/Distribution $lOs/" . $aDistributionInfo['distribution_family'] . " is not yet supported.\n");
		}
		$ret = WARN;
	}
	if ( ! function_exists('posix_getpwuid') ) {
		warn("The function posix_getpwuid is not installed. You cannot continue");
		output("installing without. Please install the package containing posix_getpwuid first:\n\n");

		switch ($aDistributionInfo['distribution_family']) {
		case 'Redhat':
			$cmd = "sudo dnf install " . getPacketList($aDistributionInfo['distribution_family']);
			output("shell> " . $cmd . "\n\n");
			break;
		case 'SUSE':
			$cmd = "sudo zypper install " . getPacketList($aDistributionInfo['distribution_family']);
			output("shell> " . $cmd . "\n\n");
			break;
		case 'Ubuntu':
		case 'Debian':
		case 'Debian GNU/Linux':
			$cmd = "sudo apt install " . getPacketList($aDistributionInfo['distribution_family']);
			output("shell> $cmd\n\n");
			break;
		default:
			$cmd = 'echo "Unknown distribution: ' . $aDistributionInfo['distribution_family'] . '"';
			output("OS/Distribution $lOs/" . $aDistributionInfo['distribution_family'] . " is not yet supported.\n");
		}
		return ERR;
	}

	if ( (! defined('SIG_IGN')) || (! defined('SIGTERM')) ) {
		warn("The constants SIG_IGN and/or SIGTERM are not known. You cannot stop");
		output("MariaDB/MySQL instances without. Please install the package containing these constants\n");
		output("first:\n");
		output("Try to install the package php-pcntl. We will help you in the meanwhile with a\n");
		output("fake...\n\n");

		switch ($aDistributionInfo['distribution_family']) {
		case 'Redhat':
			$cmd = "sudo dnf install " . getPacketList($aDistributionInfo['distribution_family']);
			output("shell> " . $cmd . "\n\n");
			break;
		case 'SUSE':
			$cmd = "sudo zypper install " . getPacketList($aDistributionInfo['distribution_family']);
			output("shell> " . $cmd . "\n\n");
			break;
		case 'Ubuntu':
		case 'Debian':
		case 'Debian GNU/Linux':
			$cmd = "sudo apt install " . getPacketList($aDistributionInfo['distribution_family']);
			output("shell> $cmd\n\n");
			break;
		default:
			$cmd = 'echo "Unknown distribution: ' . $aDistributionInfo['distribution_family'] . '"';
			output("OS/Distribution $lOs/" . $aDistributionInfo['distribution_family'] . " is not yet supported.\n");
		}
		define('SIG_IGN', 0);
		define('SIGTERM', 15);
		$ret = WARN;
	}

	// Check if php-mysqli is installed (for fromdual_bman)

	$ext = 'mysqli';
	if ( ! in_array($ext, get_loaded_extensions()) ) {
		warn("The PHP extension $ext is not installed. You cannot run some tools");
		output("without it. Please install the package containing $ext first:\n\n");

		switch ($aDistributionInfo['distribution_family']) {
		case 'Redhat':
			$cmd = "sudo dnf install " . getPacketList($aDistributionInfo['distribution_family']);
			output("shell> " . $cmd . "\n\n");
			break;
		case 'SUSE':
			$cmd = "sudo zypper install " . getPacketList($aDistributionInfo['distribution_family']);
			output("shell> " . $cmd . "\n\n");
			break;
		case 'Ubuntu':
		case 'Debian':
		case 'Debian GNU/Linux':
			$cmd = "sudo apt install " . getPacketList($aDistributionInfo['distribution_family']);
			output("shell> $cmd\n\n");
			break;
		default:
			$cmd = 'echo "Unknown distribution: ' . $aDistributionInfo['distribution_family'] . '"';
			output("OS/Distribution $lOs/" . $aDistributionInfo['distribution_family'] . " is not yet supported.\n");
		}
		$ret = ERR;
	}

	return $ret;
}

// ----------------------------------------------------------------------
function parseConfigFile($pConfigurationFile, $scanner_mode = INI_SCANNER_NORMAL)
/*

	We have different problems parsing MariaDB/MySQL config files:
	parse_ini_file() seems to be not a good idea to use with MariaDB/MySQL my.cnf files
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
		$msg = 'File ' . "'" . $pConfigurationFile . "'" . ' does not exist.' . " (rc=$rc)";
		return array($rc, $msg);
	}

	if ( ! is_readable($pConfigurationFile) ) {
		$rc = 571;
		$msg = "File $pConfigurationFile is not readable. " . " (rc=$rc)";
		return array($rc, $msg);
	}

	if ( ($aRawConf = file($pConfigurationFile, FILE_IGNORE_NEW_LINES | FILE_SKIP_EMPTY_LINES)) === false ) {
		$rc = 528;
		$msg = "Cannot read file $pConfigurationFile" . " (rc=$rc)";
		return array($rc, $msg);
	}

	$tmp = '/var/tmp' . '/' . posix_getpid() . '.cnf';
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
		$msg = 'Troubles reading configuration file ' . $pConfigurationFile . " (rc=$rc)";
		if ( ! isset($_ENV['MYENV_DEBUG']) ) {
			unlink($tmp);
		}
		return array($rc, $msg);
	}
	if ( ! isset($_ENV['MYENV_DEBUG']) ) {
		unlink($tmp);
	}

	if ( count($aConfig) == 0 ) {
		$rc = 524;
		$msg = 'Config file ' . $pConfigurationFile . ' was empty.' . " (rc=$rc)";
		return array($rc, $msg);
	}

	// Check if config file was formed correctly:
	foreach ( $aConfig as $section => $values ) {
		if ( ! is_array($values) ) {
			$rc = 500;
			$msg = "Malformed section $section is not an array." . " (rc=$rc)";
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
	if ( 'Linux' == $aUname['sysname'] ) {
		$exe = 'md5sum --binary';
	}
	elseif ( 'Darwin' == $aUname['sysname'] ) {
		$exe = 'md5 -r';
	}
	else {
		$rc = 563;
		$msg = 'Operating system ' . $aUname['sysname'] . ' is not yet supported.' . " (rc=$rc)";
		error($msg);
		return array($rc, $md5);
	}

	$cmd = $exe . ' ' . $file;

	if ( ! array_key_exists('simulate', $aOptions) ) {

		list($ret, $output, $aStdout, $aStderr) = my_exec($cmd . ' 2>&1');
		$s = implode("\n", $aStdout);

		if ( OK != $ret ) {
			$rc = 556;
			$msg = "Command $exe failed for file $file" . " (rc=$rc)";
			error($msg);
			foreach ( $aStderr as $line ) {
				error($line);
			}
			return array($rc, $md5);
		}
		if ( preg_match("/^([0-9a-f]{32})\s?.*$/i", $s, $matches) ) {
			$md5 = $matches[1];
		}
		else {
			$rc = 557;
			$msg = 'Error building md5.' . " (rc=$rc)";
			error($msg);
			return array($rc, $md5);
		}
	}
	return array($ret, $md5);
}

// ----------------------------------------------------------------------
function checkForOldStuff($pBasedir)
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
		output("Variable MYSQL_HOME is missing in file $file (rc=$rc).\n");
	}

	if ( ! preg_match('/datadir/i', $line) ) {
		$rc = 562;
		output("Variable MYSQL_HOME is NOT set correctly in file $file (rc=$rc).\n");
		output($line . "\n");
		output("Please change the MYSQL_HOME line from basedir to datadir e.g. with the following\ncommand:\n\n");
		output("shell> sed --in-place '0,/basedir/{s/basedir/datadir/}' $file\n\n");
	}


	// Check that myenv.server template file is same as /etc/init.d/myenv

	$ini = '/etc/init.d/myenv';
	if ( file_exists($ini) ) {
		$tpl = $pBasedir . '/' . 'tpl/myenv.server';

		list($ret, $md5_1) = getMd5sum2($tpl, array());
		list($ret, $md5_2) = getMd5sum2($ini, array());
		if ( $md5_1 != $md5_2 ) {
			$rc = 561;
			output("File $ini is old and possibly not up to date (rc=$rc).\n");
			output("Please fix this with the following command:\n\n");
			output("shell> sudo cp $tpl $ini\n\n");
		}
	}



	// Check that systemd.myenv.unit.template file is same as /etc/systemd/system/myenv.service

	$ini = '/etc/systemd/system/myenv.service';
	if ( file_exists($ini) ) {
		$tplMySQL = $pBasedir . '/' . 'tpl/systemd.myenv.mysql.unit.template';
		$tplMariaDB = $pBasedir . '/' . 'tpl/systemd.myenv.mariadb.unit.template';

		list($ret, $md5_0) = getMd5sum2($tplMariaDB, array());
		list($ret, $md5_1) = getMd5sum2($tplMySQL, array());
		list($ret, $md5_2) = getMd5sum2($ini, array());

		if ( ($md5_1 != $md5_2) && ($md5_0 != $md5_2) ) {
			$rc = 519;
			$msg = 'File ' . $ini . ' is old and possibly not up to date.' . " (rc=$rc)";
			output($msg . "\n");
			output('Please fix this with the following command:' . "\n\n");
			output('shell> sudo cp {' . $tplMySQL . ' | ' . $tplMariaDB . '} ' . $ini . "\n");
			output('shell> sudo systemctl daemon-reload' . "\n\n");
		}
	}

	return $rc;
}

// ----------------------------------------------------------------------
function my_exec($pCmd)
// We have seen some cases, where return values are not returned correctly
// To be really sure also check the stderr!
// Do NOT use for ssh/scp commands remotely! Using exec() directly seems
// to work better here...
// ----------------------------------------------------------------------
{
	$rc = OK;

	$file = sys_get_temp_dir() . '/my_exec.stderr.' . getmypid();
	touch($file);
	unset($aStdout);
	// http://unix.stackexchange.com/questions/87745/what-does-lc-all-c-do
	// The PATH is because it is somehow hidden on Rocky8
	// Caution! ${PATH} should be handed over!!!
	$cmd = 'export LC_ALL=C; ' . $pCmd . ' 2>' . $file;
	$output = exec($cmd, $aStdout, $rc);

	$aStderr = array();
	$fh = fopen($file, 'r');

	if ( $fh !== false ) {

		while ( $line = fgets($fh) ) {
			$line = trim($line);

			// Skip this nasty warning:
			if ( $line == 'Warning: Using a password on the command line interface can be insecure.' ) {
				continue;
			}
			array_push($aStderr, $line);
		}
		fclose($fh);
	}
	// fopen failed
	else {
		$a = error_get_last();
		$msg = 'Cannot read file ' . $file . '. Reason: ' . $a['message'];
		array_push($aStderr, $msg);
	}
	unlink($file);

	if ( '' == $output ) {
		if ( count($aStderr) > 0 ) {
			$output = $aStderr[0];
		}
		else {
			if ( count($aStdout) > 0 ) {
				$output = $aStdout[0];
			}
		}
	}

	return array($rc, $output, $aStdout, $aStderr);
}

// ----------------------------------------------------------------------
function getVersionAndBranchFromDaemon($pBasedir, $pTestString = '')
// Gets version from mariadbd/mysqld --version
// $branch: MariaDB or MySQL
// See test automation
// ----------------------------------------------------------------------
{
	$version = 'unknown';
	$branch  = 'unknown';

	try {

		$aExecutables = array('bin/mariadbd', 'sbin/mariadbd', 'libexec/mariadbd', 'bin/mysqld', 'sbin/mysqld', 'libexec/mysqld');
		$exe = searchExecutable($pBasedir, $aExecutables);

		if ( $exe != '' ) {

			// /usr/sbin/mysqld --version  Ver 5.5.38-0ubuntu0.14.04.1 for debian-linux-gnu on x86_64 ((Ubuntu))
			$cmd = $exe . ' --version';
			debug($cmd);
			list($rc, $output, $aStdout, $aStderr) = my_exec($cmd);
			// var_dump($rc, $output, $aStdout, $aStderr, $cmd);

			if ( OK != $rc ) {
				$rc = 591;
				$msg = $aStderr[0] . '.' . " (rc=$rc)";
				throw new Exception($msg, $rc);
			}


			// Overwrite output for test automation

			if ( $pTestString != '' ) {
				$output = $pTestString;
			}

			// Version
			// Ver 5.7.20
			if ( preg_match("/Ver (\d{1,2}\.\d{1,2}\.\d{1,2})/", $output, $matches) ) {
				$version = $matches[1];
			}
			// If we do not find standard try anything else
			elseif ( preg_match("/(\d{1,2}\.\d{1,2}\.\d{1,2})/", $output, $matches) ) {
				$version = $matches[1];
			}

			// Branch
			// search everything after dirst space!!!
			$output = substr($output, strpos($output, ' '));
			if ( preg_match('/mariadb/i', $output) ) {
				$branch = 'MariaDB';
			}
			elseif ( preg_match('/mysql/i', $output) ) {
				$branch = 'MySQL';
			}

			// If branch is still unknown now we have to guess according to the version numbers
			if ( 'unknown' == $branch ) {
				list($ret, $mr_version) = getMachineReadableVersion($version);
				$branch = 'MariaDB';
				// This is true at least for the next few years...
				// Then we have to do the rule more complicated
				if ( ($mr_version >= '050700') && ($mr_version < '100000') ) {
					$branch = 'MySQL';
				}
			}
		}
	}
	catch ( Exception $e ) {
		error($e->getMessage());
	}

	return array($version, $branch);
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
	// php_uname is one line string, posix_uname is array. So posix is better!!!
	$aUname = posix_uname();
	return $aUname['sysname'];
}

// ---------------------------------------------------------------------
function getDistribution(&$rDistributionInfo = array(), $pTestFile = null)
// Result is based on /etc/os-release. See myenv/tst/tmp/os-release*
// ---------------------------------------------------------------------
{
	$rc = OK;

	try {

		$rDistributionInfo = array(
		  'operating_system'    => getOs()     // Linux, Darwin, ...
		, 'native_distribution' => 'unknown'   // NAME from /etc/os-release
		, 'distribution'        => 'unknown'   // CentOS Linux, Debian GNU/Linux, Redhat Enterprise Linux, Rocky Linux, Ubuntu, SUSE, openSUSE...
		, 'distribution_family' => 'unknown'   // Debian, Ubuntu, Redhat, SUSE, ...
		, 'full_version'        => '0.0'       // VERSION_ID from /etc/os-release: 10, 8.5, 22.04
		, 'major_version'       => '0'         // first part from full_version: 22
		, 'minor_version'       => '0'         // second part from full_version: 04
		);

		// For example Darwin
		if ( $rDistributionInfo['operating_system'] != 'Linux' ) {
			$rc = 592;
			$msg = 'O/S ' . $rDistributionInfo['operating_system'] . ' is not supported yet.' . " (rc=$rc)" . "\n";
			throw new Exception($msg, $rc);
		}

		// For automated function testing
		$lOsRelease = '/etc/os-release';
		if ( ! is_null($pTestFile) ) {
			$lOsRelease = $pTestFile;
		}

		// Check if file is readable
		if ( ! file_exists($lOsRelease) || ! is_readable($lOsRelease) ) {
			$rc = 515;
			$msg = 'Cannot open file ' . $lOsRelease . " (rc=$rc)" . "\n";
			throw new Exception($msg, $rc);
		}

		// Read file
		if ( $fh = fopen($lOsRelease, 'r') ) {

			while ( ($buffer = fgets($fh, 256)) !== false ) {

				$buffer = trim($buffer);

				if ( '' == $buffer ) {
					continue;
				}

				list($key, $value) = explode('=', $buffer);
				$value = trim($value, '"');

				switch ($key) {
					case 'NAME':
						$rDistributionInfo['native_distribution'] = $value;
						break;
					case 'VERSION_ID':
						$rDistributionInfo['full_version'] = $value;
						break;
				}
			}
			fclose($fh);
		}

		// Normalize data
		switch ($rDistributionInfo['native_distribution']) {
		case 'openSUSE':
		case 'openSUSE Leap':
		case 'openSUSE project':
			$rDistributionInfo['distribution']        = 'openSUSE';
			$rDistributionInfo['distribution_family'] = 'SUSE';
			break;
		case 'SUSE':
		case 'SUSE LINUX':
			$rDistributionInfo['distribution']        = 'SUSE';
			$rDistributionInfo['distribution_family'] = 'SUSE';
			break;
		case 'Debian GNU/Linux':
		case 'Debian':
			$rDistributionInfo['distribution']        = 'Debian GNU/Linux';
			$rDistributionInfo['distribution_family'] = 'Debian';
			break;
		case 'Ubuntu':
			$rDistributionInfo['distribution']        = 'Ubuntu';
			$rDistributionInfo['distribution_family'] = 'Ubuntu';
			break;
		case 'Fedora':
			$rDistributionInfo['distribution']        = 'Fedora';
			$rDistributionInfo['distribution_family'] = 'Redhat';
			break;
		case 'CentOS':
		case 'centos':
		case 'CentOS Linux':
			$rDistributionInfo['distribution']        = 'CentOS Linux';
			$rDistributionInfo['distribution_family'] = 'Redhat';
			break;
		case 'RedHat':
		case 'RedHatEnterpriseServer':
		case 'Red Hat Enterprise Linux Server':
		case 'Red Hat Enterprise Linux':
			$rDistributionInfo['distribution']        = 'Redhat Enterprise Linux';
			$rDistributionInfo['distribution_family'] = 'Redhat';
			break;
		case 'Rocky Linux':
		case 'rocky':
			$rDistributionInfo['distribution']        = 'Rocky Linux';
			$rDistributionInfo['distribution_family'] = 'Redhat';
			break;
		case 'Oracle Linux Server':
			$rDistributionInfo['distribution']        = 'Oracle Linux Server';
			$rDistributionInfo['distribution_family'] = 'Redhat';
			break;
		case 'CloudLinux':
		case 'cloundlinux':
			$rDistributionInfo['distribution']        = 'CloudLinux';
			$rDistributionInfo['distribution_family'] = 'Redhat';
			break;
		case 'AlmaLinux':
		case 'almalinux':
			$rDistributionInfo['distribution']        = 'AlmaLinux';
			$rDistributionInfo['distribution_family'] = 'Redhat';
			break;
		default:
			$msg = 'Distribution ' . $rDistributionInfo['native_distribution'] . ' is not supported.' . "\n";
			throw new Exception($msg, $rc);
		}

		$v = explode('.', $rDistributionInfo['full_version']);
		$rDistributionInfo['major_version'] = $v[0];
		$rDistributionInfo['minor_version'] = array_key_exists(1, $v) ? $v[1] : '';
	}
	catch ( Exception $e ) {
		output($e->getMessage());
	}

	return $rc;
}

// ---------------------------------------------------------------------
function getPacketList($pDistributionFamily)
// ---------------------------------------------------------------------
{
	$lList = 'unknown distribution ' . $pDistributionFamily;

	switch ($pDistributionFamily) {
	case 'Redhat':
		$lList = 'php-cli php-process php-pcntl php-mysqli';
		break;
	case 'SUSE':
		$lList = 'php7 php7-cli php7-posix php7-pcntl php7-mysqli php-sockets';
		break;
	case 'Ubuntu':
	case 'Debian':
		// taken out: php-posix php-pcntl
		$lList = 'php php-mysql';
		break;
	default:
		$lList = 'unknown distribution ' . $pDistributionFamily;
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
	// TODO: Use parse_url instead!

	// Connect string style:
	// user:password@host:port

	// http://en.wikipedia.org/wiki/URI_scheme
	// foo://user:password@example.com:8042

	// Use the last @ for splitting host and user
	// Use the first / or : for splitting user and password
	// Characters like : @ and / are allowed in password!
	// Allowed combinations for user are:
	// user/password (deprecated)
	// user:password
	// user
	// Allowed combinations for host are:
	// host:port
	// host

	$aTarget = array();
	$rc = OK;


	// Split first into user and host part: @ is mandatory

	$pos = strrpos($pConnectString, '@');

	if ( $pos == 0 ) {
		$rc = 583;
		// This is/was a security issue! Removed: $pConnectString
		$msg = 'Connect string does not match pattern: user:password@host:port.' . " (rc=$rc)";
		error($msg);
		return array($rc, $aTarget);
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
			$aTarget['socket'] = '/run/mysqld/mysql.sock';
		}
		else {

			if ( intval($h[1]) == 0 ) {
				$aTarget['port']   = 3306;
				$aTarget['socket'] = $h[1];
			}
			else {
				$aTarget['port']   = intval($h[1]);
				$aTarget['socket'] = '/run/mysqld/mysql.sock';
			}
		}

		if ( $aTarget['port'] == 0 ) {
			$rc = 582;
			$msg = 'Port ' . $h[1] . ' is zero or not an integer.' . " (ret=$rc)";
			error($msg);
			$aTarget['port'] = '';
			return array($rc, $aTarget);
		}
	}

	return array($rc, $aTarget);
}

// -------------------------------------------------------------------
function getMachineReadableVersion($pVersion)
// -------------------------------------------------------------------
{
	$rc = OK;
	$mr_version = '';

	$pattern='/(\d+)\.(\d+).(\d+)/';
	preg_match($pattern, $pVersion, $matches);
	$mr_version = sprintf("%02d%02d%02d", $matches[1], $matches[2], $matches[3]);

	return array($rc, $mr_version);
}

// ----------------------------------------------------------------------
function extractBranch($pVersionComment, $pVersion = '')
// We found out that version_comment is not sufficient in all case!
// We also need version to know it for sure!
// SHOW GLOBAL VARIABLES LIKE 'version%';
// ----------------------------------------------------------------------
{
	$branch = 'unknown';

	/*
		Expected patterns for version_comment:

		MySQL Community Server (GPL)
		MariaDB Server
		mariadb.org binary distribution, wsrep_25.10.r4144
		Percona Server with XtraDB (GPL), Release rel64.2, Revision 569
		Homebrew
		(Ubuntu)
		Ubuntu 22.04
		Debian 11
	*/

	if ( preg_match("/^\(?([a-z]+)\)?/i", $pVersionComment, $matches) ) {

		switch ( strtolower($matches[1]) ) {
		case 'mysql':
			$branch = 'MySQL';
			break;
		// MySQL 5.7 from Homebrew on MacOS
		case 'homebrew':
			$branch = 'MySQL';
			break;
		case 'mariadb':
			$branch = 'MariaDB';
			break;
		case 'percona':
			$branch = 'Percona';
			break;
		case 'ubuntu':
		case 'debian':

			// Really hard guessing here what it is:
			// version = 5.7.41-0ubuntu0.18.04.1-log

			if ( stripos($pVersion, 'MariaDB') !== false ) {
				$branch = 'MariaDB';
			}
			elseif ( stripos($pVersion, 'MySQL') !== false ) {
				$branch = 'MySQL';
			}

			// 5.7 or 8.0
			if ( in_array(substr($pVersion, 0, 3), array('5.7', '8.0')) ) {
				$branch = 'MySQL';
			}

			break;
		}
	}

	return $branch;
}

// ---------------------------------------------------------------------
function recursivelyRemoveDirectory($dir)
// ---------------------------------------------------------------------
{
	$rc = OK;

	$files = glob($dir . '/*');
	foreach ( $files as $file ) {
		is_dir($file) ? recursivelyRemoveDirectory($file) : unlink($file);
	}
	rmdir($dir);

	return $rc;
}

// ---------------------------------------------------------------------
function getInstanceInfo($pVersion, $pVersionComment)
/*
  SHOW GLOBAL VARIABLES WHERE variable_name IN ('version_comment', 'version');
  version         | 10.3.3-MariaDB
  version_comment | MariaDB Server
  version         | 5.7.21-log
  version_comment | MySQL Community Server (GPL)
*/
// ---------------------------------------------------------------------
{
	$rc = OK;
	$aInstanceInfo = array();

	// Version

	if ( preg_match("/(\d{1,2}\.\d{1,2}\.\d{1,2}).*/", $pVersion, $matches) ) {
		$aInstanceInfo['version'] = $matches[1];
	}
	else {
		$rc = 578;
	}


	// MR Version

	list($rc, $aInstanceInfo['mr_version']) = getMachineReadableVersion($aInstanceInfo['version']);


	// Version comment

	$aInstanceInfo['branch'] = extractBranch($pVersionComment, $pVersion);

	return array($rc, $aInstanceInfo);
}

// ----------------------------------------------------------------------
function searchExecutable($pBasedir, $aExecutables)
// First matches
// ----------------------------------------------------------------------
{
	$executable = '';

	foreach ( $aExecutables as $e ) {

		// /usr/sbin/mysqld
		$exe = $pBasedir . '/' . $e;
		if ( is_executable($exe) ) {
			$executable = $exe;
			break;
		}
	}

	return $executable;
}

// ---------------------------------------------------------------------
function formatTime($pTimeInSeconds)
// ---------------------------------------------------------------------
{
	$str = '';

	$sec = $pTimeInSeconds % 60;
	$r = $pTimeInSeconds / 60;

	$min = (int)$r % 60;
	$r = $r / 60;

	$hrs = (int)$r % 24;
	$day = intval($r / 24);

	$str = $day . 'd ' . $hrs . 'h ' . $min . "' " . $sec . '"';

	return $str;
}

// ---------------------------------------------------------------------
function substituteDirectoryInPath($pPath, $pBasedir, $pOld, $pNew)
// ---------------------------------------------------------------------
{

	// Delete myenv PATH
	$new = $pBasedir . '/' . 'bin';
	debug('delete PATH ' . $new);
	$pPath = deleteDirectoryFromPath($pPath, $new);

	// Add myenv PATH to the left new
	if ( is_dir($new) ) {
		debug('add PATH left ' . $new);
		$pPath = addDirectoryToPath($pPath, $new, '', 'left');
	}

	// Add instance basedir
	if ( $pNew != '' ) {

		$old = '';

		foreach ( array('scripts', 'libexec', 'bin') as $dir ) {

			// Delete old
			if ( $pOld != '' ) {
				$old = $pOld . '/' . $dir;
				if ( $old != '/usr/bin' ) {
					debug('delete PATH ' . $old);
					$pPath = deleteDirectoryFromPath($pPath, $old);
				}
			}

			// Delete new as well to avoid redundancy
			$new = $pNew . '/'. $dir;
			if ( $old != '/usr/bin' ) {
				debug('delete PATH ' . $old);
				$pPath = deleteDirectoryFromPath($pPath, $new);
			}

			// Add new
			if ( is_dir($new) ) {
				// Clean-up to be sure
				debug('add PATH left ' . $new);
				$pPath = addDirectoryToPath($pPath, $new, '', 'left');
			}
		}
	}

	// Add myenv utl path to the right
	$new = $pBasedir . '/' . 'utl';
	if ( is_dir($new) ) {
		debug('add PATH right ' . $new);
		$pPath = addDirectoryToPath($pPath, $new, '', 'right');
	}

	// Remove redundancies
	$pPath = implode(':', array_unique(explode(':', $pPath)));

	return $pPath;
}

?>
