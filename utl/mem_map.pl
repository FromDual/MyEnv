#!/usr/bin/perl
#
# mem_map.pl
#

my $argc = @ARGV;
if ( $argc != 1 ) {
  print "Please use one PID as argument.\n";
  exit;
}

die "Usage: $0 PID\n" unless $ARGV[0] > 0;

$file = sprintf "/proc/%s/maps", $ARGV[0];

open(IN, "<$file") or die;
while ( <IN> ) {

  ($mem, $prot, $offset, $dev, $inode, $type) = split();
  ($start, $stop) = split("-", $mem);

  $size = hex($stop) - hex($start);
  $total += $size;

  printf "%-60s %s %8.0f Kbyte\n", $type, $prot, $size/1024;

  if ($prot =~ /^r-/ and $inode != 0) {
    $share += $size;
  }
}
close(IN);

printf "\n";
printf "share   = %8.0f Kbyte\n", $share/1024;
printf "private = %8.0f Kbyte\n", ($total-$share)/1024;
printf "total   = %8.0f Kbyte (%8.2f %% shareable)\n", $total/1024, $share/$total*100
