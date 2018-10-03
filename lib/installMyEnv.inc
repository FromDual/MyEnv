<?php

/*

	getInstanceBlackList()
	copyFromTemplate($pConfFileTemplate, $pConfFile)
	addInstance()
	changeInstance($pInstance)
	getInstances($aConfiguration)
	installInstance($aConfiguration)
	askForMyCnf($aConfiguration)
	askForSocket($aGlobalConfiguration, $aConfiguration, $tSocket = '')
	askForBasedir($aConfiguration)
	downloadChunkedFile($url, $outfile)
	askForDatadir($aConfiguration)
	replaceInitScripts($tKey = '')
	addMyEnvInitScript($pInitScript, $tKey = '')
	addMyEnvHook($pMyenvHook, $tKey = '')
	addMyEnvProfile($pHomeDir, $tKey = '')
	createHomeDir($pHomeDir, $tKey = '')
	checkHomeDir(&$pHomeDir, $pUser, $tKey = '')
	writeConfigurationFile($aConfiguration, $pConfFile)
	deleteInstance2($aInstance)
	deleteInstance($pInstance)
	changeInstance2($aInstance)
	copyConfFileFromTemplate($pConfFile, $pConfFileTemplate, $tKey = '')
	useOrCopyConfFile($pConfFile, $pConfFileTemplate)
	checkHomeDirOwner($pEtcMyenv, $pOwner)
	checkMyEnvDir($pEtcMyEnv, $pUser)
	checkOsUser($pUser)
	checkForRootUser($pCurrentUser)
	checkForNonMySQLUser($pCurrentUser)
	askForStage($aConfiguration)
	printUsage()
	checkOptions($aArgv, $aOptions, $pConfFile)
	deleteInstanceAutomatized($aOptions, $pConfFile)
	askForInstancedir($aConfiguration)
	askForAngel($aConfiguration)
	askForCgroups($aConfiguration)
	askForPort($aInstanceConfiguration, $tPort = '')
	addInstanceAutomatized($aOptions, $pConfFile)

*/

// ---------------------------------------------------------------------
function getInstanceBlackList()
// ---------------------------------------------------------------------
{
	return array('mysql', 'mysqld', 'default');
}

// ---------------------------------------------------------------------
function copyFromTemplate($pConfFileTemplate, $pConfFile)
// ---------------------------------------------------------------------
{
	$rc = OK;

	// Copy template
	output("Copy $pConfFileTemplate to $pConfFile\n");
	if ( ! @copy($pConfFileTemplate, $pConfFile) ) {

		$rc = 716;
		$a = error_get_last();
		$msg = "Cannot copy template $pConfFileTemplate to $pConfFile. Reason: " . $a['message'] . " (rc=$rc).";
		output("$msg\n");
		output("Aborting...\n");
		return $rc;
	}

	return $rc;
}

// ---------------------------------------------------------------------
function addInstance()
// ---------------------------------------------------------------------
{
	global $aConfiguration;
	$rc = OK;

	// todo: check if default already exists!
	$default = 'mysqld1';
// 	output("\n");
	output("You have to enter a new instance name. An instance name can consist of 0-9, a-z, A-Z, - and _.\n");
	output("\n");
	output("Examples: test, prod, dev, [$default] mysqld-3306\n");
	output("\n");

	$instance = readline("Enter a new instance name: ");
	if ( $instance == '' ) {
		$instance = $default;
	}

	// Check if instance name contains '.' character
	if ( strstr($instance, '.') !== false ) {
		$rc = 703;
		$msg = "Instance name $instance cannot contain '.' caracter." . " (rc=$rc)" . "\n";
		error($msg);
		return $rc;
	}


	// Check for existing instance name here!
	foreach ( $aConfiguration as $instances ) {
		if ( $instances == $instance ) {
			$rc = 717;
			error("Instance name $instance already exists (rc=$rc).\n");
			return $rc;
		}
	}

	// Check if instance name is on black list:
	$aBlackList = getInstanceBlackList();
	foreach ( $aBlackList as $name ) {
		if ( $name == $instance ) {
			$rc = 718;
			error("Instance name $instance is on blacklist (rc=$rc).");
			readline("Press Enter to choose an other one...");
			return $rc;
		}
	}

	$aConfiguration[$instance]               = array();
	$aConfiguration[$instance]['start']      = 'yes';
	$aConfiguration[$instance]['stop']       = 'yes';
	$aConfiguration[$instance]['hideschema'] = 'mysql,performance_schema,lost+found,sys';
	$aConfiguration[$instance]['type']       = 'mysqld';
	$rc = changeInstance($instance);

	return $rc;
}

// ---------------------------------------------------------------------
function changeInstance($pInstance)
// ---------------------------------------------------------------------
{
	$rc = OK;

	// todo: Bug: This is wrong should not be $aConfiguration but only the raw section!?!
	global $aConfiguration;
	$lHomeDir = $_ENV['HOME'];

	if ( ! in_array($pInstance, getInstances($aConfiguration)) ) {
		$rc = 701;
		error("Instance $pInstance does NOT exist in configuration file (rc=$rc).");
		return $rc;
	}

	output("\n");
	output("Changing instance $pInstance:\n");
	// Just to make sure it is set:
	if ( ! isset($aConfiguration[$pInstance]['name']) ) {
		$aConfiguration[$pInstance]['name'] = $pInstance;
	}


	// Ask for basedir

	$basedir = askForBasedir($aConfiguration[$pInstance]);
	$aConfiguration[$pInstance]['basedir'] = $basedir;


	// Install binaries as well

	if ( $aConfiguration[$pInstance]['basedir'] == '/usr' ) {
		output("\n");
		output("You have choosen the default package location of MariaDB/MySQL basedir.\n");
		output("Please install MariaDB/MySQL with your package manager if not done yet.\n");
	}
	elseif ( (is_dir($aConfiguration[$pInstance]['basedir'])
	  && (count(glob($aConfiguration[$pInstance]['basedir'] . '/*')) == 0))
	    || ( ! file_exists($aConfiguration[$pInstance]['basedir']))
	   ) {

		output("\n");
		$question = 'Shall I download and install MariaDB/MySQL binaries under ' . $aConfiguration[$pInstance]['basedir'] . ' (Y/n)? ';
		$key = answerQuestion($question, array('y', 'n'), 'y');
		if ( $key == 'y' ) {

			$lProductDir = dirname($aConfiguration[$pInstance]['basedir']);
			$lTmpDir     = $_ENV['HOME'] . '/' . 'tmp';


			// Download URL

			output("\n");
			output("For example:\n");
			$aSources = array(
			  'http://dev.mysql.com/get/Downloads/MySQL-5.5/mysql-5.5.40-linux2.6-x86_64.tar.gz/from/http://cdn.mysql.com/'
			, 'http://dev.mysql.com/get/Downloads/MySQL-5.6/mysql-5.6.21-linux-glibc2.5-x86_64.tar.gz/from/http://cdn.mysql.com/'
			, 'http://www.percona.com/redir/downloads/Percona-Server-5.5/Percona-Server-5.5.39-36.0/binary/tarball/Percona-Server-5.5.39-rel36.0-697.Linux.x86_64.tar.gz'
			, 'http://www.percona.com/redir/downloads/Percona-Server-5.6/Percona-Server-5.6.20-68.0/binary/tarball/Percona-Server-5.6.20-rel68.0-656.Linux.x86_64.tar.gz'
			, 'https://downloads.mariadb.org/f/mariadb-5.5.39/bintar-centos5-amd64/mariadb-5.5.39-linux-x86_64.tar.gz/from/http%3A/ftp.ddg.lth.se/mariadb?server'
			, 'https://downloads.mariadb.org/f/mariadb-10.0.14/bintar-linux-x86_64/mariadb-10.0.14-linux-x86_64.tar.gz/from/http%3A/ftp.ddg.lth.se/mariadb?server'
			);
			foreach ( $aSources as $source ) {
				output('  ' . $source . "\n");
			}
			output("\n");
			do {
				$url = readline("Please enter the URL for the binary-tarball (Linux-Generic Compressed TAR) to download: ");
			} while ( $url == '');

			$parts = parse_url($url);

			output("\n");
			$lVersion = extractVersion($parts['path']);
			if ( $lVersion == '' ) {
				output("Version cannot be determined.\n");
			}
			else {
				output("The version is: $lVersion\n");
			}
			// todo: compare download with platform

			// todo: check if wget or curl is available
			// downloadChunkedFile($url, $outfile)

			// download
			output("\nPlease be patient, this can take a while...\n");
			$command = "wget --trust-server-names --directory-prefix=$lTmpDir $url 2>&1";
			output($command . "\n");
			list($ret, $output, $stdout, $stderr) = my_exec($command);

			if ( $ret != 0 ) {
				$rc = 704;
				error("Download failed. Please try again manually (rc=$rc).");
				error(print_r($stderr, true));
			}

			// get binary path from stdout!
			// Saving to: /home/myenv/tmp/mysql-5.1.41-linux-i686-glibc23.tar.gz
			if ( ( $rc == OK ) && preg_match('|.*/(.*\.tar\.gz)|', $parts['path'], $matches) ) {
				$lTarBall = $matches[1];
				output("Tar-ball file is: $lTarBall\n");
			}
			else {
				$rc = 705;
				error("Cannot extract filename from output. Please try manually (rc=$rc).");
				error("tar xf $lTmpDir" . '/' . 'mysql-5.8.99.tar.gz' . " --directory $lProductDir");
			}

			// todo: check checksum

			// extract binary
			if ( $rc == OK ) {
				$command = "tar xf $lTmpDir" . '/' . "$lTarBall --directory $lProductDir";
				output($command . "\n");
				list($ret, $output, $stdout, $stderr) = my_exec($command);

				$lBinaryDir = substr($lTarBall, 0, strlen($lTarBall)-strlen('.tar.gz'));

				// create link
				// symlink('/tmp/oli/product/mysql-5.8.99_x86_64', 'mysql-5.8.99');"

				$src = $lProductDir . '/' . $lBinaryDir;
				$lOs = getOs();
				if ( ($lOs == 'Linux') || ($lOs == 'Darwin') ) {
					chdir($lProductDir);
					// old: $lnk = 'mysql-' . $lVersion;
					$lnk = basename($aConfiguration[$pInstance]['basedir']);
					output("cd $lProductDir; ln -s " . $src . ' ' . $lnk . "\n");
				}
				// Windows
				else {
					// relative links do NOT work on Windows!
					// old: $lnk = $lProductDir . '/' . 'mysql-' . $lVersion;
					$lnk = $lProductDir . '/' . basename($aConfiguration[$pInstance]['basedir']);
					output('mklink /D ' . $lnk . ' ' . $src . "\n");
				}

				if ( is_file($lnk) === false ) {
					if ( @symlink($src, $lnk) === false ) {
						$rc = 723;
						$error = error_get_last();
						$msg = $error['message'];
						error("$msg (rc=$rc).");
					}
				}
				else {
					$rc = 724;
					$msg = 'Symlink ' . $lnk . ' already exists';
					error("$msg (rc=$rc).");
				}
			}
		}
	}
	else {
		output($aConfiguration[$pInstance]['basedir'] . " seems to exist and has already some files in it.\n");
		output("\n");
		output("Omitting installation of binary tar ball.\n");
	}


	// Ask for instancedir

	$instancedir = askForInstancedir($aConfiguration[$pInstance]);
	$aConfiguration[$pInstance]['instancedir'] = $instancedir;


	// Ask for datadir

	$datadir = askForDatadir($aConfiguration[$pInstance]);
	$aConfiguration[$pInstance]['datadir'] = $datadir;


	// Install instance

	$aConfiguration[$pInstance]['user'] = getCurrentUser();
	$ret = installInstance($aConfiguration[$pInstance]);


	// Ask for port

	$port = askForPort($aConfiguration[$pInstance]);
	$aConfiguration[$pInstance]['port'] = $port;


	// Ask for socket

	$socket = askForSocket($aConfiguration, $aConfiguration[$pInstance]);
	$aConfiguration[$pInstance]['socket'] = $socket;


	// Ask for my.cnf

	list($rc, $mycnf) = askForMyCnf($aConfiguration[$pInstance]);
	if ( $rc != OK ) {
		$aConfiguration[$pInstance]['my.cnf'] = '';
		$msg = 'Creating instance configuration failed. Aborting...';
		error($msg);
		return $rc;
	}
	$aConfiguration[$pInstance]['my.cnf'] = $mycnf;


	// Ask for angel

	$angel = askForAngel($aConfiguration[$pInstance]);
	$aConfiguration[$pInstance]['angel'] = $angel;


	// Ask for cgroups

	$ret = askForCgroups($aConfiguration[$pInstance]);
	if ( $ret == 'yes' ) {
		$aConfiguration[$pInstance]['cgroups'] = 'yes';
	}
	else {
		$aConfiguration[$pInstance]['cgroups'] = 'no';
	}


	// Ask for stage

	$stage = askForStage($aConfiguration[$pInstance]);
	$aConfiguration[$pInstance]['stage'] = $stage;

	// Do cleanup from raw config again because of the inheritence problem of the
	// default section
	// todo: this is just a workaround and should be fixed later!!!

	// todo: This is also pretty evil and should be removed by ONE config asap!
	$lConfFile         = '/etc/myenv/myenv.conf';
	list($ret, $aRawConfiguration) = parseConfigFile($lConfFile, INI_SCANNER_RAW);

	if ( isset($aRawConfiguration[$pInstance]) ) {
		if ( ! array_key_exists('angel', $aRawConfiguration[$pInstance]) ) {
			unset($aConfiguration[$pInstance]['angel']);
		}
	}
	unset($aConfiguration[$pInstance]['default']);

	return $rc;
}

// ---------------------------------------------------------------------
function getInstances($aConfiguration)
// ---------------------------------------------------------------------
{
	$aInstance = array();

	foreach ( $aConfiguration as $key => $value ) {

		if ( $key != 'default' ) {
			array_push($aInstance, $key);
		}
	}
	return $aInstance;
}

// ---------------------------------------------------------------------
function installInstance($aConfiguration)
// ---------------------------------------------------------------------
{
	$rc = OK;

	// Check if datadir exists and is empty directory
	if ( is_dir($aConfiguration['datadir']) && (count(glob($aConfiguration['datadir'] . '/mysql/*')) == 0) ) {

		output("\n");
		$question = "Shall I install a MariaDB/MySQL instance under " . $aConfiguration['datadir'] . " (Y/n)? ";
		$key = answerQuestion($question, array('y', 'n'), 'y');
		if ( $key == 'y' ) {

			// Check if libaio is installed
			$cmd = '/sbin/ldconfig -p | grep libaio';
			list($ret, $output, $stdout, $stderr) = my_exec($cmd);
			if ( $ret != OK ) {
				$rc = 725;
				$msg = "\nCommand $cmd failed. Possibly library libaio / libaio1 is missing. Please install libaio first:\n";
				output($msg);

				$lOs = getOs();
				$lDistribution = 'unknown';
				if ( $lOs == 'Linux' ) {
					$lDistribution = getDistribution();
				}

				switch ($lDistribution) {
				case 'CentOS':
				case 'RedHat':
				case 'Fedora':
				case 'RedHatEnterpriseServer':
					$cmd = "sudo yum install libaio";
					output("  shell> $cmd\n\n");
					break;
				case 'SUSE':
				case 'SUSE LINUX':
				case 'openSUSE project':
					$cmd = "sudo zypper install libaio";
					output("  shell> $cmd\n\n");
					break;
				case 'Ubuntu':
				case 'Debian':
					$cmd = "sudo apt-get install libaio1";
					output("  shell> $cmd\n\n");
					break;
				default:
					$cmd = 'echo "Unknown distribution: ' . $lDistribution . '"';
					output("OS/Distribution $lOs/$lDistribution is not yet supported.\n");
				}

				output("\n");
				$key = answerQuestion('Done yourself already, Skip this step, Abort installation or Installer shall do it [D/s/a/i]? ', array('d', 's', 'a', 'i'), 'd');

				switch ( $key ) {
				case 'a':
					$rc = 728;
					output("Abort installing instance... (rc=$rc)\n");
					return $rc;
					break;
				case 'd':
					break;
				case 's':
					break;
				case 'i':
					output("$cmd\n");
					list($ret, $output, $stdout, $stderr) = my_exec($cmd);
					break;
				default:
				}
			}


			// Check /var/run/mysqld

			$run = '/var/run/mysqld';
			if ( ! is_dir($run) ) {
			  output("\n");
				output("Directory $run does not exist. Please create it as follows:\n");
				$cmd = "sudo mkdir $run ; sudo chown " . $aConfiguration['user'] . ": $run";
				output("  shell> $cmd\n");
				output("\n");
				$key = answerQuestion('Done yourself already, Skip this step, Abort installation or Installer shall do it [D/s/a/i]? ', array('d', 's', 'a', 'i'), 'd');

				// Do NOT use switch here because of break!
				if ( $key == 'a' ) {
					$rc = 741;
					output("Abort installing instance... (rc=$rc)\n");
					return $rc;
				}
				elseif ( $key == 'd' ) {
					null;
				}
				elseif ( $key == 's' ) {
					null;
				}
				elseif ( $key == 'i' ) {
					output("$cmd\n");
					list($ret, $output, $stdout, $stderr) = my_exec($cmd);
				}
			}

			if ( $key != 's' ) {
				$uid = fileowner($run);
				$aUser = posix_getpwuid($uid);
				if ( $aUser['name'] != $aConfiguration['user'] ) {
					error("User " . $aUser['name'] . " of directory $run does not match database user " . $aConfiguration['user'] . " this will cause troubles later.");
				}
			}


			// Get version of MySQL/MariaDB

			$version = getVersionFromMysqld($aConfiguration['basedir']);
			if ( $version == 'unknown' ) {
				$rc = 736;
				$msg = 'Cannot get version from ' . "mysqld --version. (rc=$rc)";
				error("$msg\n");
				return $rc;
			}

			list($ret, $mr_version) = getMachineReadableVersion($version);
			if ( ($ret != OK) || ($mr_version == '') ) {
				$rc = 737;
				$msg = "Cannot get machine readable version from $mr_version. (rc=$rc)";
				error("$msg\n");
				return $rc;
			}


			// Determine which binary to user
			//        - 050705 -> MySQL + MariaDB 5.7.5
			// 050706 - 099999 -> MySQL 5.7.6 - 9.99.99
			// 100000 -        -> MariaDB 10.0.0

			// old style mysqld_install_db
			if ( ($mr_version <= '050705') || ($mr_version >= '100000') ) {
				$inst = 'mysql_install_db';
				$sec  = '';
			}
			// new style mysqld --initialize-insecure
			else {
				$inst = 'mysqld';
				$sec  = '--initialize-insecure';
			}

			// Check mysql_install_db/mysqld

			if ( file_exists($aConfiguration['basedir'] . '/scripts/' . $inst) ) {
				$bin = $aConfiguration['basedir'] . '/scripts/' . $inst;
			}
			else {
				$bin = $aConfiguration['basedir'] . '/bin/' . $inst;
			}

			if ( file_exists($bin) && is_executable($bin) ) {
				output("Installing MariaDB/MySQL instance. Please be patient, this can take a while...\n\n");

				// Check if $basedir/my.cnf exists already
				// if not delete it afterwards again.
				$lMyCnfExisted = false;
				if ( is_file($aConfiguration['basedir'] . '/my.cnf') ) {
					$lMyCnfExisted = true;
				}

				$cmd = $bin . " --no-defaults $sec --user=" . $aConfiguration['user'] . " --datadir=" . $aConfiguration['datadir'] . " --basedir=" . $aConfiguration['basedir'];
				output("$cmd\n");
				// Here it seems like mysql_install_db does not return return code correctly!!!
				list($ret, $output, $stdout, $stderr) = my_exec($cmd);
				// var_dump($ret, $output, $stdout, $stderr);
				if ( $ret != OK ) {
					foreach ( $stdout as $line ) {
						error($line);
						// Do not exit here!
					}
					foreach ( $stderr as $line ) {
						error($line);
						// Do not exit here!
					}

					// Check if SELinux or AppArmor is installed

					output("Please check if advanced Linux security is enabled.\n");
					$lOs = getOs();
					if ( $lOs == 'Linux' ) {

						$distribution = getDistribution();
						if ( ($distribution == 'CentOS')
							|| ($distribution == 'RedHat')
							|| ($distribution == 'Fedora')
							) {
							output("\nINFO: SELinux is in status (sudo sestatus):\n");
							$cmd = "/usr/sbin/sestatus | egrep 'status|mode'";
							list($ret, $output, $stdout, $stderr) = my_exec($cmd);
							output("\n");
						}
						elseif ( ($distribution == 'Ubuntu')
									|| ($distribution == 'Debian')
									|| ($distribution == 'SUSE')
									|| ($distribution == 'openSUSE project')
									) {
							$cmd = '/usr/sbin/apparmor_status';
							if ( is_executable($cmd) ) {
								output("\nINFO: AppArmor is in status (sudo apparmor_status):\n");
								list($ret, $output, $stdout, $stderr) = my_exec($cmd);
								output("\n");
							}
						}
						else {
							output("I do not know and support distribution: $distribution\n");
						}
					}

					if ( $distribution == 'openSUSE project' ) {
						output("  INFO: There is a bug in some SuSE installers (FATAL ERROR: Could not find /fill_help_tables.sql).\n");
					}
				}
				// We have seen cases where rc was 0 but error occured
				if ( ($ret == OK) && (count($stderr) > 0) ) {
					foreach ( $stderr as $line ) {
						if ( stristr($line, 'error') !== false ) {
							error($line);
						}
						// Do not exit here!
					}
				}

				// Delete my.cnf again if it did not exist before
				if ( $lMyCnfExisted === false ) {
				  $mycnf = $aConfiguration['basedir'] . '/my.cnf';
					if ( is_file($mycnf) ) {
						output("  INFO: Deleted file $mycnf again which was created by mysql_install_db.\n");
						unlink($mycnf);
					}
				}
			}
			else {
				$rc = 727;
				error("File $bin does not exists or is not executable (rc=$rc).\n");
				error("Omitting installation of mysql database.\n");
				return $rc;
			}
		}
	}
	else {
		output($aConfiguration['datadir'] . " seems to exist and has already some files in it.\n");
		output("Omitting installation of mysql database.\n");
		// No error, no return here!
	}

	return $rc;
}

// ---------------------------------------------------------------------
function askForMyCnf($aConfiguration)
// ---------------------------------------------------------------------
{
	$rc = OK;

	$aMyCnfCandidates = array(
		'/etc/my.cnf'
	, '/etc/mysql/my.cnf'
	, $aConfiguration['datadir'] . '/my-' . $aConfiguration['port'] . '.cnf'
	, $aConfiguration['datadir'] . '/my.cnf'
	, $aConfiguration['instancedir'] . '/' . 'etc' . '/my.cnf'
	);

	// Default my.cnf:
	if ( isset($aConfiguration['my.cnf']) && ($aConfiguration['my.cnf'] == '') ) {
		$default = $aConfiguration['instancedir'] . '/' . 'etc' . '/my.cnf';
	}
	elseif ( isset($aConfiguration['my.cnf']) && ($aConfiguration['my.cnf'] != '') ) {
		$default = $aConfiguration['my.cnf'];
		array_push($aMyCnfCandidates, $default);
	}
	else {
		$default = $aConfiguration['instancedir'] . '/' . 'etc' . '/my.cnf';
	}

	$aMyCnfCandidates = array_unique($aMyCnfCandidates);

	output("\n");
	output("Choose location of my.cnf:\n");
	output("Example: ");

	foreach ( $aMyCnfCandidates as $mycnf ) {
		if ( $mycnf == $default ) {
			output('[' . $mycnf . ']');
		}
		else {
			output($mycnf);
		}
		output(' ');
	}
	output("\n");

	$aConfiguration['my.cnf'] = readline("my.cnf  = ");
	if ( $aConfiguration['my.cnf'] == '' ) {
		$aConfiguration['my.cnf'] = $default;
	}
	output("\n");

	if ( $aConfiguration['my.cnf'] == '' ) {
		output("WARNING: empty my.cnf!\n");
		output("Please create manually.\n");
	}
	// Create my.cnf if not exits
	elseif ( ! is_file($aConfiguration['my.cnf']) ) {

		output("INFO: There is no such file " . $aConfiguration['my.cnf'] . " yet.\n");

		$etcdir = dirname($aConfiguration['my.cnf']);
		// directory does not exist yet
		if ( file_exists($etcdir) === false ) {
			output("Creating directory " . $etcdir . "\n");
			if ( @mkdir($etcdir, 0750, true) === false ) {
				$rc = 755;
				error("Failed to create directory " . $etcdir);
				$a = error_get_last();
				$msg = "Reason: " . $a['message'] . " (rc=$rc).";
				error($msg);
				output("Skipping...\n");
				return array($rc, $aConfiguration['my.cnf']);
			}
		}
		// touch of my.cnf comes further down


		// Ask for stripping

		output("\n");
		$question = 'Shall I strip comments from the configuration file (Y/n)? ';
		$key = answerQuestion($question, array('y', 'n'), 'y');
		$lStrip = 'yes';
		if ( strtolower($key) == 'n' ) {
			$lStrip = 'no';
		}


		// todo: would be better to get it from outside!
		$basedir = dirname(dirname(__FILE__));


		// read our template and create my.cnf

		// Substitue all %VARIABLES%

		$aSearch = array('%MYCNF%', '%PORT%', '%SOCKET%', '%BASEDIR%', '%DATADIR%', '%SERVERID%', '%UNAME%', '%INSTANCEDIR%', '%INSTANCE%', '__');
		$aReplace = array($aConfiguration['my.cnf'], $aConfiguration['port'], $aConfiguration['socket'], $aConfiguration['basedir'], $aConfiguration['datadir'], $aConfiguration['port'], php_uname('n'), $aConfiguration['instancedir'], $aConfiguration['name'], '_');

		$aVarSearch  = array('/^# wsrep_/', '/^# WSREP parameter/');
		$aVarReplace = array('', '');

		$lTemplateFile = $basedir . '/tpl/my.cnf.template';
		$srcFh = fopen($lTemplateFile, 'r');
		ini_set('track_errors', 1);
		// touch file
		output("Creating file " . $aConfiguration['my.cnf'] . "\n");
		$newFh = @fopen($aConfiguration['my.cnf'], 'w');
		if ( $newFh === false ) {
			$rc = 753;
			$msg = 'Opening file ' . $aConfiguration['my.cnf'] . ' for writing failed. ' . "(rc=$rc)";
			error($msg);
			return array($rc, $aConfiguration['my.cnf']);
		}
		while( ($line = fgets($srcFh)) !== false ) {

			$line = str_replace($aSearch, $aReplace, $line);
			if ( 'yes' == $lStrip ) {
				// Use preg_replace for array search. preg_match would also do the job
				// but only for strings!
				$lNewLine = preg_replace($aVarSearch, $aVarReplace, $line);
				// We got a match, skip/strip the line
				if ( $line != $lNewLine ) {
					continue;
				}
			}
			fputs($newFh, $line);
		}
		fclose($newFh);
		chmod($aConfiguration['my.cnf'], 0600);
		fclose($srcFh);
	}
	// Opening my.cnf for writing failed (e.g. /etc/my.cnf)
	else {
		$rc = 706;
		error("Writing to file " . $aConfiguration['my.cnf'] . " failed (rc=$rc)");
		// Does not exist in this context here: error($php_errormsg);
	}

	output("\n");

	return array($rc, $aConfiguration['my.cnf']);
}

// ---------------------------------------------------------------------
function askForSocket($aGlobalConfiguration, $aInstanceConfiguration, $tSocket = '')
// ---------------------------------------------------------------------
{

	$aSocketCandidates = array(
		'/var/run/mysqld/mysql-' . $aInstanceConfiguration['port'] . '.sock'
	, '/var/run/mysqld/mysql-' . $aInstanceConfiguration['name'] . '.sock'
	, '/var/run/mysqld/mysql.sock'
	);

	// Default socket:
	if ( isset($aInstanceConfiguration['socket']) && ($aInstanceConfiguration['socket'] == '') ) {
		// /var/run/mysqld/mysql-3306.sock
		$default =  "/var/run/mysqld/mysql-" . $aInstanceConfiguration['port'] . ".sock";
	}
	elseif ( isset($aInstanceConfiguration['socket']) && ($aInstanceConfiguration['socket'] != '') ) {
		$default =  $aInstanceConfiguration['socket'];
		array_push($aSocketCandidates, $default);
	}
	else {
		$default = '/var/run/mysqld/mysql.sock';
	}

	$aSocketCandidates = array_unique($aSocketCandidates);

	output("\n");
	output("Set socket.\n");
	output("Example: ");
	foreach ( $aSocketCandidates as $socket ) {
		if ( $socket == $default ) {
			output('[' . $socket . ']');
		}
		else {
			output($socket);
		}
		output(' ');
	}
	output("\n");


	// For testing purposes only:

	if ( $tSocket == '' ) {
		$aInstanceConfiguration['socket'] = readline("socket  = ");
	}
	else {
		$aInstanceConfiguration['socket'] = $tSocket;
	}

	if ( $aInstanceConfiguration['socket'] == '' ) {
		$aInstanceConfiguration['socket'] = $default;
	}
	output("\n");


	// Check if socket already is in use

	$aSocketConflicts = array();
	foreach( getInstances($aGlobalConfiguration) as $instance ) {

		if ( ($instance != $aInstanceConfiguration['name']) && ($aGlobalConfiguration[$instance]['socket'] == $aInstanceConfiguration['socket']) ) {
			array_push($aSocketConflicts, $instance);
		}
	}
	if ( count($aSocketConflicts) > 0 ) {
		$insts = implode(', ', $aSocketConflicts);
		output("WARNING: Socket " . $aInstanceConfiguration['socket'] . " already exists under instance(s) $insts.\n");
		output("         Please resolve conflict by renaming socket file. For example add instance name or port to socket\n");
	}

	return $aInstanceConfiguration['socket'];
}

// ---------------------------------------------------------------------
function askForBasedir($aConfiguration)
// ---------------------------------------------------------------------
{
	$lHomeDir = $_ENV['HOME'];

	// Guess candidates from /home/mysql/product
	$aCandidates = glob($lHomeDir . '/product/*', GLOB_ONLYDIR);

	$aBasedirCandidates = array_merge($aCandidates, array(
	  $lHomeDir . '/product/mysql-5.8.99'
	, '/usr/local/mysql'
	, '/opt/mysql'
	, '/usr'
	));

	// Default basedir:
	if ( isset($aConfiguration['basedir']) && ($aConfiguration['basedir'] == '') ) {
		// /home/mysql/product/mysql
		$default =  $lHomeDir . '/product/mysql';
	}
	elseif ( isset($aConfiguration['basedir']) && ($aConfiguration['basedir'] != '') ) {
		$default =  $aConfiguration['basedir'];
		array_push($aBasedirCandidates, $default);
	}
	else {
		$default = '/usr';
	}

	// Cleanup
	$aBasedirCandidates = array_unique($aBasedirCandidates);

	foreach ( $aBasedirCandidates as $key => $basedir ) {

		// No mysqld exists so no candiate
		if ( ! file_exists($basedir . '/bin/mysqld') ) {
			unset($aBasedirCandidates[$key]);
		}
	}


	output("\n");
	output("Set basedir. The basedir is the directory where your MariaDB/MySQL binaries are located.\n");
	output("Example: \n");
	foreach ( $aBasedirCandidates as $basedir ) {
		if ( $basedir == $default ) {
			output('[' . $basedir . ']');
		}
		else {
			output($basedir);
		}
		output("\n");
	}
	output("\n");

	$aConfiguration['basedir'] = rtrim(readline("basedir = "), '/');
	if ( $aConfiguration['basedir'] == '' ) {
		$aConfiguration['basedir'] = $default;
	}
	output("\n");

	return $aConfiguration['basedir'];
}

// ---------------------------------------------------------------------
function downloadChunkedFile($url, $outfile)
// ---------------------------------------------------------------------
{
	$chunksize = 10 * (1024 * 1024);

	$parts = parse_url($url);
	$i_handle = fsockopen($parts['host'], 80, $errstr, $errcode, 5);
	$o_handle = fopen($outfile, 'wb');

	if ( ($i_handle === false) || ($o_handle === false) ) {
		return false;
	}

	if ( ! empty($parts['query']) ) {
		$parts['path'] .= '?' . $parts['query'];
	}


  // Send the request to the server for the file

	$request  = 'GET {' . $parts['path'] . "} HTTP/1.1\r\n";
	$request .= "Host: {" . $parts['host'] . "}\r\n";
	$request .= "User-Agent: Mozilla/5.0\r\n";
	$request .= "Keep-Alive: 115\r\n";
	$request .= "Connection: keep-alive\r\n\r\n";
	fwrite($i_handle, $request);


	// Now read the headers from the remote server. We'll need
	// to get the content length.

	$headers = array();
	while ( ! feof($i_handle) ) {
		$line = fgets($i_handle);
		if ($line == "\r\n") {
			break;
		}
		$headers[] = $line;
	}


	// Look for the Content-Length header, and get the size
	// of the remote file.

	$length = 0;
	foreach ( $headers as $header ) {
		if ( stripos($header, 'Content-Length:') == 0 ) {
			$length = (int)str_replace('Content-Length: ', '', $header);
			break;
		}
	}


	// Start reading in the remote file, and writing it to the
	// local file one chunk at a time.

	$cnt = 0;
	while ( ! feof($i_handle) ) {
		$buf = '';
		$buf = fread($i_handle, $chunksize);
		$bytes = fwrite($o_handle, $buf);
		if ( $bytes === false ) {
			return false;
		}
		$cnt += $bytes;


		// We're done reading when we've reached the conent length

		if ( $cnt >= $length ) {
			break;
		}
	}

	fclose($i_handle);
	fclose($o_handle);
	return $cnt;
}

// ---------------------------------------------------------------------
function askForDatadir($aConfiguration)
// ---------------------------------------------------------------------
{
	$lHomeDir = $_ENV['HOME'];

	$aDatadirCandidates = array(
		'/var/lib/mysql'
	, '/opt/mysql/data'
	, '/usr/local/mysql/data'
	, $lHomeDir . '/data/' . $aConfiguration['name']
	, $aConfiguration['instancedir'] . '/' . 'data'
	);

	// Default datadir:

	$default = $aConfiguration['instancedir'] . '/' . 'data';
	if ( isset($aConfiguration['datadir']) && ($aConfiguration['datadir'] == '') ) {
		// /opt/mysql/data/mysqld1
		$default =  $lHomeDir . '/data' . $aConfiguration['name'];
	}
	elseif ( isset($aConfiguration['datadir']) && ($aConfiguration['datadir'] != '') ) {
		array_push($aDatadirCandidates, $aConfiguration['datadir']);
		array_push($aDatadirCandidates, $aConfiguration['basedir'] . '/data');
		$default =  $aConfiguration['datadir'];
	}
	array_push($aDatadirCandidates, $default);

	$aDatadirCandidates = array_unique($aDatadirCandidates);

	output("\n");
	output("Set datadir. The datadir is the directory where your MariaDB/MySQL data are located.\n");
	output("\n");
	output("Example: ");

	foreach ( $aDatadirCandidates as $datadir ) {
		if ( $datadir == $default ) {
			output('[' . $datadir . ']');
		}
		else {
			output($datadir);
		}
		output(' ');
	}
	output("\n");

	$aConfiguration['datadir'] = rtrim(readline("datadir = "), '/');
	output("\n");

	if ( isset($aConfiguration['datadir']) && ($aConfiguration['datadir'] == '') ) {
		$aConfiguration['datadir'] = $default;
	}
	if ( is_dir($aConfiguration['datadir']) ) {
		output("WARNING: directory " . $aConfiguration['datadir'] ." already exist.\n");
	}
	else {
		output("Creating directory " . $aConfiguration['datadir'] . "\n");
		mkdir($aConfiguration['datadir'], 0750, true);
	}

	return $aConfiguration['datadir'];
}

// ---------------------------------------------------------------------
function replaceInitScripts($tKey = '')
// Make sure /etc/init.d/{mysql|mysqld|mysqld_multi} is replaced
// ---------------------------------------------------------------------
{
	$rc = OK;

	$aInitScritps = array(
	  '/etc/init.d/mysql'
	, '/etc/init.d/mysqld'
	, '/etc/init.d/mysqld_multi'
	, '/etc/systemd/system/mysqld.service'
	, '/etc/systemd/system/mysql.service'
	);
	$lDistribution = 'unknown';

	$lOs = getOs();
	if ( $lOs == 'Linux' ) {
		$lDistribution = getDistribution();
		$lDistributionVersion = getDistributionVersion();
		// 14.04
		// 6.8
		// 7
		// 16.04
	}

	foreach ( $aInitScritps as $script ) {

		while ( file_exists($script) ) {
			$s = basename($script);
			output("\n");
			output("You still have an old init script ($script) which can confuse\n");
			output("MyEnv during server boot. Please remove and deactivate this init script.\n\n");

			switch ($lDistribution) {
			case 'CentOS':
			case 'RedHat':
			case 'RedHatEnterpriseServer':
				// CentOS 7 has systemd
				if ( $lDistributionVersion >= '7' ) {
					$cmd = "sudo systemctl disable " . $s . "; sudo rm -f $script";
					output("  shell> $cmd\n\n");
				}
				// CentOS 6 has still sysV
				else {
					$cmd = "sudo chkconfig --del " . $s . "; sudo rm -f $script";
					output("  shell> $cmd\n\n");
				}
				break;
			case 'Fedora':
			case 'SUSE':
			case 'SUSE LINUX':
			case 'openSUSE project':
				$cmd = "sudo chkconfig --del " . $s . "; sudo rm -f $script";
				output("  shell> $cmd\n\n");
				break;
			case 'Ubuntu':
				// Ubuntu 15.04 and newer has systemd
				if ( $lDistributionVersion >= '15' ) {
					$cmd = "sudo systemctl disable " . $s . "; sudo rm -f $script";
					output("  shell> $cmd\n\n");
				}
				// Older Ubuntu has sysV or upstart
				else {
					$cmd = "sudo rm -f $script; sudo update-rc.d " . $s . " remove";
					output("  shell> $cmd\n\n");
				}
				break;
			case 'Debian':
				$cmd = "sudo rm -f $script; sudo update-rc.d " . $s . " remove";
				output("  shell> $cmd\n\n");
				break;
			default:
				$cmd = 'echo "Unknown distribution: ' . $lDistribution . '"';
				output("OS/Distribution $lOs/$lDistribution is not yet supported.\n");
			}

			// For testing purposes only:
			if ( $tKey == '' ) {
				output("\n");
				$key = answerQuestion('Done yourself already, Skip this step, Abort installation or Installer shall do it [D/s/a/i]? ', array('d', 's', 'a', 'i'), 'd');
			}
			else {
				$key = $tKey;
			}

			// Do NOT use switch here because of break!
			if ( $key == 'a' ) {
				$rc = 726;
				output("Aborting... (rc=$rc)\n");
				exit($rc);
			}
			elseif ( $key == 'd' ) {
				null;
			}
			elseif ( $key == 's' ) {
				return;
			}
			elseif ( $key == 'i' ) {
				output("$cmd\n");
				list($ret, $output, $stdout, $stderr) = my_exec($cmd);
			}
		}
	}
}

// ---------------------------------------------------------------------
function addMyEnvInitScript($tKey = '')
// ---------------------------------------------------------------------
{
	$rc = OK;
	$lDistribution = 'unknown';

	$lOs = getOs();
	if ( $lOs == 'Linux' ) {
		$lDistribution = getDistribution();
		$lDistributionVersion = getDistributionVersion();
	}
	else {
		$rc = 739;
		output("O/S $lOs is not yet supported. (rc=$rc)\n");
		return $rc;
	}

	switch ($lDistribution) {
	case 'CentOS':
	case 'RedHat':
	case 'RedHatEnterpriseServer':
		// SystemD
		// 7
		if ( $lDistributionVersion >= '7' ) {
			$lInitScript = '/etc/systemd/system/myenv.service';
			$ret = prepareUnitFile($_ENV['MYENV_BASE'] . '/tpl/systemd.myenv.unit.template', $_ENV['MYENV_BASE']);
			$cmd  = "sudo cp " . $_ENV['MYENV_BASE'] . "/tpl/systemd.myenv.unit.template " . $lInitScript;
			$cmd1 = "sudo systemctl daemon-reload ; sudo systemctl enable myenv";
		}
		// sysV
		// 6.8
		else {
			$lInitScript = '/etc/init.d/myenv';
			$cmd  = "sudo cp " . $_ENV['MYENV_BASE'] . "/tpl/myenv.server " . $lInitScript;
			$cmd1 = "sudo chkconfig --add myenv; sudo chkconfig myenv on";
		}
		break;
	case 'Fedora':
	case 'SUSE':
	case 'SUSE LINUX':
	case 'openSUSE project':
		$lInitScript = '/etc/init.d/myenv';
		$cmd  = "sudo cp " . $_ENV['MYENV_BASE'] . "/tpl/myenv.server " . $lInitScript;
		$cmd1 = "sudo chkconfig --add myenv; sudo chkconfig myenv on";
		break;
	case 'Ubuntu':
		// SystemD
		// >= 15.04
		if ( $lDistributionVersion >= '15' ) {
			$lInitScript = '/etc/systemd/system/myenv.service';
			$ret = prepareUnitFile($_ENV['MYENV_BASE'] . '/tpl/systemd.myenv.unit.template', $_ENV['MYENV_BASE']);
			$cmd = "sudo cp " . $_ENV['MYENV_BASE'] . "/tpl/systemd.myenv.unit.template " . $lInitScript;
			$cmd1 = "sudo systemctl daemon-reload ; sudo systemctl enable myenv";
		}
		// sysV/upstart
		// 14.04
		else {
			$lInitScript = '/etc/init.d/myenv';
			$cmd = "sudo cp " . $_ENV['MYENV_BASE'] . "/tpl/myenv.server " . $lInitScript;
			$cmd1 = "sudo update-rc.d myenv defaults";
		}
		break;
	case 'Debian':
		$lInitScript = '/etc/init.d/myenv';
		$cmd  = "sudo cp " . $_ENV['MYENV_BASE'] . "/tpl/myenv.server " . $lInitScript;
		$cmd1 = "sudo update-rc.d myenv defaults";
		break;
	default:
		$rc = 750;
		output("Unknown distribution: $lDistribution ");
		output("OS/Distribution $lOs/$lDistribution is not yet supported.\n");
		return $rc;
	}

	while ( ! file_exists($lInitScript) ) {

		output("\nPlease copy yourself manually the MyEnv start/stop script/unit file to the following location\n");
		output("and link it to your O/S runlevels:\n\n");
		output("  shell> $cmd\n");
		output("  shell> $cmd1\n\n");

		// For testing purposes only:
		if ( $tKey == '' ) {
			output("\n");
			$key = answerQuestion('Done yourself already, Abort installation or Installer shall do it [D/a/i]? ', array('d', 'a', 'i'), 'd');
		}
		else {
			$key = $tKey;
		}

		// Do NOT use switch here because of break!
		if ( $key == 'a' ) {
			$rc = 742;
			output("Aborting... (rc=$rc)\n");
			exit($rc);
		}
		elseif ( $key == 'd' ) {
			null;
		}
		elseif ( $key == 'i' ) {

			output("$cmd\n");
			list($ret, $output, $stdout, $stderr) = my_exec($cmd);

			output("$cmd1\n");
			list($ret, $output, $stdout, $stderr) = my_exec($cmd1);
		}
	}   // while
}

// ---------------------------------------------------------------------
function addMyEnvHook($pMyenvHook, $tKey = '')
// ---------------------------------------------------------------------
{
	$rc = OK;

	if ( ! file_exists($pMyenvHook) ) {

		output("\nWriting " . $pMyenvHook . "\n");
		if ( $fh = fopen($pMyenvHook, 'a') ) {

			$lLikedBase = preg_replace("|(.*/myenv)-.*|", "$1", $_ENV['MYENV_BASE']);
			fwrite($fh, "export MYENV_BASE=" . $lLikedBase . "\n");
			fclose($fh);
		}

		if ( ! file_exists($_ENV['MYENV_BASE'] . '/etc/aliases.conf') ) {
			copy($_ENV['MYENV_BASE'] . '/tpl/aliases.conf.template', '/etc/myenv/aliases.conf');
		}
		if ( ! file_exists($_ENV['MYENV_BASE'] . '/etc/variables.conf') ) {
			copy($_ENV['MYENV_BASE'] . '/tpl/variables.conf.template', '/etc/myenv/variables.conf');
		}
	}
}

// ---------------------------------------------------------------------
function addMyEnvProfile($pHomeDir, $tKey = '')
// ---------------------------------------------------------------------
{
	if ( is_readable("$pHomeDir/.bash_profile") ) {
		$tag = "# BEGIN MyEnv";
		$cmd = "grep --max-count=1 '$tag' $pHomeDir/.bash_profile";
		# output("$cmd\n");
		list($ret, $output, $stdout, $stderr) = my_exec($cmd);
	}
	else {
		$ret = 1;
	}

	// We have to add the hook
	if ( $ret == 1 ) {

		// todo: This is redundant with profile.template!!!
		$lSourceSequence  = "source /etc/myenv/MYENV_BASE\n";
		$lSourceSequence .= 'MYENV_PWD=$(pwd)' . "\n";
		$lSourceSequence .= 'cd $MYENV_BASE/bin' . "\n";
		$lSourceSequence .= "source myenv.profile\n";
		$lSourceSequence .= 'cd $MYENV_BASE;' . "\n";
		$lSourceSequence .= '$MYENV_BASE/bin/showMyEnvStatus.php;' . "\n";
		$lSourceSequence .= "cd - > /dev/null\n";
		$lSourceSequence .= 'cd $MYENV_PWD' . "\n";

		output("Do you want to add the following code to your ~/.bash_profile file?");
		output("\n\n");
		output($lSourceSequence);

		output("\n");
		$key = answerQuestion("(Y/n)? ", array('y', 'n'), 'y');
		if ( $key == 'y' ) {

			if ( $fh = fopen("$pHomeDir/.bash_profile", 'a') ) {

				fwrite($fh, "\n");
				fwrite($fh, "# BEGIN MyEnv\n");
				fwrite($fh, "# Written by the MyEnv installMyEnv.php script.\n");
				fwrite($fh, $lSourceSequence);
				fwrite($fh, "# END MyEnv\n");
				fclose($fh);
			}
		}
	}
}

// ---------------------------------------------------------------------
function createHomeDir($pHomeDir, $tKey = '')
// ---------------------------------------------------------------------
{
	while ( ! is_writable($pHomeDir) ) {

		output("\nI cannot write to the users HOME directory ($pHomeDir). Please change access privileges as follows:\n\n");
		$cmd = "sudo chown u+rwx $pHomeDir";
		output("  shell> $cmd\n");

		output("\n");
		$key = answerQuestion('Done yourself already, Abort installation or Installer shall do it [D/a/i]? ', array('d', 'a', 'i'), 'd');

		// Do NOT use switch here because of break!
		if ( $key == 'a' ) {
			$rc = 743;
			output("Aborting... (rc=$rc)\n");
			exit($rc);
		}
		elseif ( $key == 'd' ) {
			null;
		}
		elseif ( $key == 'i' ) {

			output("$cmd\n");
			list($ret, $output, $stdout, $stderr) = my_exec($cmd);
		}
	}
}

// ---------------------------------------------------------------------
function checkHomeDir(&$pHomeDir, $pUser, $tKey = '')
// ---------------------------------------------------------------------
{
	while ( $pHomeDir == '' ) {

		output("\nUser has no HOME directory. Please create users HOME directory with the follo-\n");
		output("wing command:\n\n");
		$pHomeDir = "/home/$pUser";
		$cmd = "sudo useradd --create-home --home $pHomeDir $pUser";
		output("  shell> $cmd\n");

		output("\n");
		$key = answerQuestion('Done yourself already, Abort installation or Installer shall do it [D/a/i]? ', array('d', 'a', 'i'), 'd');

		// Do NOT use switch here because of break!
		if ( $key == 'a' ) {
			$rc = 744;
			output("Aborting... (rc=$rc)\n");
			exit($rc);
		}
		elseif ( $key == 'd' ) {
			null;
		}
		elseif ( $key == 'i' ) {

			output("$cmd\n");
			list($ret, $output, $stdout, $stderr) = my_exec($cmd);
		}
	}
}

// ---------------------------------------------------------------------
function writeConfigurationFile($aConfiguration, $pConfFile)
// ---------------------------------------------------------------------
{
	// Writing the configuration file
	output("\nWriting the configuration file...\n");

	$lConfFileBak      = $pConfFile . '.' . date("Y-m-d-h-i-s");

	if ( file_exists($lConfFileBak) ) {
		unlink($lConfFileBak);
	}
	output("Rename $pConfFile to $lConfFileBak\n");
	if ( rename($pConfFile, $lConfFileBak) ) {

		output("Write $pConfFile\n");
		if ( $fh = fopen($pConfFile, 'w') ) {

			foreach ( $aConfiguration as $instance => $record ) {

				fwrite($fh, "[$instance]\n\n");
				foreach ( $record as $key => $value ) {
					if ( $key == 'name' ) {
						continue;
					}
					fwrite($fh, sprintf("%-20s = %s\n", $key, $value));
				}
				fwrite($fh, "\n\n");
			}
			fclose($fh);
		}
		else {
			// Writing config file failed
			// Rolling back!
		}
	}
	else {
		$rc = 714;
		output("Renaming the configuration file $pConfFile to $lConfFileBak failed.\n");
		exit($rc);
	}
}

// ---------------------------------------------------------------------
function deleteInstance2($aInstance)
// ---------------------------------------------------------------------
{
	if ( count($aInstance) > 0 ) {
		output("The following instances are available to delete:\n");
		output("\n");
		foreach ($aInstance as $instance) {
			output($instance . ' ');
		}
		output("\n");
	}
	else {
		output("No instance exists yet.\n");
	}
	output("\n");
	$instance = readline("Enter instance name to delete: ");
	if ( $instance == '' ) {
		output("Please enter an instance name.\n\n");
	}
	else {
		output("\n");
		$ret = deleteInstance($instance);
		if ( $ret != 0 ) {
			readline("Press Enter to continue...");
			output("\n\n");
		}
	}
}

// ---------------------------------------------------------------------
function deleteInstance($pInstance)
// ---------------------------------------------------------------------
{
	$rc = OK;
	global $aConfiguration;

	if ( ! isset($aConfiguration[$pInstance]) ) {
		$rc = 707;
		error("Instance $pInstance does not exist. Please choose a valid instance name (rc=$rc).");
		return $rc;
	}

	$ret = checkDatabase($aConfiguration[$pInstance]);

	if ( $ret != OK ) {

		output("Delete datadir (" . $aConfiguration[$pInstance]['datadir'] . ") and basedir (" . $aConfiguration[$pInstance]['basedir'] . ") manually.\n");
		unset($aConfiguration[$pInstance]);
		output("Instance $pInstance has been deleted from MyEnv.\n");
		output("Do not forget to save configuration before leaving installer.\n");
		readline("Press Enter to continue...");
		output("\n\n");
	}
	else {
		$rc = 754;
		error("Please stop instence first before deleting it (rc=$rc).");
		return $rc;
	}

	return $rc;
}

// ---------------------------------------------------------------------
function changeInstance2($aInstances)
// ---------------------------------------------------------------------
{
	$rc = OK;

	if ( count($aInstances) > 0 ) {
		output("The following instances are available to change:\n");
		output("\n");
		foreach ($aInstances as $instance) {
			output($instance . ' ');
		}
		output("\n");
	}
	else {
		output("No instance exists yet.\n");
	}
	output("\n");
	$instance = readline("Enter instance name to change: ");
	if ( $instance == '' ) {
		output("Please enter an instance name.\n\n");
	}
	else {
		output("\n");
		$rc = changeInstance($instance);
	}

	return $rc;
}

// ---------------------------------------------------------------------
function copyConfFileFromTemplate($pConfFile, $pConfFileTemplate, $tKey = '')
// ---------------------------------------------------------------------
{
	output("\nConfiguration file $pConfFile does NOT exist.\n");

	output("\n");
	$key = answerQuestion("Copy from template or Abort (T, a): ", array('t', 'a'), 't');
	if ( $key == 't' ) {
		if ( copyFromTemplate($pConfFileTemplate, $pConfFile) != 0 ) {
			$rc = 711;
			output("Aborting... (rc=$rc)\n");
			exit($rc);
		}
	}
	elseif ( $key == 'a' ) {
		$rc = 749;
		output("Aborting... (rc=$rc)\n");
		exit($rc);
	}
	else {
		$rc = 712;
		error("Fatal error. Please report this (rc=$rc, key=$key)!");
		exit($rc);
	}
}

// ---------------------------------------------------------------------
function useOrCopyConfFile($pConfFile, $pConfFileTemplate)
// ---------------------------------------------------------------------
{
	output("Configuration file $pConfFile already exists.\n");

	output("\n");
	$key = answerQuestion("Use this, overwrite with Template or Abort (U,t,a): ", array('u', 't', 'a'), 'u');
	output("\n");
	if ( $key == 'u' ) {
		// do nothing
	}
	elseif ( $key == 't' ) {
		if ( copyFromTemplate($pConfFileTemplate, $pConfFile) != 0 ) {
			$rc = 709;
			output("Aborting... (rc=$rc)\n");
			exit($rc);
		}
	}
	elseif ( $key == 'a' ) {
		$rc = 745;
		output("Aborting... (rc=$rc)\n");
		exit($rc);
	}
	else {
		$rc = 710;
		error("Fatal error. Please report this (rc=$rc, key=$key)!");
		exit($rc);
	}
}

// ---------------------------------------------------------------------
function checkHomeDirOwner($pEtcMyenv, $pOwner)
// ---------------------------------------------------------------------
{
	$aPwUid = posix_getpwuid(fileowner($pEtcMyenv));
	while ( $aPwUid['name'] != $pOwner ) {

		$rc = 722;
		error("Directory $pEtcMyenv does NOT belong to user $pOwner (rc=$rc).");
		output("\nPlease change as as follows:\n");
		$cmd = "sudo chown -R $pOwner: $pEtcMyenv";
		output("  shell> $cmd");

		output("\n");
		$key = answerQuestion('Done yourself already, Abort installation or Installer shall do it [D/a/i]? ', array('d', 'a', 'i'), 'd');

		// Do NOT use switch here because of break!
		if ( $key == 'a' ) {
			$rc = 740;
			output("Aborting... (rc=$rc)\n");
			exit($rc);
		}
		elseif ( $key == 'd' ) {
			$aPwUid = posix_getpwuid(fileowner($pEtcMyenv));
		}
		elseif ( $key == 'i' ) {

			output("$cmd\n");
			list($ret, $output, $stdout, $stderr) = my_exec($cmd);
			$aPwUid = posix_getpwuid(fileowner($pEtcMyenv));
		}
	}
}

// ---------------------------------------------------------------------
function checkMyEnvDir($pEtcMyEnv, $pUser)
// ---------------------------------------------------------------------
{
	while ( ! is_dir($pEtcMyEnv) ) {

		output("\n");
		$cmd = "sudo mkdir $pEtcMyEnv; sudo chown -R $pUser: $pEtcMyEnv";
		error("Directory $pEtcMyEnv does NOT exist. Please create it\n       as follows:\n\n       shell> $cmd\n");

		output("\n");
		$key = answerQuestion('Done yourself already, Abort installation or Installer shall do it [D/a/i]? ', array('d', 'a', 'i'), 'd');

		// Do NOT use switch here because of break!
		if ( $key == 'a' ) {
			$rc = 746;
			output("Aborting... (rc=$rc)\n");
			exit($rc);
		}
		elseif ( $key == 'd' ) {
			// $aPwUid = posix_getpwuid(fileowner($pEtcMyEnv));
		}
		elseif ( $key == 'i' ) {

			output("$cmd\n");
			list($ret, $output, $stdout, $stderr) = my_exec($cmd);
		}
	}
}

// ---------------------------------------------------------------------
function checkOsUser($pUser)
// ---------------------------------------------------------------------
{
	$aUserInfo = posix_getpwnam($pUser);
	if ( $aUserInfo === false ) {
		$rc = 708;
		error("User $pUser does not exist (rc=$rc).\n");
		output("\nCreate such a user as follows:\n\n");
		$cmd = "sudo groupadd $pUser; sudo useradd -r -g $pUser --home /home/$pUser --shell /bin/bash $pUser";
		output("  shell> $cmd\n\n");
		output("  http://dev.mysql.com/doc/refman/5.6/en/binary-installation.html\n");

		output("\n");
		$key = answerQuestion('Done yourself already, Abort installation or Installer shall do it [D/a/i]? ', array('d', 'a', 'i'), 'd');

		// Do NOT use switch here because of break!
		if ( $key == 'a' ) {
			$rc = 747;
			output("Aborting... (rc=$rc)\n");
			exit($rc);
			//break;
		}
		elseif ( $key == 'd' ) {
			null;
		}
		elseif ( $key == 'i' ) {

			output("$cmd\n");
			list($ret, $output, $stdout, $stderr) = my_exec($cmd);
		}
	}
}

// ---------------------------------------------------------------------
function checkForRootUser($pCurrentUser)
// ---------------------------------------------------------------------
{
	$rc = OK;

	$default = 'mysql';

	if ( $pCurrentUser == 'root' ) {

		output("You started the MyEnv installer as user root. You should better use another user like\n$default or similar.\n");

		output("\n");
		$key = answerQuestion("Do you want to install MyEnv as root, install as Other user or Abort? [r/O/a]? ", array('r', 'o', 'a'), 'o');
		if ( $key == 'o' ) {

			output("\n");
			$lMyenvUser = readline("Enter other user to install MyEnv (default = mysql): ");
			if ( $lMyenvUser == '' ) {
				$lMyenvUser = $default;
			}

			// Get user from passwd
			$aUserInfo = posix_getpwnam($lMyenvUser);
			if ( $aUserInfo === false ) {
				output("Please create MyEnv user $lMyenvUser as follows:\n");
				$cmd1 = "groupadd $lMyenvUser";
				output("  shell> $cmd1\n");
				$cmd2 = "useradd -r -g $lMyenvUser --home /home/$lMyenvUser --shell /bin/bash $lMyenvUser";
				output("  shell> $cmd2\n\n");

				output("\n");
				$key = answerQuestion('Done yourself already, Abort installation or Installer shall do it [D/a/i]? ', array('d', 'a', 'i'), 'd');

				// Do NOT use switch here because of break!
				if ( $key == 'a' ) {
					$rc = 721;
					output("Aborting... (rc=$rc)\n");
					exit($rc);
					//break;
				}
				elseif ( $key == 'd' ) {
					null;
				}
				elseif ( $key == 'i' ) {

					output("$cmd1\n");
					list($ret, $output, $stdout, $stderr) = my_exec($cmd1);
					output("$cmd2\n");
					list($ret, $output, $stdout, $stderr) = my_exec($cmd2);
				}



			}
		}
		else if ( $key == 'a' ) {
			$rc = 748;
			output("Aborting... (rc=$rc)\n");
			exit($rc);
		}
		else if ( $key == 'r' ) {
			$lMyenvUser = 'root';
		}
	}
	else {
		$lMyenvUser = $pCurrentUser;
	}
	return $lMyenvUser;
}

// ---------------------------------------------------------------------
function checkForNonMySQLUser($pCurrentUser)
// ---------------------------------------------------------------------
{
	$rc = OK;

	if ( $pCurrentUser != 'mysql' ) {

		output("It is strongly recommended to run MyEnv under user mysql but you are using $pCurrentUser.\nNobody knows what happens when you do this as user $lCurrentUser.\n");

		output("\n");
		$key = answerQuestion("Do you really want to continue? [y/N]? ", array('y', 'n'), 'n');
		if ( $key == 'n' ) {
			exit($rc);
		}
	}
}

// ---------------------------------------------------------------------
function askForStage($aConfiguration)
// ---------------------------------------------------------------------
{
	$aStages = array(
	  'production'
	, 'quality'
	, 'testing'
	, 'development'
	);

	$default = 'production';

	output("\n");
	output('Choose stage of instance ' . $aConfiguration['name'] . ":\n");
	output("Possible values: ");

	foreach ( $aStages as $stage ) {
		if ( $stage == $default ) {
			output('[' . $stage . ']');
		}
		else {
			output($stage);
		}
		output(' ');
	}
	output("\n");

	$aConfiguration['stage'] = readline("stage  = ");
	if ( $aConfiguration['stage'] == '' ) {
		$aConfiguration['stage'] = $default;
	}
	output("\n");

	return $aConfiguration['stage'];
}

// ---------------------------------------------------------------------
function printUsage()
// ---------------------------------------------------------------------
{
	$script = 'installMyEnv';

	print "
usage: $script --operation=[add|change|delete]
                    --instance=<name> [--help] ...

Options:
  operation       Type of operation.
                  - add: Add a new instance.
                  - change: Change an existing instance.
                  - delete: Delete an existing instance.
  instance        Name of the instance to manage.
  purge-data      Purges data in case of deletion.
  help            Print this help.

Examples:

  $script --operation=delete --instance=test

  $script --operation=add --instance=test --port=3306 --socket=/var/run/mysqld/mysql.sock

";
}

// ---------------------------------------------------------------------
function checkOptions($aArgv, $aOptions, $pConfFile)
// ---------------------------------------------------------------------
{
	$rc = OK;

	// Some Options or Arguments were not properly detected
	if ( count($aArgv) > (count($aOptions) + 1) ) {
		$rc = 730;
		$msg = "Invalid arguments or options. (rc=$rc)";
		error($msg);
		return $rc;
	}

	// Allowed operations
	if ( ! in_array($aOptions['operation'], array('add', 'change', 'delete')) ) {
		$rc = 731;
		$msg = "Invalid operation " . $aOptions['operation'] . ". Valid operations are add, change, delete. (rc=$rc)";
		error($msg);
		return $rc;
	}

	// Operations are not allowed yet
	if ( in_array($aOptions['operation'], array('add', 'change')) ) {
		$rc = 732;
		$msg = "Operation add and change are not supported yet. (rc=$rc)";
		error($msg);
		return $rc;
	}

  if ( ! isset($aOptions['instance']) ) {
		$rc = 751;
		$msg = "Instance name was not defined. (rc=$rc)";
		error($msg);
		return $rc;
  }

	// Check if instance exists
	list($ret, $aConfiguration) = getConfiguration($pConfFile);

	// Complain if instance does NOT exist on delete and change
	if ( in_array($aOptions['operation'], array('delete', 'change')) ) {

		if ( ! isset($aConfiguration[$aOptions['instance']]) ) {
			$rc = 733;
			$msg = "Instance " . $aOptions['instance'] . " does NOT exist. Please choose an existing instance name (rc=$rc).";
			error($msg);
			return $rc;
		}
	}
	// Complain if instance DOES exist on add
	// add
	else {

		if ( isset($aConfiguration[$aOptions['instance']]) ) {
			$rc = 702;
			$msg = "Instance " . $aOptions['instance'] . " DOES exist. Please choose a new instance name (rc=$rc).";
			error($msg);
			return $rc;
		}
	}

	// purge-data only in combination with delete
	if ( isset($aOptions['purge-data']) && ('delete' != $aOptions['operation']) ) {
		$rc = 752;
		$msg = "Option --purge-data can only be use with --operation=delete (rc=$rc).";
		error($msg);
		return $rc;
	}

	return $rc;
}

// ---------------------------------------------------------------------
function deleteInstanceAutomatized($aOptions, $pMyEnvConfFile)
// ---------------------------------------------------------------------
{
	$rc = OK;

	list($ret, $aConfiguration) = getConfiguration($pMyEnvConfFile);

	// Check if database is still running
	$ret = checkDatabase($aConfiguration[$aOptions['instance']]);

	$lInstanceConfigBackup = $aConfiguration[$aOptions['instance']];
	if ( $ret == OK ) {
		$rc = 734;
		$msg = "Please stop instence first before deleting it (rc=$rc).";
		error($msg);
		return $rc;
	}
	else {
		unset($aConfiguration[$aOptions['instance']]);
		writeConfigurationFile($aConfiguration, $pMyEnvConfFile);
	}

	// Purge database
	// We should better clean-up instance-dir
	if ( isset($aOptions['purge-data']) ) {
		output('Purging data of instance ' . $aOptions['instance'] . ' from ' . $lInstanceConfigBackup['datadir'] . "\n");
		$rc = recursivelyRemoveDirectory($lInstanceConfigBackup['datadir']);
	}

	return $rc;
}

// ---------------------------------------------------------------------
function prepareUnitFile($pUnitFile, $pMyenvBase)
// ---------------------------------------------------------------------
{
	$rc = OK;

	$lLikedBase = preg_replace("|(.*/myenv)-.*|", "$1", $pMyenvBase);

	$lUnitOrigFile = $pUnitFile . '.orig';
	if ( ! file_exists($lUnitOrigFile) ) {
		if ( ! copy($pUnitFile, $lUnitOrigFile) ) {
			$rc = 738;
			$msg = "Cannot copy $pUnitFile to $lUnitOrigFile";
			error($msg);
			// do not exit here
		}
	}

	$src = fopen($lUnitOrigFile, 'r');
	$new = fopen($pUnitFile, 'w');
	while( ($line = fgets($src)) !== false) {
		fputs($new, preg_replace('/^ExecStart=.*$/', 'ExecStart=' . $lLikedBase . '/bin/myenv_start_stop.php start', $line));
		# And what about stop??
	}
	fclose($new);
	fclose($src);

	return $rc;
}

// ---------------------------------------------------------------------
function askForInstancedir($aConfiguration)
// ---------------------------------------------------------------------
{
	$lHomeDir = $_ENV['HOME'];

	$aInstancedirCandidates = array(
		'/var/lib/mysql/' . $aConfiguration['name']
	, '/opt/mysql/' . $aConfiguration['name']
	, '/usr/local/mysql/' . $aConfiguration['name']
	, $lHomeDir . '/database/' . $aConfiguration['name']
	);


	// Default instancedir

	$default = $lHomeDir . '/database/' . $aConfiguration['name'];
	if ( isset($aConfiguration['instancedir']) && ($aConfiguration['instancedir'] == '') ) {
		// /home/mysql/database/mysqld1
		$default =  $lHomeDir . '/database' . $aConfiguration['name'];
	}
	elseif ( isset($aConfiguration['instancedir']) && ($aConfiguration['instancedir'] != '') ) {
		array_push($aInstancedirCandidates, $aConfiguration['instancedir']);
		$default =  $aConfiguration['instancedir'];
	}
	array_push($aInstancedirCandidates, $default);

	$aInstancedirCandidates = array_unique($aInstancedirCandidates);

	output("\n");
	output("Set instancedir. The instancedir is the directory where your MariaDB/MySQL data directory (datadir) is located.\n");
	output("\n");
	output("Example: ");

	foreach ( $aInstancedirCandidates as $instancedir ) {
		if ( $instancedir == $default ) {
			output('[' . $instancedir . ']');
		}
		else {
			output($instancedir);
		}
		output(' ');
	}
	output("\n");

	$aConfiguration['instancedir'] = rtrim(readline("instancedir = "), '/');
	output("\n");

	if ( isset($aConfiguration['instancedir']) && ($aConfiguration['instancedir'] == '') ) {
		$aConfiguration['instancedir'] = $default;
	}
	if ( is_dir($aConfiguration['instancedir']) ) {
		output("WARNING: directory " . $aConfiguration['instancedir'] ." already exist.\n");
	}
	else {
		output("Creating directory " . $aConfiguration['instancedir'] . "\n");
		mkdir($aConfiguration['instancedir'], 0750, true);
		mkdir($aConfiguration['instancedir'] . '/binlog', 0750);
		// datadir is created autonomously and can be located somewhere else!
		mkdir($aConfiguration['instancedir'] . '/etc', 0750);
		mkdir($aConfiguration['instancedir'] . '/log', 0750);
		mkdir($aConfiguration['instancedir'] . '/tmp', 0750);
	}

	return $aConfiguration['instancedir'];
}

// ---------------------------------------------------------------------
function askForAngel($aConfiguration)
// ---------------------------------------------------------------------
{

	// Ask if mysqld should be started with angel

	output("\n");
	$question = 'Shall mysqld be started with angel process (mysqld_safe) (y/N)? ';
	$key = answerQuestion($question, array('y', 'n'), 'n');
	$aConfiguration['angel'] = 'no';
	if ( strtolower($key) == 'y' ) {
		$aConfiguration['angel'] = 'yes';
	}

	return $aConfiguration['angel'];
}

// ---------------------------------------------------------------------
function askForCgroups($aConfiguration)
// ---------------------------------------------------------------------
{

	// Ask if instance should run under cgroups

	output("\n");
	$question = 'Shall instance run under cgroups to fences resources (y/N)? ';
	$key = answerQuestion($question, array('y', 'n'), 'n');
	$cgroups = 'no';
	if ( strtolower($key) == 'n' ) {
		$cgroups = 'no';
		return 'no';
	}


	// Ask for relative CPU shares

	output("\n");
	$default = 100;
	$rcs = readline("Enter relative CPU shares (default = 100): ");
	if ( $rcs == '' ) {
		$lRelativeCpuShares = $default;
	}
	else {
		$lRelativeCpuShares = intval($rcs);
	}

	if ( $lRelativeCpuShares == 0 ) {
		$lRelativeCpuShares = $default;
	}


	// Aktivate cgroups

	$file = $aConfiguration['instancedir'] . '/etc/cgroups.conf';
	$cgFh = fopen($file, 'w');
	fputs($cgFh, 'cpu.shares = ' . $lRelativeCpuShares . "\n");
	fclose($cgFh);

	return 'yes';
}

// ---------------------------------------------------------------------
function askForPort($aInstanceConfiguration, $tPort = '')
// ---------------------------------------------------------------------
{
	global $aConfiguration;

	output("\n");
	output("Set port.\n");
	if ( isset($aInstanceConfiguration['port']) && ('' != $aInstanceConfiguration['port']) ) {
		$default = $aInstanceConfiguration[$pInstance]['port'];
	}
	else {
		$default = 3306;
	}
	output("Example: [$default] or 3307\n");


	// For test suite only:
	if ( '' == $tPort ) {
		$aInstanceConfiguration['port'] = intval(readline("port  = "));
	}
	else {
		$aInstanceConfiguration['port'] = $tPort;
	}

	if ( 0 == $aInstanceConfiguration['port'] ) {
		$aInstanceConfiguration['port'] = $default;
	}
	output("\n");


	// Check if port already is in use

	$aPortConflicts = array();
	foreach( getInstances($aConfiguration) as $instance ) {

		if ( ($instance != $aInstanceConfiguration['name']) && ($aInstanceConfiguration['port'] == $aConfiguration[$instance]['port']) ) {
			array_push($aPortConflicts, $instance);
		}
	}
	if ( count($aPortConflicts) > 0 ) {
		$insts = implode(', ', $aPortConflicts);
		output("WARNING: Port " . $aInstanceConfiguration['port'] . " already exists under instance(s) $insts.\n");
		output("         Please resolve conflict for example by configuring different IP addresses\n");
		output("         (bind_address=...) or ports (port=...).\n");
	}

	return $aInstanceConfiguration['port'];
}

// ---------------------------------------------------------------------
function addInstanceAutomatized($aOptions, $pMyEnvConfFile)
// ---------------------------------------------------------------------
{
	$rc = OK;

	var_dump($aOptions, $pConfFile);

	// Do some checks for creation
	/*
	basedir = /usr
	instancedir = /home/mysql/database/mysqld1
	datadir = /home/mysql/database/mysqld1/data
	installation
	port = 3306
	socket /var/run/mysqld/mysql.sock
	my.cnf = /home/mysql/database/mysqld1/etc/my.cnf
	strip_comments y
	angel = y/n 
	cgroups = y/n
	stage = production
  */
	/*
	list($ret, $aConfiguration) = getConfiguration($pConfFile);

	// Check if database is still running
	$ret = checkDatabase($aConfiguration[$aOptions['instance']]);

	if ( $ret == OK ) {
		$rc = 756;
		$msg = "Please stop instence first before deleting it (rc=$rc).";
		error($msg);
		return $rc;
	}
	else {
		unset($aConfiguration[$aOptions['instance']]);
		writeConfigurationFile($aConfiguration, $pConfFile);
	}
  */

	return $rc;
}

?>
