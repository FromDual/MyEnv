#!/usr/bin/perl -w

use strict;
use warnings;

use Getopt::Long;
use File::Basename;

# Constants and Globals
# ---------------------

our $MyName = basename($0);

# Functions
# ---------

sub Usage
{
  print <<EOF;

SYNOPSIS

  $MyName flags

DESCRIPTION

  Converts MySQL output generated with \\G into table form

FLAGS

  --help, -h, -?   Print this help.
  --debug          Enable debug mode [off].
  --file, -f       File to convert
  --offset, -o     Number of rows to skip
  --rows, -r       Number of rows to build a record
  --show, -s       Show skipped rows although
  --delimiter, -d  Delimiter to use to build the table.

PARAMETERS

  none

EOF
}

# Process parameters
# ------------------

my $lHelp = 0;
my $lDebug = 0;
my $lFilename;
my $lOffset = 0;
my $lRows = 0;
my $lShow = 0;
my $lDelimiter = "\t";

my $rc = GetOptions( 'help|?|h' => \$lHelp
                   , 'debug' => \$lDebug
                   , 'file|f=s' => \$lFilename
                   , 'offset|o=i' => \$lOffset
                   , 'rows|r=i' => \$lRows
                   , 'show|s' => \$lShow
                   , 'delimiter|d=s' => \$lDelimiter
                   );

if ( $lHelp ) {
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

if ( !defined($lFilename) ) {
  print "Error: no filename\n";
  exit 1;
}

if ( ! -r $lFilename ) {
  print "Error, cannot read from $lFilename\n";
  exit 2;
}

if ( $lDebug ) {

  print "Filename  : $lFilename\n";
  print "Offset    : $lOffset\n";
  print "Rows      : $lRows\n";
  print "Delimiter : '$lDelimiter'\n";
}

open INPUT, "<$lFilename";

my $cnt = 1;
my $row = 1;

while ( <INPUT> ) {

  # Skip offset rows...
  if ( $row <= $lOffset ) {
    $row++;
    if ( $lShow ) {
      print $_;
    }
    next;
  }

  chomp;
  print $_;
  if ( $cnt < $lRows ) {
    print $lDelimiter;
    $cnt++;
  }
  else {
    $cnt = 1;
    print "\n";
  }
}
close INPUT;
