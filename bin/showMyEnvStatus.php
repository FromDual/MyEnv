#!/usr/bin/php -d variables_order=EGPCS
<?php

/* 
// Caution $_ENV variables are not be known here yet when variables_order
// does NOT contain E!!!
if ( isset($_ENV['MYENV_BASE']) ) {
  $basedir = strval($_ENV['MYENV_BASE']);
}
// We have to guess:
else {
  $basedir = dirname(dirname(__FILE__));
}
*/

// I think guessing ist better than the above stuff!!!
$basedir = dirname(dirname(__FILE__));

require_once($basedir . '/lib/myEnv.inc');
require_once($basedir . '/lib/Constants.inc');

$rc = OK;

if ( checkMyEnvRequirements() == ERR ) {
  $rc = 535;
  exit($rc);
}

// The first is possibly a bug!!!
$lHome   = $_ENV['HOME'] . '/product';
$lHome   = '/home/mysql' . '/product';

$lDebug  = isset($_ENV['MYENV_DEBUG']) ? intval($_ENV['MYENV_DEBUG']) : 0;
$basedir = $_ENV['MYENV_BASE'];

$lConfigurationFile = '/etc/myenv/myenv.conf';
if ( ! is_file($lConfigurationFile) ) {
	$rc = 534;
	output("Warning: Configuration file $lConfigurationFile does not exist!\n");
	output("         Please create or copy from $lConfigurationFile.template (rc=$rc).\n");
	exit($rc);
}

list($ret, $aConfiguration) = getConfiguration($lConfigurationFile);
if ( $ret != OK ) {
	$rc = 504;
	error(print_r($aConfiguration, true) . " (rc=$rc)");
	// Continue because it was always working this way...
}
$aDbNames = getSectionTitles($aConfiguration);

// Get available release versions

$aReleaseVersion = array();
foreach ( glob("$lHome/*", GLOB_ONLYDIR) as $dir ) {
	$v = extractVersion($dir);
	if ( $v == 'unknown' ) {
		$v = getVersionFromMysqld('/usr');
	}
	array_push($aReleaseVersion, $v);
}
$aReleaseVersion = array_unique($aReleaseVersion, SORT_REGULAR);

debug(print_r($aReleaseVersion, true));

// Get version of each instance
foreach ( $aConfiguration as $db => $value ) {

	if ( $db != 'default' ) {
		$v = extractVersion($aConfiguration[$db]['basedir']);
		if ( $v == 'unknown' ) {
			$v = getVersionFromMysqld('/usr');
		}
		$aConfiguration[$db]['version'] = $v;
	}
}

// Check if shell is interactive or not
// https://www.gnu.org/software/bash/manual/html_node/Is-this-Shell-Interactive_003f.html
// himBH
// hBc
// non-interactive
if ( isset($_ENV['PS1']) && ($_ENV['PS1'] != '') ) {
	// Get number of columns of screen:
	$cmd = 'tput cols';
	// my_exec produces the wrong result (= 80)
	// list($ret, $output, $stdout, $stderr) = my_exec($cmd);
	// $lNumberOfColumns = trim($output);
	$lNumberOfColumns = trim(`$cmd`);
}
// interactive
else {
	// Get number of columns of screen:
	$lNumberOfColumns = 80;
}

debug("Number of Columns: $lNumberOfColumns\n");


// Releases does not make sense anymore with all the differnce forks
// output("\n");
// output("Releases : " . implode(' ', $aReleaseVersion) . "\n");
output("\n");

// Display up and down instances

$aUp      = array();
$aDown    = array();
$aPassive = array();   // For passive databases on an active/passive fail-over cluster

foreach ( $aDbNames as $db ) {

  if ( count($aConfiguration[$db]) == 0 ) {
    $rc = 539;
    error("(rc=$rc)");
    continue;
  }

	// Ignore passive databases in an active/passive fail-over Cluster
	// Criteria is that datadir is missing!
	if ( isset($aConfiguration[$db]['ignore-passive'])
	  && ($aConfiguration[$db]['ignore-passive'] == 'yes')
		&& ( ! is_dir($aConfiguration[$db]['datadir']))
	  ) {
		$aPassive[$db] = "$db (" . $aConfiguration[$db]['version'] . ")";
	}
	// Database is NOT passive so check if it is up or down.
	else {

		$ret = checkDatabase($aConfiguration[$db]);

		if ( $ret != 0 ) {
			$aDown[$db] = "$db (" . $aConfiguration[$db]['version'] . ")";
		}
		else {
			$aUp[$db]   = "$db (" . $aConfiguration[$db]['version'] . ")";
		}
	}
}

if ( $lDebug > 1 ) {
	debug('Up  : ');
	debug(print_r($aUp, true));
	debug('Down: ');
	debug(print_r($aDown, true));
	debug('Passive: ');
	debug(print_r($aPassive, true));
}

// Print Up line
$cnt = 1;
foreach ( explode("\n", wordwrap(implode(' ', $aUp), $lNumberOfColumns-12)) as $line ) {

  if ( $cnt == 1 ) {
    output("Up       : " . $line . "\n");
  }
  else {
    output("           " . $line . "\n");
  }
  $cnt++;
}
output("\n");

// Print Down line
$cnt = 1;
foreach ( explode("\n", wordwrap(implode(' ', $aDown), $lNumberOfColumns-12)) as $line ) {

  if ( $cnt == 1 ) {
    output("Down     : " . $line . "\n");
  }
  else {
    output("           " . $line . "\n");
  }
  $cnt++;
}
output("\n");

// Print Passive line only if some are there
if ( count($aPassive) > 0 ) {
	$cnt = 1;
	foreach ( explode("\n", wordwrap(implode(' ', $aPassive), $lNumberOfColumns-12)) as $line ) {

		if ( $cnt == 1 ) {
			output("Passive  : " . $line . "\n");
		}
		else {
			output("           " . $line . "\n");
		}
		$cnt++;
	}
	output("\n");
}

// Find the longes instance name
$max_len = 16;
if ( count($aDbNames) > 0 ) {
	$max_len = max(array_map('strlen', $aDbNames));
}

debug(sprintf("#cols=%d, max_len=%d\n", $lNumberOfColumns, $max_len));


// Get longest bind-address

$lIpLength = 1;   // '*' has length of 1
foreach ( $aDbNames as $db ) {

	list($ret, $aMyCnf) = getConfiguration($aConfiguration[$db]['my.cnf']);
	if ( $ret == OK ) {
		foreach ( array('bind_address', 'bind-address', 'bind_addr', 'bind-addr') as $key ) {
			if ( array_key_exists($key, $aMyCnf['mysqld']) ) {
				$lIpLength = max($lIpLength, strlen($aMyCnf['mysqld'][$key])); 
			}
		}
	}
}


// Print all instances

foreach ( $aDbNames as $db ) {

	$aSchema = getSchemaNames($aConfiguration[$db]['datadir']);
	debug(print_r($aSchema, true));

	// hide schema, e.g mysql, ndbinfo, test, performance_schema, pbxt
	if ( array_key_exists('hideschema', $aConfiguration[$db]) ) {

		foreach ( explode(',', $aConfiguration[$db]['hideschema']) as $tohide ) {
		
			if ( ($key = array_search($tohide, $aSchema)) !== false ) {
				unset($aSchema[$key]);
			}
		}
	}

	// Ignore passive databases in an active/passive fail-over Cluster
	// Criteria is that datadir is missing!
	if ( isset($aConfiguration[$db]['ignore-passive'])
	  && ($aConfiguration[$db]['ignore-passive'] == 'yes')
		&& ( ! is_dir($aConfiguration[$db]['datadir']))
		) {
		// noop
	}
	else {
		// Split the schema output
		$cnt = 1;
		
		foreach ( explode("\n", wordwrap(implode(' ', $aSchema), $lNumberOfColumns-$max_len-$lIpLength-12)) as $line ) {

			if ( $cnt == 1 ) {
				$ip = '*';

				list($ret, $aMyCnf) = getConfiguration($aConfiguration[$db]['my.cnf']);
				if ( $ret == OK ) {
					foreach ( array('bind_address', 'bind-address', 'bind_addr', 'bind-addr') as $key ) {
						$ip = array_key_exists($key, $aMyCnf['mysqld']) ?  $aMyCnf['mysqld'][$key] : $ip;
					}
				}
				else {
					// Error is already printed here when necessary...
					// do not exit here. We want to see output for others still...
				}

				output(sprintf("%-" . $max_len . "s (%" . $lIpLength . "s:%-5d) : %s\n", $db, $ip, $aConfiguration[$db]['port'], $line));
			}
			else {
				output(sprintf("%s %s\n", str_repeat(' ', $max_len+$lIpLength+11), $line));
			}
			$cnt++;
		}
	}
}
output("\n");


// Interface/hook  for customer scripts/plugins

debug('Plugins to execute:');
$cnt = 0;

$lMyNameBase = basename($argv[0], '.php');
foreach ( glob($basedir . '/plg/' . $lMyNameBase . '/*') as $plugin ) {

  debug(' ' . $plugin);
  if ( is_executable($plugin) !== TRUE ) {
    error("$plugin is NOT executable.");
  }
  else {

    $cmd = $plugin;
    list($ret, $output, $stdout, $stderr) = my_exec($cmd);
    foreach ( $stdout as $line ) {
      output("$line\n");
    }

    if ( $ret != 0 ) {
      $rc = $ret;
    }
  }
  $cnt++;
}
if ( $cnt > 0 ) {
  output("\n");
}

debug('Check for old stuff:');
checkForOldStuff($basedir);

exit($rc);
?>
