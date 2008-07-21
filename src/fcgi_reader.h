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
 
/* fcgi_reader.h
 * Neko FastCGI reader for FCGI streams */

#ifndef FCGI_READER_H
#define FCGI_READER_H

#define NO_FCGI_DEFINES
#include <fcgi_stdio.h>
#include <neko_mod.h>


int fcgi_reader (readp readp, void *buffer, int size);

#endif
