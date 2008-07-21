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
 
/* request.c
 * Neko FastCGI request handling */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/wait.h>

#define NO_FCGI_DEFINES
#include <fcgi_stdio.h>

#include <neko.h>
#include <neko_mod.h>
#include <neko_vm.h>

#include "request.h"
#include "fcgi_reader.h"
#include "utils.h"

static value init_path_list () {
	void *allocated = NULL;
	char *path = "/usr/local/lib/neko"; // getenv("NEKOPATH");
	if (path != NULL) path = allocated = strdup(path);
	
	value module_path_list = val_null;
	value *next_path = &module_path_list;
	
	while (path != NULL && *path != '\0') {
		if (*path == ':') {
			++path;
		} else {
			char *path_end;
			if (path[1] != '\0') path_end = strchr(path+2, ':');
			else path_end = path + 1;
			char *path_end2 = strchr(path, ';');
			
			if (path_end == NULL || path_end2 < path_end) path_end = path_end2;
			if (path_end != NULL) *path_end = '\0';
			
			value tmp = alloc_array(2);
			*next_path = tmp;
			next_path = val_array_ptr(tmp)+1;
			*next_path = val_null;
			
			if (path[strlen(path)-1] != '/' && path[strlen(path)-1] != '\\') {
				buffer path_buffer = alloc_buffer(path);
				buffer_append(path_buffer, "/");
				val_array_ptr(tmp)[0] = buffer_to_string(path_buffer);
			} else {
				val_array_ptr(tmp)[0] = alloc_string(path);
			}
			
			if (path_end == NULL) path = NULL;
			else path = path_end + 1;
		}
	}
	free(allocated);
	
	return module_path_list;
}

static int open_module (const char *module_name, reader *reader, readp *readp) 
{
	FCGI_FILE *module_file;
	
	module_file = FCGI_fopen(module_name, "rb");
	if (module_file == NULL) return 0;
	
	*reader = fcgi_reader;
	*readp = module_file;
	return 1;
}

#define close_module(f) FCGI_fclose(f)

FCGX_Request request;

static value service_request (value module_path_list, value module_map) {
	val_check(module_path_list, array);
	val_check(module_map, object);
	
	char *module_name = FCGX_GetParam("PATH_TRANSLATED", request.envp);
	if (module_name == NULL) val_throw(alloc_string(
		"No Neko module specified"
	));
	
	print_log("Request for %s", module_name);
	
	field module_id = val_id(module_name);
	value module_value = val_field(module_map, module_id);
	
	neko_module *module;
	if (val_is_kind(module_value, neko_kind_module)) {
		module = val_data(module_value);
	} else {
		reader reader;
		readp readp;
		if (!open_module(module_name, &reader, &readp)) {
			buffer error_buffer = alloc_buffer("Couldn't open module: ");
			buffer_append(error_buffer, module_name);
			val_throw(buffer_to_string(error_buffer));
		}
		
		module = neko_read_module(reader, readp, neko_default_loader(NULL, 0));
		close_module(readp);
		
		if (module == NULL) {
			buffer error_buffer = alloc_buffer("Couldn't read module: ");
			buffer_append(error_buffer, module_name);
			val_throw(buffer_to_string(error_buffer));
		}
		
		module->name = alloc_string(module_name);
		module_value = alloc_abstract(neko_kind_module, module);
		alloc_field(module_map, module_id, module_value);
	}
	
	return neko_vm_execute(neko_vm_current(), module);
}

static void fastcgi_printer( const char *s, int len, void *out ) {
        while( len > 0 ) {
                int p = (int)FCGX_PutStr(s,len,request.out);
                if( p <= 0 ) {
                        FCGX_PutS("[ABORTED]",request.out);
                        break;
                }
                len -= p;
															                s += p;
															        }
        FCGX_FFlush(request.out);
}

static void PrintEnv(FCGX_Stream *out, char *label, char **envp)
{
    FCGX_FPrintF(out, "%s:<br>\n<pre>\n", label);
    for( ; *envp != NULL; envp++) {
        FCGX_FPrintF(out, "%s\n", *envp);
    }
    FCGX_FPrintF(out, "</pre><p>\n");
}

char ** CloneEnv(char **envp)
{
	char **newbuf;
	for(; *envp != NULL; envp++) {
		*newbuf++ = strdup(*envp);
	}
	return newbuf;
}

char * GetPostData() 
{
	char ibuffer[4096] = "";
	
	buffer result = alloc_buffer(ibuffer);

	while(FCGX_GetStr(ibuffer, 4096, request.in))
		buffer_append(result, ibuffer);	

	return val_string(buffer_to_string(result));
}

struct sigaction act, old_term, old_quit, old_int;

/**
 * Process group
 */
static pid_t pgroup;

void fastcgi_cleanup(int signal)
{
    print_log("Shutting down, pid %d", getpid());

    sigaction(SIGTERM, &old_term, 0);

    unlink(NEKO_FASTCGI_PID);

    /* Kill all the processes in our process group */
    kill(-pgroup, SIGTERM);
    exit(0);
}

void request_loop (neko_vm *vm) {
	value service_request_value = alloc_function(
		service_request, 2, "service_request"
	);
	
	value module_path_list = init_path_list();
	value module_map = alloc_object(NULL);
	value service_request_args[2] = { module_path_list, module_map };

	FCGX_Init();
	int sock = FCGX_OpenSocket(":9333", 5);
	FCGX_InitRequest(&request, sock, 0);
	neko_vm_redirect(vm, fastcgi_printer, 0);

	/* Create a process group for ourself & children */
    setsid();
	pgroup = getpgrp();

	/* Set up handler to kill children upon exit */
    act.sa_flags = 0;
	act.sa_handler = fastcgi_cleanup;
	if (sigaction(SIGTERM, &act, &old_term) ||
		sigaction(SIGINT,  &act, &old_int) ||
		sigaction(SIGQUIT, &act, &old_quit)) {
			print_log("Can't set signals");
			exit(1);
	}

	pid_t pid;

	FILE *fp = fopen(NEKO_FASTCGI_PID, "w");
	char tmpbuff[33];
	sprintf(tmpbuff, "%d", getpid());
	fputs(tmpbuff, fp);
	fclose(fp);

	print_log("Entering FCGX_Accept_r() loop.");
	while (FCGX_Accept_r(&request) >= 0) {
		int is_child = 0;
		pid = fork();

		switch(pid) {
			case 0: 
				/* we are the child, don't catch our signals */
				sigaction(SIGTERM, &old_term, 0);
				sigaction(SIGQUIT, &old_quit, 0);
				sigaction(SIGINT,  &old_int,  0);
				is_child = 1;
				break;
			case -1:
				exit(1);
				break;
		}

		if(is_child) {
			value exception = val_null;
			val_callEx(
				val_null, service_request_value, service_request_args,
				2, &exception
			);
		
			if (val_is_null(exception)) {
				FCGX_SetExitStatus(0, request.in);
			} else {
				// TODO something more appropriate
				buffer error_buffer = alloc_buffer("Uncaught exception: ");
				val_buffer(error_buffer, exception);
				FCGX_FPrintF(request.out, "Content-Type: text/html\r\nStatus: 200 OK\r\n\r\n%s\n", val_string(buffer_to_string(error_buffer)));
				FCGX_SetExitStatus(1, request.in);
			}

		//	PrintEnv(request.out, "Request environment", request.envp);
		//	FCGX_FPrintF(request.out, "\n\n\n<pre>%s:\n%s</pre>", "Post Body", GetPostData());
		
			FCGX_Finish_r(&request);
			exit(0);
		} else {
			// Reap the children
			int status = 0;
			while (wait(&status) < 0) {}
		}
	}
}
