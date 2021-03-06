
#include <stdio.h>
#include <stdbool.h>

#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

#include "wrapper.h"
#include "misc.h"
#include "logger.h"

extern bool brexit;

void handler(int signal)
{
	switch (signal) {
	case SIGINT:
	case SIGTERM:
	case _SC_PIPE:
		brexit = true;
		break;
	}
}

int init_signalhandler()
{
	size_t i;
	struct sigaction action;
	int signals[] = { SIGTERM, SIGINT, SIGPIPE };

	action.sa_handler = handler;
	action.sa_flags = 0;
	sigemptyset(&action.sa_mask);

	for (i = 0; i < sizeof signals; i++) {
		int rv;

		rv = sigaction(signals[i], &action, NULL);
		if (rv) {
			ERR(rv)
			return -1;
		}
	}
	return 0;
}

int forkoff()
{
	pid_t pid;

	// fork so that the child is not the process group leader
	pid = fork();
	if (pid > 0) {
		_exit(0);

	} else if (pid < 0) {
		_exit(1);
	}

	// become a process group and session leader
	if (setsid() == -1) {
		_exit(1);
	}

	// fork so the child cant regain control of the controlling terminal
	pid = fork();
	if (pid > 0) {
		_exit(0);

	} else if (pid < 0) {
		_exit(1);
	}

	// change base dir
	if (chdir("/")) {
		return -1;
	}

	// reset file mode creation mask
	umask(0);

	if (close(STDIN_FILENO) || close(STDOUT_FILENO) ||
	    close(STDERR_FILENO)) {
		return -2;
	}
	return 0;
}

int closepipe(int pipe[2])
{
	int rv;

	rv = close(pipe[0]);
	if (rv && errno != EBADF) {
		ERR(rv);
		return -1;
	}

	rv = close(pipe[1]);
	if (rv && errno != EBADF) {
		ERR(rv);
		return -2;
	}
	return 0;
}
