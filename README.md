# nagios-plugins-linux-memory

This package contains two nagios plugins for respectivery checking memory and
swap usage on unix systems.

Usage

	check_memory [-C] [-b,-k,-m,-g] [-w PERC] [-c PERC]
	check_swap [-b,-k,-m,-g] [-w PERC] [-c PERC]
	
	check_memory --help
	check_swap --help

Where

* -C, --caches: count buffers and cached memory as free memory
* -b,-k,-m,-g: show output in bytes, KB (the default), MB, or GB
* -w, --warning PERCENT: warning threshold
* -c, --critical PERCENT: critical threshold

Examples

	check_memory -C -m -w 80% -c 90%
	OK: 79.22% (810964 kB) used | mem_total=999MB, mem_used=791MB, mem_free=207MB, mem_shared=0MB, mem_buffers=1MB, mem_cached=190MB, mem_pageins=33803MB, mem_pageouts=18608MB
	  # mem_total    : Total usable physical RAM
	  # mem_used     : Total amount of physical RAM used by the system
	  # mem_free     : Amount of RAM that is currently unused
	  # mem_shared   : (Linux) Now always zero; not calculated
	  # mem_buffers  : (Linux) Amount of physical RAM used for file buffers
	  # mem_cached   : In-memory cache for files read from the disk (the page cache)
	  # mem_pageins
	  # mem_pageouts : (Linux) The number of memory pages the system has written in and out to disk
	
	check_swap -w 40% -c 60% -m
	WARNING: 42.70% (895104 kB) used | swap_total=2096444kB, swap_used=895104kB, swap_free=1201340kB, swap_cached=117024kB, swap_pageins=1593302kB, swap_pageouts=1281649kB
	  # swap_total   : Total amount of swap space available
	  # swap_used    : Total amount of swap used by the system
	  # swap_free    : Amount of swap space that is currently unused
	  # swap_cached  : (Linux) The amount of swap used as cache memory
	  # swap_pageins 
	  # swap_pageouts: (Linux) The number of swap pages the system has brought in and out


## Source code

The source code can be also found at https://sites.google.com/site/davidemadrisan/opensource


## Installation

This package uses GNU autotools for configuration and installation.

If you have cloned the git repository then you will need to run
`autoreconf --install` to generate the required files.

Run `./configure --help` to see a list of available install options.
The plugin will be installed by default into `LIBEXECDIR`.

It is highly likely that you will want to customise this location to
suit your needs, i.e.:

        ./configure --libexecdir=/usr/lib/nagios/plugins

After `./configure` has completed successfully run `make install` and
you're done!


## Supported Platforms

This package is written in plain C, making as few assumptions as possible, and
sticking closely to ANSI C/POSIX.

This is a list of Linux distributions this nagios plugin is known to compile
and run on

* Linux with kernel 3.10 (openmamba milestone 2.90.0)
* Linux with kernel 2.6.32 (RHEL 6.X (Santiago))
* Linux with kernel 2.6.18 (RHEL 5.X (Tikanga))
* OpenBSD 5.0

Thanks to grex.org for providing a free OpenBSD shell account.


## Bugs

If you find a bug please create an issue in the project bug tracker at
https://github.com/madrisan/nagios-plugins-memory/issues

