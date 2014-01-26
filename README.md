# nagios-plugins-linux-memory

A nagios plugin to check memory and swap usage on linux.

Usage

	check_memory [-w [@]start:end] [-c [@]start:end] -M [-b,-k,-m,-g]
	check_memory [-w [@]start:end] [-c [@]start:end] -S [-b,-k,-m,-g]
	check_memory --help
	check_memory --version

Where

* start <= end
* start and ":" is not required if start=0
* if range is of format "start:" and end is not specified, assume end is infinity
* to specify negative infinity, use "~"
* alert is raised if metric is outside start and end range (inclusive of endpoints)
* if range starts with "@", then alert if inside this range (inclusive of endpoints)

Examples

	# check memory usage
	check_memory -w 80 -c 90 -M -k
	# check swap usage
	check_memory -w 80 -c 90 -S -k


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

