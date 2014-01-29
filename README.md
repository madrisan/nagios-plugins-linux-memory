# nagios-plugins-linux-memory

A nagios plugin to check memory and swap usage on linux.

Usage

	check_memory --memory [-C] [-b,-k,-m,-g] [-w PERC] [-c PERC]
	check_memory --swap [-b,-k,-m,-g] [-w PERC] [-c PERC]
	check_memory --help
	check_memory --version

Where

* -M, --memory: show the memory usage
* -S, --swap: show the swap usagea
* -C, --caches: count buffers and cached memory as free memory
* -b,-k,-m,-g: show output in bytes, KB (the default), MB, or GB
* -w, --warning PERCENT: warning threshold
* -c, --critical PERCENT: critical threshold

Examples

	check_memory --memory -C -w 80% -c 90%
	check_memory --swap -w 40% -c 60% -m


## Source code

The source code can be also found at https://sites.google.com/site/davidemadrisan/opensource


## Installation

This package uses GNU autotools for configuration and installation.

If you have cloned the git repository then you will need to run
`autogen.sh` to generate the required files.

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

This is a list of platforms this nagios plugin is known to compile and run on

* Linux with kernel 3.10 and glibc 2.18 (openmamba milestone 2.90.0)


## Bugs

If you find a bug please create an issue in the project bug tracker at
https://github.com/madrisan/nagios-plugins-linux-memory/issues

