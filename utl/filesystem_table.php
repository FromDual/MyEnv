#!/usr/bin/php
<?php

// Turn error reporting off. We handle it ourself?
error_reporting(0);

$rc = 0;
$gMyName = basename($argv[0]);

// $shortopts = 'u:h:p:P:S:';
$shortopts = '';
$longopts  = array(
  'user:'
, 'host:'
, 'port:'
, 'password:'
, 'socket:'
, 'datadir:'
, 'database:'
, 'create'
, 'help'
, 'instance:'
);

// Defaults
$gOptions  = array(
  'host' => 'localhost'
, 'user' => ''
, 'password' => ''
, 'port'     => 3306
, 'database' => ''
, 'socket'   => '/var/run/mysqld/mysql.sock'
, 'datadir'  => '/var/lib/mysql'
, 'create'   => 0
, 'help'     => 0
, 'instance' => ''
);

// Consider ENV

if ( isset($_ENV['MYSQL_UNIX_PORT']) ) {
  $gOptions['socket'] = $_ENV['MYSQL_UNIX_PORT'];
}
if ( isset($_ENV['MYSQL_TCP_PORT']) ) {
  $gOptions['port'] = $_ENV['MYSQL_TCP_PORT'];
}
if ( isset($_ENV['USER']) ) {
  $gOptions['user'] = $_ENV['USER'];
}

// -----------------------------------------------------------------------------
function usage()
// -----------------------------------------------------------------------------
{
  global $gMyName, $gOptions;

  echo "
Usage: $gMyName [OPTION]

File a table with the stat information of (MySQL table) files.

  --user=      User for login if not current user.
  --password=  Password to use when connecting to server.
  --host=      Connect to host (default " . $gOptions['host'] . ").
  --port=      Port number to use for connection (default " . $gOptions['port'] . ").
  --socket=    Socket file to use for connection (default " . $gOptions['socket'] . ").
  --create     Create needed table (optional).
  --database=  Database where table to store data is located.
  --datadir=   Directory where MySQL files are located (default ". $gOptions['datadir'] . ").
  --help       Print this help.

Examples:

  $gMyName --user=root --host=127.0.0.1 --port=" . $gOptions['port'] . " --database=test --create
  $gMyName --user=root --host=" . $gOptions['host'] . " --socket=" . $gOptions['socket'] . " --database=test --datadir=" . $gOptions['datadir']. "

Report bugs to <remote-dba@fromdual.com>.

";
}

// -----------------------------------------------------------------------------
// MAIN
// -----------------------------------------------------------------------------

// $options = getopt($shortopts);
$options = getopt($shortopts, $longopts);

foreach ( $options as $key => $value ) {

  switch ( $key ) {
  case 'password':
  case 'socket':
  case 'user':
  case 'datadir':
  case 'database':
  case 'instance':
  case 'host':
    if ( $value != '' ) {
      $gOptions[$key] = $value;
    }
  break;
  case 'port':
    if ( intval($value) != 0 ) {
      $gOptions[$key] = intval($value);
    }
  break;
  case 'create':
    $gOptions[$key] = 1;
  break;
  case 'help':
    usage();
    exit($rc);
  break;
  default:
    $rc = 314;
    usage();
    exit($rc);
  break;
  }
}

// Check variables

if ( $gOptions['database'] == '' ) {
  echo "ERROR: No database selected.\n";
  usage();
  $rc = 315;
  exit($rc);
}

$mysqli = mysqli_connect($gOptions['host'], $gOptions['user'], $gOptions['password'], $gOptions['database'], $gOptions['port'], $gOptions['socket']);

if ( ! $mysqli ) {
  echo "ERROR: " . mysqli_connect_error() . "\n";
  usage();
  $rc = 316;
  exit($rc);
}

$mysqli->select_db($gOptions['database']);

if ( $gOptions['create'] == 1 ) {

  $sql = "CREATE TABLE `file_access` (
  `instance` varchar(64) DEFAULT NULL,
  `schema` varchar(64) DEFAULT NULL,
  `file_name` varchar(255) NOT NULL,
  `table_name` varchar(64) DEFAULT NULL,
  `file_type` char(3) DEFAULT NULL,
  `size` bigint(20) unsigned NOT NULL,
  `atime` timestamp NOT NULL DEFAULT '0000-00-00 00:00:00',
  `mtime` timestamp NOT NULL DEFAULT '0000-00-00 00:00:00',
  `ctime` timestamp NOT NULL DEFAULT '0000-00-00 00:00:00',
  `blksize` int(10) unsigned NOT NULL,
  `blocks` int(10) unsigned NOT NULL,
  PRIMARY KEY (`instance`, `schema`, `file_name`)
)";

  if ( ! $result = $mysqli->query($sql) ) {
    $rc = 317;
    echo "ERROR: " . $mysqli->error . "\n";
    exit($rc);
  }
  else {
    echo "Table created.\n";
  }
}

// Check if table exists

$sql = "SELECT * FROM file_access LIMIT 1";
if ( ! $result = $mysqli->query($sql) ) {
  $rc = 318;
  echo "ERROR: " . $mysqli->error . "\n";
  echo "Please create with --create first.\n";
  usage();
  exit($rc);
}

// Collect all files

$aPattern = array (
  $gOptions['datadir'] . "/ibdata*"
, $gOptions['datadir'] . "/ib_logfile?"
, $gOptions['datadir'] . "/*/*.frm"
, $gOptions['datadir'] . "/*/*.MY?"
, $gOptions['datadir'] . "/*/*.ibd"
);

$aFiles = array();
foreach ( $aPattern as $pattern ) {
  $aFiles = array_merge(glob($pattern), $aFiles);
}

if ( count($aFiles) == 0 ) {
  echo "ERROR: No files with the following pattern found:\n";
  var_dump($aPattern);
  $rc = 319;
  exit($rc);
}

// Get stat information of files

foreach ( $aFiles as $filename ) {

  //  7  size     size in bytes
  //  8  atime    time of last access (Unix timestamp)
  //  9  mtime    time of last modification (Unix timestamp)
  // 10  ctime    time of last inode change (Unix timestamp)
  // 11  blksize  blocksize of filesystem IO **
  // 12  blocks   number of 512-byte blocks allocated **
  $aStat = stat($filename);

  $fn = substr($filename, strlen($gOptions['datadir'])+1);
  $a = explode('/', $fn);
  if ( isset($a[1]) ) {
    $schema = $a[0];
    $file   = $a[1];
  }
  else {
    $schema = '';
    $file   = $a[0];
  }

  $te = explode('\.', $file);
  if ( count($te) == 1 ) {
    $table = '';
    $ext = '';
  }
  else {
    $table = $te[0];
    $ext   = $te[1];
  }

  $sql = sprintf("INSERT INTO file_access (
  instance, `schema`, file_name, table_name, file_type, size, atime, mtime, ctime, blksize, blocks)
VALUES ('%s', '%s', '%s', '%s', '%s', %d, FROM_UNIXTIME(%d), FROM_UNIXTIME(%d), FROM_UNIXTIME(%d), %d, %d)
ON DUPLICATE KEY UPDATE size = %d, atime = FROM_UNIXTIME(%d), mtime = FROM_UNIXTIME(%d), ctime = FROM_UNIXTIME(%d), blksize = %d, blocks = %d", $gOptions['instance'], $schema, $file, $table, $ext, $aStat['size'], $aStat['atime'], $aStat['mtime'], $aStat['ctime'], $aStat['blksize'], $aStat['blocks'], $aStat['size'], $aStat['atime'], $aStat['mtime'], $aStat['ctime'], $aStat['blksize'], $aStat['blocks']);

  if ( ! $result = $mysqli->query($sql) ) {
    echo "ERROR: " . $mysqli->error . "\n";
  }
}

$mysqli->close();
echo "Data writen.\n";
exit($rc);

?>
