<?php

/*

	getInstanceBlackList()
	copyFromTemplate($pConfFileTemplate, $pConfFile)
	addInstance()
	changeInstance($pInstance)
	getInstances($aConfiguration)
	installInstance($aConfiguration)
	askForMyCnf($aConfiguration)
	askForSocket($aConfiguration, $tSocket = '')
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
	changeInstance($instance);

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

	// Do not create directory here otherwise link will fail later
	/*
	if ( ! is_dir($aConfiguration[$pInstance]['basedir']) ) {
		output("WARNING: Directory " . $aConfiguration[$pInstance]['basedir'] . " is NOT a directory or does not exist.\n");
		output("Creating directory " . $aConfiguration[$pInstance]['basedir'] . "\n");
		mkdir($aConfiguration[$pInstance]['basedir'], 0755, true);
	}
	*/


	// Install binaries as well

	if ( $aConfiguration[$pInstance]['basedir'] == '/usr' ) {
		output("\n");
		output("You have choosen the default package location of MySQL basedir.\n");
		output("Please install MySQL with your package manger if not done yet.\n");
	}
	elseif ( (is_dir($aConfiguration[$pInstance]['basedir'])
	  && (count(glob($aConfiguration[$pInstance]['basedir'] . '/*')) == 0))
	    || ( ! file_exists($aConfiguration[$pInstance]['basedir']))
	   ) {

		output("\n");
		$question = 'Shall I download and install MySQL binaries under ' . $aConfiguration[$pInstance]['basedir'] . ' (Y/n)? ';
		$key = answerQuestion($question, array('y', 'n'), 'y');
		if ( $key == 'y' ) {

			// This seems to be complete nonsense here?
			/*
			if ( $aConfiguration[$pInstance]['basedir'] == '' ) {
				$rc = 702;
				error("MyEnv is not installed yet. Please install MyEnv first with the command installMyEnv.php");
				exit($rc);
			}

			// require($aConfiguration[$pInstance]['basedir'] . "/lib/myEnv.inc");
			// $lConfFile = $_ENV['MYENV_BASE'] . '/' . 'etc/' . 'myenv.conf';
			// todo: hard coded fix this!
			$lConfFile = '/etc/myenv/myenv.conf';
			debug("ConfFile: $lConfFile");

			if ( ! file_exists($lConfFile) ) {
				$rc = 703;
				error("MyEnv configuration file $lConfFile does not exist.");
				exit($rc);
			}

			list($ret, $aConfiguration) = getConfiguration($lConfFile);
			if ( $ret != OK ) {
				$msg = "MyEnv configuration file $lConfFile has an error.";
				error($msg);
				error(print_r($aConfiguration, true));
				return $ret;
			}
			*/

			$lProductDir = dirname($aConfiguration[$pInstance]['basedir']);
			$lTmpDir     = $_ENV['HOME'] . '/' . 'tmp';

			// Do not create otherwiese link below will fail
			/*
				if ( ! is_dir($lProductDir) ) {
				output("\n");
				output("INFO: Product directory $lProductDir does not exist yet.\n");
				output("INFO: Creating it now.\n");
				mkdir($lProductDir, 0755, true);
			}
			*/

			if ( ! is_dir($lTmpDir) ) {
				output("\n");
				output("INFO: Tmp directory $lTmpDir does not exist yet.\n");
				output("INFO: Creating it now.\n");
				mkdir($lTmpDir, 0755, true);
			}

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

	$mycnf = askForDatadir($aConfiguration[$pInstance]);
	$aConfiguration[$pInstance]['datadir'] = $mycnf;


	// Install instance

	$aConfiguration[$pInstance]['user'] = getCurrentUser();
	$ret = installInstance($aConfiguration[$pInstance]);


	// port

	output("\n");
	output("Set port.\n");
	if ( isset($aConfiguration[$pInstance]['port']) && ($aConfiguration[$pInstance]['port'] != '') ) {
		$default = $aConfiguration[$pInstance]['port'];
	}
	else {
		$default = 3306;
	}
	output("Example: 3307 or [$default]\n");
	$input = intval(readline("port    = "));
	if ( $input == 0 ) {
		$aConfiguration[$pInstance]['port'] = $default;
	}
	else {
		$aConfiguration[$pInstance]['port'] = $input;
	}
	output("\n");

	// Check if port already is in use
	foreach( getInstances($aConfiguration) as $instance ) {

		if ( ($aConfiguration[$instance]['port'] == $aConfiguration[$pInstance]['port']) && ($instance != $pInstance) ) {
			output("WARNING: Port " . $aConfiguration[$pInstance]['port'] . " already exists under instance $instance.\n");
			output("         Please make resolving conflict for example by having different IP addresses\n");
			output("         (bind_address) or reconfiguring instance.\n");
		}
	}


	// Ask for socket

	$socket = askForSocket($aConfiguration[$pInstance]);
	$aConfiguration[$pInstance]['socket'] = $socket;


	// Ask for my.cnf

	$mycnf = askForMyCnf($aConfiguration[$pInstance]);
	$aConfiguration[$pInstance]['my.cnf'] = $mycnf;


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
		$question = "Shall I install a MySQL database under " . $aConfiguration['datadir'] . " (Y/n)? ";
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
					$cmd = "sudo yum install libaio";
					output("  shell> $cmd\n\n");
					break;
				case 'SUSE':
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
				output("Installing MySQL instance. Please be patient, this can take a while...\n");

				// Check if $basedir/my.cnf exists already
				// if not delete it afterwards again.
				$lMyCnfExisted = false;
				if ( is_file($aConfiguration['basedir'] . '/my.cnf') ) {
					$lMyCnfExisted = true;
				}

				$cmd = $bin . " --no-defaults $sec --user=" . $aConfiguration['user'] . " --datadir=" . $aConfiguration['datadir'] . " --basedir=" . $aConfiguration['basedir'];
				output("  $cmd\n");
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
	$aMyCnfCandidates = array(
		'/etc/my.cnf'
	, '/etc/mysql/my.cnf'
	, $aConfiguration['datadir'] . '/my-' . $aConfiguration['port'] . '.cnf'
	, $aConfiguration['datadir'] . '/my.cnf'
	);

	// Default my.cnf:
	if ( isset($aConfiguration['my.cnf']) && ($aConfiguration['my.cnf'] == '') ) {
		$default = $aConfiguration['datadir'] . "/my.cnf";
	}
	elseif ( isset($aConfiguration['my.cnf']) && ($aConfiguration['my.cnf'] != '') ) {
		$default = $aConfiguration['my.cnf'];
		array_push($aMyCnfCandidates, $default);
	}
	else {
		$default = $aConfiguration['datadir'] . '/my.cnf';
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
		output("Creating file " . $aConfiguration['my.cnf'] . "\n");

		// todo: would be better to get it from outside!
		$basedir = dirname(dirname(__FILE__));
		// read our template
		$tpl = file_get_contents($basedir . '/tpl/my.cnf.template');
		// Substitue all %VARIABLES%
		$aSearch = array('%MYCNF%', '%PORT%', '%SOCKET%', '%BASEDIR%', '%DATADIR%', '%SERVERID%', '%UNAME%', '%INSTANCE%');
		if ( $aConfiguration['name'] != 'mysqld' ) {
			$lInstance = $aConfiguration['name'] . '_';
		}
		else {
			$lInstance = '';
		}
		$aReplace = array($aConfiguration['my.cnf'], $aConfiguration['port'], $aConfiguration['socket'], $aConfiguration['basedir'], $aConfiguration['datadir'], $aConfiguration['port'], php_uname('n'), $lInstance);
		$cnf = str_replace($aSearch, $aReplace, $tpl);

		ini_set('track_errors', 1);
		if ( $fh = @fopen($aConfiguration['my.cnf'], 'w') ) {
			fwrite($fh, $cnf);
			fclose($fh);
			chmod($aConfiguration['my.cnf'], 0600);
		}
		// Opening my.cnf for writing failed (e.g. /etc/my.cnf)
		else {
			$rc = 706;
			error("Writing to file " . $aConfiguration['my.cnf'] . " failed (rc=$rc)");
			error($php_errormsg);
		}
	}

	output("\n");

	return $aConfiguration['my.cnf'];
}

// ---------------------------------------------------------------------
function askForSocket($aConfiguration, $tSocket = '')
// ---------------------------------------------------------------------
{
	$aSocketCandidates = array(
		'/var/run/mysqld/mysql-' . $aConfiguration['port'] . '.sock'
	, '/var/run/mysqld/mysql-' . $aConfiguration['name'] . '.sock'
	, '/var/run/mysqld/mysql.sock'
	);

	// Default socket:
	if ( isset($aConfiguration['socket']) && ($aConfiguration['socket'] == '') ) {
		// /var/run/mysqld/mysql-3306.sock
		$default =  "/var/run/mysqld/mysql-" . $aConfiguration['port'] . ".sock";
	}
	elseif ( isset($aConfiguration['socket']) && ($aConfiguration['socket'] != '') ) {
		$default =  $aConfiguration['socket'];
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
		$aConfiguration['socket'] = readline("socket  = ");
	}
	else {
		$aConfiguration['socket'] = $tSocket;
	}

	if ( $aConfiguration['socket'] == '' ) {
		$aConfiguration['socket'] = $default;
	}
	output("\n");

	return $aConfiguration['socket'];
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

	$aBasedirCandidates = array_unique($aBasedirCandidates);

	output("\n");
	output("Set basedir. The basedir is the directory where your MySQL binaries are located.\n");
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
	);

	// Default datadir:

	$default = $lHomeDir . '/data/' . $aConfiguration['name'];
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
	output("Set datadir. The datadir is the directory where your MySQL data are located.\n");
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
		mkdir($aConfiguration['datadir'], 0755, true);
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
	);
	$lDistribution = 'unknown';

	$lOs = getOs();
	if ( $lOs == 'Linux' ) {
		$lDistribution = getDistribution();
	}

	foreach ( $aInitScritps as $script ) {

		while ( file_exists($script) ) {
			$s = explode('/', $script);
			output("\n");
			output("You still have an old init script ($script) which can confuse\n");
			output("MyEnv during server boot. Please remove and deactivate this init script.\n\n");

			switch ($lDistribution) {
			case 'CentOS':
			case 'RedHat':
			case 'Fedora':
			case 'SUSE':
			case 'openSUSE project':
				$cmd = "sudo chkconfig --del " . $s[3] . "; sudo rm -f $script";
				output("  shell> $cmd\n\n");
				break;
			case 'Ubuntu':
			case 'Debian':
				$cmd = "sudo rm -f $script; sudo update-rc.d " . $s[3] . " remove";
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
function addMyEnvInitScript($pInitScript, $tKey = '')
// ---------------------------------------------------------------------
{
	$rc = OK;
	$lDistribution = 'unknown';

	$cmd = "sudo cp " . $_ENV['MYENV_BASE'] . "/bin/myenv.server " . $pInitScript;

	$lOs = getOs();
	if ( $lOs == 'Linux' ) {
		$lDistribution = getDistribution();
	}

	switch ($lDistribution) {
	case 'CentOS':
	case 'RedHat':
	case 'Fedora':
	case 'SUSE':
	case 'openSUSE project':
		$cmd1 = "sudo chkconfig --add myenv; sudo chkconfig myenv on";
		break;
	case 'Ubuntu':
	case 'Debian':
		$cmd1 = "sudo update-rc.d myenv defaults";
		break;
	default:
		$cmd1 = 'echo "Unknown distribution: ' . $lDistribution . '"';
		output("OS/Distribution $lOs/$lDistribution is not yet supported.\n");
	}

	while ( ! file_exists($pInitScript) ) {

		output("\nPlease copy yourself manually the MyEnv start/stop script to the following location\n");
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
		$lSourceSequence  = ". /etc/myenv/MYENV_BASE\n";
		$lSourceSequence .= 'MYENV_PWD=$(pwd)' . "\n";
		$lSourceSequence .= 'cd $MYENV_BASE/bin' . "\n";
		$lSourceSequence .= ". myenv.profile\n";
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
		changeInstance($instance);
	}
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
			break;
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
		$key = answerQuestion("Do you want to install MyEnv as 	root, install as Other user or Abort? [r/O/a]? ", array('r', 'o', 'a'), 'o');
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
					break;
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
  help            Print this help.

Examples:

  $script --operation=delete --instance=test

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
		return($rc);
	}

	// Allowed operations
	if ( ! in_array($aOptions['operation'], array('add', 'change', 'delete')) ) {
		$rc = 731;
		$msg = "Invalid operation " . $aOptions['operation'] . ". Valid operations are add, change, delete. (rc=$rc)";
		error($msg);
		return($rc);
	}

	// Operations are not allowed yet
	if ( in_array($aOptions['operation'], array('add', 'change')) ) {
		$rc = 732;
		$msg = "Operation add and change are not supported yet. (rc=$rc)";
		error($msg);
		return($rc);
	}

	// Check if instance exists
	list($ret, $aConfiguration) = getConfiguration($pConfFile);

	if ( ! isset($aConfiguration[$aOptions['instance']]) ) {
		$rc = 733;
		$msg = "Instance " . $aOptions['instance'] . " does not exist. Please choose a valid instance name (rc=$rc).";
		error($msg);
		return $rc;
	}

	return($rc);
}

// ---------------------------------------------------------------------
function deleteInstanceAutomatized($aOptions, $pConfFile)
// ---------------------------------------------------------------------
{
	$rc = OK;

	list($ret, $aConfiguration) = getConfiguration($pConfFile);

	// Check if database is still running
	$ret = checkDatabase($aConfiguration[$aOptions['instance']]);

	if ( $ret == OK ) {
		$rc = 734;
		$msg = "Please stop instence first before deleting it (rc=$rc).";
		error($msg);
		return $rc;
	}
	else {
		unset($aConfiguration[$aOptions['instance']]);
		writeConfigurationFile($aConfiguration, $pConfFile);
	}

	return($rc);
}

?>
