#!/usr/bin/php
<?php

# watch -d -n 1 'grep "^TCP:" /proc/net/sockstat | sed "s/^.*\(tw [0-9]*\).*$/\1/"'

$lHost     = '127.0.0.1';
$lUser     = 'app';
$lPassword = 'secret';
$lDatabase = 'test';
$lPort     = '3306';
$lSocket   = '/var/run/mysqld/mysql.sock';

$rc  = 0;

while ( true ) {

	$mysqli = @new mysqli($lHost, $lUser, $lPassword, $lDatabase, $lPort, $lSocket);

	if ( mysqli_connect_error() ) {
		print '-';
	}
	else {
		$mysqli->close();
		print '+';
	}
}

exit($rc);

?>
