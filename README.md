# 3SM: Simple System Suspend Manager.

Used to change system state (often suspend) on low battery at a specified
percentage.

I use `gcc -Wall -Werror -pedantic -std=c99 -O2 3sm.c -o 3sm` to compile.

## Notes

Requires root privileges to run.
Either run as root or prepend `./3sm ...` command with `sudo`.

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
	$ ./3sm d 10

	# to freeze at 40%, and run as a daemon.
	$ ./3sm f 40 &> /dev/null &

	# suspend to ram at `pi`%.
	$ ./3sm m 3.14

