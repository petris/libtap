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

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "tap_params.h"
#include "tap.h"

extern char __start___tap_info[];
extern char __stop___tap_info[];

TAP_INFO(libtap_version, "1.05");

static const char opt_help[] = "\
Options:\n\
  -i .............. Print informations about this TC\n\
  -v .............. Verbose execution\n\
  -p param=value .. Override value of parameter 'param'\n\
  -r range ........ Execute only for parameters specified by range (eg: 2,7-11,15)\n\
  -c count ........ Execute the test count times for every parameters set.\n\
  -h .............. Print this message\n\
\n\
Variables:\n\
  HARNESS_ACTIVE .. If set, newline will be printed on stderr if test fails\n\
";

int tap_params_override_active = 0;

int tap_verbose = 0;

char tap_params_def[] __attribute__ ((weak)) = "";

char tap_params_values_def[] __attribute__ ((weak)) = "";

void *tap_params_values[1] __attribute__ ((weak));

unsigned long tap_params_size __attribute__ ((weak)) = 0;

unsigned long tap_params_values_nmemb __attribute__ ((weak)) = 0;

unsigned long tap_flags __attribute__ ((weak)) = TAP_FLAGS_DEFAULT;

void *tap_params_current __attribute__ ((weak));

struct record_s { char *name; int count; };

#define LINE_LEN 80
static void tap_print_record(struct record_s *record, bool array, int max_len)
{
	int n;
	char *chr;

	for (n = 0, chr = __start___tap_info; chr && chr < __stop___tap_info; 
			chr = index(chr, 0) + 1) {
		char *sep = index(chr, '=');
		int len, indent;
		bool ml;
		int i;

		if (!sep) {
			continue;
		}
		
		len = sep - chr;
		if (strncmp(record->name, chr, len)) {
			continue;
		}
			
		ml = index(sep + 1, '\n') != NULL;
		if (!ml && !array && strlen(sep + 1) < LINE_LEN - max_len - 1) {
			// String can fit onto the current line
			printf("%-*s%s", max_len - len, " ", sep + 1);
			break;
		}

		if (!ml && array && strlen(sep + 1) < LINE_LEN - 6) {
			// Array element can fit onto the current line
			printf("\n    - %s", sep + 1);
			continue;
		} 

		// Print long or multi-line string
		if (array) {
			printf("\n    - >");
			indent = 6;
		} else {
			printf("%-*s>", max_len - len, " ");
			indent = 4;
		}

		sep++;
		while (*sep) {
			char *nl = strchr(sep, '\n');
			printf("\n% *s", indent , " ");

			if (nl && nl - sep < LINE_LEN - indent) {
				do {
					putchar(*sep);
				} while (*sep++ != '\n');
				continue;
			} else {
				// FIXME: Fix handling of lines with very long words
				len = strlen(sep);
				if (len > LINE_LEN - indent) {
					int print = indent;
					while (1) {
						char *sp = index(sep, ' ');
						if (sp && sp - sep + print < LINE_LEN) {
							print += sp - sep;
							while (sep != sp) {
								putchar(*sep++);
							}
							if (print < LINE_LEN) {
								print++;
								putchar(' ');
							}
							sep++;
						} else {
							break;
						}
					}
					if (print == indent) {
						while (*sep && *sep != ' ') {
							putchar(*sep++);
						}
					}
				} else {
					printf("%s", sep);
					break;
				}
			}
		}
	}
	putchar('\n');
}

static void tap_print_info(void)
{
	const char *chr = __start___tap_info;
	struct record_s *records;
	int max_len = 0;
	int i, n;

	for (n = 0, chr = __start___tap_info; chr < __stop___tap_info; chr++) {
		if (*chr == '\0') {
			n++;
		}
	}

	records = calloc(n, sizeof *records);

	for (n = 0, chr = __start___tap_info; chr && chr < __stop___tap_info; 
			chr = index(chr, 0) + 1) {
		char *sep = index(chr, '=');
		if (sep) {
			int len = sep - chr;
			for (i = 0; i < n; i++) {
				if (len == strlen(records[i].name) &&
				    0 == strncmp(records[i].name, chr, len)) {
					records[i].count++;
					break;
				}
			}
			if (i == n) {
				records[n].name = strndup(chr, len);
				records[n++].count = 1;
				if (max_len < len) {
					max_len =  len;
				}
			}
		}
	}

	printf("---\n");

	for (i = 0; i < n; i++) {
		printf("%s:", records[i].name);
		if (records[i].count == 1) {
			tap_print_record(records + i, false, max_len + 1);
		} else {
			tap_print_record(records + i, true, max_len + 1);
		}
	}

	printf("...\n");

	free(records);
}

int tap_start(int argc, char *argv[]) 
{ 
	int opt;
	char *tmp;
	int count = 1;

	tap_params_init(tap_params_def);

	while ((opt = getopt(argc, argv, "vhir:p:c:")) != -1) {
		switch (opt) {
			case 'h':
				printf("Usage: %s [OPTIONS]\n%s\n", argv[0], opt_help);
				tap_params_info();
				exit(0);
			case 'p':
				tmp = index(optarg, '=');
				if (tmp == NULL) {
					fprintf(stderr, "Option -p requires an "
							"argument in the format "
							"'parameter=value' (got '%s').\n", 
							optarg);
					exit(1);
				} else {
					*tmp = '\0';
					tap_param_override(optarg, tmp + 1);
					tap_params_override_active = 1;
				}
				break;
			case 'i':
				tap_print_info();
				exit(0);
			case 'v':
				tap_verbose++;
				break;
			case 'r':
				if (0 != tap_param_skip(optarg, tap_params_values, tap_params_size, tap_params_values_nmemb)) {
					fprintf(stderr, "Option -r requires an "
							"argument in the format "
							"[num|start-end][,num|start-end]..."
							" (got '%s').\n", optarg);
					exit(1);
				}
				break;
			case 'c':
				if (1 != sscanf(optarg, "%d%c", &count, &opt) || count < 1) {
					fprintf(stderr, "Option -c requires an "
							"integer argument (got '%s').\n", 
							optarg);
					exit(1);
				}
				break;
		}
	}

	tap_params_main(tap_params_def, tap_params_values_def, 
			tap_params_values, &tap_params_current,
			tap_params_size, tap_params_values_nmemb, 
			count);

	return exit_status();
}


int main(int argc, char *argv) __attribute__ ((weak, alias ("tap_start")));

void tap_main(int round) __attribute__((weak)); 

void tap_main(int round) 
{
	BAIL_OUT("You must define void tap_main(int round) in your test");
}
