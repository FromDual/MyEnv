#!/usr/bin/php
<?php

$gOratab = '/etc/oratab';

$basedir = dirname(dirname(__FILE__));
require_once($basedir . '/lib/Constants.inc');

// ---------------------------------------------------------------------
function printUsage()
// ---------------------------------------------------------------------
{
  global $gMyName;

  print "
Usage: $gMyName {start | stop | status} [<SID>]

Examples:

  $gMyName status agent97b

  $gMyName status

  $gMyName start agent97a

";
}

// ---------------------------------------------------------------------
function getOratab( $pOratab )
// ---------------------------------------------------------------------
{
	$rc = OK;
	$aOratab = array();

	$fh = @fopen($pOratab, 'r');
	if ( ! $fh ) {
		$rc = 401;
		fprintf(STDERR, "ERROR: Cannot open file $pOratab (rc=$rc).\n");
		return array($rc, $aOratab);
	}

	while ( ($buffer = fgets($fh, 4096)) !== false ) {
		array_push($aOratab, trim($buffer));
	}
	if ( ! feof($fh) ) {
		$rc = 395;
		fprintf(STDERR, "ERROR: Unexpected fgets() fail (rc=$rc).\n");
		return array($rc, $aOratab);
	}
	fclose($fh);

	return array($rc, $aOratab);
}

// ---------------------------------------------------------------------
function getAgents( $aOratab )
// ---------------------------------------------------------------------
{
	$rc = OK;
	$aAgents = array();

	foreach ( $aOratab as $line ) {

		// Match line with agent and Dummy
		// There are possibly several agents
		if ( preg_match('/^agent.*:D$/', $line) ) {
			$a = explode(':', $line);
			array_push($aAgents,
				array( 'name' => $a[0]
							, 'ORACLE_HOME' => $a[1]
							, 'type' => $a[2]
							)
			);
		}
	}

	// No agent found
	if ( count($aAgents) == 0 ) {
		$rc = 396;
	}

	return array($rc, $aAgents);
}

// ---------------------------------------------------------------------
function startAgent( $aAgent )
// ---------------------------------------------------------------------
{
	$rc = OK;
	$msg = '';

	$emctl = $aAgent['ORACLE_HOME'] . '/bin/emctl';
	if ( is_executable($emctl) ) {
		$cmd = $emctl . ' start agent';
		list($rc, $output, $stdout, $stderr) = my_exec($cmd);
	}
	else {
		fprintf(STDERR, "ERROR: $emctl is not executable (rc=$rc).\n");
		$rc = 402;
	}

	return array($rc, $msg);
}

// ---------------------------------------------------------------------
function stopAgent( $aAgent )
// ---------------------------------------------------------------------
{
	$rc = OK;
	$msg = '';

	$emctl = $aAgent['ORACLE_HOME'] . '/bin/emctl';
	if ( is_executable($emctl) ) {
		$cmd = $emctl . ' stop agent';
		list($rc, $output, $stdout, $stderr) = my_exec($cmd);
	}
	else {
		fprintf(STDERR, "ERROR: $emctl is not executable (rc=$rc).\n");
		$rc = 399;
	}

	return array($rc, $msg);
}

// ---------------------------------------------------------------------
function getAgentStatus( $aAgent )
// ---------------------------------------------------------------------
{
	global $gDebug;

	$rc = OK;
	$msg = '';
  
	// We ignore agents which are not here but on the other node in an
	// active/passive fail-over set-up and set state to 'n.a.'
	if ( ! is_dir($aAgent['ORACLE_HOME']) ) {
		$state = 'n.a.';
	}
	else {

		$emctl = $aAgent['ORACLE_HOME'] . '/bin/emctl';
		if ( is_executable($emctl) ) {
			$cmd = $emctl . ' status agent';
			list($rc, $output, $stdout, $stderr) = my_exec($cmd);
		}
		else {
			$rc = 391;
		}

		// Agent is Running and Ready
		$state = '';
		if ( $rc == 0 ) {
			$state = 'up';
		}
		// Agent is Not Running
		else {
			$rc = 393;
			$state = 'down';
		}
	}

	// Print the whole stuff
	if ( $state == 'n.a.' ) {
		// Ignore and do NOT print
		if ( $gDebug > 0 ) {
			printf("Agent %-12s: %-7s %s\n", $aAgent['name'], $state, $aAgent['ORACLE_HOME']);
		}
	}
	else {
		printf("Agent %-12s: %-7s %s\n", $aAgent['name'], $state, $aAgent['ORACLE_HOME']);
	}

	return array($rc, $msg);
}

// ---------------------------------------------------------------------
function getAgentStatusV2( $aAgent )
// emctl status is a bad idea because it is very slow
// use ps instead!
// ---------------------------------------------------------------------
{
	global $gDebug;

	$rc  = OK;
	$msg = '';

	// We ignore agents which are not here but on the other node in an
	// active/passive fail-over set-up and set state to 'n.a.'
	if ( ! is_dir($aAgent['ORACLE_HOME']) ) {
		$state = 'n.a.';
	}
	else {

		// /m00/mysql96a/AGENT12/agent_inst -> /m00/mysql96a/AGENT12
		$lAgentBase = dirname($aAgent['ORACLE_HOME']);

		// /m00/mysql96a/AGENT12/core/12.1.0.2.0/jdk/bin/java -Xmx128M -XX:MaxPermSize=96M -server -Djava.security.egd=file:///dev/./urandom -Dsun.lang.ClassLoader.allowArraySyntax=true -XX:+UseLinuxPosixThreadCPUClocks -XX:+UseConcMarkSweepGC -XX:+CMSClassUnloadingEnabled -XX:+UseCompressedOops -Dwatchdog.pid=25116 -cp /m00/mysql96a/AGENT12/core/12.1.0.2.0/jdbc/lib/ojdbc5.jar:/m00/mysql96a/AGENT12/core/12.1.0.2.0/ucp/lib/ucp.jar:/m00/mysql96a/AGENT12/core/12.1.0.2.0/modules/oracle.http_client_11.1.1.jar:/m00/mysql96a/AGENT12/core/12.1.0.2.0/lib/xmlparserv2.jar:/m00/mysql96a/AGENT12/core/12.1.0.2.0/lib/jsch.jar:/m00/mysql96a/AGENT12/core/12.1.0.2.0/lib/optic.jar:/m00/mysql96a/AGENT12/core/12.1.0.2.0/modules/oracle.dms_11.1.1/dms.jar:/m00/mysql96a/AGENT12/core/12.1.0.2.0/modules/oracle.odl_11.1.1/ojdl.jar:/m00/mysql96a/AGENT12/core/12.1.0.2.0/modules/oracle.odl_11.1.1/ojdl2.jar:/m00/mysql96a/AGENT12/core/12.1.0.2.0/sysman/jlib/log4j-core.jar:/m00/mysql96a/AGENT12/core/12.1.0.2.0/jlib/gcagent_core.jar:/m00/mysql96a/AGENT12/core/12.1.0.2.0/sysman/jlib/emagentSDK-intg.jar:/m00/mysql96a/AGENT12/core/12.1.0.2.0/sysman/jlib/emagentSDK.jar oracle.sysman.gcagent.tmmain.TMMain

		$cmd = "ps -ef -www | grep -P --color '$lAgentBase\/core\/[\d\.]+\/jdk\/bin\/java .*oracle.sysman.gcagent'";
		$output = exec($cmd, $stdout, $rc);
		// var_dump($rc, $output, $stdout);

		// Agent is Running and Ready
		$state = '';
		if ( count($stdout) > 0 ) {
			$state = 'up';
		}
		// Agent is Not Running
		else {
			$rc = 394;
			$state = 'down';
		}
	}

	// Print the whole stuff
	if ( $state == 'n.a.' ) {
		// Ignore and do NOT print
		if ( $gDebug > 0 ) {
			printf("Agent %-12s: %-7s %s\n", $aAgent['name'], $state, $aAgent['ORACLE_HOME']);
		}
	}
	else {
		printf("Agent %-12s: %-7s %s\n", $aAgent['name'], $state, $aAgent['ORACLE_HOME']);
	}

	return array($rc, $msg);
}

// ---------------------------------------------------------------------
// MAIN
// ---------------------------------------------------------------------

$rc = OK;

$gMyName = basename($argv[0]);
$gDebug  = isset($_ENV['MYENV_DEBUG']) ? intval($_ENV['MYENV_DEBUG']) : 0;

if ( $argc == 1 ) {
	$option   = 'status';
	$instance = '';
}
elseif ( $argc == 2 ) {
	$option   = $argv[1];
	$instance = '';
}
elseif ( $argc == 3 ) {
	$option   = $argv[1];
	$instance = $argv[2];
}
else {
	$rc = 397;
	printUsage();
	exit($rc);
}

// Check values
if ( ($option == 'start') || ($option == 'stop') ) {
	if ( $instance == '' ) {
		$rc = 398;
		fprintf(STDERR, "Instance name cannot be empty (rc=$rc).\n");
		printUsage();
		exit($rc);
	}
}

list($rc, $aOratab) = getOratab($gOratab);
// print_r($aOratab);
if ( $rc != 0 ) {
	exit($rc);
}
list($rc, $aAgents) = getAgents($aOratab);
// print_r($aAgents);
if ( $rc != 0 ) {
	fprintf(STDERR, "No agent found (rc=$rc).\n");
	exit($rc);
}

if ( $option == 'status' ) {
	foreach ( $aAgents as $agent ) {
		// if instance is empty show all
		// otherwise only the wanted one
		if ( ($instance == '')
			|| ($instance == $agent['name']) ) {
			list($ret, $msg) = getAgentStatusV2($agent);
		}
	}
}
elseif ( $option == 'start' ) {
	foreach ( $aAgents as $agent ) {
		// if instance is empty start all
		// otherwise only the wanted one
		if ( ($instance == '')
			|| ($instance == $agent['name']) ) {
			list($ret, $msg) = startAgent($aAgents);
		}
	}
}
elseif ( $option == 'stop' ) {
	foreach ( $aAgents as $agent ) {
		// if instance is empty stop all
		// otherwise only the wanted one
		if ( ($instance == '')
			|| ($instance == $agent['name']) ) {
			list($ret, $msg) = stopAgent($aAgents);
		}
	}
}
else {
	$rc = 392;
	fprintf(STDERR, "Option $option does not exists (rc=$rc).\n");
	printUsage();
	exit($rc);
}

?>
