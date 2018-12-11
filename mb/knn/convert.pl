#!/usr/bin/perl -w

# Script for converting data files from th UCI machine learning repository

$n = -1; # columns
$m = 0; # rows

open(DATA, "<", $ARGV[0]) or die "Can't open $ARGV[0]: $!";

while (<DATA>) {
    chomp;
    next if /^$/; # skip empty lines
    @l = split /, ?/;

    # check number of coordinates
    $c = $#l + 1;
    if ($n != $c) {
        if ($n == -1) {
            $n = $c;
            for ($i = 0; $i < $n; $i++) {
                 $count[$i] = 0;
            }
        } else {
            die "Different number of coordinates: $_";
        }
    }

    for ($i = 0; $i < $n; $i++) {
        $v = $l[$i];
        if (!defined $byval{"$i:$v"}) {
            $bykey{"$i:$count[$i]"} = $v;
            $byval{"$i:$v"} = $count[$i]++;
        }
    }

    $m++;
}

# check numerical fields
for ($i = 0; $i < $n; $i++) {
    $num = 1;
    for ($j = 0; $j < $count[$i]; $j++) {
        if ($bykey{"$i:$j"} !~ /^[-+eE.0-9]*$/) {
            $num = 0;
            last;
        }
    }
    $count[$i] = 0 if ($num);
}

# print header
print "$m $n\n";
for ($i = 0; $i < $n; $i++) {
    print $count[$i];
    for ($j = 0; $j < $count[$i]; $j++) {
            print ',', $bykey{"$i:$j"};
    }
    print "\n";
}

seek DATA, 0, 0; # rewind

# print data
while (<DATA>) {
    chomp;
    next if /^$/; # skip empty lines
    @l = split /, ?/;

    for ($i = 0; $i < $n; $i++) {
        print ',' if ($i != 0);
        $v = $l[$i];
        if ($count[$i] == 0) {
            # numerical value
            print $v;
        } else {
            print $byval{"$i:$v"};
        }
    }
    print "\n";
}

