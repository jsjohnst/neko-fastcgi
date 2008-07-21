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
 
/* fcgi_reader.c
 * Neko FastCGI reader for FCGI streams */

#include "fcgi_reader.h"


int fcgi_reader (readp readp, void *buffer, int size) {
	int length = 0;
	
	while (size > 0) {
		int bytes_read = (int)(FCGI_fread(
			buffer, 1, size, (FCGI_FILE *)(readp)
		));
		
		if (bytes_read <= 0) {
			return length;
		}
		
		size -= bytes_read;
		length += bytes_read;
		buffer = (char *)(buffer + bytes_read);
	}
	
	return length;
}
