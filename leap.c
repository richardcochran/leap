/*
 * leap.c
 *
 * Copyright (C) 2012 Richard Cochran <richardcochran@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/timex.h>
#include <time.h>
#include <unistd.h>

#include "lstab.h"

static void print_time_status(FILE *fp, int status)
{
	if (status & STA_PLL)
		fprintf(fp, "\tSTA_PLL\tenable PLL updates (rw)\n");
	if (status & STA_PPSFREQ)
		fprintf(fp, "\tSTA_PPSFREQ\tenable PPS freq discipline (rw)\n");
	if (status & STA_PPSTIME)
		fprintf(fp, "\tSTA_PPSTIME\tenable PPS time discipline (rw)\n");
	if (status & STA_FLL)
		fprintf(fp, "\tSTA_FLL\tselect frequency-lock mode (rw)\n");
	if (status & STA_INS)
		fprintf(fp, "\tSTA_INS\tinsert leap (rw)\n");
	if (status & STA_DEL)
		fprintf(fp, "\tSTA_DEL\tdelete leap (rw)\n");
	if (status & STA_UNSYNC)
		fprintf(fp, "\tSTA_UNSYNC\tclock unsynchronized (rw)\n");
	if (status & STA_FREQHOLD)
		fprintf(fp, "\tSTA_FREQHOLD\thold frequency (rw)\n");
	if (status & STA_PPSSIGNAL)
		fprintf(fp, "\tSTA_PPSSIGNAL\tPPS signal present (ro)\n");
	if (status & STA_PPSJITTER)
		fprintf(fp, "\tSTA_PPSJITTER\tPPS signal jitter exceeded (ro)\n");
	if (status & STA_PPSWANDER)
		fprintf(fp, "\tSTA_PPSWANDER\tPPS signal wander exceeded (ro)\n");
	if (status & STA_PPSERROR)
		fprintf(fp, "\tSTA_PPSERROR\tPPS signal calibration error (ro)\n");
	if (status & STA_CLOCKERR)
		fprintf(fp, "\tSTA_CLOCKERR\tclock hardware fault (ro)\n");
	if (status & STA_NANO)
		fprintf(fp, "\tSTA_NANO\tresolution (0 = us, 1 = ns) (ro)\n");
	if (status & STA_MODE)
		fprintf(fp, "\tSTA_MODE\tmode (0 = PLL, 1 = FLL) (ro)\n");
	if (status & STA_CLK)
		fprintf(fp, "\tSTA_CLK\tclock source (0 = A, 1 = B) (ro)\n");
}

static int clear_status = 1;
static int delay_usec = 1000000;
static int insert_leap_second = 1;
static int print_all = 0;
static int set_date = 1;
static int set_synchronized = 1;
static int set_tai_offset = 1;
static int verbose_status_bits = 0;

static int leap_test(int index)
{
	struct timex tx;
	struct timespec ts;
	struct leap_second *ls = lstab_leap_second(index);
	int tc;
	FILE *fp = stdout;

	if (clear_status) {
		memset(&tx, 0, sizeof(tx));
		tx.modes = ADJ_STATUS;
		tc = adjtimex(&tx);
		if (tc < 0) {
			perror("ADJ_STATUS");
			return -1;
		}
		sleep(1);
	}
	if (set_date) {
		ts.tv_sec = ls->epoch.utc - 5;
		ts.tv_nsec = 0;
		if (clock_settime(CLOCK_REALTIME, &ts)) {
			perror("clock_settime");
			return -1;
		}
	}
	if (set_synchronized) {
		memset(&tx, 0, sizeof(tx));
		tx.modes = ADJ_ESTERROR | ADJ_MAXERROR;
		tx.esterror = 10;
		tx.maxerror = 300000;
		tc = adjtimex(&tx);
		if (tc < 0) {
			perror("ADJ_xxxERROR");
			return -1;
		}
	}
	if (clear_status) {
		memset(&tx, 0, sizeof(tx));
		tx.modes = ADJ_STATUS;
		tc = adjtimex(&tx);
		if (tc < 0) {
			perror("ADJ_STATUS");
			return -1;
		}
	}
	if (set_tai_offset) {
		memset(&tx, 0, sizeof(tx));
		tx.modes = ADJ_TAI;
		tx.constant = ls->leap.offset;
		tc = adjtimex(&tx);
		if (tc < 0) {
			perror("ADJ_TAI");
			return -1;
		}
	}
	if (insert_leap_second) {
		memset(&tx, 0, sizeof(tx));
		tx.modes = ADJ_STATUS;
		tx.status = STA_INS;
		tc = adjtimex(&tx);
		if (tc < 0) {
			perror("STA_INS");
			return -1;
		}
	}

	ts.tv_sec = delay_usec / 1000000;
	ts.tv_nsec = (delay_usec % 1000000) * 1000;

	while (1) {
		memset(&tx, 0, sizeof(tx));
		tx.modes = ADJ_NANO;
		tc = adjtimex(&tx);
		if (tc < 0) {
			perror("adjtimex");
			return -1;
		}
		if (set_date && tx.time.tv_sec >= ls->epoch.utc + 5) {
			break;
		}
		if (print_all || abs(tx.time.tv_sec - ls->epoch.utc) < 2) {
			fprintf(fp, "%d %c%c %d %ld.%09ld\n",
				tc,
				tx.status & STA_INS ? 'I' : '-',
				tx.status & STA_DEL ? 'D' : '-',
				tx.tai,
				tx.time.tv_sec, tx.time.tv_usec);
			if (verbose_status_bits)
				print_time_status(fp, tx.status);
		}
		if (delay_usec)
			clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);
	}

	return 0;
}

static void usage(char *progname)
{
	fprintf(stderr,
		"\nusage: %s [options]\n\n"
		" -f [file] time table of leap seconds\n"
		" -h        prints this message and exits\n"
		" -i        leap second table index to use (default 1)\n"
		" -p [num]  print 'num' entries of the leap second table\n\n"
		"leap test options:\n\n"
		" -a        print all calls to adjtimex (not just epoch +- 1)\n"
		" -c        do NOT clear all the status bits\n"
		" -d        do NOT set the historic date and time.\n"
		" -l        do NOT schedule a leap second insertion.\n"
		" -s        do NOT pretend to be synchronized.\n"
		" -t        do NOT set the historic TAI offset.\n"
		" -u [num]  usec delay between calls to adjtimex (default 1 second)\n"
		" -v        print the status bits verbosely\n"
		"\n",
		progname);
}

int main(int argc, char *argv[])
{
	char *timetable = NULL, *progname;
	int c, index = 1, print_table = 0;

	/* Process the command line arguments. */
	progname = strrchr(argv[0], '/');
	progname = progname ? 1+progname : argv[0];
	while (EOF != (c = getopt(argc, argv, "fi:p:" "acdlstu:v" "h"))) {
		switch (c) {
		case 'f': timetable = optarg; break;
		case 'i': index = atoi(optarg); break;
		case 'p': print_table = atoi(optarg); break;
	/**/
		case 'a': print_all = 1; break;
		case 'c': clear_status = 0; break;
		case 'd': set_date = 0; break;
		case 'l': insert_leap_second = 0; break;
		case 's': set_synchronized = 0; break;
		case 't': set_tai_offset = 0; break;
		case 'u': delay_usec = atoi(optarg); break;
		case 'v': verbose_status_bits = 1; break;
	/**/
		case 'h': usage(progname); return 0;
		default:  usage(progname); return -1;
		}
	}

	lstab_init();

	if (timetable && lstab_read(timetable))
		return 1;

	if (print_table)
		lstab_print(stdout, print_table);

	leap_test(index);

	return 0;
}
