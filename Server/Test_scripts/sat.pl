#!/usr/bin/perl

use HTML::Parser;
use LWP::Simple;

my $onesaturl  = "http://www.heavens-above.com/PassSummary.aspx?showAll=t";
#my $allsatsurl = "http://www.heavens-above.com/allsats.aspx";

my %locations = (
    "Aarhus" => "&lat=56.156361&lng=10.188631&alt=40&loc=Aarhus&tz=CET",
#    "SJC  => "&lat=37.339&lng=-121.894&alt=0&loc=San+Jose&TZ=PST"
);

my %satellites = (    
    "ISS"      => "&satid=25544",
#    Mir      => "",
    "STS-98" => "&satid=26698"
);

print "Satellite Pass Info for " . (join ', ', sort keys %satellites) . " and all >-3.5 objects.\n";
print "(updated " . scalar(localtime) . " from www.heavens-above.com)\n\n";

my $p = HTML::Parser->new(api_version => 3,
                          start_h => [\&start, "tagname, attr"],
                          text_h  => [\&text,  "dtext"],
                          end_h   => [\&end,   "tagname"],
                          unbroken_text => 1);
my $state;
my ($row, $location, $satellite);
$^L = "";
$- = 0;
foreach $location (keys %locations) {

    # get all the selected satellites
    foreach $satellite (keys %satellites) {
        my $url = "$onesaturl$locations{$location}$satellites{$satellite}\n";
        $state = {};
        $p->parse(get($url));
        foreach $row (@{$state->{table}}) {
            next unless $row->[0] =~ /\S/;
            unshift @{$row}, $satellite;
            write;
            last;
        }
    }

    # also get all the bright (<= Mag -3.3) satellites
#    my $url  = "$allsatsurl?$locations{$location}&Mag=-3.5";

#   $state = {};
#    $p->parse(get($url));
#    foreach $row (@{$state->{table}}) {
#        next unless $row->[0] =~ /\S/;
        
        # need to add date in (it's missing)
#        my $satellite = shift @{$row};
#        unshift @{$row}, "";  # should be the date, but i'll omit it for now.
                              # (need to infer it based on time, which is
                              #  a pain)
#        unshift @{$row}, $satellite;        
#        write;
#    }
}


sub start {
    my ($tagname, $attr) = @_;
    $state->{in}{$tagname} = 1;
    
    if ($tagname eq "table" && $attr->{border}) {
        $state->{interesting} = 1;
    }

    if ($tagname eq "table") {
        $state->{rownum} = 0;
    }

    if ($tagname eq "tr") {
        $state->{colnum} = -1;
        $state->{rownum}++;
    }

    if ($tagname eq "td") {
        $state->{colnum}++;
    }
}

sub end {
    my ($tagname) = @_;

    if ($tagname eq "table" && $state->{interesting}) {
        $state->{interesting} = 0;
    }

    $state->{in}{$tagname} = 0;
}                           

sub text {
    my ($text) = @_;

    return unless $state->{interesting};

    if ($state->{in}{td}) {
        $text =~ s/\n//g;
        $text =~ s/^\s*//g;
        $text =~ s/\s*$//g;
	$text =~ s///g;
        if ($state->{colnum} >= 0) {
            $state->{table}[$state->{rownum}][$state->{colnum}] .= $text;
        }
    }
}


format STDOUT_TOP =
                                  +-- Start --+  +--- Max ---+  +--- End ---+
Loc        Sat.       Date   Mag  Time  Alt Az   Time  Alt Az   Time  Alt Az
---------- ---------- ------ ---- ----- --- ---  ----- --- ---  ----- --- ---
.


format STDOUT = 
@<<<<<<<<< @<<<<<<<<< @<<<<< @>>> @<<<< @<< @<<  @<<<< @<< @<<  @<<<< @<< @<<
$location,@{$row}
.

