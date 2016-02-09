#!/usr/bin/perl -w

use strict;
use warnings;

eval "use Time::HiRes qw(time);";
my $load_hires = ! $@;

use DBI;
use DBD::mysql;
use Getopt::Long;
use File::Basename;

# Constants and Globals
# ---------------------

my $separator='#';
our $gEnd = 0;
our $MyName = basename($0);

# Functions
# ---------

sub Usage
{
    print("
SYNOPSIS

  $MyName flags

DESCRIPTION

  Creates a trace file from SHOW FULL PROCESSLIST.

FLAGS

  help, h      Print this help.
  debug        Enable debug mode [off].
  user, u      User to connect to [root]
  password, p  Password ['']
  host, h      Host [localhost]
  port, P      Port [3306]
  append, a    If trace should be appended to trace file or not [off].
  trace, t     Name of tracefile [mysql_processlist.trc]
  interval, i  Interval between snaphots [50 ms]
  include      Comma separated list of pids to trace
  exclude      Comma separated list of pids NOT to trace

PARAMETERS

  none

");
}

# Process parameters
# ------------------

my $optHelp = 0;
my $optDebug = 0;
my $optUser = 'root';
my $optPassword = '';
my $optHost = 'localhost';
my $optPort = '3306';
my $optAppend = 0;
my $optTracefile = 'mysql_processlist.trc';
my $optInterval = 50;
my $optInclude = '';
my $optExclude = '';

my $rc = GetOptions( 'help|?' => \$optHelp
                   , 'debug' => \$optDebug
                   , 'user|u=s' => \$optUser
                   , 'password|p=s' => \$optPassword
                   , 'host|h=s' => \$optHost
                   , 'port=i' => \$optPort
                   , 'append|a' => \$optAppend
                   , 'trace|t=s' => \$optTracefile
                   , 'interval|i=i' => \$optInterval
                   , 'include=s' => \$optInclude
                   , 'exclude=s' => \$optExclude
                   );

if ( $optHelp ) {
    &Usage();
    exit(0);

}

if ( ! $rc) {
    &Usage();
    exit(1);
}

if(@ARGV != 0) {
    &Usage();
    exit(2);
}

# Start here
# ----------

my ($dbh, $sql, $sth, @optInclude, @optExclude);

$dbh = DBI->connect("DBI:mysql::$optHost:$optPort"
                  , $optUser, $optPassword
                  , { RaiseError => 1 }
                   );

$sql = "
  SHOW FULL PROCESSLIST
";

$sth = $dbh->prepare( $sql );
if ( ! $dbh) {
  print("Error preperation: ", $dbh->errstr(), "\n");
  exit(3);
}

if ( $optAppend ) {
  open(TRACE, ">>". $optTracefile);
}
else {
  $optTracefile .= ".$$";
  open(TRACE, ">". $optTracefile);
}

if ( $optInclude ) {
  @optInclude = split(',', $optInclude);
}

if ( $optExclude ) {
  @optExclude = split(',', $optExclude);
}

$SIG{'INT'} = 'TERM';

sub TERM {
  our $gEnd = 1;
}

my ($timestamp, $id, $user, $host, $db, $command, $time, $state, $info);
my ($output, $exclude);
while ( ! $gEnd ) {

  $sth->execute();
  if (! $sth ) {
    print("Error execute: ", $sth->errstr(), "\n");
    exit(4);
  }

  $sth->bind_columns(undef, \$id, \$user, \$host, \$db, \$command, \$time, \$state, \$info);

  $timestamp = &Time::HiRes::time();
  while( $sth->fetchrow_arrayref ) {

    $exclude = 0;
    # Print to screen
    if ( $optDebug ) {
      print("$output\n");
    }

    if ( ! defined($id) ) { $id = "NULL"; } ;

    if ( $optExclude ) {

      foreach (@optExclude) {

        if ( $_ == $id ) {
          $exclude = 1;
          last;
        }
      }
    }

    if ( ! defined($user) ) { $user = "NULL"; } ;
    if ( ! defined($host) ) { $host = "NULL"; } ;
    if ( ! defined($db) ) { $db = "NULL"; } ;
    if ( ! defined($command) ) { $command = "NULL"; } ;
    if ( ! defined($time) ) { $time = "NULL"; } ;
    if ( ! defined($state) ) { $state = "NULL"; } ;
    if ( ! defined($info) ) { $info = "NULL"; } ;

    $output = join($separator, $timestamp, $id, $user, $host, $db, $command, $time, $state, $info);

    if ( $optInclude ) {

      $exclude = 1;
      foreach (@optInclude) {

        if ( $_ == $id ) {
          $exclude = 0;
          last;
        }
      }
    }

    if ( ! $exclude ) {
      # Print to trace file
      print(TRACE "$output\n");
    }
  }

  &Time::HiRes::usleep( $optInterval * 1000 );
}
$sth->finish;

close(TRACE);

$dbh->disconnect;

exit(0);
