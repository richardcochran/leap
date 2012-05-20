/*
 * lstab.c
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

#include "lstab.h"

/*
 * We keep a history of the TAI - UTC offset in a lookup table.
 *
 * Each entry gives the NTP/TAI/UTC time when a new offset came into
 * effect. This is always the second immediately after a leap second.
 *
 * The size of the table is the number of entries from the NIST table,
 * plus room for two hundred more entries to be added at run time.
 * Since there can be at most two leap seconds per year, this allows
 * for at least one hundred years.
 *
 * The table data are available from
 *
 * ftp://time.nist.gov/pub/leap-seconds.list
 *
 * When updating this table, do not forget to set N_HISTORICAL_LEAPS.
 */

#define BASE_TAI_OFFSET		10
#define N_HISTORICAL_LEAPS	26
#define N_LEAPS			(N_HISTORICAL_LEAPS + 200)
#define NTP_UTC_OFFSET		2208988800UL
#define NTP_TO_UTC(x)		(x ## UL - 2208988800UL)

static struct leap_second lstab[N_LEAPS];
static int lstab_length;

static uint32_t offset_table[N_LEAPS * 2] = {
	2272060800UL,	10,	/* 1 Jan 1972 */
	2287785600UL,	11,	/* 1 Jul 1972 */
	2303683200UL,	12,	/* 1 Jan 1973 */
	2335219200UL,	13,	/* 1 Jan 1974 */
	2366755200UL,	14,	/* 1 Jan 1975 */
	2398291200UL,	15,	/* 1 Jan 1976 */
	2429913600UL,	16,	/* 1 Jan 1977 */
	2461449600UL,	17,	/* 1 Jan 1978 */
	2492985600UL,	18,	/* 1 Jan 1979 */
	2524521600UL,	19,	/* 1 Jan 1980 */
	2571782400UL,	20,	/* 1 Jul 1981 */
	2603318400UL,	21,	/* 1 Jul 1982 */
	2634854400UL,	22,	/* 1 Jul 1983 */
	2698012800UL,	23,	/* 1 Jul 1985 */
	2776982400UL,	24,	/* 1 Jan 1988 */
	2840140800UL,	25,	/* 1 Jan 1990 */
	2871676800UL,	26,	/* 1 Jan 1991 */
	2918937600UL,	27,	/* 1 Jul 1992 */
	2950473600UL,	28,	/* 1 Jul 1993 */
	2982009600UL,	29,	/* 1 Jul 1994 */
	3029443200UL,	30,	/* 1 Jan 1996 */
	3076704000UL,	31,	/* 1 Jul 1997 */
	3124137600UL,	32,	/* 1 Jan 1999 */
	3345062400UL,	33,	/* 1 Jan 2006 */
	3439756800UL,	34,	/* 1 Jan 2009 */
	3550089600UL,	35,	/* 1 Jul 2012 */
};

static void leap_second_init(struct leap_second *ls, uint32_t val,
			     unsigned int offset, unsigned int prev_offset)
{
	/* the new epoch */

	ls->epoch.ntp = val;
	ls->epoch.utc = val - NTP_UTC_OFFSET;
	ls->epoch.tai = val - NTP_UTC_OFFSET + offset;
	ls->offset = offset;

	/* the leap second before */
	
	ls->leap.tai = val - NTP_UTC_OFFSET + offset - 1;
	ls->leap.offset = prev_offset;
}

void lstab_init(void)
{
	int i;
	struct leap_second *ls;
	uint32_t val, offset, prev = 0;

	for (i = 0; i < N_HISTORICAL_LEAPS; i++) {
		ls = lstab + i;
		val = offset_table[2*i];
		offset = offset_table[2*i+1];
		leap_second_init(ls, val, offset, prev);
		prev = offset;
	}
	lstab_length = i;
}

struct leap_second *lstab_leap_second(unsigned int index)
{
	if (index < lstab_length)
		return lstab + index;
	return NULL;
}

void lstab_print(FILE *fp, int len)
{
	int i;
	if (len > lstab_length) {
		len = lstab_length;
	}
	fprintf(fp, "NTP\t\tOFF\tTAI\t\tUTC\n");
	for (i = 0; i < len; i++) {
		fprintf(fp, "%u\t%u\t%-10ld\t%-10ld\t%2d\n",
		       lstab[i].epoch.ntp, lstab[i].offset,
		       lstab[i].epoch.tai, lstab[i].epoch.utc, i);
	}
}

int lstab_read(char *name)
{
	FILE *fp;
	struct leap_second *ls;
	uint32_t val, offset, prev = 0;
	int index = 0;
	char buf[1024];

	fp = fopen(name, "r");
	if (!fp)
		return 1;

	while (1) {
		if (!fgets(buf, sizeof(buf), fp))
			break;
		if (2 == sscanf(buf, "%u %u", &val, &offset)) {
			ls = lstab + index;
			leap_second_init(ls, val, offset, prev);
			prev = offset;
			index++;
		}
	}
	lstab_length = index;

	return 0;
}
