#!/usr/bin/php
<?php

// Turn error reporting off. We handle it ourself?
error_reporting(0);

$rc = 0;
$gMyName = basename($argv[0]);

// $shortopts = 'u:h:p:P:S:';
$shortopts = '';
$longopts  = array(
  'help'
, 'interval:'
, 'count:'
, 'type:'
);

// Defaults
$gOptions  = array(
  'help'     => 0
, 'interval' => ''
, 'count'    => 1
, 'type'     => ''
);

$gDsn1 = 'root@127.0.0.1:3306';
$gDsn2 = 'root@127.0.0.1:3306';

// -----------------------------------------------------------------------------
function usage()
// -----------------------------------------------------------------------------
{
  global $gMyName, $gOptions;

  echo "
Usage: $gMyName [OPTION] DSN [DSN]

Compares 2 MySQL states

  --interval   Interval between 2 compares.
  --count      Number of times to compare.
  --type       Type of compare. Supported at the moment are:
               * variables
               * status
  --help       Print this help.

A DSN looks as follows: user/password@host:port

Examples:

  $gMyName --type=status --interval=10 --count=3 root@127.0.0.1:3306
  $gMyName --type=variables root/secret@192.168.1.31:3306 root/secret@192.168.1.32:3306

Report bugs to <remote-dba@fromdual.com>.

";
}

// -----------------------------------------------------------------------------
function splitDSN( $dsn )
// -----------------------------------------------------------------------------
{
  $u = '';
  $h = '';

  $a1 = explode('@', $dsn);
  if ( count($a1) == 1 ) {
    $u = $a1[0];
    $h = '';
  }
  elseif ( count($a1) == 2 ) {
    $u = $a1[0];
    $h = $a1[1];
  }
  else {
    $rc = 305;
    echo "ERROR: Illegal DSN: $dsn (rc=$rc).\n";
    exit($rc);
  }

  $user = '';
  $password = '';
  $host = '';
  $port = '';

  $a2 = explode('/', $u);
  if ( count($a2) == 1 ) {
    $user = $a2[0];
    $password = '';
  }
  elseif ( count($a2) == 2 ) {
    $user = $a2[0];
    $password = $a2[1];
  }
  else {
    $rc = 306;
    echo "ERROR: Illegal DSN: $dsn (rc=$rc).\n";
    exit($rc);
  }

  $a3 = explode(':', $h);
  if ( count($a3) == 1 ) {
    $host = $a3[0];
    $port = 3306;
  }
  elseif ( count($a3) == 2 ) {
    $host = $a3[0];
    $port = intval($a3[1]);
  }
  else {
    $rc = 373;
    echo "ERROR: Illegal DSN: $dsn (rc=$rc).\n";
    exit($rc);
  }
  return array('user' => $user, 'password' => $password, 'host' => $host, 'port' => $port);
}

// -----------------------------------------------------------------------------
function getConnection($dsn)
// -----------------------------------------------------------------------------
{
  // var_dump($dsn);
  $mysqli = mysqli_connect($dsn['host'], $dsn['user'], $dsn['password'], null, $dsn['port'], null);

  if ( ! $mysqli ) {
    $rc = 309;
    echo "ERROR: " . mysqli_connect_error() . " (rc=$rc).\n";
    usage();
    exit($rc);
  }
  return $mysqli;
}


// -----------------------------------------------------------------------------
function compareVariables($aDSN)
// -----------------------------------------------------------------------------
{

  $aDSN1 = splitDSN($aDSN[0]);
  $aDSN2 = splitDSN($aDSN[1]);
  // var_dump($aDSN1);
  // var_dump($aDSN2);

  $mysqli1 = getConnection($aDSN1);
  $mysqli2 = getConnection($aDSN2);

  $sql = "SHOW GLOBAL VARIABLES";
  if ( ! $result = $mysqli1->query($sql) ) {
    $rc = 310;
    echo "ERROR: Invalid query: $sql, " . $mysqli1->error . " (rc=$rc).\n";
    exit($rc);
  }

  $aVariables1 = array();
  while ( $record = $result->fetch_assoc() ) {
    $aVariables1[$record['Variable_name']] = $record['Value'];
  }

  if ( ! $result = $mysqli2->query($sql) ) {
    $rc = 311;
    echo "ERROR: Invalid query: $sql, " . $mysqli2->error . " (rc=$rc).\n";
    exit($rc);
  }

  $aVariables2 = array();
  while ( $record = $result->fetch_assoc() ) {
    $aVariables2[$record['Variable_name']] = $record['Value'];
  }

  $mysqli1->close();
  $mysqli2->close();

  // var_dump($aVariables1);
  // var_dump($aVariables2);

  $aDiff = array();
  foreach ( $aVariables1 as $key => $value ) {
    $aDiff[$key] = array($value, null);
  }

  // var_dump($aDiff);

  foreach ( $aVariables2 as $key => $value ) {

    // Variable exists already
    if ( isset($aDiff[$key]) ) {
      $v = $aDiff[$key];
      $v[1] = $value;
      $aDiff[$key] = $v;
    }
    // Variable does not exist
    else {
      $aDiff[$key] = array(null, $value);
    }
  }

  // var_dump($aDiff);

  printf("%-30s  %-40s  %-40s\n", 'Variable', 'DSN 1', 'DSN 2');
  printf("%-30s  %-40s  %-40s\n", '---------------------', '--------------------', '-------------------');

  foreach ( $aDiff as $key => $value ) {

    if ( $value[0] != $value[1] ) {
      printf("%-30s  %-40s  %-40s\n", substr($key, 0, 30), substr($value[0], 0, 40), substr($value[1], 0, 40));
    }
  }
}

// -----------------------------------------------------------------------------
function compareStatus($aDSN, $pInterval = 1, $pCount = 3)
// -----------------------------------------------------------------------------
{
  $aDSN1 = splitDSN($aDSN[0]);
  $mysqli = getConnection($aDSN1);

  $sql = "SHOW /*!50000 GLOBAL */ STATUS";
  if ( ! $result = $mysqli->query($sql) ) {
    $rc = 321;
    echo "ERROR: Invalid query: $sql, " . $mysqli->error . " (rc=$rc).\n";
    exit($rc);
  }

  $aStatusOld = array();
  $aStatusNew = array();
  while ( $record = $result->fetch_assoc() ) {
    $aStatusNew[$record['Variable_name']] = $record['Value'];
  }

  for ( $i = 1; $i <= $pCount; $i++ ) {

    sleep($pInterval);
    $aStatusOld = $aStatusNew;

    if ( ! $result = $mysqli->query($sql) ) {
      $rc = 320;
      echo "ERROR: Invalid query: $sql, " . $mysqli->error . " (rc=$rc).\n";
      exit($rc);
    }
    while ( $record = $result->fetch_assoc() ) {
      $aStatusNew[$record['Variable_name']] = $record['Value'];
    }

    printf("\n%-25s  %-20s  %-20s  %-20s  %-20s\n", 'Status', 'old', 'new', 'delta', '1/s');
    printf("%-25s  %-20s  %-20s  %-20s  %-20s\n", '---------------------', '---------------------', '--------------------', '-------------------', '-------------------');
    foreach ( $aStatusNew as $key => $value ) {
      if ( $aStatusNew[$key] != $aStatusOld[$key] ) {
      printf("%-25s  %-20s  %-20s  %-20s  %-20s\n", substr($key, 0, 30), substr($aStatusOld[$key], 0, 40), substr($aStatusNew[$key], 0, 40), $aStatusNew[$key]-$aStatusOld[$key], ($aStatusNew[$key]-$aStatusOld[$key])/$pInterval);
      }
    }
  }

  $mysqli->close();
}

// -----------------------------------------------------------------------------
// MAIN
// -----------------------------------------------------------------------------

// $options = getopt($shortopts);
$options = getopt($shortopts, $longopts);

foreach ( $options as $key => $value ) {

  switch ( $key ) {
  case 'type':
    if ( $value != '' ) {
      $gOptions[$key] = $value;
    }
  break;
  case 'interval':
  case 'count':
    if ( intval($value) != 0 ) {
      $gOptions[$key] = intval($value);
    }
  break;
  case 'help':
    usage();
    exit($rc);
  break;
  default:
    $rc = 302;
    usage();
    exit($rc);
  break;
  }
}

// var_dump($argv);

$aDSN = array();
for ( $i = 1; $i < count($argv); $i++ ) {

  if ( substr($argv[$i], 0, 1) != '-' ) {
    array_push($aDSN, $argv[$i]);
  }
}
// var_dump($aDSN);

// Check variables

// none

if ( strtoupper($gOptions['type']) == 'VARIABLES' ) {

  if ( count($aDSN) != 2 ) {
    $rc = 307;
    echo "ERROR: We need exact 2 DSN (rc=$rc).\n";
    exit($rc);
  }

  compareVariables($aDSN);
}
elseif ( strtoupper($gOptions['type']) == 'STATUS' ) {
  if ( count($aDSN) != 1 ) {
    $rc = 308;
    echo "ERROR: We need exact 1 DSN (rc=$rc).\n";
    exit($rc);
  }

  compareStatus($aDSN, $gOptions['interval'], $gOptions['count']);
}
else {
  $rc = 303;
  echo "ERROR: Invalid type " . $gOptions['type'] . "\n";
  echo "       Use one of VARIABLES, STATUS (rc=$rc).\n";
  usage();
  exit($rc);
}

exit($rc);

?>
