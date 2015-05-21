#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif /* _BSD_SOURCE */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>

/* Time to wait between checking battery in microseconds. */
#define CHECK_INTERVAL_USEC	4000000 /* 4 seconds */

/* Prefix of pm-action executables, found via `sudo which pm-suspend'
 * and battery/system files and prefixes. */

#define BAT_PREFIX	"/sys/class/power_supply/"
#define CHARGE_FULL	BAT_PREFIX "BAT1/charge_full"
#define CHARGE_NOW	BAT_PREFIX "BAT1/charge_now"
#define ENERGY_FULL	BAT_PREFIX "BAT0/energy_full"
#define ENERGY_NOW	BAT_PREFIX "BAT0/energy_now"
#define VOLTAGE_NOW	BAT_PREFIX "BAT0/voltage_now"
#define PM_PREFIX	"/usr/sbin/"
#define STATE		"/sys/power/state"

/* Support flags for use with pm-utils. */
struct pm_support_t {
	unsigned int suspend        : 1;
	unsigned int hibernate      : 1;
	unsigned int suspend_hybrid : 1;
} pm_support;

/* Read an int from a file. */
int freadint(const char *path, int *read)
{
	FILE *fd;
	int status = 0;

	if (access(path, R_OK) == 0) {
		fd = fopen(path, "r");
		status = fscanf(fd, "%d", read);
		fclose(fd);
	}

	return status;
}

/* Get battery percentage. */
float getbattery(void)
{
	int charge_now, charge_full, energy_now, energy_full, voltage_now;

	if (freadint(ENERGY_NOW, &energy_now)) {

		if (!(freadint(ENERGY_FULL, &energy_full)
				&& freadint(VOLTAGE_NOW, &voltage_now)))
			return -1.0;

		return ((float)energy_now * 1000 / (float)voltage_now) * 100 /
			((float)energy_full * 1000 / (float)voltage_now);

	} else if (freadint(CHARGE_NOW, &charge_now)) {

		if (!freadint(CHARGE_FULL, &charge_full))
			return -1.0;

		return (((float) charge_now / (float) charge_full) * 100.0);
	} else {
		return -1.0;
	}
}

/* Check for pm-action support. */
int pm_supported(const char *pm_type)
{
	int status;
	pid_t child_pid;

	if ((child_pid = fork()) >= 0) { /* fork() successful. */
		if (child_pid == 0) { /* Child process. */
			/* If `pm-is-supported' doesn't exist, then a -1 will be returned,
			 * not the 0, that counts as support by pm-is-supported, and thus be
			 * counted as false (not supported). */
			return execl("pm-is-supported", "pm-is-supported", pm_type,
					(char *) NULL);
		} else { /* Parent process. */
			wait(&status);
			if (WIFEXITED(status) && (WEXITSTATUS(status) == 0))
				/* return 1; */
				exit(1);
			else
				/* return 0; */
				exit(0);
		}
	} else { /* fork() failed. */
		perror("pm_supported: check for pm support fork() failed");
		exit(5);
	}
}

/* Execute a pm-action. */
void pm_action(const char *pm_cmd)
{
	int status;
	pid_t child_pid;
	char pm_cmd_full[64];

	strncpy(pm_cmd_full,  PM_PREFIX,  64);
	strncat(pm_cmd_full,  pm_cmd,     64 - strlen(pm_cmd_full) - 1);

	fflush(NULL); /* Be sure everything is output before pm-action. */

	if ((child_pid = fork()) >= 0) { /* fork() successful. */
		if (child_pid == 0) { /* Child process. */
			execlp(pm_cmd_full, pm_cmd, (char *) NULL);
			fprintf(stderr, "child pm_action returned! oops!\n");
			exit(7);
		} else { /* Parent process. */
			wait(&status);
		}
	} else { /* fork() failed. */
		perror("pm_action fork failed");
		exit(6);
	}
}

int use_pm(char suspend_type)
{
	switch (suspend_type) {
		case 'm':
			if (pm_support.suspend)
				pm_action("pm-suspend");
			break;

		case 'd':
			if (pm_support.suspend_hybrid)
				pm_action("pm-suspend-hybrid");
			else if (pm_support.hibernate)
				pm_action("pm-hibernate");
			break;

		default:
			return 0; /* Did not use pm. */
			break;
	}

	return 1; /* Used pm. */
}

int main(int argc, const char *argv[])
{
	int num_states, usage, i, times_low, valid_state;
	long long uptime  = 0;
	FILE *fd          = NULL;
	float cur_percent = 100.0; /* Default value while reading first time. */
	float min_percent = 0.15;  /* Float between 0.0 and 100.0 */
	char suspend_type = 'd';   /* One of 'f', 's', 'm', or 'd'. */
	char state[16];
	char states[4][16];        /* There are 4 states, each less than 16 chars
								* long: freeze, standby, mem, and disk. */

	num_states = usage = i = times_low = valid_state = 0;
	memset(states, '\0', 64);

	/* Open `/sys/power/state' for reading available suspend states. */
	if (access(STATE, R_OK) == -1) {
		perror("main: Could not read from '" STATE "'");
		return 1;
	} else {
		fd = fopen(STATE, "r");
	}

	/* Get the states. */
	while ((i = fscanf(fd, "%s", states[num_states++])) != EOF)
		;
	num_states--;

	/* Check pm-is-supported for supported pm-suspend commands. */
	pm_support.suspend        = pm_supported("--suspend");
	pm_support.hibernate      = pm_supported("--hibernate");
	pm_support.suspend_hybrid = pm_supported("--suspend-hybrid");

	/* Validate arguments and permissions. */
	if (argc != 3) {
		/* Validate arguments. */
		fprintf(stderr, "Not 2 arguments (got %d).\n", argc - 1);
		usage = 1;
	} else if (getuid() < 0 || getuid() != geteuid()) {
		/* Check if being ran by root properly. */
		fprintf(stderr, "Program must be ran as root.\n");
		usage = 1;
	} else {
		/* Parse argv options to suspend, hibernate, etc.
		 * and at a % total battery. */
		sscanf(argv[1], "%c", &suspend_type);
		sscanf(argv[2], "%f", &min_percent);

		/* Validate suspend_type. */
		if (!(suspend_type == 'f'
					|| suspend_type == 's'
					|| suspend_type == 'm'
					|| suspend_type == 'd')) {
			fprintf(stderr, "`%c' is not a recognized state.\n", suspend_type);
			usage = 1;
		}

		/* Validate min_percent. */
		if (!(isdigit(argv[2][0]) || argv[2][0] == '.')
				|| min_percent <= 0.0 || min_percent >= 100.0) {
			fprintf(stderr, "Need a valid percentage.\n");
			usage = 1;
		}
	}

	/* Print usage. */
	if (usage) {
		fprintf(stderr,
				"Usage: 3sm <suspend_type> <min_percent>\n"
				"\tsuspend_types: ");
		for (i = 0; i < num_states; i++)
			fprintf(stderr, "%s%s", states[i],
					((i < num_states - 1) ? ", " : ".\n"));
		fprintf(stderr, "\tmin_percent: interval (0.0, 100.0)\n");

		return 0;
	}

	/* Init info. */
	fprintf(stderr,
			"RUNNING 3SM: Checks every %d seconds.\n\n"
			"Your system seems to be capable of %d states:\n",
			CHECK_INTERVAL_USEC / 1000000, num_states);
	for (i = 0; i < num_states; i++)
		fprintf(stderr, "\t%s\n", states[i]);

	if (pm_support.suspend || pm_support.hibernate || pm_support.suspend_hybrid) {
		fprintf(stderr, "\nAnd pm-actions:\n");

		if (pm_support.suspend)
			fprintf(stderr, "\tpm-suspend\n");
		if (pm_support.hibernate)
			fprintf(stderr, "\tpm-hibernate\n");
		if (pm_support.suspend_hybrid)
			fprintf(stderr, "\tpm-suspend-hybrid\n");
	}
	putchar('\n');

	/* Set state from arg char to valid state string. */
	switch (suspend_type) {
		case 'f':
			strncpy(state,  "freeze",   16);
			break;
		case 's':
			strncpy(state,  "standby",  16);
			break;
		case 'm':
			strncpy(state,  "mem",      16);
			break;
		case 'd':
			strncpy(state,  "disk",     16);
			break;
		default: /* Should never happen. */
			return 3;
			break;
	}

	/* Validate against possible states. */
	for (i = 0; i < num_states; i++)
		if (suspend_type == states[i][0])
			valid_state = 1;

	if (!valid_state) {
		fprintf(stderr,
				"The state %s is not supported on your system.\n", state);
		return 4;
	}

	if (access(STATE, W_OK) == -1) {
		perror("main: Cannot write to '" STATE "' (permissions?)");
		return 2;
	} else {
		fd = freopen(STATE, "w", fd);
	}

	/* Get battery percentage for the first run of loop. */
	cur_percent = getbattery();

	/* Start the monitoring and suspending loop. */
	for (uptime = 0LL; uptime != -1LL; uptime++) { /* forever */
		if (times_low < 3)
			printf(" [uptime: ~%llds] [bat: %f%%] : ",
					uptime * (CHECK_INTERVAL_USEC / 1000000), cur_percent);

		if ((cur_percent = getbattery()) <= min_percent) {
			if (times_low > 0) { /* Suspending is not working. */
				if (times_low == 3) {
					printf("Stopping output until reboot, "
							"charged battery, or termination.\n\n");
					times_low++;
				} else if (times_low < 3) {
					printf("Battery is below %f%%, but unable to "
							"suspend or already suspended.\n",
							min_percent);
					times_low++;
				}
			} else {
				printf("Battery is below %f%%, suspending ... ", min_percent);

				/* Suspend, and check if an error occured. */
				if (!use_pm(suspend_type)) { /* If no pm-action,
												use /sys/power/state method. */
					if (fprintf(fd, state) != strlen(state)) {
						fprintf(stderr, "\n\tferror(): %d\n", ferror(fd));
						perror("main: Failed to suspend");
					}
					fflush(fd);
				}

				printf("resumed.\n");
				times_low++;
				uptime = -1LL;
			}
		} else {
			printf("Battery is above %f%%.\r", min_percent);
			times_low = 0;
		}

		fflush(stdout);
		usleep(CHECK_INTERVAL_USEC);
	}

	return 0;
}

