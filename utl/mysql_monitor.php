#!/usr/bin/php
<?php

// ---------------------------------------------------------------------
function setDefauls($aOptions)
// ---------------------------------------------------------------------
{
//   var_dump($aOptions);

  $aDefaults = array (
    'receiver-email' => 'contact@fromdual.com'
  , 'host'           => '127.0.0.1'
  , 'user'           => 'root'
  , 'password'       => ''
  , 'database'       => 'check_db'
  , 'port'           => 3306
  );

  // Make longopts from shortopts
  if ( isset($aOptions['h']) ) {
    $aOptions['help'] = true;
    unset($aOptions['h']);
  }

  foreach ( $aDefaults as $key => $value ) {

    if ( (! isset($aOptions[$key])) || ($aOptions[$key] == '') ) {
      $aOptions[$key] = $value;
    }
  }

//   var_dump($aOptions);
}

// ---------------------------------------------------------------------
function checkArguments($aOptions)
// ---------------------------------------------------------------------
{
  $ret = 0;

//   var_dump($aOptions);

//   var_dump($aOptions);
  return $ret;
}

// ---------------------------------------------------------------------
function printUsage()
// ---------------------------------------------------------------------
{
  global $gBasename;

  $script = $gBasename;

  print "
usage:
  $script --host=<hostname> --port=<port> --database=<databasename>
    --user=<username> --password=<password>
    --receiver-email=<email> [--test]

Options:
  host            127.0.0.1
  port            3306
  database        ''
  user            root
  password        secret
  receiver-email  contact@fromdual.com
  test
  help, h, ?      Help

Examples:

  $script --host=127.0.0.1 --port=3306 --database=test --user=root --password=secret \
  --receiver-email=contact@fromdual.com

";
}

// ---------------------------------------------------------------------
function sendMail($aTo, $subject, $body)
// ---------------------------------------------------------------------
{
  $ret = 0;

  // requires php-posix
  $uname = posix_uname();
  $sender_host   = $uname['nodename'];
  $sender_user   = get_current_user();
  $sender_mail   = $sender_user . '@' . $sender_host;
  $headers = 'From: "' . $sender_user .'" <' . $sender_mail . '>"' . PHP_EOL
          . 'X-Mailer: PHP-' . phpversion();

  $subject = $sender_host . ' - ' . $subject;

  print "Subject: " . $subject . "\n";
  print "Body   : " . $body . "\n";

  foreach ( $aTo as $to ) {

    if ( mail('"MySQL Monitor" <' . $to . '>', $subject, $body, $headers) ) {
      print "Mail sent to $to successfully.\n";
    }
    else {
      $ret++;
      print "Sending mail to $to failed.\n";
    }
  }

  return $ret;
}

// ---------------------------------------------------------------------
// MAIN
// ---------------------------------------------------------------------

$rc = 0;

// We catch all errors properly! If not, set this to -1:
// error_reporting(-1);
error_reporting(0);

$gBasename = basename($argv[0]);

$shortopts  = 'h';

$longopts  = array(
  'host:'
, 'port:'
, 'user:'
, 'password:'
, 'database:'
, 'receiver-email:'
, 'test'
, 'help'
);

$aOptions = getopt($shortopts, $longopts);
setDefauls($aOptions);
$ret = checkArguments($aOptions);
if ( $ret != 0 ) {
  $rc = 359;
  printUsage();
  exit($rc);
}

# Send only test mail
if ( isset($aOptions['test']) ) {

  $subject = 'MySQL Database Monitor test. Status: OK';
  $body    = 'MySQL monitoring works. Test successfull.';
  $ret = sendMail(array(0 => $aOptions['receiver-email']), $subject, $body);
  exit($ret);
}

$subject = 'MySQL Database Monitor. Status: ERROR';

// Here starts the MySQL part

$dbh = mysqli_connect($aOptions['host'], $aOptions['user'], $aOptions['password'], $aOptions['database'], $aOptions['port']);

// This error is so serious that we do not return at all!
if ( ! $dbh ) {

  $rc = 360;
  $body = "Connection failed: " . mysqli_connect_error();
  $ret = sendMail(array(0 => $aOptions['receiver-email']), $subject, $body);
  exit($ret);
}

/*

CREATE TABLE heartbeat (
  id  INT UNSIGNED NOT NULL AUTO_INCREMENT
, ts  timestamp
, val INT UNSIGNED NOT NULL
, PRIMARY KEY (id)
) ENGINE = InnoDB;

GRANT SELECT, INSERT, DELETE ON check_db.heartbeat TO 'check_db'@'%' IDENTIFIED BY 'check_db';

*/

$rand = rand(1, 42);
$sql = sprintf("INSERT INTO heartbeat (id, ts, val) VALUES (NULL, NULL, %d)", $rand);

if ( ! $result = $dbh->query($sql) ) {
  $rc = 365;
  $body    = "ERROR: Invalid query: $sql" . PHP_EOL . $dbh->error . ".";
  $ret = sendMail(array(0 => $aOptions['receiver-email']), $subject, $body);
  exit($rc);
}

$lId = $dbh->insert_id;

$sql = sprintf("SELECT * FROM heartbeat WHERE id = %d", $lId);

if ( ! $result = $dbh->query($sql) ) {
  $rc = 366;
  $body    = "ERROR: Invalid query: $sql" . PHP_EOL . $dbh->error . ".\n";
  $ret = sendMail(array(0 => $aOptions['receiver-email']), $subject, $body);
  exit($rc);
}
if ( $record = $result->fetch_array(MYSQLI_ASSOC) ) {

  // Values do not match!
  if ( $record['val'] != $rand ) {
    $rc = 367;
    $body    = 'MySQL Database has a problem. Values do not match.';
    $ret = sendMail(array(0 => $aOptions['receiver-email']), $subject, $body);
    exit($rc);
  }
}
// failed --> error!
else {
  $rc = 368;
  $body    = 'MySQL Database has a problem. I could not fetch a row.' . PHP_EOL;
  $body   .= "ERROR: " . $dbh->error . ".";
  $ret = sendMail(array(0 => $aOptions['receiver-email']), $subject, $body);
  exit($rc);
}

// Clean-up table

$sql = sprintf("DELETE FROM heartbeat WHERE id = %d", $lId);

if ( ! $result = $dbh->query($sql) ) {
  $rc = 369;
  $body    = "ERROR: Invalid query: $sql" . PHP_EOL . $dbh->error . ".";
  $ret = sendMail(array(0 => $aOptions['receiver-email']), $subject, $body);
  exit($rc);
}

$dbh->close();
exit($rc);

?>
