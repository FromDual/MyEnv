#!/usr/bin/php
<?php

/*

   Fix encoding is made to fix encoding issues as as described here:

   https://www.fromdual.com/mysql-questions-and-answers#wrong-encoding

*/

// ---------------------------------------------------------------------
function checkArguments($aOptions)
// ---------------------------------------------------------------------
{
  $rc = 0;

//   var_dump($aOptions);

//   var_dump($aOptions);
  return $rc;
}

// ---------------------------------------------------------------------
function setDefauls($aOptions)
// ---------------------------------------------------------------------
{
//   var_dump($aOptions);

  $aDefaults = array(
    'host'           => '127.0.0.1'
  , 'user'           => 'root'
  , 'password'       => ''
  , 'database'       => 'test'
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

Options:
  host            127.0.0.1
  port            3306
  database        ''
  user            root
  password        secret
  help, h, ?      Help

Important:
  It is highly recommended to dump the schema structure before and after the
  change and compare the results...

  mysqldump --host=127.0.0.1 --port=3306 --user=root --password=secret --database=test --no-data > structure_dump_before.sql
  $script ...
  mysqldump --host=127.0.0.1 --port=3306 --user=root --password=secret --database=test --no-data > structure_dump_after.sql
  diff structure_dump_before.sql structure_dump_after.sql

Examples:

  $script --host=127.0.0.1 --port=3306 --user=root --password=secret --database=test > fix.sql
  mysql --host=127.0.0.1 --port=3306 --user=root --password=secret --database=test < fix.sql

";
}

// ---------------------------------------------------------------------
// MAIN
// ---------------------------------------------------------------------

$rc = 0;

// We catch all errors properly! If not, set this to -1:
error_reporting(-1);
//error_reporting(0);

$gBasename = basename($argv[0]);

$shortopts  = 'h';

$longopts  = array(
  'host:'
, 'port:'
, 'user:'
, 'password:'
, 'database:'
, 'help'
);

$aOptions = getopt($shortopts, $longopts);
setDefauls($aOptions);
$ret = checkArguments($aOptions);
if ( $ret != 0 ) {
  $rc = 400;
  printUsage();
  exit($rc);
}

if ( isset($aOptions['help']) ) {
  printUsage();
  exit($rc);
}

// Here starts the MySQL part

$dbh = mysqli_connect($aOptions['host'], $aOptions['user'], $aOptions['password'], $aOptions['database'], $aOptions['port']);

// This error is so serious that we do not return at all!
if ( ! $dbh ) {

  $rc = 375;
  $body = "Connection failed: " . mysqli_connect_error();
  exit($ret);
}


print "\n";
print "USE " . $aOptions['database'] . "\n";
print "\n";


// Fetch all tables of schema

$sql = sprintf("SELECT table_name AS 'table_name'
  FROM information_schema.tables
 WHERE table_schema = '%s'
   AND table_type = 'BASE TABLE'
", $aOptions['database']);

if ( ! $result = $dbh->query($sql) ) {
  $rc = 374;
  $body    = "ERROR: Invalid query: $sql" . PHP_EOL . $dbh->error . ".\n";
  exit($rc);
}

// Loop over all tables

while ( $record = $result->fetch_array(MYSQLI_ASSOC) ) {

  print "\n";
  print '-- Table ' . $record['table_name'] . "\n";


  // SHOW CREATE TABLE to get DEFAULT CHARSET

  $sql = sprintf("SHOW CREATE TABLE %s.%s", $aOptions['database'], $record['table_name']);

  if ( ! $result2 = $dbh->query($sql) ) {
    $rc = 377;
    $body    = "ERROR: Invalid query: $sql" . PHP_EOL . $dbh->error . ".\n";
    exit($rc);
  }

  if ( ! $show_create_table = $result2->fetch_array(MYSQLI_ASSOC) ) {
    $rc = 376;
    print "ERROR (rc=$rc)!\n";
    exit($rc);
  }

  $statement = $show_create_table['Create Table'];
  $pattern = '/ DEFAULT CHARSET=(.*)/';
  if ( ! preg_match($pattern, $statement, $matches) ) {
    $rc = 378;
    print "ERROR (rc=$rc)\n";
    exit($rc);
  }

  $character_set = $matches[1];
  print "-- Character set: " . $character_set . "\n";
  print "\n";

  // Get Columns for ALTER TABLE statement

  $sql = sprintf("SELECT column_name AS 'column_name', column_default AS 'column_default', is_nullable AS 'is_nullable', data_type AS 'data_type', column_type AS 'column_type'
  FROM information_schema.columns
 WHERE table_schema = '%s'
   AND table_name = '%s'", $aOptions['database'], $record['table_name']);

  if ( ! $result3 = $dbh->query($sql) ) {
    $rc = 379;
    $body    = "ERROR: Invalid query: $sql" . PHP_EOL . $dbh->error . ".\n";
    exit($rc);
  }

  $aStep1 = array();
  $aStep2 = array();
  $aStep3 = array();

  while ( $column = $result3->fetch_array(MYSQLI_ASSOC) ) {

    if ( ($column['data_type'] == 'varchar')
      || ($column['data_type'] == 'char')
      || ($column['data_type'] == 'tinytext')
      || ($column['data_type'] == 'text')
      || ($column['data_type'] == 'mediumtext')
      || ($column['data_type'] == 'longtext') ) {

      if ( $column['is_nullable'] == 'NO' ) {
        $null = ' NOT NULL';
      }
      else {
        $null = ' NULL';
      }

      if ( is_null($column['column_default']) ) {
        $default = '';
      }
      else {
        $default = " DEFAULT '" . $column['column_default'] . "'";
      }

      if ( $column['data_type'] == 'varchar' ) {
        $transformation_data_type = str_replace('varchar', 'varbinary', $column['column_type']);
      }
      elseif ( $column['data_type'] == 'char' ) {
        $transformation_data_type = str_replace('char', 'binary', $column['column_type']);
      }
      elseif ( $column['data_type'] == 'tinytext' ) {
        $transformation_data_type = str_replace('tinytext', 'tinyblob', $column['column_type']);
      }
      elseif ( $column['data_type'] == 'text' ) {
        $transformation_data_type = str_replace('text', 'blob', $column['column_type']);
      }
      elseif ( $column['data_type'] == 'mediumtext' ) {
        $transformation_data_type = str_replace('mediumtext', 'mediumblob', $column['column_type']);
      }
      elseif ( $column['data_type'] == 'longtext' ) {
        $transformation_data_type = str_replace('longtext', 'longblob', $column['column_type']);
      }
      else {
        $rc = 384;
        print "ERROR (rc=$rc)\n";
        exit($rc);
      }

      if ( $character_set == 'latin1' ) {

        array_push($aStep1, 'MODIFY COLUMN `' . $column['column_name'] . '` ' . $transformation_data_type . "\n");
        array_push($aStep2, 'MODIFY COLUMN `' . $column['column_name'] . '` ' . $column['column_type'] . ' CHARACTER SET  utf8' . "\n");
        array_push($aStep3, 'MODIFY COLUMN `' . $column['column_name'] . '` ' . $column['column_type'] . $null . $default . ' CHARACTER SET  latin1' . "\n");
      }
      elseif ( $character_set == 'utf8' ) {

        array_push($aStep1, 'MODIFY COLUMN `' . $column['column_name'] . '` ' . $column['column_type'] . ' CHARACTER SET latin1' . "\n");
        array_push($aStep2, 'MODIFY COLUMN `' . $column['column_name'] . '` ' . $transformation_data_type . "\n");
        array_push($aStep3, 'MODIFY COLUMN `' . $column['column_name'] . '` ' . $column['column_type'] . $null . $default . "\n");
      }
      else {
        $rc = 382;
        print "ERROR: Wrong character set: $character_set (rc=$rc).\n";
        exit($rc);
      }
    }   // varchar || char || text
  }

  // Assemble the whole stuff

  if ( count($aStep1) > 0 ) {

    $step1 = 'ALTER TABLE `' . $record['table_name'] . "`\n  " . implode(', ', $aStep1) . ";\n\n";
    print $step1;

    $step2 = 'ALTER TABLE `' . $record['table_name'] . "`\n  " . implode(', ', $aStep2) . ";\n\n";
    print $step2;

    $step3 = 'ALTER TABLE `' . $record['table_name'] . "`\n  " . implode(', ', $aStep3) . ";\n\n";
    print $step3;
  }
  else {
    print '-- No changes for table ' . $record['table_name'] . "\n";
    print "\n";
  }
}   // End of while table loop

$dbh->close();
exit($rc);

?>
