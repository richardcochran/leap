/*
 * lstab.h
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
#ifndef HAVE_LEAP_SECONDS_H
#define HAVE_LEAP_SECONDS_H

#include <stdint.h>
#include <stdio.h>
#include <time.h>

struct leap_second {
	unsigned int offset;		/* TAI - UTC offset of epoch */
	struct {
		uint32_t ntp;		/* NTP time of epoch */
		time_t tai;		/* TAI time of epoch */
		time_t utc;		/* UTC time of epoch */
	} epoch;
	struct {
		time_t tai;		/* TAI time of leap second */
		unsigned int offset;	/* Offset in effect before new epoch */
	} leap;
};

void lstab_init(void);

struct leap_second *lstab_leap_second(unsigned int index);

void lstab_print(FILE *fp, int len);

int lstab_read(char *filename);

#endif
