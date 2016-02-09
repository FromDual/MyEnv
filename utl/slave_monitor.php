#!/usr/bin/php
<?php

$rc = 0;

$shortopts  = "";

$longopts  = array(
  'socket:'
, 'help'
, 'host:'
, 'port:'
, 'user:'
, 'password:'
, 'database:'
, 'receiver-name:'
, 'receiver-email:'
, 'test'
);

$aOptions = getopt($shortopts, $longopts);

if ( isset($aOptions['help']) ) {
  printUsage();
  exit($rc);
}

// remove argv[0] (script name)
unset($argv[0]);

// too many arguments
if ( count($argv) != count($aOptions) ) {
  $rc = 326;
  print "ERROR: Too many or invalid arguments:\n";
  print print_r($argv, true);
  printUsage();
  exit($rc);
}

$ret = checkArguments($aOptions);
if ( $ret != 0 ) {
  $rc = 327;
  printUsage();
  exit($rc);
}

// ---------------------------------------------------------------------
function checkArguments($aOptions)
// ---------------------------------------------------------------------
{
  $ret = 0;

//   var_dump($aOptions);

  // Defaults
  if ( (! isset($aOptions['receiver-email'])) || ($aOptions['receiver-email'] == '') ) {
    $aOptions['receiver-email'] = 'contact@fromdual.com';
  }
  if ( (! isset($aOptions['receiver-name'])) || ($aOptions['receiver-name'] == '') ) {
    $aOptions['receiver-name']  = 'FromDual Support';
  }
  if ( (! isset($aOptions['host'])) || ($aOptions['host'] == '') ) {
    $aOptions['host']           = '127.0.0.1';
  }
  if ( (! isset($aOptions['user'])) || ($aOptions['user'] == '') ) {
    $aOptions['user']           = 'root';
  }
  if ( (! isset($aOptions['password'])) || ($aOptions['password'] == '') ) {
    $aOptions['password']       = '';
  }
  if ( (! isset($aOptions['database'])) || ($aOptions['database'] == '') ) {
    $aOptions['database']       = '';
  }
  if ( (! isset($aOptions['port'])) || ($aOptions['port'] == '') ) {
    $aOptions['port']           = 3306;
  }
  if ( (! isset($aOptions['socket'])) || ($aOptions['socket'] == '') ) {
    $aOptions['socket']         = null;
  }
//   var_dump($aOptions);
}

// ---------------------------------------------------------------------
function printUsage()
// ---------------------------------------------------------------------
{
  global $gLogFile, $gBackupDir;

  $script = 'slave_monitoring.php';

  print "
usage: $script --host=<hostname> --port=<port> --database=<databasename>
               --user=<username> --password=<password>
               --receiver-email=<email> --receiver-name=<name>
               [--test]

Options:
  host            127.0.0.1
  port            3306
  database        ''
  user            root
  password        secret
  receiver-email  contact@fromdual.com
  receiver-name   FromDual
  test

Examples:

  $script --host=127.0.0.1 --port=3306 --database=test --user=root --password=secret \
  --receiver-email=contact@fromdual.com --receiver-name='FromDual Support'

";
}

// requires php-posix
$uname = posix_uname();

$sender_name   = $uname['nodename'];
$sender_mail   = get_current_user() . '@' . $sender_name;

$to      = '"' . $aOptions['receiver-name'] . '" <' . $aOptions['receiver-email'] . '>';
$headers = 'From: "' . $sender_name .'" <' . $sender_mail . '>"' . PHP_EOL
         . 'X-Mailer: PHP-' . phpversion() . PHP_EOL;

# Send only test mail
if ( isset($aOptions['test']) ) {

  $subject = 'MySQL Slave Monitoring test. Status: OK';
  $message = 'MySQL Slave monitoring works.' . PHP_EOL;

  print $subject . "\n";
  print $message . "\n";
  if ( mail($to, $subject, $message, $headers) ) {
    $rc = 0;
    print "Mail success.\n";
  }
  else {
    $rc = 361;
    print "Mail failed.\n";
  }
  exit($rc);
}
// else {
//   print "Slave works.\n";
// }

# Here starts the MySQL part

$dbh = mysqli_connect($aOptions['host'], $aOptions['user'], $aOptions['password'], $aOptions['database'], $aOptions['port'], $aOptions['socket']);

// This error is so serious that we do not return at all!
if ( ! $dbh ) {

  $rc = 362;
  $subject = 'MySQL Slave Monitoring. Status: ERROR';
  $message = "Read connection failed: " . mysqli_connect_error() . PHP_EOL;

  print $subject . "\n";
  print $message . "\n";
  if ( mail($to, $subject, $message, $headers) ) {
    print "Mail success.\n";
  }
  else {
    print "Mail failed.\n";
  }
  exit($rc);
}

$dbh->query('SET CHARACTER SET utf8');

$sql = 'SHOW SLAVE STATUS';
if ( ! $result = $dbh->query($sql) ) {
  $rc = 363;
  print "ERROR: Invalid query: $sql, " . $dbh->error . "\n";
  exit($rc);
}
$record = $result->fetch_array(MYSQLI_ASSOC);
# var_dump($record);

# Either IO or SQL Thread is not running!
if ( ($record['Slave_IO_Running'] != 'Yes') || ($record['Slave_SQL_Running'] != 'Yes') ) {

  // Does not exist in MySQL 5.0 yet
  if ( ! isset($record['Last_SQL_Error']) ) {
    $record['Last_SQL_Errno'] = '';
    $record['Last_SQL_Error'] = 'n.a. in MySQL 5.0';
    $record['Last_IO_Errno']  = '';
    $record['Last_IO_Error']  = 'n.a. in MySQL 5.0';
  }

  $subject = 'MySQL Slave Monitoring. Status: ERROR';
  $message = 'MySQL Replication has a problem.' . PHP_EOL
           . 'Error is ' . $record['Last_Errno'] . ' / ' . $record['Last_Error'] . PHP_EOL
           . 'Last_SQL_Error: ' . $record['Last_SQL_Errno'] . ' / ' . $record['Last_SQL_Error'] . PHP_EOL
           . 'Last_IO_Error: ' . $record['Last_IO_Errno'] . ' / ' . $record['Last_IO_Error'] . PHP_EOL;

  print $subject . "\n";
  print $message . "\n";
  if ( mail($to, $subject, $message, $headers) ) {
    $rc = 0;
    print "Mail success.\n";
  }
  else {
    $rc = 364;
    print "Mail failed.\n";
  }
  exit($rc);
}

$dbh->close();
exit($rc);

?>
