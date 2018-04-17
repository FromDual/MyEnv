#!/usr/bin/php -d variables_order=EGPCS
<?php

// Caution $_ENV variables are not be known here yet when variables_order
// does NOT contain E!!!
if ( isset($_ENV['MYENV_BASE']) ) {
  $basedir = strval($_ENV['MYENV_BASE']);
}
// We have to guess:
else {
  $basedir = dirname(dirname(__FILE__));
}

$_ENV['MYENV_BASE'] = $basedir;

require_once($basedir . '/lib/Constants.inc');
require_once($basedir . '/lib/myEnv.inc');

$rc = OK;

$productdir = dirname($basedir);

output("\n");
output('The following FromDual Toolbox Packages are installed:' . "\n");
output('------------------------------------------------------------------------' . "\n");
output('MyEnv:           2.0.0 (966) ' . "\n");

// todo: Check in releases what names and versions are available
$aLocations = array('brman', 'fromdual_bman', 'fromdual_brman', 'fromdual_brman');
$aExecutables = array('bman', 'brman', 'fromdual_bman', 'fromdual_brman');
$version = 'not found';

foreach ( $aLocations as $location ) {

	foreach ( $aExecutables as $executable ) {
	
		$exe = $productdir . '/' . $location . '/bin/' . $executable;
		if ( is_executable($exe) ) {

			$cmd = $exe . ' --version';
			list($rc, $output, $stdout, $stderr) = my_exec($cmd);
			$version = $output;
			break 2;
		}
	}
}

output('BRman:           ' . $version . "\n");

// Do we want to provide this information at all for security reasons?
$version = 'not available';
output('OpsCenter:       ' . $version . "\n");
/*
  Not supported yet
  https://127.0.0.1/focmm/api/version.json request -> json
*/

// todo: Check in releases what names and versions are available
$aLocations = array('/opt/fpmmm', $productdir . '/fpmmm', '/opt/mpm', $productdir . '/mpm');
$aExecutables = array('fpmmm', 'mpm');

$version = 'not found';

foreach ( $aLocations as $location ) {

	foreach ( $aExecutables as $executable ) {
	
		$exe = $location . '/bin/' . $executable;
		if ( is_executable($exe) ) {

			$cmd = $exe . ' --version';
			list($rc, $output, $stdout, $stderr) = my_exec($cmd);
			$version = $output;
			break 2;
		}
	}
}

output('Fpmmm:           ' . $version . "\n");


$aLocations = array($productdir . '/nagios-mysql-plugins', '/opt/nagios-mysql-plugins');
$aExecutables = array('check_db_mysql.pl');

$version = 'not found';

foreach ( $aLocations as $location ) {

	foreach ( $aExecutables as $executable ) {

		$exe = $location . '/' . $executable;
		if ( is_executable($exe) ) {

			$cmd = $exe . ' --version';
			list($rc, $output, $stdout, $stderr) = my_exec($cmd);
			if ( $rc == OK ) {
				$version = $output;
			}
			else {
				$version = 'Not supported yet.';
			}
			break 2;
		}
	}
}

output('Nagios plug-ins: ' . $version . "\n");


$lOs = getOs();
$lDistribution = getDistribution();
output('O/S:             ' . $lOs . ' / ' . $lDistribution . "\n");

/*
lsb_release -a
No LSB modules are available.
Distributor ID: Ubuntu
Description:    Ubuntu 16.04.3 LTS
Release:        16.04
Codename:       xenial

*/


$lConfigurationFile = '/etc/myenv/myenv.conf';
if ( ! is_file($lConfigurationFile) ) {
	$rc = 554;
	$msg = 'Warning: Configuration file ' . $lConfigurationFile . ' does not exist!' . " (rc=$rc)";
	error($msg . "\n");
	exit($rc);
}

list($ret, $aConfiguration) = getConfiguration($lConfigurationFile);
if ( $ret != OK ) {
	$rc = 555;
	$msg = print_r($aConfiguration, true) . " (rc=$rc)";
	error($msg . "\n");
	exit($rc);
}
$aDbNames = getSectionTitles($aConfiguration);

$aDBbinaries = array();
foreach ( $aDbNames as $db ) {

	$bin = basename($aConfiguration[$db]['basedir']);
	if ( ! in_array($bin, $aDBbinaries) ) {
		array_push($aDBbinaries, $bin);
	}
}

$i = 1;
foreach ( $aDBbinaries as $bin ) {
	$head = '                 ';
	if ( $i == 1 ) {
		$head = 'Binaries:        ';
	}
  output($head . $bin . "\n");
  $i++;
}

output('------------------------------------------------------------------------' . "\n");
output("\n");

exit($rc);

?>
