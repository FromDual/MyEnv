#!/usr/bin/php -d variables_order=EGPCS
<?php

$rc = 0;

$lDebug = isset($_ENV['MYENV_DEBUG']) ? intval($_ENV['MYENV_DEBUG']) : 0;
$lMyNameBase = basename(__FILE__);

$aDefaults = array(
  'user'        => 'root'
, 'password'    => ''
, 'host'        => '127.0.0.1'
, 'port'        => '3306'
, 'schema-from' => 'test'
, 'convert'     => 'simple'
, 'schema-to'   => 'new'
, 'engine-to'   => 'InnoDB'
, 'socket'      => '/run/mysqld/mysql.sock'
);

$aOptions = array();
// interactive
if ( $argc == 1 ) {

	printf("%-27s %12s : ", 'User', '[' . $aDefaults['user'] . ']');
	$user =  trim(readline(''));
	$aOptions['user'] = $user == '' ? $aDefaults['user'] : $user;

	printf("%-27s %12s : ", 'Password', '[' . $aDefaults['password'] . ']');
	exec('/bin/stty -echo');   # Disable echoing
	$password = trim(fgets(STDIN));
	exec('/bin/stty echo');   # Turn it back on
	printf("\n");
	$aOptions['password'] = $password == '' ? $aDefaults['password'] : $password;

	printf("%-27s %12s : ", 'Host', '[' . $aDefaults['host'] . ']');
	$host = trim(readline(''));
	$aOptions['host'] = $host == '' ? $aDefaults['host'] : $host;

	printf("%-27s %12s : ", 'Port', '[' . $aDefaults['port'] . ']');
	$port = trim(readline(''));
	$aOptions['port'] = $port == '' ? $aDefaults['port'] : $port;

	printf("%-27s %12s : ", 'Schema from (or all)', '[' . $aDefaults['schema-from'] . ']');
	$schema_from =  trim(readline(''));
	$aOptions['schema-from'] = $schema_from == '' ? $aDefaults['schema-from'] : $schema_from;

	if ( $aOptions['schema-from'] != 'all' ) {

		printf("Simple  : ALTER TABLE xxx ENGINE = ...;\n");
		printf("Advanced: Copy table from one schema to\n");
		printf("          another with changing the\n");
		printf("          Storage Engine.\n");
		printf("%-27s %12s : ", 'Convert simple or advanced', '[' . $aDefaults['convert'] . ']');
		$convert =  trim(readline(''));
		$aOptions['convert'] = $convert == '' ? $aDefaults['convert'] : $convert;

		if ( $aOptions['convert'] == "advanced" ) {

			printf("%-27s %12s : ", 'Schema to', '[' . $aDefaults['schema-to'] . ']');
			$schema_to =  trim(readline(''));
			$aOptions['schema-to'] = $schema_to == '' ? $aDefaults['schema-to'] : $schema_to;
			if ( $aOptions['schema-to'] == $aOptions['schema-from'] ) {
				$rc = 386;
				printf("Schema TO cannot be identical with schema FROM (rc=$rc).\n");
				exit($rc);
			}
		}
		// simple
		else {
			$aOptions['schema-to'] = $aOptions['schema-from'];
			$aOptions['convert']   = $aDefaults['convert'];
		}
	}
	// all schemas
	else {
		$aOptions['schema-to'] = $aOptions['schema-from'];
		$aOptions['convert']   = $aDefaults['convert'];
	}

	printf("%-27s %12s : ", 'Engine to', '[' . $aDefaults['engine-to'] . ']');
	$engine_to = trim(readline(''));
	$aOptions['engine-to'] = $engine_to == '' ? $aDefaults['engine-to'] : $engine_to;
}
// CLI options
else {
	// Parse command line
	$shortopts  = "";
	$longopts  = array(
	  'user:'
	, 'password:'
	, 'host:'
	, 'port:'
	, 'help'
	, 'debug'
	, 'schema-from:'
	, 'socket:'
	, 'convert:'
	, 'schema-to:'
	, 'engine-to:'
	);

	$aOptions = getopt($shortopts, $longopts);
	foreach ( $aDefaults as $key => $value ) {
		if ( ! array_key_exists($key, $aOptions) ) {
			$aOptions[$key] = $value;
		}
	}
}

if ( array_key_exists('help', $aOptions) ) {
	printUsage($lMyNameBase, $aDefaults);
	exit($rc);
}

if ( array_key_exists('debug', $aOptions) ) {
	$lDebug = 1;
}

if ( $lDebug > 0 ) {
	printf("MYENV_DEBUG is $lDebug\n");
}

// ---------------------------------------------------------------------
function printUsage($pMyNameBase, $aDefaults)
// ---------------------------------------------------------------------
{
  printf("
Creates a script to alter the MySQL/MariaDB storage engine. This script does NOT
change any data!

usage: $pMyNameBase [--user=<user>] [--password=<password>] [--host=<hostname>]
       [--port=<port> | --socket=<socket>]
       [--schema-from=[<schema>|all]] [--convert=[simple|advanced]]
       [--schema-to=<schema>] [--engine-to=<storage engine>]
       [--debug] [--help]

Options:
              No options activates the interactive mode.

  user        Database user who should gather the data (default = " . $aDefaults['user'] . ").
  password    Password of the database user (default = '').
  host        Hostname or IP address where database is located (default
              = " . $aDefaults['host'] . ").
  port        Port where database is listening (default = " . $aDefaults['port'] . ").
  schema-from The schema which should be converted or all (default = " . $aDefaults['schema-from'] . ").
  convert     How to convert: simple or advanced (default = " . $aDefaults['convert'] . ").
              simple  : Only changes the Storage Engine
              advanced: Copies tables to another schema with changing the Storage
                        Engine.
  schema-to   Schema where converted tables should be copied to. Is only used in
              advanced convert mode (default = " . $aDefaults['schema-to'] . ").
  engine-to   Target Storage Engine which should be used (default = " . $aDefaults['engine-to'] . ").
  help        Prints this help.
  debug       Prints all debugging information.

Examples:

  $pMyNameBase

  $pMyNameBase --user=" . $aDefaults['user'] . " --password=secret --host=" . $aDefaults['host'] . " --schema-from=all --engine-to=" . $aDefaults['engine-to'] . "

  $pMyNameBase --user=" . $aDefaults['user'] . " --password=secret --host=" . $aDefaults['host'] . " --schema-from=" . $aDefaults['schema-from'] . " --convert=" . $aDefaults['convert'] . " --engine-to=" . $aDefaults['engine-to'] . "

  $pMyNameBase --user=" . $aDefaults['user'] . " --password=secret --host=" . $aDefaults['host'] . " --schema-from=" . $aDefaults['schema-from'] . " --convert=advanced --schema-to=" . $aDefaults['schema-to'] . " --engine-to=" . $aDefaults['engine-to'] . "

");
}


// -----------------------------------------------------------------------------
// MAIN
// -----------------------------------------------------------------------------

$lOutFile;
if ( $aOptions['schema-from'] == 'all' ) {
	$lOutFile = '/tmp/alter_table_all.sql';
}
else {
	$lOutFile = '/tmp/alter_table_' . $aOptions['schema-from'] . '.sql';
}

$mysqli = @new mysqli($aOptions['host'], $aOptions['user'], $aOptions['password'], null, $aOptions['port'], $aOptions['socket']);

if ( mysqli_connect_error() ) {
	$rc = 341;
	fprintf(STDERR, "ERROR: Connect failed: (%d) %s (rc=$rc).\n", mysqli_connect_errno(), mysqli_connect_error());
	exit($rc);
}

$mysqli->query('SET NAMES utf8');

$sql = "SHOW GLOBAL VARIABLES";
if ( $result = $mysqli->query($sql) ) {
	while ( $record = $result->fetch_assoc() ) {
	}
}
else {
	$rc = 342;
 	fprintf(STDERR, "ERROR: Invalid query: $sql, " . $mysqli->error . " (rc=$rc).\n");
	exit($rc);
}


// Some comments to out file first

$fh = fopen($lOutFile, 'w');
fprintf($fh, "-- Commented (--) lines means that these tables are already using the wanted Storage Engine.\n\n");
if ( $aOptions['convert'] == 'advanced' ) {
	fprintf($fh, 'CREATE SCHEMA IF NOT EXISTS ' . $aOptions['schema-to'] . ";\n\n");
}
fprintf($fh, "warnings\n\n");

if ( $aOptions['convert'] == 'simple' ) {

	$sql = sprintf(
	  "SELECT table_schema AS 'table_schema', table_name AS 'table_name', engine AS 'engine', row_format AS 'row_format'" . "\n"
	. "  FROM information_schema.tables" . "\n"
	. " WHERE table_schema NOT IN ('mysql', 'information_schema', 'performance_schema')" . "\n"
	. "   AND table_type = 'BASE TABLE'"
	);
	if ( $aOptions['schema-from'] != 'all' ) {
		$sql .= sprintf("\n" . "   AND table_schema = '%s'", $aOptions['schema-from']);
	}
	if ( $lDebug > 0 ) {
		printf("\n" . $sql . "\n\n");
	}

	if ( $result = $mysqli->query($sql) ) {

		while ( $record = $result->fetch_assoc() ) {

			$row_format = '';
			if ( $record['row_format'] == 'Fixed' ) {
				$row_format = ' ROW_FORMAT=Compact';
			}
			$comment = '';
			if ( $record['engine'] == $aOptions['engine-to'] ) {
				$comment = '-- ';
			}
			$cmd = $comment . 'ALTER TABLE `' . $record['table_schema'] . '`.`' . $record['table_name'] . '` ENGINE=' . $aOptions['engine-to'] . $row_format . ';';
			fprintf($fh,  $cmd . "\n");
			if ( $lDebug > 0 ) {
				printf($cmd . "\n");
			}
		}
	}
	else {
		$rc = 343;
		fprintf(STDERR, "ERROR: Invalid query: $sql, " . $mysqli->error . " (rc=$rc).\n");
		exit($rc);
	}
}
elseif ( $aOptions['convert'] == 'advanced' ) {

	$sql = sprintf(
	  "SELECT table_schema AS 'table_schema', table_name AS 'table_name', engine AS 'engine', row_format AS 'row_format'" . "\n"
	. "  FROM information_schema.tables" . "\n"
	. " WHERE table_schema = '%s'" . "\n"
	. "   AND table_type = 'BASE TABLE'"
	, $aOptions['schema-from']);

	if ( $result = $mysqli->query($sql) ) {

		while ( $record = $result->fetch_assoc() ) {

			$row_format = '';
			if ( $record['row_format'] == 'Fixed' ) {
				$row_format = ' ROW_FORMAT=Compact';
			}

			fprintf($fh, 'CREATE TABLE `' . $aOptions['schema-to'] . '`.`' . $record['table_name'] . '` LIKE `' . $record['table_schema'] . '`.`' . $record['table_name'] . '`;' . "\n");
			fprintf($fh, 'ALTER TABLE `'  . $aOptions['schema-to'] . '`.`' . $record['table_name'] . '` ENGINE=' . $aOptions['engine-to'] . $row_format . ';' . "\n");
			fprintf($fh, 'INSERT INTO `'  . $aOptions['schema-to'] . '`.`' . $record['table_name'] . '` SELECT * FROM `' . $record['table_schema'] . '`.`' . $record['table_name'] . '`;' . "\n");
			fprintf($fh, '-- DROP TABLE `' . $aOptions['schema-from'] . '`.`' . $record['table_name'] . '`;' . "\n");
		}
	}
	else {
		$rc = 344;
		fprintf(STDERR, "ERROR: Invalid query: $sql, " . $mysqli->error . " (rc=$rc).\n");
		exit($rc);
	}
}
else {
	$rc = 345;
	fprintf(STDERR, "ERROR: Mode " . $aOptions['convert'] . " is not allows. Use simple or advanced. (rc=$rc).\n");
	exit($rc);
}

fclose($fh);


// Get MySQL Version

$sql = "SHOW GLOBAL VARIABLES WHERE Variable_name = 'version'";

if ( ! ($result = $mysqli->query($sql)) ) {
	$rc = 346;
	fprintf(STDERR, "ERROR: Invalid query: $sql, " . $mysqli->error . " (rc=$rc).\n");
	exit($rc);
}

if ( $record = $result->fetch_assoc() ) {

	$lVersion    = '';
	$lMrVersion = '';

	printf("\n");

	if ( preg_match('/^(\d+)\.(\d+)\.(\d+).*/', $record['Value'], $matches) ) {
		$lVersion = $matches[1] . '.' . $matches[2] . '.' . $matches[3];
		printf("Version is   : $lVersion\n");
		$lMrVersion = sprintf("%02d%02d%02d", $matches[1], $matches[2], $matches[3]);
		printf("MR Version is: $lMrVersion\n");
	}
	else {
		fprintf(STDERR, "ERROR: Cannot determine version from " . $record['Value'] . "\n");
	}

	// Complain for not supported MySQL versions

	if ( $lMrVersion < '050000' ) {
		printf("\nThe used MySQL version is 4.1 or less. These versions are not supported.\nResults are not predictable any more...\n");
	}
}
else {
  $rc = 347;
  fprintf(STDERR, "ERROR: No record found. Fatal error. Please report this as bug (rc=$rc).\n");
  exit($rc);
}


// Find tables without a Primary Key

$sql = "SELECT DISTINCT t.table_schema AS 'table_schema', t.table_name AS 'table_name'" . "\n"
	   . "  FROM information_schema.tables AS t" . "\n"
	   . "  LEFT JOIN information_schema.columns AS c ON t.table_schema = c.table_schema AND t.table_name = c.table_name AND c.column_key = 'PRI'" . "\n"
	   . " WHERE t.table_schema NOT IN ('information_schema', 'mysql', 'performance_schema')" . "\n"
	   . "   AND c.table_name IS NULL" . "\n"
	   . "   AND t.table_type != 'VIEW'"
	   ;
if ( $aOptions['schema-from'] != 'all' ) {
	$sql .= sprintf("\n" . "   AND t.table_schema = '%s'", $aOptions['schema-from']);
}

if ( $lDebug > 0 ) {
	printf("\n" . $sql . "\n");
}

if ( $result = $mysqli->query($sql) ) {

	if ( $result->num_rows > 0 ) {

		printf("\n");
		printf("WARNING: The following tables might not have a Primary Key.\n");
		printf("Tables not having a Primary Key will negatively affect performance and data con-\n");
		printf("sistency in MySQL Master/Slave replication and GaleraCluster replication:\n\n");

		while ( $record = $result->fetch_assoc() ) {
			printf('  ' . $record['table_schema'] . '.' . $record['table_name'] . "\n");
		}
	}
	else {
		printf("\nNo table without Primary Key found.\n\n");
	}
}
else {
	$rc = 348;
	fprintf(STDERR, "ERROR: Invalid query: $sql, " . $mysqli->error . " (rc=$rc).\n");
	exit($rc);
}


// Find tables with AUTO_INCREMENT at 2nd position

if ( $aOptions['engine-to'] == 'InnoDB' ) {

	$sql = "SELECT c.table_schema AS 'table_schema ', c.table_name AS 'table_name', c.column_name AS 'column_name', c.column_key AS 'column_key', c.extra AS 'extra', kcu.ordinal_position AS 'ordinal_position'" . "\n"
			. "  FROM information_schema.columns AS c" . "\n"
			. "  JOIN information_schema.key_column_usage AS kcu ON kcu.table_schema = c.table_schema AND kcu.table_name = c.table_name AND kcu.column_name = c.column_name" . "\n"
			. " WHERE c.table_schema NOT IN ('information_schema', 'mysql', 'performance_schema')" . "\n"
			. "   AND c.column_key = 'PRI'" . "\n"
			. "   AND kcu.ordinal_position > 1" . "\n"
			. "   AND c.extra = 'auto_increment'"
			;
	if ( $aOptions['schema-from'] != 'all' ) {
		$sql .= sprintf("\n" . "   AND c.table_schema = '%s'", $aOptions['schema-from']);
	}

	if ( $lDebug > 0 ) {
		printf("\n" . $sql . "\n");
	}

	if ( $result = $mysqli->query($sql) ) {

		if ( $result->num_rows > 0 ) {

			printf("\n");
			printf("WARNING: The following tables have the AUTO_INCREMENT column NOT at the first\n");
			printf("position of the Primary Key. This is NOT supported by InnoDB.\n\n");

			while ( $record = $result->fetch_assoc() ) {
				printf('  ' . $record['table_schema'] . '.' . $record['table_name'] . "\n");
			}
		}
		else {
			printf("\nNo table with AUTO_INCREMENT NOT at first position.\n\n");
		}
	}
	else {
		$rc = 349;
		fprintf(STDERR, "ERROR: Invalid query: $sql, " . $mysqli->error . " (rc=$rc).\n");
		exit($rc);
	}
}

/*
// Check for too long Primary Keys for InnoDB

if ( $aOptions['engine-to'] == 'InnoDB' ) {

	$sql = "SELECT table_schema AS 'table_schema', table_name AS 'table_name'
--     , data_type AS 'data_type', character_octet_length AS 'character_octet_length', column_key AS 'column_key'
, SUM(IF(data_type = 'varchar', character_octet_length
		, IF(data_type = 'char', character_octet_length
		, IF(data_type = 'enum', 2
		, IF(data_type = 'tinyint', 1
		, IF(data_type = 'smallint', 2
		, IF(data_type = 'mediumint', 3
		, IF(data_type = 'int', 4
		, IF(data_type = 'bigint', 8
		, IF(data_type = 'timestamp', 4
		, IF(data_type = 'datetime', 8
		, IF(data_type = 'time', 4
		, IF(data_type = 'date', 4
		, NULL))))))))))))) AS 'column_length'
	FROM information_schema.columns
WHERE column_key = 'PRI'
	AND table_schema NOT IN ('information_schema', 'mysql', 'performance_schema')";

	if ( $aOptions['schema-from'] != 'all' ) {
		$sql .= sprintf("\n" . "   AND table_schema = '%s'", $aOptions['schema-from']);
	}

	$sql .= "GROUP BY table_schema, table_name" . "\n"
	      . "HAVING column_length > 767";

	if ( $result = $mysqli->query($sql) ) {

		printf("\n");
		printf("WARNING: The following tables might have a too long Primary Key for InnoDB (> 767 bytes).\n");
		printf("Use innodb_large_prefix in combination with innodb_file_format=barracuda and innodb_file_per_table=1\n");
		printf("or choose smaller Primary Key attributes (VARCHAR in combination with UTF8).\n");
		printf("See also: http://dev.mysql.com/doc/refman/5.6/en/innodb-parameters.html#sysvar_innodb_large_prefix\n\n");

		while ( $record = $result->fetch_assoc() ) {
			printf('  ' . $record['table_schema'] . '.' . $record['table_name'] . ' (' . $record['column_length'] . ' byte)' . "\n");
		}
	}
	else {
		$rc = 387;
		fprintf(STDERR, "ERROR: Invalid query: $sql, " . $mysqli->error . " (rc=$rc).\n");
		exit($rc);
	}
}
*/

// Check for FULLTEXT index in MySQL 5.1 and 5.5

if ( 'InnoDB' == $aOptions['engine-to'] ) {

	if ( $lMrVersion < '050100' ) {
		printf("\nWARNING: We cannot find FULLTEXT indexes in MySQL version is 5.0 or older...\n");
	}
	else {

		if ( $lMrVersion < '050600' ) {

			printf("\n");
			printf("WARNING: The following tables might have a FULLTEXT index (which is only supported\nin MySQL 5.6 and newer):\n\n");

			$sql = "SELECT table_schema AS 'table_schema', table_name AS 'table_name', column_name AS 'column_name'" . "\n"
			     . "  FROM information_schema.statistics" . "\n"
			     . " WHERE index_type = 'FULLTEXT'";

			if ( $aOptions['schema-from'] != 'all' ) {
				$sql .= sprintf("\n" . "   AND table_schema = '%s'", $aOptions['schema-from']);
			}

			if ( $result = $mysqli->query($sql) ) {

				while ( $record = $result->fetch_assoc() ) {
					printf('  ' . $record['table_schema'] . '.' . $record['table_name'] . "\n");
				}
			}
			else {
				$rc = 388;
				fprintf(STDERR, "ERROR: Invalid query: $sql, " . $mysqli->error . " (rc=$rc).\n");
				exit($rc);
			}
		}
	}
}

$mysqli->close();

printf("\n");
printf("Output written to $lOutFile\n\n");
printf("After reviewing it you can apply it with mysql --user=root --password=secret < $lOutFile\n\n");

exit($rc);

?>
