if (@ARGV < 1) {
    usage();
    exit(); 
}

my @version = split(/\./, $ARGV[0]); 

if (!$ARGV[1]) {
    $ARGV[1] = 1;
}

mkdir "languages";
chdir "languages";
mkdir "unsigned";
open(CSV,"../languages.csv") or die "Cannot open languages.csv for reading\n"; 

while (<CSV>) { 
    chomp;
    my @list = split (/,/); 
    print "$list[0], $list[1], $list[2]";

    open(TEMPLATE,"../languages.pkg") or die "Cannot open languages.pkg for reading\n"; 
    open(NEW,">unsigned/mobbler_$version[0]_$version[1]_$version[2]_$list[2]_$ARGV[1].pkg") or die "Cannot open mobbler_$version[0]_$version[1]_$version[2]_$list[2]_$ARGV[1].pkg for writing\n";

    while (<TEMPLATE>) { 
        print $_;
        s/MAJORBUILDVERSION/$version[0]/g;
        s/MINORBUILDVERSION/$version[1]/g;
        s/BUILDVERSION/$version[2]/g;
        s/LANGUAGEVERSION/$ARGV[1]/g;
        s/LANGUAGENUMBER/$list[0]/g;
        s/LANGUAGECODE/$list[1]/g;
        s/LANGUAGENAME/$list[2]/g;
		foreach $key (sort keys(%ENV))
		{
			#print "\n$key = $ENV{$key}";
			s/\$\($key\)/$ENV{$key}/gm;
		} 
        print $_;
        print NEW $_;
    }
}


sub usage
{
    print "Usage: perl languages.pl mobblerversionnumber [languageversionnumber=1]\n";
    print "For example: perl languages.pl 0.3.2\n";
    print "For example: perl languages.pl 0.3.2 2\n";
}
