#!/usr/bin/php
<?php

$skip = false;
if ( array_key_exists(1, $argv) && ($argv[1] == '--skip-zero') ) {
	$skip = true;
}

$aNetstat = file('/proc/net/netstat');

// TcpExt

$a = explode(': ', $aNetstat[0]);
$aTcpExtTitle = $a[0];
$aTcpExtKeys  = explode(' ', trim($a[1]));

// print_r($aTcpExtTitle);
// print_r($aTcpExtKeys);

$aTcpExtValues = explode(': ', $aNetstat[1]);
$aTcpExtValues  = explode(' ', trim($aTcpExtValues[1]));
// print_r($aTcpExtValues);

$aTcpExt = array();
printf("%s:\n", $aTcpExtTitle);
foreach ( array_keys($aTcpExtKeys) as $key ) {

	$aTcpExt[$aTcpExtKeys[$key]] = $aTcpExtValues[$key];

	if ( $skip && ($aTcpExtValues[$key] == 0) ) {
		null;
	}
	else {
		printf("  %-26s %12d\n", $aTcpExtKeys[$key], $aTcpExtValues[$key]);
	}
}


// IpExt

$a = explode(': ', $aNetstat[2]);
$aIpExtTitle = $a[0];
$aIpExtKeys  = explode(' ', trim($a[1]));

// print_r($aIpExtTitle);
// print_r($aIpExtKeys);

$aIpExtValues = explode(': ', $aNetstat[3]);
$aIpExtValues  = explode(' ', trim($aIpExtValues[1]));
// print_r($aIpExtValues);

$aIpExt = array();
printf("\n%s:\n", $aIpExtTitle);
foreach ( array_keys($aIpExtKeys) as $key ) {

	$aIpExt[$aIpExtKeys[$key]] = $aIpExtValues[$key];

	if ( $skip && ($aIpExtValues[$key] == 0) ) {
		null;
	}
	else {
		printf("  %-26s %12d\n", $aIpExtKeys[$key], $aIpExtValues[$key]);
	}
}

?>
