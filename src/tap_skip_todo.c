/*-
 * Copyright (c) 2013 Petr Malat
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

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include "tap_skip_todo.h"
#include "tap.h"

/* SKIPB functionality */

typedef struct tap_skip_s {
	struct tap_skip_s *prev;
	int cond_evals;
} tap_skip_t;

#ifdef HAVE_LIBPTHREAD
#include <pthread.h>

static pthread_key_t tap_skip_key;

void tap_skip_init(void)
{
	if (0 != pthread_key_create(&tap_skip_key, NULL)) {
		BAIL_OUT("Failed to initialize SKIP structure");
	}
}

static inline tap_skip_t *tap_skip_get(void)
{
	return pthread_getspecific(tap_skip_key);
}

static inline void tap_skip_set(tap_skip_t *ptr)
{
	pthread_setspecific(tap_skip_key, ptr);
}
#else // HAVE_LIBPTHREAD
static tap_skip_t *tap_skip_ptr;

void tap_skip_init(void)
{
}

static inline tap_skip_t *tap_skip_get(void)
{
	return tap_skip_ptr;
}

static inline void tap_skip_set(tap_skip_t *ptr)
{
	tap_skip_ptr = ptr;
}
#endif // HAVE_LIBPTHREAD

void tap_skip_start(void)
{
	tap_skip_t *new = malloc(sizeof *new);

	new->prev = tap_skip_get();
	new->cond_evals = 0;

	tap_skip_set(new);
}

int tap_skip_cond(void)
{
	tap_skip_t *current = tap_skip_get();
	current->cond_evals++;

	if (current->cond_evals == 1) {
		return 1;
	} else if (current->cond_evals != 2) {
		BAIL_OUT("SKIP block flow broken");
	}

	tap_skip_set(current->prev);
	free(current);

	return 0;
}

/* TODO functionality */

typedef struct tap_todo_s {
	struct tap_todo_s *prev;
	const char *msg;
	int cond_evals;
} tap_todo_t;

#ifdef HAVE_LIBPTHREAD
static pthread_key_t tap_todo_key;

void tap_todo_init(void)
{
	if (0 != pthread_key_create(&tap_todo_key, NULL)) {
		BAIL_OUT("Failed to initialize TODO structure");
	}
}

static inline tap_todo_t *tap_todo_get(void)
{
	return pthread_getspecific(tap_todo_key);
}

static inline void tap_todo_set(tap_todo_t *ptr)
{
	pthread_setspecific(tap_todo_key, ptr);
}
#else // HAVE_LIBPTHREAD
static tap_todo_t *tap_todo_ptr;

void tap_todo_init(void)
{
}

static inline tap_todo_t *tap_todo_get(void)
{
	return tap_todo_ptr;
}

static inline void tap_todo_set(tap_todo_t *ptr)
{
	tap_todo_ptr = ptr;
}
#endif // HAVE_LIBPTHREAD

void tap_todo_start(const char *fmt, ...)
{
	tap_todo_t *new = malloc(sizeof *new);

	new->prev = tap_todo_get();

	if (fmt) {
		va_list ap;
		va_start(ap, fmt);
		if (vasprintf(&new->msg, fmt, ap) < 0) {
			new->msg = NULL;
		}
		va_end(ap);
	} else {
		new->msg = NULL;
	}

	new->cond_evals = 0;
	tap_todo_set(new);
}

int tap_todo_cond(void)
{
	tap_todo_t *current = tap_todo_get();
	current->cond_evals++;

	if (current->cond_evals == 1) {
		return 1;
	} else if (current->cond_evals != 2) {
		BAIL_OUT("TODO block flow broken");
	}

	tap_todo_set(current->prev);
	free(current->msg);
	free(current);

	return 0;
}

/** Get current TODO message or NULL if we are not in TODO block */
const char *tap_todo_msg(void)
{
	tap_todo_t *current = tap_todo_get();
	
	if (current) {
		if (current->msg) {
			return current->msg;
		} else {
			return "";
		}
	} else {
		return NULL;
	}
}
