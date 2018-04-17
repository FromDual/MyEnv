#!/usr/bin/php
<?php

$lHost     = '127.0.0.1';
$lUser     = 'app';
$lPassword = 'secret';
$lDatabase = 'test';
$lPort     = '3306';
$lSocket   = '/var/run/mysqld/mysql.sock';

$rc = 0;

$mysqli = new mysqli($lHost, $lUser, $lPassword, $lDatabase, $lPort, $lSocket);

if ( mysqli_connect_error() ) {
  $rc = 383;
  fprintf(STDERR, "ERROR: Connect failed: (%d) %s\n", mysqli_connect_errno(), mysqli_connect_error());
  exit($rc);
}
$mysqli->query('SET NAMES utf8');

$sql = 'INSERT INTO test values (NULL, "Test data insert", NULL);';

while ( true ) {

  if ( ! $mysqli->query($sql) ) {
  
    fprintf(STDERR, "\nERROR %d: %s\n", $mysqli->errno, $mysqli->error);
    do {

      $retry = 5;
      fwrite(STDERR, "Retry in $retry seconds...\n");
      sleep($retry);

      $mysqli = new mysqli($lHost, $lUser, $lPassword, $lDatabase, $lPort, $lSocket);

      if ( mysqli_connect_error() ) {
        fprintf(STDERR, "ERROR: Connect failed: (%d) %s\n", mysqli_connect_errno(), mysqli_connect_error());
      }
      else {
        $mysqli->query('SET NAMES utf8');
      }
    } while ( mysqli_connect_error()  != null );
  }
  else {
    print '.';
    usleep(10);
  }
}

$mysqli->close();
exit($rc);

?>
