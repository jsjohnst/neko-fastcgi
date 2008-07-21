/* neko-fastcgi: FastCGI wrapper for Neko
 * Copyright Jeremy Johnstone 2008
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA */
 
/* utils.c
 * Neko FastCGI utility functions */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#define NO_FCGI_DEFINES
#include <fcgi_stdio.h>

#include "utils.h"

char * getlogtime() 
{
	char outstr[100];
	time_t t;
	struct tm *tmp;
	t = time(NULL);
	tmp = localtime(&t);
	strftime(outstr, sizeof(outstr), "%Y-%m-%d %H:%M:%S", tmp);
	return strdup(outstr);
}

void print_log(char *message, ...) 
{
	va_list ap;
	// Let's hope this is long enough
	int bufflen = strlen(message)*3;
	int newbufflen = 0;
	char *logline = (char *) malloc(bufflen);
	int fit = 1;

	va_start(ap, message);
	do {
		fit = 1;
		if((newbufflen = vsnprintf(logline, bufflen, message, ap)) > bufflen) {
			fit = 0;
			bufflen = newbufflen;
			realloc(logline, bufflen);
			fprintf(stdout, "NekoVM FastCGI [%s]: Missed buffer allocation and had to resize for log\n", getlogtime());
		} 
	} while(!fit);
	va_end(ap);
	
	fprintf(stdout, "NekoVM FastCGI [%s]: %s\n", getlogtime(), logline);
	free(logline);
}

