#!/usr/bin/perl

#
# Copyright (c) 2011 - 2024 FromDual GmbH
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

use strict;
use warnings;

# use Time::HiRes qw( gettimeofday );
use DBD::mysql;
use File::Basename;
use Cwd qw(abs_path);
use Getopt::Long;

our $gMyName     = basename($0);
our $gMyNameBase = basename($0, '.pl');
our $gMyAbsPath  = dirname(abs_path($0));

sub Usage
{

  print <<_EOF;

SYNOPSIS

  $gMyName [options] command [objects]

DESCRIPTION

  Starts, stops and failovers MySQL Cluster repliation channels.

OPTIONS

  -f, --config-file=FILE     Use this configuration file.
  -d, --debug                Enable debug mode.
  -h, --help                 Display help.

COMMANDS

  status                     Display status of object.
  stop                       Stop channel.
  start                      Start channel.
  failover                   Failover channel 1 to channel 2.

OBJECTS

  channel
  channel_group

EXAMPLES

  $gMyName status

  $gMyName stop ch1

  $gMyName start ch1

  $gMyName failover ch1 ch2

_EOF
}

sub status
{
  my $lConfFile = $_[0];
  my $objects = $_[1];
  my @objects = @_;

  my @configuration = readConfigurationFile($lConfFile);

  if ( $objects eq 'all' ) {

    print "\nStatus for all channel groups and channels:\n\n";
    my @sections = &getSectionsOfType('channel_group', @configuration);

    foreach my $section ( @sections) {

      print "  Channel group: $section\n\n";

      my %section = &readSection(\@configuration, $section);

      foreach my $key ( sort(keys %section) ) {

        my @values = split(':', $section{$key});
        if ( $values[0] eq 'channel' ) {

          print "    Channel $key (M: " . $values[1] . ", S: " . $values[2] . ")\n";
          my $status = checkSlave($values[2], @configuration);
          print "    $status\n\n";
        }
      }
      print "\n";
    }
  }
  elsif ( $objects eq 'selection' ) {
    my $b = 0;
  }
  else {
    print "This case should never happen: $objects\n";
    exit(6);
  }
}

sub stopChannel
{
  my $lConfFile = $_[0];
  my $channel = $_[1];

  my @configuration = readConfigurationFile($lConfFile);

  my @sections = &getSectionsOfType('channel_group', @configuration);

  foreach my $section ( @sections) {

    my %section = &readSection(\@configuration, $section);

    foreach my $key ( sort(keys %section) ) {

      my @values = split(':', $section{$key});

      if ( ($values[0] eq 'channel') && ($channel eq $key) ) {

        print "Found channel $channel with Master: " . $values[1] . " and Slave: " . $values[2] . "\n";
        print "Stopping Slave " . $values[2] . "...\n";
        &stopSlave($values[2], @configuration);
      }
    }
  }
}

sub startChannel
{
  my $lConfFile = $_[0];
  my $channel = $_[1];

  my @configuration = readConfigurationFile($lConfFile);

  my @sections = &getSectionsOfType('channel_group', @configuration);

  foreach my $section ( @sections) {

    my %section = &readSection(\@configuration, $section);

    foreach my $key ( sort(keys %section) ) {

      my @values = split(':', $section{$key});

      if ( ($values[0] eq 'channel') && ($channel eq $key) ) {

        print "Found channel $channel with Master: " . $values[1] . " and Slave: " . $values[2] . "\n";
        print "Starting Slave " . $values[2] . "...\n";
        &startSlave($values[2], @configuration);
      }
    }
  }
}

sub failoverChannel
{
  my $lConfFile = $_[0];
  my $channel_from = $_[1];
  my $channel_to   = $_[2];

  &stopChannel($lConfFile, $channel_from);
  &startChannel($lConfFile, $channel_to);
}

sub stopSlave
{
  my $slave = $_[0];
  my @configuration = @_;

  my %section = &readSection(\@configuration, $slave);

  if ( ! defined($section{'user'}) || ! defined($section{'host'}) || ! defined($section{'port'}) ) {
    print "ERROR: section $slave is not configured correctly!\n";
    exit(5);
  }

  my $dbh = &getDatabaseConnection(\%section);
  if ( ! defined($dbh) ) {
    print "ERROR: cannot connect to Slave $slave!\n";
    exit(12);
  }

  my $sql = 'STOP SLAVE';
  my $sth = $dbh->prepare($sql);
  $sth->execute();
  &releaseDatabaseConnection($dbh);
}

sub startSlave
{
  my $slave = $_[0];
  my @configuration = @_;

  my %section = &readSection(\@configuration, $slave);

  if ( ! defined($section{'user'}) || ! defined($section{'host'}) || ! defined($section{'port'}) ) {
    print "ERROR: section $slave is not configured correctly!\n";
    exit(8);
  }

  my $dbh = &getDatabaseConnection(\%section);
  if ( ! defined($dbh) ) {
    print "ERROR: cannot connect to Slave $slave!\n";
    exit(9);
  }

  my $sql = 'START SLAVE';
  my $sth = $dbh->prepare($sql);
  $sth->execute();
  &releaseDatabaseConnection($dbh);
}

sub checkSlave
{
  my $slave = $_[0];
  my @configuration = @_;

  my $status = 'down';
  my %section = &readSection(\@configuration, $slave);

  if ( ! defined($section{'user'}) || ! defined($section{'host'}) || ! defined($section{'port'}) ) {
    print "ERROR: section $slave is not configured correctly!\n";
    $status = 'err';
    return $status;
  }

  my $dbh = &getDatabaseConnection(\%section);
  if ( ! defined($dbh) ) {
    $status = 'failed';
    return $status;
  }

  my %hSlaveStatus = &getSlaveStatus(\%section, $dbh);
  &releaseDatabaseConnection($dbh);

  if ( keys(%hSlaveStatus) > 0 ) {
    print "      IO_thread : " . $hSlaveStatus{'Slave_IO_Running'};
    if ( $hSlaveStatus{'Slave_IO_Running'} ne 'Yes' ) {
      if ( $hSlaveStatus{'Last_IO_Errno'} != 0 ) {
        print " - Errno: " . $hSlaveStatus{'Last_IO_Errno'} . " - " . $hSlaveStatus{'Last_IO_Error'};
      }
    }
    print "\n";

    print "      SQL_thread: " . $hSlaveStatus{'Slave_SQL_Running'};
    if ( $hSlaveStatus{'Slave_SQL_Running'} ne 'Yes' ) {
      if ( $hSlaveStatus{'Last_SQL_Errno'} != 0 ) {
        print " - Errno: " . $hSlaveStatus{'Last_SQL_Errno'} . " - " . $hSlaveStatus{'Last_SQL_Error'};
      }
    }
    print "\n";

    if ( ($hSlaveStatus{'Slave_IO_Running'} eq 'Yes') && ($hSlaveStatus{'Slave_SQL_Running'} eq 'Yes' ) ) {
      $status = 'up';
    }
  }
  return $status;
}

sub readConfigurationFile
{
  my $file = $_[0];

  if ( ! -e "$file" ) {
    print "Configuration file $file does not exist or is not readable.\n";
    exit(1);
  }

  if ( ! -r "$file" ) {
    print "Cannot read configuration file $file\n";
    exit(2);
  }

  my @config;
  open CONFIG, $file or die $!;
  while ( <CONFIG> ) {
    chomp;                  # remove newlines
    s/^\s*#.*//;            # remove comment
    s/^\s+//;               # remove leading white space
    s/\s+$//;               # remove trailing white space
    next unless length;     # anything left?
    push(@config, $_);
  }
  close(CONFIG);
  return @config;
}

sub getSections
{
  my @config = @_;

  my @sections;
  foreach my $line ( @config ) {

    if ( $line =~ /^\[(.+)\]/ ) {
      push(@sections, $1);
    }
  }

  return @sections;
}

sub getSectionsOfType
{
  my $type = $_[0];
  my @configuration = @_;

  my @sections = &getSections(@configuration);
  my @s_ret;
  foreach my $section ( @sections ) {

    my %section = &readSection(\@configuration, $section);
    if ( defined($section{'type'}) && ($section{'type'} eq 'channel_group') ) {
      push(@s_ret, $section);
    }
  }

  return @s_ret;
}

sub readSection
{
  my $config = shift;
  my $section = shift;

  my %section;
  my $in_section = 0;
  foreach my $line ( @$config ) {

    if ( $line =~ /^\[(.+)\]/ ) {
      if ( $1 eq $section ) {
        $in_section = 1;
        next;
      }
      else {
        if ( $in_section == 1 ) {
          last;
        }
        $in_section = 0;
        next;
      }
    }

    if ( $in_section && ( $line =~ /^(\S+)\s*=\s*(.+)\s*$/ ) ) {
      $section{$1} = $2;
    }
  }

  return %section;
}

sub getSlaveStatus
{
  my $conf_ref = shift;
  my $dbh = shift;

  my $sql = 'SHOW SLAVE STATUS';

  my $sth = $dbh->prepare($sql);
  my $hSlaveStatus;
  if ( $sth->execute() )
  {
    $hSlaveStatus = $sth->fetchrow_hashref();
    $sth->finish();
  }

  # show global variables like 'log_slave_updates';
  # +-------------------+-------+
  # | Variable_name     | Value |
  # +-------------------+-------+
  # | log_slave_updates | OFF   |
  # +-------------------+-------+
  #
  # show global variables like 'read_only';
  # +----------------------+--------+
  # | Variable_name        | Value  |
  # +----------------------+--------+
  # | read_only            | OFF    |
  # +----------------------+--------+

  return %{$hSlaveStatus};
}

sub getDefaultParameterXXX
{
  my $config = shift;

  my %hDefaults;

  # Hard coded defaults

  $hDefaults{'ClusterLog'}    = '/var/lib/mysql-cluster/ndb_1_cluster.log';
  $hDefaults{'Disabled'}      = 'false';
  $hDefaults{'FetchMethod'}   = 'DBI';
  $hDefaults{'LogFile'}       = '/var/log/zabbix/FromDualMySQLagent.log';
  $hDefaults{'Modules'}       = 'mysql myisam process';
  $hDefaults{'MysqlHost'}     = '127.0.0.1';
  $hDefaults{'MysqlPort'}     = '3306';
  $hDefaults{'Password'}      = '';
  $hDefaults{'PidFile'}       = '';
  $hDefaults{'Socket'}        = '/run/mysqld/mysql.sock';
  $hDefaults{'Type'}          = 'mysqld';
  $hDefaults{'Username'}      = 'root';
  $hDefaults{'ZabbixServer'}  = '';

  # Read default from configuration
  my %default = &readSection(\@$config, 'default');

  foreach my $key ( keys %default ) {

    if ( $default{$key} ne '' ) {
      $hDefaults{$key} = $default{$key};
    }
  }

  return %hDefaults
}

sub getParameterXXX
{
  my $config = shift;
  my $section = shift;

  my %defaults = getDefaultParameter(\@$config);

  my %parameter = readSection(\@$config, $section);

  foreach my $key ( keys %parameter ) {

    if ( $parameter{$key} ne '' ) {
      $defaults{$key} = $parameter{$key};
    }
  }
  $defaults{'Hostname'} = $section;

  return %defaults;
}

sub logXXX
{
  my $logfile = shift;
  my $lvl = shift;
  my $str = shift;

  if ( (! defined $logfile) || ($logfile eq '') ) {
    print "Logfile is not defined or empty.\n";
    exit(3);
  }

  # Logfile does not exist try to create it
  # This has to be done before the -w check because otherwiese -w would fail the
  # time!
  if ( ! -e $logfile ) {
    open LOG, ">>" . $logfile or die $!;
    close(LOG);
  }

  if ( ! -w $logfile ) {
    print "Cannot write to logfile $logfile. Please check permissions.\n";
    exit(4);
  }

  my $severity = '';
  my $PID = $$;

  open LOG, ">>" . $logfile or die $!;
  my ($seconds, $microseconds) = gettimeofday();
  my ($sec, $min, $hour, $mday, $mon, $year, $wday, $yday, $isdst) = localtime($seconds);
  printf(LOG "%5d:%4d-%02d-%02d %02d:%02d:%02d.%03d - %s: %s\n", $PID, $year+1900,$mon+1,$mday,$hour,$min,$sec, $microseconds / 1000, $severity, $str);
  close(LOG);

  # Do a log rotate if necessary if logfile is > 10M
  my $filesize = -s $logfile;
  if ( $filesize > 10*1024*1024 ) {
    my $new = $logfile . '.rotated';
    rename($logfile, $new) or system("mv", $logfile, $new);
  }
}

sub getDatabaseConnection
{
  my $conf_ref = shift;

  my $dbh = DBI->connect('DBI:mysql:database=mysql;host=' . $$conf_ref{'host'} . ';port=' . $$conf_ref{'port'}
                         , $$conf_ref{'user'}, $$conf_ref{'password'}
                         , { 'RaiseError' => 0, 'PrintError' => 0 }
                           );

  if ( defined $DBI::err ) {
    print 'ERROR: DBI connect with database=mysql, host=' . $$conf_ref{'host'} . ', port=' . $$conf_ref{'port'} . ' and user=' . $$conf_ref{'user'} . ' failed: ' . $DBI::errstr . "\n";
  }

  return $dbh;
}

sub releaseDatabaseConnection
{
  my $dbh = shift;

  $dbh->disconnect();
}

sub initValuesXXX
{

  my $variables_ref = shift;
  my $variables_to_send_ref = shift;

  foreach my $key ( @$variables_to_send_ref ) {
    $$variables_ref{$key} = 0;
  }
}

sub getGlobalVariablesXXX
{

  my $conf_ref = shift;
  my $dbh = shift;
  my $variables_ref = shift;

  my $sql = 'SHOW GLOBAL VARIABLES';
  if ( $$conf_ref{'FetchMethod'} eq 'DBI' ) {

    my $sth = $dbh->prepare($sql);
    if ( $sth->execute() ) {
      while ( my $ref = $sth->fetchrow_hashref() ) {
        $$variables_ref{$ref->{'Variable_name'}} = $ref->{'Value'};
      }
      $sth->finish();
    }
  }
  elsif ( $$conf_ref{'FetchMethod'} eq 'mysql' ) {

    my $cmd = 'mysql --user=' . $$conf_ref{'Username'} . ' --host=' . $$conf_ref{'MysqlHost'} . ' --port=' . $$conf_ref{'MysqlPort'} . ' --password=' . $$conf_ref{'Password'} .' --execute="' . $sql . '"';

    my @stdout = `$cmd 2>&1`;

    if ( $? != 0 ) {
      &log($$conf_ref{'LogFile'}, 0, "$cmd failed.");
    }
    else {

      foreach my $line ( @stdout ) {

        if ( $line =~ m/^(\S+)\s+(.*)\s*$/ ) {
          if ( "$1" ne 'Variable_name' ) {
            $$variables_ref{$1} = "$2";
          }
        }
      }
    }
  }
}

sub getGlobalStatusXXX
{

  my $conf_ref = shift;
  my $dbh = shift;
  my $status_ref = shift;

  my $sql = 'SHOW /*!50000 GLOBAL */ STATUS';
  if ( $$conf_ref{'FetchMethod'} eq 'DBI' ) {

    my $sth = $dbh->prepare($sql);
    if ( $sth->execute() ) {
      while ( my $ref = $sth->fetchrow_hashref() ) {
        $$status_ref{$ref->{'Variable_name'}} = $ref->{'Value'};
      }
      $sth->finish();
    }
  }
  elsif ( $$conf_ref{'FetchMethod'} eq 'mysql' ) {

    my $cmd = 'mysql --user=' . $$conf_ref{'Username'} . ' --host=' . $$conf_ref{'MysqlHost'} . ' --port=' . $$conf_ref{'MysqlPort'} . ' --password=' . $$conf_ref{'Password'} . " --execute='" . $sql . "'";
    my @stdout = `$cmd 2>&1`;

    if ( $? != 0 ) {
      &log($$conf_ref{'LogFile'}, 0, "$cmd failed.");
    }
    else {

      foreach my $line ( @stdout ) {

        if ( $line =~ m/^(\S+)\s+(.*)\s*$/ ) {
          if ( "$1" ne 'Variable_name' ) {
            $$status_ref{$1} = "$2";
          }
        }
      }
    }
  }
}

# ---------------------------------------------------------------------

# Process parameters
# ------------------

my  $lHelp  = 0;
our $gDebug = 0;

our $gConfFile = '';
# Look for config file at the following locations:
# 1. /etc/channel_failover.conf
# 2. /etc/mysql/channel_failover.conf
# 3. ./channel_failover.conf
# 4. -f config-file

my $path = '/etc';
if ( -r "$path/$gMyNameBase.conf" ) {
  $gConfFile = "$path/$gMyNameBase.conf";
}

$path = '/etc/mysql';
if ( -r "$path/$gMyNameBase.conf" ) {
  $gConfFile = "$path/$gMyNameBase.conf";
}

if ( -r "$gMyAbsPath/$gMyNameBase.conf" ) {
  $gConfFile = "$gMyAbsPath/$gMyNameBase.conf";
}

my $rc = GetOptions(
  'help|?|h'        => \$lHelp
, 'debug|d'         => \$gDebug
, 'config-file|f=s' => \$gConfFile
);

if ( $lHelp ) {
  &Usage();
  exit(0);
}

if ( ! $rc) {
  &Usage();
  exit(10);
}

# Start here
# ----------

if ( ! defined($gConfFile) ) {
  print "ERROR: No configuration file.\n";
  exit(13);
}

if ( ! -r $gConfFile ) {
  print "ERROR: Cannot read from $gConfFile\n";
  exit(14);
}

if ( $gDebug != 0 ) {
  print "DEBUG: Configuration file is: $gConfFile\n";
}

if ( @ARGV == 0 ) {
  print "Argument id required.\n";
  &Usage();
  exit(15);
}

my $command = $ARGV[0];
if ( $gDebug != 0 ) {
  print "DEBUG: Command: $command\n";
}

if ( $command eq 'status' ) {

  # command can contain some objects (channels or channel_groups)
  # otherwise display all

  my @objects;
  my $objects;
  my $i = 1;

  # No argument
  if ( ! defined($ARGV[$i]) ) {
    $objects = 'all';
  }
  else {
    $objects = 'selection';
    while ( defined($ARGV[$i]) ) {

      print "DEBUG: Object found: " . $ARGV[$i] . "\n";
      push(@objects, $ARGV[$i]);
      $i++;
    }
  }

  if ( $gDebug != 0 ) {
    print "DEBUG: Objects: $objects\n";
  }

  &status($gConfFile, $objects, @objects);
}
elsif ( $command eq 'stop' ) {

  # stop command can contain a channel
  my $channel = '';
  if ( defined($ARGV[1]) ) {
    $channel = $ARGV[1];
  }

  print "DEBUG: Stop channel $channel\n";

  &stopChannel($gConfFile, $channel);
}
elsif ( $command eq 'start' ) {

  # start command can contain a channel
  my $channel = '';
  if ( defined($ARGV[1]) ) {
    $channel = $ARGV[1];
  }

  print "DEBUG: Start channel $channel\n";

  &startChannel($gConfFile, $channel);
}
elsif ( $command eq 'failover' ) {

  # failover command has two channels
  my $channel_from = '';
  my $channel_to   = '';
  if ( defined($ARGV[1]) ) {
    $channel_from = $ARGV[1];
  }
  if ( defined($ARGV[2]) ) {
    $channel_to   = $ARGV[2];
  }

  if ( ($channel_from eq '') || ($channel_to eq '') ) {
    print "ERROR: You have to specify 2 channels.\n";
    exit(11);
  }

  print "DEBUG: Failover channel $channel_from to $channel_to\n";

  &failoverChannel($gConfFile, $channel_from, $channel_to);
}
else {
  print "Command $command does not exist.\n";
  &Usage();
  exit(7);
}

exit(0);
