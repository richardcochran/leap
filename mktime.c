#include <inttypes.h>
#include <time.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define NTP_UTC_OFFSET	((uint64_t)2208988800)
#define EPOCH		((uint64_t)3692217600)

int main(int argc, char *argv[])
{
	struct tm tm;
	time_t t1, t2, t3, t4;

	/* Ensure that mktime(3) returns a value in the UTC time scale. */
	setenv("TZ", "UTC", 1);

	tm.tm_hour = 23;
	tm.tm_min  = 59;
	tm.tm_sec  = 58;
	tm.tm_mday = 31;
	tm.tm_mon  = 11;
	tm.tm_year = 2016 - 1900;

	t1 = mktime(&tm);

	tm.tm_sec = 59;
	t2 = mktime(&tm);

	tm.tm_sec = 60;
	t3 = mktime(&tm);

	tm.tm_hour = 0;
	tm.tm_min  = 0;
	tm.tm_sec  = 0;
	tm.tm_mday = 1;
	tm.tm_mon  = 0;
	tm.tm_year = 2017 - 1900;

	t4 = mktime(&tm);

	printf("epoch %" PRId64 "\n"
	       "t1    %lu\n"
	       "t2    %lu\n"
	       "t3    %lu\n"
	       "t4    %lu\n"
	       ,
	       EPOCH - NTP_UTC_OFFSET, t1, t2, t3, t4);

	return 0;
}
