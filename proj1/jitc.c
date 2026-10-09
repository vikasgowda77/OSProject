/**
 * Tony Givargis
 * Copyright (C), 2023-2026
 * University of California, Irvine
 *
 * CS 238P - Operating Systems
 * jitc.c
 */

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <dlfcn.h>
#include "system.h"
#include "jitc.h"

/**
 * Needs:
 *   fork()
 *   execv()
 *   waitpid()
 *   WIFEXITED()
 *   WEXITSTATUS()
 *   dlopen()
 *   dlclose()
 *   dlsym()
 */

/* research the above Needed API and design accordingly */
struct jitc {
	void *handle;
};

int jitc_compile(const char *input, const char *output)
{
	const char *compiler = "/usr/bin/gcc";
	pid_t pid, result;
	int status;
    
    char *args[7];
    
    args[0] = (char *)compiler;
    args[1] = "-shared";
    args[2] = "-fPIC";
    args[3] = "-o";
    args[4] = (char *)output;
    args[5] = (char *)input;
    args[6] = NULL;

	if (!safe_strlen(input) || !safe_strlen(output)) {
		TRACE("invalid argument");
		return -1;
	}

	pid = fork();
	if (pid < 0) {
		TRACE("fork()");
		return -1;
	}

	if (pid == 0) {
		execv(compiler, args);
		_exit(127);
	}

	do {
		result = waitpid(pid, &status, 0);
	} while ((result < 0) && (EINTR == errno));

	if ((result < 0) || !WIFEXITED(status) || WEXITSTATUS(status)) {
		TRACE("error compiling");
		return -1;
	}

	return 0;
}

struct jitc * jitc_open(const char *pathname)
{
	struct jitc *jitc;

	if (safe_strlen(pathname) == 0) {
		TRACE("invalid argument");
		return NULL;
	}

	if (!(jitc = malloc(sizeof (struct jitc)))) {
		TRACE("out of memory");
		return NULL;
	}

	memset(jitc, 0, sizeof (struct jitc));
	if (!(jitc->handle = dlopen(pathname, RTLD_NOW))) {
		TRACE("dlopen()");
		FREE(jitc);
		return NULL;
	}

	return jitc;
}

void jitc_close(struct jitc *jitc)
{
	if (jitc) {
		if (jitc->handle) {
			dlclose(jitc->handle);
			jitc->handle = NULL;
		}
		FREE(jitc);
	}
}

long jitc_lookup(struct jitc *jitc, const char *symbol)
{
	void *address;

	if (!jitc || !jitc->handle || !safe_strlen(symbol)) {
		TRACE("invalid argument");
		return 0;
	}

	address = dlsym(jitc->handle, symbol);
	if (!address) {
		TRACE("dlsym()");
		return 0;
	}

	return (long)address;
}
