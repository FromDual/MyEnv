#! /usr/bin/perl
#
#      DIFF-status      Compute the difference of two files with "show global status" output
#

use strict ;

use Data::Dumper;
use Getopt::Long;

sub print_help
{
    print <<EOF;
Usage: DIFF-status --start=PATH --stop=PATH --diff=PATH

Read 2 files with MySQL "show global status" output and compute the differences.

Options:
      --start      the file with the measurements at the beginning
      --stop       the file with the measurements at the end
      --diff       the file receiving al measurements and the differences
                   The three file name paranmeters are mandatory!
  -h, --help       write this text, then exit
EOF

exit 1;
}

my %status_start = ();
my %status_stop  = ();

sub read_status_into_hash
{
    my ( $filename, $value_hash ) = @_ ;
    my $line ;
    my $name ;
    my $value ;
    
    open FILE_IN, "$filename" or die "Cannot open input file '$filename': $!\n";
    
    while ( $line !~ /\| Variable_name *\| Value *\|/ ) {
        $line = <FILE_IN> or die "Did not find table header in file '$filename': $!\n"
    }
    $line = <FILE_IN> ; # separator line in table-style output
    
    while ( $line = <FILE_IN> ) {
        chomp $line ;
        if ( $line =~ /^\+-*\+-*\+$/ ) { last }
        if ( $line =~ /^\| (\w+)  *\| (\S+)  *\|$/ ) {
            $name = $1 ;
            $value = $2 ;
            $value_hash -> {$name} = $value
        } elsif ( $line =~ /^\| (\w+)  *\|  *\|$/ ) {
            $name = $1 ;
            $value = '' ;
            $value_hash -> {$name} = $value
        } else {
            print "Format error in file '$filename':\n$line\n\n"
        }
    }
    
    # Found EOF or table end line
    close FILE_IN ;
}


my $ret;

my %option = ();

$ret = GetOptions(
    \%option,
    "start=s",
    "stop=s",
    "diff=s",
    "help|h"
) or print_help();

if ( "$option{'help'}" ) { print_help() }

if ( ! "$option{'start'}" || ! "$option{'stop'}" || ! "$option{'diff'}" ) {
    print "One or more mandatory file names are missing - ABORT\n\n";
    print_help()
}

read_status_into_hash ( $option{'start'}, \%status_start );
read_status_into_hash ( $option{'stop'}, \%status_stop );

open FILE_DIFF, ">$option{'diff'}"  or die "Cannot open 'diff' result file '$option{'diff'}': $!\n";

my %union = ();
my $elem ;
foreach $elem ( keys %status_start, keys %status_stop ) { $union{$elem} = 1 }

foreach $elem ( sort keys %union ) {
    print FILE_DIFF sprintf "%-50s  %20ld  %20ld  %20ld\n", 
                $elem, $status_start{$elem}, $status_stop{$elem}, $status_stop{$elem} - $status_start{$elem}, "\n"
}

exit 0;

