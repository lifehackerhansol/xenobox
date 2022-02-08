#!/usr/bin/perl --
use strict;
use warnings;
use IO::String;
use LWP::UserAgent;
use Archive::Zip;
use XML::Simple;
use FileHandle;

my $gamelist_name=$ARGV[0]?$ARGV[0]:"GameList.txt";
print STDERR "Phase1: Read $gamelist_name\n";
my $gamelist;
open(my $fh,"<",$gamelist_name);
read($fh, $gamelist, (-s $gamelist_name));
close($fh);

print STDERR "Phase2: Download ADVANsCEne_NDScrc.zip\n";
my $datzip="";
my $res = LWP::UserAgent->new->get(
	"http://www.advanscene.com/offline/datas/ADVANsCEne_NDScrc.zip",
	':content_cb' => sub {
		my($chunk, $res, $proto) = @_;
		$datzip.=$chunk;
		my $size = length($datzip);
		if (my $total = $res->header('Content-Length')) {
			printf STDERR "%d / %d (%.1f%%)\r", $size, $total, $size/$total * 100;
		}else{
			printf STDERR "%d / Unknown bytes\r", $size;
		}
	}
);

print STDERR "Phase3: Decompress ADVANsCEne_NDScrc.zip\n";
my $zip = Archive::Zip->new();
my $status = $zip->readFromFileHandle(IO::String->new($datzip));
my $dat = $zip->contents("ADVANsCEne_NDScrc.xml");

print STDERR "Phase4: Parse ADVANsCEne_NDScrc.xml\n";
my $response = XML::Simple->new->XMLin($dat);

print STDERR "Phase5: Modify $gamelist_name\n";
foreach my $game(@{$response->{"games"}->{"game"}}){
	my $gamecrc=$game->{"files"}->{"romCRC"}->{"content"};
	my $gameid=$game->{"serial"};
	$gameid=~/^.+-(.+)-.+$/; $gameid=$1;
	$gamelist=~s/\[$gamecrc\]/\[$gamecrc\]\{$gameid\}/;
}

open($fh,">",$gamelist_name);
print $fh $gamelist;
close($fh);
print STDERR "Done.\n";
