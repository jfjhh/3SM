#include <stdio.h>

int main(int argc, const char *argv[])
{
	/* TODO: Parse argv options to suspend or hibernate, and at X% battery. */
	
	/* For now, just suspend. */
	FILE *fd = fopen("w", "/sys/class/SUSPEND_FILE HERE (TODO)");
	return 0;
}
