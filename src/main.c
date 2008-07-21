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
 
/* main.c
 * Neko FastCGI main entry point */

#include <neko.h>
#include <neko_vm.h>

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "request.h"
#include "utils.h"

int main (int argc, char **argv) {
	setsid();
	pid_t pid = fork();
	if(pid != 0) exit(0);

	print_log("Starting up, pid %d", getpid());
	neko_global_init(NULL);
	neko_vm *vm = neko_vm_alloc(NULL);
	neko_vm_select(vm);
	neko_vm_jit(vm, 1);
	request_loop(vm);
}
