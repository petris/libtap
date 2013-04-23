/*-
 * Copyright (c) 2004 Nik Clayton
 *           (c) 2010 Shlomi Fish
 *           (c) 2013 Petr Malat
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <errno.h>


#ifdef HAVE_LIBPTHREAD
#ifdef __linux__
#define __USE_GNU
#include <pthread.h>
#ifndef PTHREAD_MUTEX_RECURSIVE
#define PTHREAD_MUTEX_RECURSIVE PTHREAD_MUTEX_RECURSIVE_NP
#endif
#else
#include <pthread.h>
#endif // __linux__
#define LOCK pthread_mutex_lock(&tap_shm->lock);
#define UNLOCK pthread_mutex_unlock(&tap_shm->lock);
#else // HAVE_LIBPTHREAD
#define LOCK
#define UNLOCK
#endif // HAVE_LIBPTHREAD

#define INIT do { if (initialized == 0) { tap_implicit_init(); } } while (0)

#include "tap.h"
#include "tap_main.h"
#include "tap_skip_todo.h"

/** True, if the library was already initialized */
static int initialized = 0;

static void tap_implicit_init(void)
{
#ifdef HAVE_LIBPTHREAD
	static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_lock(&lock);
#endif
	if (initialized == 0) {
		tap_init(tap_flags);
	}
#ifdef HAVE_LIBPTHREAD
	pthread_mutex_unlock(&lock);
#endif
}


struct tap_shm_s {
#ifdef HAVE_LIBPTHREAD
	pthread_mutex_t lock;
#endif
	int no_plan;
	int skip_all;
	int have_plan;
	unsigned int test_count; /* Number of tests that have been run */
	unsigned int e_tests;    /* Expected number of tests to run */
	unsigned int failures;   /* Number of tests that failed */
	int test_died;
	int main_pid;
};

struct tap_shm_s tap_shm_nofork = {
#ifdef HAVE_LIBPTHREAD
#ifdef __linux__
	.lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP,
#else
	.lock = PTHREAD_MUTEX_INITIALIZER,
#endif
#endif
};

static struct tap_shm_s *tap_shm = &tap_shm_nofork;

static void _expected_tests(unsigned int);
static void _cleanup(void);

extern unsigned long tap_flags;

/** Generate a test results
 * @param ok - true if the test passed
 * @param func - name of caller
 * @param file - file name of caller
 * @param line - line from which we ware called
 * @param test_name - format string generating the test name
 * @param ap - arguments for format
 *
 * @return 1 if the test passed
 */
static unsigned int _vgen_result(int ok, const char *condition, 
		const char *actual, const char *expected,
		const char *func, const char *file, unsigned int line, 
		const char *test_name, va_list ap)
{
	char *local_test_name = NULL;
	char *c;
	int name_is_digits;
	int old_errno = errno;
	int print_flags = 0;
	const char *todo;

	LOCK;

	if (tap_flags & TAP_FLAGS_TRACE) {
		printf("# Trace: %s %s:%d\n", func, file, line);
	}

	todo = tap_todo_msg();

	tap_shm->test_count++;
	if (!ok && !todo) {
		tap_shm->failures++;
	}

	/* Start by taking the test name and performing any printf()
	   expansions on it */
	if (test_name != NULL && *test_name < 10 && *test_name != 0) {
		print_flags = *(test_name++);
	}

	if (test_name != NULL && *test_name != 0) {
		if (vasprintf(&local_test_name, test_name, ap) < 0) {
			local_test_name = NULL;
		}
	} else if (condition != NULL && *condition != 0) {
		if (asprintf(&local_test_name, "%s", condition) < 0) {
			local_test_name = NULL;
		}
	} else {
		if (asprintf(&local_test_name, "%s:%d", func, line) < 0) {
			local_test_name = NULL;
		}
	}

	/* Make sure the test name contains more than digits
	   and spaces.  Emit an error message and exit if it
	   does */
	if (local_test_name) {
		name_is_digits = 1;
		for (c = local_test_name; *c != '\0'; c++) {
			if (!isdigit(*c) && !isspace(*c)) {
				name_is_digits = 0;
				break;
			}
		}

		if (name_is_digits) {
			diag("    You named your test '%s'.  You shouldn't use numbers for your test names.", local_test_name);
			diag("    Very confusing.");
		}
	}

	printf("%sok %d", ok ? "" : "not ", tap_shm->test_count);

	/* Print the test name, escaping any '#' characters it
	   might contain */
	if (local_test_name != NULL) {
		printf(" - ");
		for(c = local_test_name; *c != '\0'; c++) {
			if(*c == '#')
				fputc('\\', stdout);
			fputc((int)*c, stdout);
		}
	}

	if (!ok && (tap_flags & TAP_FLAGS_ERRNO)) {
		printf(" # ERRNO: %d '%s'", old_errno, strerror(old_errno));
	}

	if (todo) {
		printf(" # TODO %s", todo);
	}

	printf("\n");

	if (!ok) {
		if (getenv("HARNESS_ACTIVE") != NULL) {
			fputs("\n", stderr);
		}

		if (tap_flags & TAP_FLAGS_YAMLISH) {
			printf("  ---\n");
			if (test_name && test_name[0]) {
				printf("  name: %s\n", test_name);
			}
			if (condition) {
				printf("  message: Condition '%s' evaluated to false\n", condition);
			}
			printf("  file: %s\n", file);
			printf("  line: %d\n", line);
			printf("  severity: %s\n", todo ? "todo" : "fail");
			if (actual != NULL) { 
				printf("  actual: %s\n", actual);
			}
			if (expected != NULL) { 
				printf("  expected: %s\n", expected);
			}
			printf("  ...\n");
		} else {
			diag("    Failed %stest in %s at line %d", 
					todo ? "(TODO) " : "", file, line);
			if (test_name && condition) {
				diag("    Condition: %s", condition);
			}
		}

		if (print_flags == MP[0]) {
			BAIL_OUT_f(func, file, line, "It was mandatory for the last test to pass");
		}
	}
	free(local_test_name);

	UNLOCK;

	/* We only care (when testing) that ok is positive, but here we
	   specifically only want to return 1 or 0 */
	errno = old_errno;
	return ok ? 1 : 0;
}

/** Generate a test results
 * @param ok - true if the test passed
 * @param condition - evaluated condition
 * @param func - name of caller
 * @param file - file name of caller
 * @param line - line from which we ware called
 * @param test_name - format string generating the test name
 * @param ... - arguments for format
 *
 * @return 1 if the test passed
 */
unsigned int _gen_result(int ok, const char *condition, const char *func, 
		const char *file, unsigned int line, const char *test_name, 
		...) 
{
	unsigned int rtn;
	va_list ap;

	va_start(ap, test_name);
	rtn = _vgen_result(ok, condition, NULL, NULL, func, file, line, test_name, ap);
	va_end(ap);

	return rtn;
}

/** Initialise TAP library */
void tap_init_f(long flags, const char *func, const char *file, unsigned int line)
{
	if (initialized) {
		BAIL_OUT("Library is already initialized");
	}
	tap_flags = flags;
	initialized = 1;

	atexit(_cleanup);
	setbuf(stdout, 0);

	tap_skip_init();
	tap_todo_init();

	if (flags & TAP_FLAGS_FORK) {
		tap_shm = mmap(NULL, sizeof *tap_shm, 
				PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS,
				-1, 0);

		if (tap_shm == MAP_FAILED) {
			BAIL_OUT("Failed mapping shared memory");
		}

		tap_shm->main_pid = getpid();
	}

#ifdef HAVE_LIBPTHREAD
	{
		pthread_mutexattr_t attr;
		pthread_mutexattr_init(&attr);
		pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
		pthread_mutex_init(&tap_shm->lock, &attr);
		pthread_mutexattr_destroy(&attr);
	}
#endif
}

/*
 * Note that there's no plan.
 */
int plan_no_plan(void)
{
	INIT;
	LOCK;

	if(tap_shm->have_plan != 0) {
		fprintf(stderr, "You tried to plan twice!\n");
		tap_shm->test_died = 1;
		UNLOCK;
		exit(255);
	}

	tap_shm->have_plan = 1;
	tap_shm->no_plan = 1;

	UNLOCK;

	return 1;
}

/*
 * Note that the plan is to skip all tests
 */
int plan_skip_all(const char *reason)
{
	INIT;
	LOCK;

	tap_shm->skip_all = 1;

	printf("1..0");

	if(reason != NULL)
		printf(" # SKIP %s", reason);

	printf("\n");

	UNLOCK;

	exit(0);
}

/*
 * Note the number of tests that will be run.
 */
int plan_tests(unsigned int tests)
{
	INIT;
	LOCK;

	if(tap_shm->have_plan != 0) {
		fprintf(stderr, "You tried to plan twice!\n");
		tap_shm->test_died = 1;
		UNLOCK;
		exit(255);
	}

	if(tests == 0) {
		fprintf(stderr, "You said to run 0 tests!  You've got to run something.\n");
		tap_shm->test_died = 1;
		UNLOCK;
		exit(255);
	}

	tap_shm->have_plan = 1;

	_expected_tests(tests);

	UNLOCK;

	return tap_shm->e_tests;
}

void diag(const char *fmt, ...)
{
	va_list ap;

	INIT;
	LOCK;

	fputs("# ", stderr);

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	fputs("\n", stderr);

	UNLOCK;
}

void _expected_tests(unsigned int tests)
{
	INIT;
	LOCK;

	printf("1..%d\n", tests);
	tap_shm->e_tests = tests;

	UNLOCK;
}

int skip_f(unsigned int n, const char *fmt, ...)
{
	va_list ap;
	char *skip_msg;

	INIT;
	LOCK;

	va_start(ap, fmt);
	if(vasprintf(&skip_msg, fmt, ap) < 0) {
		skip_msg = NULL;
	}
	va_end(ap);

	while (n-- > 0) {
		tap_shm->test_count++;
		printf("ok %d # skip %s\n", tap_shm->test_count, 
		       skip_msg != NULL ? 
		       skip_msg : "libtap():malloc() failed");
	}

	free(skip_msg);

	UNLOCK;

	return 1;
}

int exit_status(void)
{
	int r;

	LOCK;

	if (tap_shm->main_pid && tap_shm->main_pid != getpid()) {
		UNLOCK;
		return 0;
	}

	/* If there's no plan, just return the number of failures */
	if(tap_shm->no_plan || !tap_shm->have_plan) {
		UNLOCK;
		return tap_shm->failures;
	}

	/* Ran too many tests?  Return the number of tests that were run
	   that shouldn't have been */
	if(tap_shm->e_tests < tap_shm->test_count) {
		r = tap_shm->test_count - tap_shm->e_tests;
		UNLOCK;
		return r;
	}

	/* Return the number of tests that failed + the number of tests 
	   that weren't run */
	r = tap_shm->failures + tap_shm->e_tests - tap_shm->test_count;
	UNLOCK;

	return r;
}

/*
 * Cleanup at the end of the run, produce any final output that might be
 * required.
 */
void _cleanup(void)
{

	LOCK;

	if (tap_shm->main_pid && tap_shm->main_pid != getpid()) {
		UNLOCK;
		return;
	}

	/* If plan_no_plan() wasn't called, and we don't have a plan,
	   and we're not skipping everything, then something happened
	   before we could produce any output */
	if(!tap_shm->no_plan && !tap_shm->have_plan && !tap_shm->skip_all) {
		diag("Looks like your test died before it could output anything.");
		UNLOCK;
		return;
	}

	if(tap_shm->test_died) {
		diag("Looks like your test died just after %d.", tap_shm->test_count);
		UNLOCK;
		return;
	}


	/* No plan provided, but now we know how many tests were run, and can
	   print the header at the end */
	if(!tap_shm->skip_all && (tap_shm->no_plan || !tap_shm->have_plan)) {
		printf("1..%d\n", tap_shm->test_count);
	}

	if((tap_shm->have_plan && !tap_shm->no_plan) && tap_shm->e_tests < tap_shm->test_count) {
		diag("Looks like you planned %d %s but ran %d extra.",
		     tap_shm->e_tests, tap_shm->e_tests == 1 ? "test" : "tests", tap_shm->test_count - tap_shm->e_tests);
		UNLOCK;
		return;
	}

	if((tap_shm->have_plan || !tap_shm->no_plan) && tap_shm->e_tests > tap_shm->test_count) {
		diag("Looks like you planned %d %s but only ran %d.",
		     tap_shm->e_tests, tap_shm->e_tests == 1 ? "test" : "tests", tap_shm->test_count);
		UNLOCK;
		return;
	}

	if(tap_shm->failures)
		diag("Looks like you failed %d %s of %d.", 
		     tap_shm->failures, tap_shm->failures == 1 ? "test" : "tests", tap_shm->test_count);

	UNLOCK;
}

int is_charp_f(const char *got, const char *expected, const char *condition, 
		const char *func, const char *file, int line, const char *fmt,
		...)
{
	va_list ap;
	int rtn;

	va_start(ap, fmt);
	rtn = _vgen_result(0 == strcmp(got, expected), condition, got, expected,
			func, file, line, fmt, ap);
	va_end(ap);

	return rtn;
}

int is_longlong_f(long long got, long long expected, const char *condition,
		const char *func, const char *file, int line, const char *fmt, 
		...)
{
	char got_buf[24], expected_buf[24];
	va_list ap;
	int rtn;

	snprintf(got_buf, sizeof got_buf, "%lld", got);
	snprintf(expected_buf, sizeof expected_buf, "%lld", expected);

	va_start(ap, fmt);
	rtn = _vgen_result(got == expected, condition, got_buf, expected_buf, 
			func, file, line, fmt, ap);
	va_end(ap);

	return rtn;
}

int is_ulonglong_f(unsigned long long got, unsigned long long expected, 
		const char *condition, const char *func, const char *file, 
		int line, const char *fmt, ...)
{
	char got_buf[24], expected_buf[24];
	va_list ap;
	int rtn;

	snprintf(got_buf, sizeof got_buf, "%llu", got);
	snprintf(expected_buf, sizeof expected_buf, "%llu", expected);

	va_start(ap, fmt);
	rtn = _vgen_result(got == expected, condition, got_buf, expected_buf, 
			func, file, line, fmt, ap);
	va_end(ap);

	return rtn;
}

int isnt_charp_f(const char *got, const char *expected, const char *condition,
		const char *func, const char *file, int line, const char *fmt,
		...)
{
	va_list ap;
	int rtn;

	va_start(ap, fmt);
	rtn = _vgen_result(strcmp(got, expected), condition, got, NULL, 
			func, file, line, fmt, ap);
	va_end(ap);

	return rtn;
}

int isnt_longlong_f(long long got, long long expected, const char *condition,
		const char *func, const char *file, int line, const char *fmt,
		...)
{
	char got_buf[24];
	va_list ap;
	int rtn;

	snprintf(got_buf, sizeof got_buf, "%lld", got);

	va_start(ap, fmt);
	rtn = _vgen_result(got != expected, condition, got_buf, NULL, 
			func, file, line, fmt, ap);
	va_end(ap);

	return rtn;
}

int isnt_ulonglong_f(unsigned long long got, unsigned long long expected, 
		const char *condition, const char *func, const char *file, 
		int line, const char *fmt, ...)
{
	char got_buf[24];
	va_list ap;
	int rtn;

	snprintf(got_buf, sizeof got_buf, "%llu", got);

	va_start(ap, fmt);
	rtn = _vgen_result(got != expected, condition, got_buf, NULL, 
			func, file, line, fmt, ap);
	va_end(ap);

	return rtn;
}

int cmp_ok_f(int result, long long got, long long expected, const char *op, 
		const char *condition, const char *func, const char *file, 
		int line, const char *fmt, ...)
{
	char got_buf[26], expected_buf[32];
	va_list ap;
	int rtn;

	snprintf(got_buf, sizeof got_buf, "0x%llx", got);
	snprintf(expected_buf, sizeof got_buf, "%s 0x%llx", op, expected);
	
	va_start(ap, fmt);
	rtn = _vgen_result(result, condition, got_buf, expected_buf, 
			func, file, line, fmt, ap);
	va_end(ap);

	return rtn;
}

void BAIL_OUT_f(const char *func, const char *file, int line, const char *fmt, 
		...)
{
	va_list ap;

	// BAIL_OUT is not allowed to lock

	printf("Bail out! ");
	if (fmt) {
		va_start(ap, fmt);
		vprintf(fmt, ap);
		va_end(ap);
	}
	printf(" at %s:%d\n", file, line);

	exit(255);
}
