#!/usr/bin/php -d variables_order=EGPCS
<?php

$basedir = dirname(dirname(__FILE__));

require_once($basedir . '/lib/myEnv.inc');
require_once($basedir . '/lib/Constants.inc');

$rc = OK;

$lConfigurationFile = '/etc/myenv/myenv.conf';

if ( ! is_file($lConfigurationFile) ) {
	$rc = 513;
	error("Warning: Configuration file $lConfigurationFile does not exist!\n");
	exit($rc);
}

list($ret, $aConfiguration) = getConfiguration($lConfigurationFile);
if ( $ret != OK ) {
	$rc = 514;
	error(print_r($aConfiguration, true) . " (rc=$rc)");
	exit($rc);
}

$aDbNames = getSectionTitles($aConfiguration);
output(implode(' ', $aDbNames) . "\n");

exit($rc);
?>
