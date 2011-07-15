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
my $OS_CHRT = '/usr/bin/chrt';

my $opt_benchmark       = qr/.*/;
my $opt_repeats         = 3;
my $opt_iterations      = 1;
my $opt_build_directory = '.';
my $opt_check           = 0;
my $opt_output          = '/dev/null';
my $opt_verbose         = 0;
my $opt_discard = qr/cat-yegorushkin-const-string/ ; # Is quadratic in this context, just as PyStringObject::Concat, skip it.
my $opt_scheduling = '';
my $opt_valgrind = 0;

my $opt_time_format = # should eval() to a Perl hash when passed through time(1) --format
  q['{ user => %U, real => %e, system => %S, cpu => "%P", text => %X, data => %D, "max-memory" => %M, "average-memory" => %K,] .
  q[ input => %I, output => %O, major => %F, minor => %R, swaps => %W, pagesize => %Z, "unvolontary-context-switches" => %c, ] .
  q[ "volontary-context-switches" => %w, signals => %k, "exit-status" => %x, "average-stack-size" => %p }'];

GetOptions(
            'benchmark=s'       => \$opt_benchmark,
            'repeats=i'         => \$opt_repeats,
            'iterations=i'      => \$opt_iterations,
            'path=s'            => \$opt_build_directory,
            'checksum!'         => \$opt_check,
            'output=s'          => \$opt_output,
            'discard=s'         => \$opt_discard,
            'verbose!'          => \$opt_verbose,
            'fifo-scheduling=i' => \$opt_scheduling,
            'grind!'            => \$opt_valgrind,
            'time=s'            => \$OS_TIME,
            'hash=s'            => \$OS_HASH,
            'chrt=s'            => \$OS_CHRT
          ) or die "Bad arguments.\n";

sub say
{
    print STDERR "[INFO] @_\n" if $opt_verbose;
}

sub main
{
    $opt_repeats = 1 if $opt_repeats <= 0;
    $opt_iterations = 1 if $opt_iterations <= 0;
    $opt_scheduling = "$OS_CHRT --fifo $opt_scheduling " if length $opt_scheduling;

    my %programs;
    my $count = 0;

    for my $benchmark ( @benchmarks )
    {
        $count +=
          @{ $programs{ $benchmark } =
              [ grep { -x and /$opt_benchmark/o } <$opt_build_directory/$benchmark*> ] };
    }

    die
      "No program(s) found: run (c)make first or specify a build directory with -p build-path\n"
      unless $count;

    die
      "No input file(s): expected some input file names as arguments to feed the benchmarks\n"
      unless map { die  "[ERROR] No such file: $_\n" unless -f $_; } @ARGV;

    say "Benchmarking $count programs against", scalar @ARGV, "input (${opt_repeats}x$opt_iterations times each):";

    run( \%programs );
}

sub run
{
    my ( $programs ) = @_;

    my %scores;

    while ( my ( $benchmark, $programs ) = each %$programs )
    {
        for my $program ( @$programs )
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
            die "[ERROR] Failed benchmark $benchmark with $program against $input\n";
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
      "$opt_scheduling$OS_TIME --format=$opt_time_format --output=$time_report $program $input $opt_iterations";

    my @results;
    eval {
        for ( 1 .. $opt_repeats )
        {
            my $status = system( "$command > $opt_output" );
            die "$OS_TIME returned $status.\n"
              if $status;
            my $result = do $time_report
              or die
              "unsupported $OS_TIME format in $time_report, use --time=/path/to/GNU/time.\n";
            $status = $result->{ 'exit-status' };
            die "$program returned $status, fix the code.\n" if $status;
            push @results, $result;
        }
    };

    unlink( $time_report );

    if ( $@ )
    {
        my $message = "[ERROR] $program failed against $input, reason: $@\n";

        $message =~ s/$/# Command was:\n\t\$ $command/ if $opt_verbose;

        die $message;
    }
    else
    {
        my $time = take_best( @results );
        $time->{ 'input-data' } = $input;
        $time->{ 'code-size' }  = -s $program;
        if ( $opt_check )
        {
            my $checksum = qx[$program $input $opt_iterations | $OS_HASH];
            $checksum =~ s/ .*//s;
            $time->{ 'md5' } = $checksum;
        }
        if ( $opt_valgrind )
        {
            $time->{ 'valgrind' } = valgrind("$program $input $opt_iterations");
        }
        return $time;
    }
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

sub valgrind
{
    my ( $command ) = @_;

    my $report = qx[valgrind $command 2>&1];

    return
      if $?
          or not my @valgrind = map { s/,//g; int($_) }
          ( $report =~ /total heap usage: *(.*?) *allocs, *(.*?) *frees, *(.*?) *bytes/g );

    return \@valgrind;
}

main();
