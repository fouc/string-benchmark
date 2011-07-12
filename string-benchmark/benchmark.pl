#!/usr/bin/env perl

use strict;
use warnings;

use Getopt::Long;
use File::Basename;
use Tie::IxHash;
use Data::Dumper;

my @benchmarks = qw(cat cmp slice);

my $OS_TIME = '/usr/bin/time';
my $OS_HASH = '/usr/bin/md5sum';

my $opt_benchmark       = qr/.*/;
my $opt_repeats         = 3;
my $opt_build_directory = '.';
my $opt_check           = 0;
my $opt_output          = '/dev/null';
my $opt_valgrind        = 0;
my $opt_verbose         = 0;
my $opt_discard         = qr/cat-yegorushkin-const-string/
  ; # Just like Python 2.7.1 C String API, boost::const_string is too slow in this context (it does one malloc per call), skip it.

my $opt_time_format =
  q{'{ user => %U, real => %e, system => %S, cpu => "%P", text => %X, data => %D, "max-memory" => %M, "average-memory" => %K, input => %I, output => %O, major => %F, minor => %R, swaps => %W, pagesize => %Z, "unvolontary-context-switches" => %c, "volontary-context-switches" => %w, signals => %k, "exit-status" => %x, "average-stack-size" => %p }'};

GetOptions(
            'benchmark=s' => \$opt_benchmark,
            'repeats=i'   => \$opt_repeats,
            'path=s'      => \$opt_build_directory,
            'check!'      => \$opt_check,
            'output=s'    => \$opt_output,
            'discard=s'   => \$opt_discard,
            'verbose!'    => \$opt_verbose,
            'time=s'      => \$OS_TIME,
            'hash=s'      => \$OS_HASH
          );

my $output = $opt_check ? "| $OS_HASH 1>&2" : "> $opt_output";
$opt_repeats = 1 if $opt_repeats <= 0;

sub say
{
    print STDERR "[INFO] @_\n" if $opt_verbose;
}

sub main
{
    my @programs = map {
        grep { -x }
          <$opt_build_directory/$_*>
    } grep { /$opt_benchmark/o } @benchmarks;

    die
      'No program(s) found: run (c)make first or specify a build directory with -p build-path'
      unless @programs;

    die
      'No input file(s): expected some input file names as arguments to feed the benchmarks'
      unless map { die "[ERROR] No such file: $_" unless -f $_ } @ARGV;

    say 'Benchmarking', scalar @programs, 'programs against', scalar @ARGV,
      "input ($opt_repeats times each):";

    run();
}

sub run
{
    my %scores;

    for my $benchmark ( grep { /$opt_benchmark/o } @benchmarks )
    {
        for my $program ( grep { -x } <$opt_build_directory/$benchmark*> )
        {
            my $result = score( $benchmark, $program );
            if ( $result )
            {
                push @{ $scores{ $benchmark } }, $result;
            }
        }
    }

    report( \%scores );
}

sub score
{
    my ( $benchmark, $program ) = @_;

    my $name = basename( $program );
    if ( $name =~ /$opt_discard/o )
    {
        say "Skipping $name benchmark, use --discard=None to include" and return;
    }

    tie my %score, 'Tie::IxHash',
      name   => $name,
      real   => 0,
      user   => 0,
      system => 0;

    for my $input ( @ARGV )
    {
        my $time = benckmark( $name, $program, $input );
        if ( defined $time )
        {
            $score{ real }   += $time->{ real };
            $score{ user }   += $time->{ user };
            $score{ system } += $time->{ system };
            push @{ $score{ details } }, $time;
        }
        else
        {
            die "[ERROR] Failed benchmark $benchmark with $program against $input";
        }
    }
    return \%score;
}

sub benckmark
{
    my ( $name, $program, $input ) = @_;

    say "Running $name against $input";

    my $time_report = "time.$name\_$input";
    $time_report =~ s/[.\/]/_/g;
    my $command =
      "$OS_TIME --format=$opt_time_format --output=$time_report $program < $input $output";

    my @results;
    for ( 1 .. $opt_repeats )
    {
        my $status = system( $command );
        die
          "[ERROR] $OS_TIME failed timing $program against $input (exit: $status) command was:\n    \$ $command\n#"
          if $status;
        my $result = do $time_report
          or die
          "[ERROR] unsupported $OS_TIME format in $time_report, use --time=/path/to/GNU/time";
        die "[ERROR] $program returned bad status against $input, fix the code"
          if $result->{ 'exit-status' };
        push @results, $result;
    }
    unlink( $time_report );

    my $time = take_best( @results );
    $time->{ 'input-data' } = $input;
    $time->{ 'code-size' }  = -s $program;
    return $time;
}

sub report
{
    my ( $scores ) = @_;

    while ( my ( $benchmark, $implementations ) = each %$scores )
    {
        $scores->{ $benchmark } = [ sort fastest @$implementations ];
    }

    print 'our ', Dumper $scores;
}

sub take_best
{
    my @times = sort fastest @_;
    return $times[ 0 ];
}

sub fastest
{
    return (
        $a->{ real } == $b->{ real }      ## focus on wall clock time
        ? $a->{ user } == $b->{ user }    ## tie on user time if equal
              ? $a->{ system } <=> $b->{ system }    ## tie on kernel time otherwise
              : $a->{ user } <=> $b->{ user }
        : $a->{ real } <=> $b->{ real }
    );
}

main();
