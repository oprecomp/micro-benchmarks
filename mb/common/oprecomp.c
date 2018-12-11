// Copyright (c) 2017 OPRECOMP Project
// Fabian Schuiki <fschuiki@iis.ee.ethz.ch>
// Florian Scheidegger <eid@zurich.ibm.com>

#include "oprecomp.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>


enum state {
	UNINIT,
	STARTED,
	STOPPED,
};

struct session {
	/// The current state of the measurement.
	enum state state;
	/// The start and stop times of the kernel.
	struct timespec start_tp, stop_tp;
	/// The minimum amount of time (in seconds) the kernel must take. This is
	/// used in the oprecomp_iterate function to suggest multiple iterations to
	/// be taken.
	int duration;
	/// The number of iterations the kernel performed.
	int iterations;
	/// The socket connected to AMESTER, or 0 if unused.
	int fd;
};

static struct session session = {UNINIT};


/// Get the value of an environment variable, or a default value if it is not
/// defined.
static const char *
env_or_default (const char *name, const char *def) {
	const char *value = getenv(name);
	return value && strlen(value) > 0 ? value : def;
}


/// Calculate the difference of two timespecs, i.e. `a = a-b`.
static void
timespec_subtract (struct timespec *a, const struct timespec *b) {
	a->tv_sec -= b->tv_sec;
	if (a->tv_nsec < b->tv_nsec) {
		a->tv_sec -= 1;
		a->tv_nsec += 1000000000;
	}
	a->tv_nsec -= b->tv_nsec;
}


/// Send a buffer to the AMSTER server.
static void
send_command (const char *command) {
	int result = write(session.fd, command, strlen(command));
	if (result < 0) {
		perror("write");
		exit(1);
	}
}


/// Send a command to the AMESTER server and wait for acknowledgement.
static void
send_command_ack (const char *command) {
	int result;

	// Send the command.
	send_command(command);

	// Read at least 3 bytes of data.
	char buffer[1024];
	char *ptr = buffer;
	size_t len = sizeof(buffer)-1;
	do {
		result = read(session.fd, ptr, len);
		if (result < 0) {
			perror("read");
			exit(1);
		}

		assert(len >= (size_t)result);
		ptr += result;
		len -= result;
	} while (ptr < buffer + 3);
	*ptr = 0;

	// Check if we actually received an "ack".
	if (strncmp(buffer, "ack", 3) != 0) {
		fprintf(stderr, "amester: expected \"ack\", received \"%s\"\n", buffer);
		exit(1);
	}
}


/// Start the measurement of the hot part of a kernel.
void
oprecomp_start () {
	assert(session.state == UNINIT && "oprecomp_start called multiple times");
	session.state = STARTED;

	// Read the AMESTER_* environment variables to determine whether we should
	// try to do some power measurements or not.
	int enable = strcmp(env_or_default("AMESTER_ENABLE", "0"), "1") == 0;
	session.duration = atoi(env_or_default("AMESTER_DURATION", "0"));

	// Establish connection to AMESTER and request start of measurements.
	if (enable) {
		int result;

		// Create the socket.
		session.fd = socket(AF_INET, SOCK_STREAM, 0);
		if (session.fd < 0) {
			perror("socket");
			exit(1);
		}

		// Ensure we have a minimum-delay connection.
		int flag = 1;
		result = setsockopt(session.fd, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag));
		if (result < 0) {
			perror("setsockopt");
			exit(1);
		}

		// Resolve the host name.
		struct hostent *host = gethostbyname(env_or_default("AMESTER_HOST", "localhost"));
		if (host == NULL) {
			perror("gethostname");
			exit(1);
		}

		// Setup the socket address.
		struct sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		memcpy(&addr.sin_addr.s_addr, host->h_addr, host->h_length);
		addr.sin_port = htons(atoi(env_or_default("AMESTER_PORT", "6090")));

		// Connect to the host.
		result = connect(session.fd, (struct sockaddr*)&addr, sizeof(addr));
		if (result < 0) {
			perror("connect");
			exit(1);
		}

		// Send the configuration.
		char buffer[1024];
		snprintf(buffer, 1024, "config:%s\n", env_or_default("AMESTER_CONFIG", "BASIC_PWR:ACC"));
		send_command_ack(buffer);

		// Send the file base string.
		snprintf(buffer, 1024, "filebase:%s\n", env_or_default("AMESTER_FILE", "measurement"));
		send_command_ack(buffer);

		// Start the measurement.
		send_command_ack("start\n");
	}

	// Start time measurement.
	clock_gettime(CLOCK_MONOTONIC, &session.start_tp);
}


/// Stop the measurement of the hot part of a kernel.
void
oprecomp_stop () {
	assert(session.state == STARTED && "oprecomp_stop called without prior call to oprecomp_start");
	session.state = STOPPED;

	// Stop time measurement and calculate the elapsed time in nanoseconds.
	clock_gettime(CLOCK_MONOTONIC, &session.stop_tp);
	struct timespec tp = session.stop_tp;
	timespec_subtract(&tp, &session.start_tp);
	uint64_t elapsed = (uint64_t)tp.tv_sec * 1000000000 + tp.tv_nsec;

	// If AMESTER was enabled, request a stop.
	if (session.fd) {
		int result;
		send_command_ack("stop\n");
		send_command("close\n");

		// Close the socket.
		result = close(session.fd);
		if (result < 0) {
			perror("close");
			exit(1);
		}
		session.fd = 0;
	}

	// If no iteration was registered via oprecomp_iterate, assume there has
	// been only one.
	if (session.iterations == 0)
		session.iterations = 1;

	// Emit the timing results.
	fprintf(stdout, "# time_total: %f\n", elapsed*1e-9);
	fprintf(stdout, "# iterations: %d\n", session.iterations);
	fprintf(stdout, "# time_avg: %f\n", elapsed*1e-9 / session.iterations);
}


/// Check whether enough time has passed for the benchmark to be useful. Also
/// informs the library about how many iterations of the same kernel have been
/// performed. Call this as the condition of a do-while loop around the actual
/// code of your benchmark if you don't want to ensure proper runlength
/// yourself.
int
oprecomp_iterate () {
	assert(session.state == STARTED && "oprecomp_iterate called outside of start/stop pair");
	++session.iterations;

	// If no minimum duration was configured, return 0 as to indicate that no
	// further iteration is necessary.
	if (session.duration == 0)
		return 0;

	// Otherwise measure the time we've been running, and judge whether
	// additional iterations are required.
	struct timespec tp;
	clock_gettime(CLOCK_MONOTONIC, &tp);
	timespec_subtract(&tp, &session.start_tp);
	return tp.tv_sec < session.duration;
}
