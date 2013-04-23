/* Copyright (c) 2004 Nik Clayton
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

#ifndef TAP_H
#define TAP_H

/** @defgroup public_api Public API 
 * libTAP public interface
 *
 * @{
 * @}
 */

/** plan_no_plan - I have no idea how many tests I'm going to run.
 *
 * In some situations you may not know how many tests you will be running, or
 * you are developing your test program, and do not want to update the
 * plan_tests() call every time you make a change.  For those situations use
 * plan_no_plan() instead of plan_tests().  It indicates to the test harness
 * that an indeterminate number of tests will be run.
 *
 * Remember, if you fail to plan, you plan to fail.
 *
 * @b Example:
 * @code
 * plan_no_plan();
 * while (random() % 2)
 *     ok(some_test());
 * exit(exit_status());
 * @endcode
 *
 * @ingroup public_api
 */
int plan_no_plan(void);

/** Indicate that you will skip all tests.
 * @param reason - the string indicating why you can't run any tests.
 *
 * If your test program detects at run time that some required functionality
 * is missing (for example, it relies on a database connection which is not
 * present, or a particular configuration option that has not been included
 * in the running kernel) use plan_skip_all() instead of plan_tests().
 *
 * @b Example:
 * @code
 * if (!have_some_feature) {
 *     plan_skip_all("Need some_feature support");
 *     exit(exit_status());
 * }
 * plan_tests(13);
 * @endcode
 *
 * @ingroup public_api
 */
int plan_skip_all(const char * reason);

/** Announce the number of tests you plan to run
 * @param tests - the number of tests
 *
 * This should be the first call in your test program: it allows tracing
 * of failures which mean that not all tests are run.
 *
 * If you don't know how many tests will actually be run, assume all of them
 * and use SKIP() if you don't actually run some tests.
 *
 * @b Example:
 * @code
 * plan_tests(13);
 * @endcode
 *
 * @ingroup public_api
 */
int plan_tests(unsigned int tests);

/** Print a diagnostic message (use instead of printf/fprintf)
 * @param fmt - the format of the printf-style message
 *
 * diag ensures that the output will not be considered to be a test
 * result by the TAP test harness.  It will append '\n' for you.
 *
 * @b Example:
 * @code
 * diag("Now running complex tests");
 * @endcode
 *
 * @ingroup public_api
 */
void diag(const char *fmt, ...);

/** The value that main should return.
 *
 * For maximum compatibility your test program should return a particular exit
 * code (ie. 0 if all tests were run, and every test which was expected to
 * succeed succeeded).
 *
 * @b Example:
 * @code
 * exit(exit_status());
 * @endcode
 *
 * @ingroup public_api
 */
int exit_status(void);

#define MP "\001"

enum tap_flags_e {
	TAP_FLAGS_DEFAULT    =   0,
	TAP_FLAGS_FORK       =   1,
	TAP_FLAGS_ERRNO      =   2,
	TAP_FLAGS_NONAME     =   4,
	TAP_FLAGS_REPEAT_10  =   8,
	TAP_FLAGS_REPEAT_40  =  16,
	TAP_FLAGS_REPEAT_120 =  32,
	TAP_FLAGS_TRACE      =  64,
	TAP_FLAGS_YAMLISH    = 128,
} tap_flags_t;


/** Initialize the TAP library
 * @param flags - Combination of tap_flags_t flags
 *
 * @ingroup public_api
 */
#define tap_init(flags) \
	tap_init_f(flags, __func__, __FILE__, __LINE__)

/** Interrupt the execution and fail
 * @param reason - Format string for reason message
 * @param ... - Arguments for the reason format string
 *
 * @ingroup public_api
 */
#define BAIL_OUT(...) \
	BAIL_OUT_f(__func__, __FILE__, __LINE__, __VA_ARGS__ + 0)

/** Conditional test with a name
 * @param e - The expression which we expect to be true
 * @param ... - Format string and argument for the test name (optional)
 *
 * If the expression is true, the test passes.  The name of the test will be
 * the filename, line number, and the printf-style string. This can be clearer
 * than simply the expression itself.
 *
 * Example:
 *	ok(init_subsystem() == 0, "Second initialization should fail");
 *
 * @ingroup public_api
 */
#define ok(e, ...) \
	_gen_result(!!(e), #e, __func__, __FILE__, __LINE__, __VA_ARGS__ + 0)

/** Note that a test passed
 * @...: the printf-style name of the test.
 *
 * For complicated code paths, it can be easiest to simply call pass() in one
 * branch and fail() in another.
 *
 * Example:
 *	x = do_something();
 *	if (!checkable(x) || check_value(x))
 *		pass("do_something() returned a valid value");
 *	else		
 *		fail("do_something() returned an invalid value");
 *
 * @ingroup public_api
 */
#define pass(...) \
	_gen_result(1, "Force PASS", __func__, __FILE__, __LINE__, __VA_ARGS__ + 0)

/** Note that a test failed
 * @...: the printf-style name of the test.
 *
 * For complicated code paths, it can be easiest to simply call pass() in one
 * branch and fail() in another.
 *
 * @ingroup public_api
 */
#define fail(...) \
	_gen_result(0, "Force FAIL", __func__, __FILE__, __LINE__, __VA_ARGS__ + 0)

#ifdef __GNUC__
/** Test if arguments are the same and evaluate the test (GCC only)
 * @param got - Tested value
 * @param expected - Expected value 
 * @param ... - Format string and arguments composing the test name (optional)
 *
 * @ingroup public_api
 */
#define is(got, expected, ...) \
	__builtin_choose_expr(	                                                    \
		__builtin_types_compatible_p(typeof(expected), char *) ||           \
		__builtin_types_compatible_p(typeof(expected), const char *),       \
			is_charp(got, expected, __VA_ARGS__ + 0),                   \
	__builtin_choose_expr(                                                      \
		__builtin_types_compatible_p(typeof(expected), int) ||              \
		__builtin_types_compatible_p(typeof(expected), long) ||             \
		__builtin_types_compatible_p(typeof(expected), long long),          \
			is_longlong(got, expected, __VA_ARGS__ + 0),                \
	__builtin_choose_expr(                                                      \
		__builtin_types_compatible_p(typeof(expected), unsigned) ||         \
		__builtin_types_compatible_p(typeof(expected), unsigned long) ||    \
		__builtin_types_compatible_p(typeof(expected), unsigned long long), \
			is_ulonglong(got, expected, __VA_ARGS__ + 0),               \
	BAIL_OUT("Unsupported type passed to is()"))))

/** Test if arguments are different and evaluate the test (GCC only)
 * @param got - Tested value
 * @param forbidden - Forbidden value 
 * @param ... - Format string and arguments composing the test name (optional)
 *
 * @ingroup public_api
 */
#define isnt(got, forbidden, ...) \
	__builtin_choose_expr(	                                                     \
		__builtin_types_compatible_p(typeof(forbidden), char *) ||           \
		__builtin_types_compatible_p(typeof(forbidden), const char *),       \
			isnt_charp(got, forbidden, __VA_ARGS__ + 0),                 \
	__builtin_choose_expr(                                                       \
		__builtin_types_compatible_p(typeof(forbidden), int) ||              \
		__builtin_types_compatible_p(typeof(forbidden), long) ||             \
		__builtin_types_compatible_p(typeof(forbidden), long long),          \
			isnt_longlong(got, forbidden, __VA_ARGS__ + 0),              \
	__builtin_choose_expr(                                                       \
		__builtin_types_compatible_p(typeof(forbidden), unsigned) ||         \
		__builtin_types_compatible_p(typeof(forbidden), unsigned long) ||    \
		__builtin_types_compatible_p(typeof(forbidden), unsigned long long), \
			isnt_ulonglong(got, forbidden, __VA_ARGS__),                 \
	BAIL_OUT("Unsupported type passed to is()"))))
#endif 

/** Compare two numbers using specified operator
 * @param got - Tested value
 * @param op - The operator
 * @param expected - Value, we are comparing to
 * @param ... - Format string and arguments composing the test name (optional)
 *
 * @ingroup public_api
 */
#define cmp_ok(got, op, expected, ...) \
	do {                                                           \
		typeof(got)      g = (got);                            \
		typeof(expected) e = (expected);                       \
		cmp_ok_f(!!(g op e), (long long)g, (long long)e, #op,  \
			#got " " #op " " #expected,                    \
			__func__, __FILE__, __LINE__, __VA_ARGS__ + 0);\
	} while (0)

/** Test if two strings are the same and evaluate the test
 * @param got - Tested string
 * @param expected - Expected string
 * @param ... - Format string and arguments composing the test name (optional)
 *
 * @ingroup public_api
 */
#define is_charp(got, expected, ...) \
	is_charp_f((const char *)(long)(got), (const char *)(long)(expected), \
		   #got " =:= " #expected,                                    \
		   __func__, __FILE__, __LINE__, __VA_ARGS__ + 0)

/** Test if two long long numbers are the same and evaluate the test
 * @param got - Tested value
 * @param expected - Expected value
 * @param ... - Format string and arguments composing the test name (optional)
 *
 * @ingroup public_api
 */
#define is_longlong(got, expected, ...) \
	is_longlong_f((long long)(got), (long long)(expected), \
		      #got " == " #expected,                   \
		      __func__, __FILE__, __LINE__, __VA_ARGS__ + 0)

/** Test if two unsigned long long numbers are the same and evaluate the test
 * @param got - Tested value
 * @param expected - Expected value
 * @param ... - Format string and arguments composing the test name (optional)
 *
 * @ingroup public_api
 */
#define is_ulonglong(got, expected, ...) \
	is_ulonglong_f((unsigned long long)(got),      \
		       (unsigned long long)(expected), \
		       #got " == " #expected,          \
		       __func__, __FILE__, __LINE__, __VA_ARGS__ + 0)



/** Test if two strings are different and evaluate the test
 * @param got - Tested string
 * @param forbidden - Expected string
 * @param ... - Format string and arguments composing the test name (optional)
 *
 * @ingroup public_api
 */
#define isnt_charp(got, forbidden, ...) \
	isnt_charp_f((const char *)(long)(got),       \
		     (const char *)(long)(forbidden), \
		     #got " !:= " #forbidden,         \
		     __func__, __FILE__, __LINE__, __VA_ARGS__ + 0)

/** Test if two long long numbers are different and evaluate the test
 * @param got - Tested value
 * @param forbidden - Expected value
 * @param ... - Format string and arguments composing the test name (optional)
 *
 * @ingroup public_api
 */
#define isnt_longlong(got, forbidden, ...) \
	isnt_longlong_f((long long)(got), (long long)(forbidden), \
		        #got " != " #forbidden,                   \
		        __func__, __FILE__, __LINE__, __VA_ARGS__ + 0)

/** Test if two unsigned long long numbers are different and evaluate the test
 * @param got - Tested value
 * @param forbidden - Expected value
 * @param ... - Format string and arguments composing the test name (optional)
 *
 * @ingroup public_api
 */
#define isnt_ulonglong(got, forbidden, ...) \
	isnt_ulonglong_f((unsigned long long)(got),       \
			 (unsigned long long)(forbidden), \
		         #got " != " #forbidden,          \
		         __func__, __FILE__, __LINE__, __VA_ARGS__ + 0)

/** Start SKIP() block 
 *
 * Sometimes tests cannot be run because the test system lacks some feature:
 * you should explicitly document that you're skipping tests using skip().
 *
 * From the Test::More documentation:
 *   If it's something the user might not be able to do, use SKIP.  This
 *   includes optional modules that aren't installed, running under an OS that
 *   doesn't have some feature (like fork() or symlinks), or maybe you need an
 *   Internet connection and one isn't available.
 *
 * @b Example:
 * @code
 * SKIP (0 == getuid(), 2, "Requires other user than root") {
 *     is(kill(1, 9), -1, "Init can't be killed");
 *     is(errno, EPERM, "No permission to kill init");
 * }
 * @endcode
 *
 * @ingroup public_api
 */
#define SKIP(cond, how_many, ...) \
		if (cond) { skip_f(how_many, __VA_ARGS__ + 0); } else

/** Start SKIP block 
 *
 * Like SKIP(), but allows you to handle more complicated skip conditions
 *
 * @b Example:
 * @code
 * SKIP2 {
 *     if (0 == getuid()) {
 *         skip(2, "Requires other user than root");
 *     }
 *     is(kill(1, 9), -1, "Init can't be killed");
 *     is(errno, EPERM, "No permission to kill init");
 * }
 * @endcode
 *
 * @ingroup public_api
 */
#define SKIP2 \
		for (tap_skip_start(); tap_skip_cond();)

/** Skip command for the skip block
 * See SKIP2 for details
 *
 * @param how_many - How many test cases there is in the SKIP block
 * @param ... - Format string and arguments for reason of skip
 *
 * @ingroup public_api
 */
#define skip(how_many, ...) \
		skip_f(how_many, __VA_ARGS__ + 0); continue

/** Mark tests that you expect to fail.
 * @param fmt - the reason they currently fail.
 *
 * It's extremely useful to write tests before you implement the matching fix
 * or features: surround these tests by todo_start()/todo_end().  These tests
 * will still be run, but with additional output that indicates that they are
 * expected to fail.
 *
 * This way, should a test start to succeed unexpectedly, tools like prove(1)
 * will indicate this and you can move the test out of the todo block.  This
 * is much more useful than simply commenting out (or '# if 0') the tests.
 * 
 * From the Test::More documentation:
 *   If it's something the programmer hasn't done yet, use TODO.  This is for
 *   any code you haven't written yet, or bugs you have yet to fix, but want to
 *   put tests in your testing script (always a good idea).
 *
 * @b Example:
 * @code
 * TODO ("dwim() not returning true yet") {
 *     ok(dwim(), "Did what the user wanted");
 * }
 * @endcode
 *
 * @ingroup public_api
 */
#define TODO(...) \
		for (tap_todo_start(__VA_ARGS__ + 0); tap_todo_cond();)

#ifdef __GNUC__
/** Define set of parameters.
 * @params ... - Parameters definition body, same syntax as in struct definition
 *
 * @b Example:
 * @code
 * TAP_PARAMS_DEFINITION(
 *     unsigned long flags;
 *     enum {
 *         FIRST_COMMAND,
 *         SECOND_COMMAND,
 *     } command;
 *     char *data;
 * )
 * @endcode
 *
 * @ingroup public_api
 */
#define TAP_PARAMS_DEFINITION(...) \
	typedef struct tap_params_s {                          \
		tap_params_header_t tap;                       \
		__VA_ARGS__                                    \
	} tap_params_t;                                        \
	const char tap_params_def[] = #__VA_ARGS__;            \
	unsigned long tap_params_size = sizeof(tap_params_t);

/** Define one or more values of TAP_PARAMS_DEFINITION.
 *
 * @b Example:
 * @code
 * TAP_PARAMS_VALUES_ARRAY(
 *     TAP_PARAMS_VALUES(
 *         .tap.plan = 4,
 *         .flags = 42,
 *         .command = FIRST_COMMAND,
 *         .data = "Hello world",
 *     ),
 *     TAP_PARAMS_VALUES(
 *         .tap.plan = 5,
 *     	   .command = SECOND_COMMAND,
 *     	   .data = ".flags is zero"
 *     ),
 * )
 * @endcode
 *
 * @ingroup public_api
 */
#define TAP_PARAMS_VALUES_ARRAY(...) \
	tap_params_t tap_params_values[] = {__VA_ARGS__};  \
	const char tap_params_values_def[] = #__VA_ARGS__; \
	extern tap_params_t *tap_params_current;           \
	unsigned long tap_params_values_nmemb =            \
		sizeof(tap_params_values)/sizeof(tap_params_values[0]);

/** Define values of TAP_PARAMS_DEFINITION. 
 * See @link TAP_PARAMS_VALUES_ARRAY for details.
 *
 * @ingroup public_api
 */
#define TAP_PARAMS_VALUES(...) \
	{__VA_ARGS__}

/** Get current value of parameter 
 * @param name - name of the parameter
 *
 * @b Example:
 * @code
 * rtn = ioctl(dev, TAP_PARAM(command), TAP_PARAM(data));
 * @endcode
 *
 * @ingroup public_api
 */
#define TAP_PARAM(name) \
	(tap_params_override_active ?                                        \
		*(typeof(tap_params_values[0].name)*)tap_get_override(       \
			#name, &tap_params_current->name) :                  \
		tap_params_current->name)

/** Set library flags
 *
 * Library initialization flags can be defined statically using this macro.
 * They will be used, if the library is initialized implicitly (without explicit
 * call to tap_init()).
 *
 * @param flags - Combination of tap_flags_t flags
 *
 * @ingroup public_api
 */
#define TAP_FLAGS(flags) \
	unsigned long tap_flags = (flags);

/** Specify number of tests in test case, which uses tap_main(), but doesn't
 * have any parameters.
 *
 * @param num - Number of tests in this test case
 *
 * @ingroup public_api
 */
#define TAP_PLAN(num) \
	TAP_PARAMS_DEFINITION() \
	TAP_PARAMS_VALUES_ARRAY(TAP_PARAMS_VALUES(.tap.plan = (num)))

/** TAP_STRINGIFY helper */
#define __TAP_STRINGIFY(...)  #__VA_ARGS__

/** Stringify arguments */
#define TAP_STRINGIFY(...)  __TAP_STRINGIFY(__VA_ARGS__)

/** TAP_IDENT helper */
#define __TAP_IDENT(a, b) __tap_## a ## b

/** Generate identificator */
#define TAP_IDENT(a, b) __TAP_IDENT(a, b)

/** TAP_INFO helper */
#define __TAP_INFO(tag, name, info) \
	static const char TAP_IDENT(name, __LINE__)[] \
		__attribute__((section("__tap_info"),used)) = \
			TAP_STRINGIFY(tag) "=" info

/** Add generic information to the test case binary 
 *
 * This information can be read later by running the binary with -i option. If
 * the same name is used several times, values will form an array.
 *
 * @param tag  - Information name
 * @param info - Information value
 *
 * @b Example:
 * @code
 * TAP_INFO(author, "Petr Malat");
 * @endcode
 *
 * @ingroup public_api
 */
#define TAP_INFO(tag, info) __TAP_INFO(tag, tag, info)

#endif // __GNUC__

/* Internal function used by macros *****************************************/

/* From tap.c */

int is_charp_f(const char *get, const char *expected, const char *name, 
		const char *func, const char *file, int line, const char *fmt,
		 ...);

int is_longlong_f(long long get, long long expected, const char *name, 
		const char *func, const char *file, int line, const char *fmt, 
		...);

int is_ulonglong_f(unsigned long long get, unsigned long long expected, 
		const char *name, const char *func, const char *file, int line, 
		const char *fmt, ...);

int isnt_charp_f(const char *get, const char *expected, const char *name,
		const char *func, const char *file, int line, const char *fmt, 
		...);

int isnt_longlong_f(long long get, long long expected, const char *name,
		const char *func, const char *file, int line, const char *fmt, 
		...);

int isnt_ulonglong_f(unsigned long long get, unsigned long long expected, 
		const char *name, const char *func, const char *file, int line, 
		const char *fmt, ...);

int cmp_ok_f(int result, long long get, long long expected, const char *op, 
		const char *name, const char *func, const char *file, int line, 
		const char *fmt, ...);

void BAIL_OUT_f(const char *func, const char *file, int line, const char *fmt, ...);

unsigned int _gen_result(int, const char *, const char *, const char *,
		unsigned int, const char *, ...);

void tap_init_f(long flags, const char *func, const char *file, unsigned int line);

/* From tap_skip_todo.c */

void tap_skip_start(void);

int tap_skip_cond(void);

void tap_todo_start(const char *fmt, ...);

int tap_todo_cond(void);

/* From tap_param.c */

/** PARAMS_VALUES header */
typedef struct tap_params_header_s {
	/** True, if this parameter combination should be skipped */
	int skip;
	/** How many test cases should be executed for this values */
	int plan;
} tap_params_header_t;

extern int tap_params_override_active;

void *tap_get_override(const char *name, void *value);

#endif /* TAP_H */
