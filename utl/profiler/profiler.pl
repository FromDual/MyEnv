#!/usr/bin/perl -w

use strict;
use warnings;

use Getopt::Long;
use File::Basename;

# Constants and Globals
# ---------------------

my $separator='#';
our $MyName = basename($0);

# Functions
# ---------

sub Usage
{
    print("
SYNOPSIS

  $MyName flags

DESCRIPTION

  Profiles a trace file generated with tracer.pl

FLAGS

  help, h      Print this help.
  debug        Enable debug mode [off].
  trace, t     Name of tracefile [mysql_processlist.trc]
  pid          Profile only this id.
  user, u      Profile only this user.
  host, h      Profile only this host.
  database, D  Profile only this database.

PARAMETERS

  none

");
}

# Process parameters
# ------------------

my $optHelp = 0;
my $optDebug = 0;
my $optTracefile = 'mysql_processlist.trc';
my $optPid = 0;
my $optUser = '';
my $optHost = '';
my $optDatabase = '';

my $rc = GetOptions( 'help|?' => \$optHelp
                   , 'debug' => \$optDebug
                   , 'trace|t=s' => \$optTracefile
                   , 'user|u=s' => \$optUser
                   , 'host|h=s' => \$optHost
                   , 'pid=i' => \$optPid
                   , 'database|D=s' => \$optDatabase
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

my %command;
my %state;
my $slotCounter = 1;   # Because we loose the first one!
my $timeSum = 0;
my $oldTimestamp = 0;
my ($timestamp, $pid, $user, $host, $database, $command, $time, $state, $info);
my ($match, $total, $skipped, $matched) = (undef, 0, 0, 0);
open(TRACE, '<'. $optTracefile) or die "Problems opening file $optTracefile";
while ( <TRACE> ) {

  chomp;
  $total++;
  if (
       m/^(\d{10}\.\d+)$separator(\d+)$separator([\w\s]+)$separator([\w\.:]*)$separator([\w]+)$separator([\w\s]+)$separator(\w+)$separator([\w\s]*)$separator([\w\s]*)/
     ) {

    $matched++;
    if ( $optDebug ) {
      print "$_\n";
    }

    $timestamp = $1;
    $pid = $2;
    $user = $3;
    $host = $4;
    $database = $5;
    $command = $6;
    $time = $7;
    $state = $8;
    $info = $9;

    # Only for the very first time
    if ( $oldTimestamp == 0 ) {
      $oldTimestamp = $timestamp;
    }

    if ( $command eq '' ) {
      $command = "Idling";
    }

    if ( $state eq '' ) {
      $state = "Idling";
    }

    if ( $timestamp ne $oldTimestamp ) {

      $slotCounter++;
      $timeSum += ($timestamp - $oldTimestamp);
      $oldTimestamp = $timestamp;
    }

    # Filters only apply to this
    # --------------------------

    $match = 0;
    # One of these filters match
    if ( $optPid or $optUser or $optHost or $optDatabase ) {

      if ( $optPid == $pid ) {
        $match = 1;
      }
      if ( $optUser eq $user ) {
        $match = 1;
      }
      if ( $optHost eq $host ) {
        $match = 1;
      }
      if ( $optDatabase eq $database ) {
        $match = 1;
      }
    }
    # No filters, take all
    else {
      $match = 1;
    }

    if ( $match ) {
      $command{$command} += 1;
      $state{$state} += 1;
    }
  }
  else {

    $skipped++;
    if ( $optDebug ) {
      print "Debug-skipped: $_\n";
    }
  }
}
close(TRACE);

if ( $optDebug ) {
  print "timeSum = $timeSum\n";
  print "slotCounter = $slotCounter\n";
}

my $avgSlotTime = $timeSum/$slotCounter;
my $filters = "";

if ( $optPid ) {
  $filters .= "pid = $optPid, ";
}
if ( $optUser ) {
  $filters .= "user = $optUser, ";
}
if ( $optHost ) {
  $filters .= "host = $optHost, ";
}
if ( $optDatabase ) {
  $filters .= "database = $optDatabase, ";
}

if ( ! $filters ) {
  $filters .= "none";
}

# Now output
# ----------

sub state_after_value
{
  $state{$a} <=> $state{$b};
}

sub command_after_value
{
  $command{$a} <=> $command{$b};
}

print "\n";
print "General infos\n";
print "-------------\n";
printf("Slots         : %8d\n", $slotCounter);
printf("Time          : %12.3f s\n", $timeSum);
printf("Interval      : %12.3f s\n", $avgSlotTime);
printf("Filters       : %s\n", $filters);
printf("Lines total   : %8d\n", $total);
printf("Lines skipped : %8d\n", $skipped);
printf("Lines matched : %8d\n", $matched);

my $vSum = 0;
my $tSum = 0;
my ($key, $value);

# ------------------------------------------------------------------------------

print "\n";
print "Commands\n";
print "--------\n";
$vSum = 0;
$tSum = 0;
# Calculate total first
foreach $key ( keys(%command) ) {
  $value = $command{$key};
  $vSum += $value;
  $tSum += ($value*$avgSlotTime);
}
# Then do the chart
foreach $key ( reverse sort command_after_value keys(%command) ) {

  $value = $command{$key};
  printf("%-25s   %8d   %12.3f s   %5.1f %%\n", $key, $value, $value*$avgSlotTime, ($value*$avgSlotTime/$tSum*100.0));
}
printf("-------------------------   --------   --------------   -------\n");
printf("%-25s   %8d   %12.3f s   %5.1f %%\n", "Total", $vSum, $tSum, 100.0);

# ------------------------------------------------------------------------------

print "\n";
print "State\n";
print "-----\n";
$vSum = 0;
$tSum = 0;
# Calculate total first
foreach $key ( keys(%state) ) {
  $value = $state{$key};
  $vSum += $value;
  $tSum += ($value*$avgSlotTime);
}
# Then do the chart
foreach $key ( reverse sort state_after_value keys(%state) ) {

  $value = $state{$key};
  printf("%-25s   %8d   %12.3f s   %5.1f %%\n", $key, $value, $value*$avgSlotTime, ($value*$avgSlotTime/$tSum*100.0));
}
printf("-------------------------   --------   --------------   -------\n");
printf("%-25s   %8d   %12.3f s   %5.1f %%\n", "Total", $vSum, $tSum, 100.0);

exit(0);
