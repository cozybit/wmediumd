/*
 *	wmediumd, wireless medium simulator for mac80211_hwsim kernel module
 *	Copyright (c) 2011 cozybit Inc.
 *
 *	Author: Javier Lopez	<jlopex@cozybit.com>
 *		Javier Cardona	<javier@cozybit.com>
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version 2
 *	of the License, or (at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 *	02110-1301, USA.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h> 

int msnprintf (char **retval, int max_size, const char *format, ...)
{
	va_list val;

	va_start (val, format);
	int siz = vsnprintf (NULL, 0, format, val);
	va_end (val);

	if (siz >= 0)
	{
        	char *buffer;

		if (max_size && max_size < siz + 1)
		{
			buffer = (char *) malloc (max_size);
			siz = max_size;
		}
		else
		{
			// Allocate space for size + terminating null
			buffer = (char *) malloc (++siz);
		}

		if (!buffer)
			return -1;

		va_start (val, format);
		siz = vsnprintf (buffer, siz, format, val);
		va_end (val);

		if (siz < 0)
		{
			free (buffer);
			return siz;
		}

		*retval = buffer;
	}
	return siz;
}

