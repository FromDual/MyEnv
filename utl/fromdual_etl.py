#!/usr/bin/python3

# TODO: make later python3 ready!
# http://www.kitebird.com/articles/pydbapi.html

# See also flat planet phone!

# Problem: field in cdrcosts!!! cdrcost_taxes

# pip install pymysql
# sudo apt-get install python3-mysqldb
#import MySQLdb
import mysql.connector
#import mysql.connector
#from mysql.connector import FieldType

'''

sudo apt-cache show python-mysqldb
Package: python-mysqldb
Version: 1.2.3-1ubuntu0.1


ii  mysql-connector-python                   2.0.1-1                                             MySQL database driver written in pure Python
ii  python-mysqldb                           1.2.3-1ubuntu0.1                                    Python interface to MySQL

print(get_python_lib())

'''

import sys

field_type = {
    0: 'DECIMAL'
,   1: 'TINY'
,   2: 'SHORT'
,   3: 'LONG'
,   4: 'FLOAT'
,   5: 'DOUBLE'
,   6: 'NULL'
,   7: 'TIMESTAMP'
,   8: 'LONGLONG'
,   9: 'INT24'
,  10: 'DATE'
,  11: 'TIME'
,  12: 'DATETIME'
,  13: 'YEAR'
,  14: 'NEWDATE'
,  15: 'VARCHAR'
,  16: 'BIT'
, 246: 'NEWDECIMAL'
, 247: 'INTERVAL'
, 248: 'SET'
, 249: 'TINY_BLOB'
, 250: 'MEDIUM_BLOB'
, 251: 'LONG_BLOB'
, 252: 'BLOB'
, 253: 'VAR_STRING'
, 254: 'STRING'
, 255: 'GEOMETRY'
}

format_specifier = {
    0: "%s"
,   1: "%s"
,   2: "%s"
,   3: "%s"
,   4: "%s"
,   5: '%s'
,   6: 'NULL'
,   7: "%s"
,   8: "%s"
,   9: "%s"
,  10: "%s"
,  11: "%s"
,  12: "%s"
,  13: "%s"
,  14: "%s"
,  15: "%s"
,  16: 'BIT'
, 246: "%s"
, 247: 'INTERVAL'
, 248: 'SET'
, 249: 'TINY_BLOB'
, 250: 'MEDIUM_BLOB'
, 251: 'LONG_BLOB'
, 252: 'BLOB'
, 253: "%s"
, 254: "%s"
, 255: 'GEOMETRY'
}	

'''

CREATE TABLE fromdual_etl (
  id              TINYINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY
, src_schema      VARCHAR(64)
, src_table       VARCHAR(64)
, ts_field        VARCHAR(64)
, last_ts         BIGINT UNSIGNED
, increments      INT UNSIGNED
, row_per_chunk   INT UNSIGNED
, lag             INT UNSIGNED
, dst_schema      VARCHAR(64)
, dst_table       VARCHAR(64)
);

INSERT INTO fromdual_etl VALUES (NULL, 'enswitch', 'cdrs',          'start', 0, 60000, 1000, 3600, 'enswitch_arc', 'cdrs');
INSERT INTO fromdual_etl VALUES (NULL, 'enswitch', 'cdrcosts',      'time',  0, 60,    1000, 3600, 'enswitch_arc', 'cdrcosts');
INSERT INTO fromdual_etl VALUES (NULL, 'enswitch', 'cdrcost_taxes', 'tax',   0, 60,    1000, 3600, 'enswitch_arc', 'cdrcost_taxes');

Generate test data:

INSERT INTO cdrs (server, uniqueid, start, end)
VALUES (CONCAT('master_', FLOOR(RAND()*3+1)), UUID(), CAST(UNIX_TIMESTAMP(CURRENT_TIMESTAMP())-FLOOR(RAND()*86400*365) AS UNSIGNED INTEGER)*1000, 0);
INSERT INTO cdrs (server, uniqueid, start, end)
SELECT CONCAT('master_', FLOOR(RAND()*3+1)), UUID(), CAST(UNIX_TIMESTAMP(CURRENT_TIMESTAMP())-FLOOR(RAND()*86400*365) AS UNSIGNED INTEGER)*1000, 0 FROM cdrs;
...
UPDATE cdrs SET end = start + FLOOR(RAND()*3600+1);

'''

print("FromDual ETL job");

try:
  srcDb = MySQLdb.connect(
    host   = '127.0.0.1'
  , port   = 35619
  , user   = 'etl'
  , passwd = 'secret'
  , db     = 'enswitch')
except (RuntimeError, TypeError, NameError) as e:
  print("Error({0}): {1}".format(e.errno, e.strerror))
  null
except IOError as e:
  print("I/O error({0}): {1}".format(e.errno, e.strerror))
  null
except ValueError:
  print("Could not convert data to an integer.")
  null
except:
  print("Unexpected error:", sys.exc_info()[0])
  raise

srcDb.autocommit(False)

print("Connection to source: ... OK")
  
try:
  dstDb = MySQLdb.connect(
    host   = '127.0.0.1'
  , port   = 35619
  , user   = 'etl'
  , passwd = 'secret'
  , db     = 'enswitch_arc')
except:
  print("Unexpected error:", sys.exc_info()[0])
  raise

dstDb.autocommit(False)

print("Connection to destination: ... OK")

sc1 = srcDb.cursor(MySQLdb.cursors.DictCursor)


# Check if fromdual_etl table is there

try:
  sql1 = 'SELECT * FROM fromdual_etl'
  sc1.execute(sql1)
except:
  print("Unexpected error:", sys.exc_info()[0])
  raise

print("ETL job for Table...")
cnt = 0
# Loop over all tables
for sTable in sc1.fetchall():
	# print(sTable)
	print("  Check tables to transfer")
	print('  ' + sTable['src_schema'] + '.' + sTable['src_table'] + '(' + sTable['ts_field'] + ')')
	cnt += 1

	if sTable['last_ts'] == 0:
		print("  This seems to be the first run. Get start value from source table.")
		sc2 = srcDb.cursor(MySQLdb.cursors.DictCursor)
		try:
			sql2 = 'SELECT MIN(' + sTable['ts_field'] + ') AS min FROM `' + sTable['src_schema'] + '`.`' + sTable['src_table'] + '`'
			sc2.execute(sql2)
		except:
			print("Unexpected error:", sys.exc_info()[0])
			raise

		# row = cursor.fetchone ()
		sMin = sc2.fetchall()
		if sMin[0]['min'] == None:
			print("Table ... does NOT contain rows...")
		else:
			start = sMin[0]['min']
			print(sTable)
			print("  loop over all rows")
			sql3 = 'SELECT * FROM `' + sTable['src_schema'] + '`.`' + sTable['src_table'] + '` WHERE `' + sTable['ts_field'] + '` >= ' + str(start) + ' AND `' + str(sTable['ts_field']) + '` < ' + str(start) + '+' + str(sTable['increments'])
			# print(sql3)
			# DictCursor is not good here because of multi-row-insert
			sc3 = srcDb.cursor()
			sc3.execute(sql3)
			chunk = ''
			
			print(sc3.description)
			aColumns = []
			aFieldTypes = []
			aFormatSpecifiers = []
			for i in sc3.description:
				aColumns.append(i[0])
				aFieldTypes.append(i[1])
				aFormatSpecifiers.append(format_specifier[i[1]])
			print(aColumns)
			
			aData = [];
			lrowCount = 0
			for row in sc3.fetchall():
				print(row)
				aData.append(row)
				lrowCount += 1
				if lrowCount >= sTable['row_per_chunk']:
					sql4 = 'INSERT INTO `' + sTable['dst_schema'] + '`.`' + sTable['dst_table'] + '` (' + ', '.join(aColumns) + ') VALUES (' + ', '.join(aFormatSpecifiers) + ')'
					print(sql4)
					print(aData)
					dc1 = dstDb.cursor()
					dc1.executemany(sql4, aData)
					dc1.close()

				# dstDb.start_transaction()
				sql4 = 'START TRANSACTION'
				dc1 = dstDb.cursor()
				dc1.execute(sql4)
				dc1.close()

				print("\n\n")
				sql4 = 'INSERT INTO `' + sTable['dst_schema'] + '`.`' + sTable['dst_table'] + '` (' + ', '.join(aColumns) + ') VALUES (' + ', '.join(aFormatSpecifiers) + ')'
				print(sql4)
				print(aData)
				dc1 = dstDb.cursor()
				dc1.executemany(sql4, aData)
				dstDb.commit()
				dc1.close()
					
					'''  read rows, log, in chunk
						write rows, log
					write in junks of n rows (mutli-row insert)
					# cursor.executemany('INSERT INTO 'tablename' ('column1', 'column2') VALUES (%s, %s)',
					#     [sub.values() for sub in shelf.values()])

						delete rows, log if applicapble
						'''
					exit(0)

  
if cnt == 0:
	print("Number of tables to transfer was ZERO.")
	exit(1)

print("done")
print("Add times")
print("Replace")
exit(1)
