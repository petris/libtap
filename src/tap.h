/*-
 * Copyright (c) 2004 Nik Clayton
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

/* ## __VA_ARGS__ is a gcc'ism */   
#define ok(e, test, ...) ((e) ?						\
			  _gen_result(1, __func__, __FILE__, __LINE__,  \
				      test, ## __VA_ARGS__) :		\
			  _gen_result(0, __func__, __FILE__, __LINE__,  \
				      test, ## __VA_ARGS__))

#define ok1(e) ((e) ? \
		_gen_result(1, __func__, __FILE__, __LINE__, "%s", #e) : \
		_gen_result(0, __func__, __FILE__, __LINE__, "%s", #e))

#define pass(test, ...) ok(1, test, ## __VA_ARGS__);
#define fail(test, ...) ok(0, test, ## __VA_ARGS__);

#define SKIP_START(test, n, fmt, ...)			\
	do {						\
		if((test)) {				\
			skip(n, fmt, ## __VA_ARGS__);	\
			continue;			\
		}

#define SKIP_END } while(0);

void _reset(void);		/* Exposed for testing */

void _gen_result(int, const char *, char *, unsigned int, char *, ...);

int plan_no_plan(void);
int plan_skip_all(char *);
int plan_tests(unsigned int);

void diag(char *, ...);

void skip(unsigned int, char *, ...);

int exit_status(void);
