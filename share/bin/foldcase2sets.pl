#!/usr/bin/perl -w

#
# Copyright 2008-2017 Katherine Flavel
#
# See LICENCE for the full copyright terms.
#

use strict;
use warnings;

my ($lx) = @ARGV;

die "$0: Usage: $0 foldcase.lx\n"
    unless defined $lx;

open(my $fh, $lx)
    or die qq[$0: Failed to open "$lx": $!\n];

my %sets;
my %line;
my $maxid;

while (<$fh>) {
    if (/^# \$u(\d+): (.+)/) {
	my ($id, $elems) = ($1, $2);
	die "$0: $lx: $.: Duplicate set '$id' (previous: $line{$id})\n"
	    if defined $sets{$id};
	$sets{$id} = [ split(' ', $elems) ];
	$line{$id} = $.;
	$maxid //= $id;
	$maxid = $id if $id > $maxid;
    }
}

my @expid = 0..$maxid;
my %expid;
my %unexp;
@expid{@expid} = ();
@unexp{keys %sets} = ();
delete @expid{keys %sets};
delete @unexp{@expid};

unless (keys %expid == 0) {
    die sprintf("$0: Expected ids missing: expected %d, missing %d\n",
		scalar @expid, scalar keys %expid);
}
if (scalar keys %unexp > 0) {
    die sprintf("$0: Unexpected ids found: unexpected %d\n",
		scalar keys %unexp);
}

# We need to find out what each TOK_nnn maps to, integer-wise.
# We could depend on the inside information of the implementation,
# or we can generate code to do it without any inside information.

sub get_tok_map {
    use File::Temp qw[tempfile];

    my ($fhc, $fnc) = tempfile(SUFFIX => '.c', UNLINK => 1);
    my ($fhh, $fnh) = tempfile(SUFFIX => '.h', UNLINK => 1);
    my ($fhx, $fnx) = tempfile(UNLINK => 1);

    my $gen_hdr = "lx -lh < $lx > $fnh";
    system($gen_hdr) == 0
	or die qq[$0: Failed to generate header with "$gen_hdr": $!\n];

    print { $fhc } qq[#include <stdio.h>\n];
    print { $fhc } qq[#include "$fnh"\n];
    print { $fhc } qq[int main() {\n];
    for my $id (0..$maxid) {
	print { $fhc } qq[printf("$id %d\\n", TOK_U$id);\n];
    }
    print { $fhc } qq[}\n];
    close($fhc);
    my $gen_exe = "cc -o $fnx $fnc";
    system($gen_exe) == 0
	or die qq[$0: Failed to generate executable with "$gen_exe": $!\n];
    my $run_exe = "$fnx |";

    open(my $map_fh, $run_exe)
	or die qq[$0: Failed to run executable with "$run_exe": $!\n];
    my %tok_map;
    while (<$map_fh>) {
	if (/^(\d+) (\d+)$/) {
	    die qq[$0: Duplicate mapping of $1\n] if defined $tok_map{$1};
	    $tok_map{$1} = $2;
	}
    }
    my @tok_map = sort { $a <=> $b } keys %tok_map;
    die qq[$0: Illegal mapping\n]
	unless @tok_map == $maxid + 1 &&
	       $tok_map[0] == 0 &&
	       $tok_map[-1] == $maxid &&
	       exists $tok_map{0} &&
	       exists $tok_map{$maxid};

    return \%tok_map;
}

my $tok_map = get_tok_map();

my $tok_foldcase_h = "/tmp/tok_foldcase.h";
open(my $genh, ">", $tok_foldcase_h)
    or die qq[$0: Failed to create "$tok_foldcase_h": $!\n];
print qq[$0: Created $tok_foldcase_h\n];
select($genh);

print <<__EOF__;
#ifndef TOK_FOLDCASE_H
#define TOK_FOLDCASE_H

const char * const * tok_to_foldcase(unsigned tok, unsigned *count);

#endif /* #ifndef TOK_FOLDCASE_H */
__EOF__
close($genh);
select(STDOUT);

my $tok_foldcase_c = "/tmp/tok_foldcase.c";
open(my $genc, ">", $tok_foldcase_c)
    or die qq[$0: Failed to create "$tok_foldcase_c": $!\n];
print qq[$0: Created $tok_foldcase_c\n];
select($genc);

print <<__EOF__;
#include "tok_foldcase.h"

#include <stdlib.h>

__EOF__

for my $id (0..$maxid) {
    print qq[static const char * foldcase_${id}[] = { ];
    my @e = @{ $sets{$id} };
    for my $ei (0..$#e) {
	printf(qq["%s"%s], $e[$ei], $ei < $#e ? ", " : "");
    }
    print qq[ };\n];
}
print qq[\n];

print qq[static const struct { unsigned count; const char * const * elems; } foldcase[] = { ];
for my $id (0..$maxid) {
    my @e = @{ $sets{$tok_map->{$id}} };
    printf(qq[\t{ %d, foldcase_$tok_map->{$id} }%s\n],
	   scalar @e, $id < $maxid ? ", " : "");
}
print qq[};\n\n];

print <<__EOF__;
const char * const * tok_to_foldcase(unsigned tok, unsigned *count) {
    if (tok > $maxid) {
	return NULL;
    }
    *count = foldcase[tok].count;
    return foldcase[tok].elems;
}
__EOF__

close($genc);
select(STDOUT);

exit(0);
