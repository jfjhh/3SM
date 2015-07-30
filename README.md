# 3SM: Simple System Suspend Manager.

![Build Status](https://travis-ci.org/jfjhh/3SM.svg?branch=master)

Used to change system state (often to suspend or hibernate) on low battery at a
specified percentage.

On my system, this program runs via an entry in root's
[`crontab(5)`](http://linux.die.net/man/5/crontab) file:

    @reboot /usr/bin/screen -dmS 3SM /usr/local/bin/3sm d 15

## Installation

You should be able to do a standard install with `./configure && make && make
install`, but only after `configure`, `Makefile`, and others are created. You
will first need to have the Autotools installed, and run `autoreconf -i`.

## Notes

Requires root privileges to run.
Either run `3sm <args>` as root or do `sudo 3sm <args>`.

Available kernel sleep states are:
 *	'f' => 'freeze'.
 *	's' => 'standby'.
 *	'm' => 'mem'.
 *	'd' => 'disk'.

However, some of these may not exist on your system. Find out what is available
by issuing `cat /sys/power/state`. `3sm` will not allow an attempt to execute a
state that does not exist.

States are substituted with their
[`pm-action (8)`](http://linux.die.net/man/8/pm-action)
counterparts if they exist, instead of just the raw state change.

Documentation on these modes can be found
[here](https://www.kernel.org/doc/Documentation/power/states.txt).

## Examples

	# to suspend to disk on 10% battery.
	$ 3sm d 10

	# to freeze at 40%, and run as a daemon.
	$ 3sm f 40 &> /dev/null &

	# suspend to ram at `pi`%.
	$ 3sm m 3.14

