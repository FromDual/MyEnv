#!/usr/bin/perl

use warnings;
use strict;

our $dumpdir  = '/tmp';
our $user     = 'root';
our $password = '';
our $host     = 'localhost';
our $port     = 3306;
our $socket   = '/run/mysqld/mysql.sock';
# PID file is needed for determine if database is up and running
# When having corrupt blocks the database could crash from time to time
our $pid_file = '/var/lib/mysql/' . gethostname() . '.pid';
our $DUMP     = 'mysqldump';
# skip-opt contains extented-insert=false
our $EXTENDED = '--skip-opt';
our $retry    = 3;
our $DEBUG    = 0;
our $sleep    = 1;

our $database = 'test';
our $table    = 'test';
our $column   = 'id';

my $start_id = 1;
my $end_id   = 4200000;
# step must be 10 or a multiple of 10
my $step     = 10000;

sub loop
{
  my $start_id = $_[0];
  my $end_id   = $_[1];
  my $step     = $_[2];

  if ( $step < 1 ) {
    return;
  }

  # print "start_id = $start_id, end_id = $end_id, step = $step\n";
  for ( my $i = $start_id ; $i <= $end_id ; $i += $step ) {

    my $last = $i+$step-1;
    my $cmd  = "$DUMP --user=$user --password=$password --host=$host --port=$port $EXTENDED --no-create-info --where='$column >= $i AND $column <= " . $last . "' $database $table";
    my $file = "$dumpdir/$database" . "_" . $table . "_dump_" . sprintf("%08d-%08d", $i, $last) . ".sql";
    # print "$cmd\nto $file\n";
    print "$database, $table, $i, $last\n";

    # Retry 3 times:
    my $ret = 0;
    my $j = 0;
    for ( $j = 1 ; $j <= $retry ; $j++ ) {

      print "Try $j: ";
      system("$cmd > $file");
      $ret = $?;

      # For testing purposes: Broke rows:
      if ( $DEBUG == 1 ) {
        my @aBrokenRows = (90000, 91000, 91001, 91002, 91003, 92000);
        my $br;
        foreach $br (@aBrokenRows) {
          if ( ($br >= $i) && ($br <= $last) ) {
            $ret = 99;
          }
        }
      }

      if ( $ret != 0 ) {
        print "ERROR: $ret\n";
        unlink($file);
        print "Broken row in block $i - $last\n";
        sleep($sleep);
      }
      else {
        print "success.\n";
        last;
      }
    }

    # If we do not suceed after 3 times we try one recursion deeper
    if ( ($ret != 0) && ($j >= $retry) ) {
      loop($i, $i+$step-1, $step/10);
    }
  }
}

loop($start_id, $end_id, $step);
exit(0);

