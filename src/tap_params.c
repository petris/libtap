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

#include <sys/queue.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "tap_params.h"
#include "tap_main.h"
#include "tap.h"

extern int tap_params_override_active;

struct {
	char *type; char *fmt; size_t size;
} static numbers[] = {
	{"unsigned int", "i", sizeof(unsigned int)},
	{"int", "i", sizeof(int)},
	{"unsigned long", "li", sizeof(unsigned long)},
	{"long", "li", sizeof(long)},
	{"unsigned long long", "lli", sizeof(unsigned long long)},
	{"long long", "lli", sizeof(long long)},
	{"char", "c", sizeof(char)},
};


TAILQ_HEAD(listhead, tap_param_s) tap_param_list;

struct tap_param_s {
	char *name;
	char *type;
	void *override;
	int overriden;
	TAILQ_ENTRY(tap_param_s) entries;
};

static int tap_param_name_len = 0;

static void add_entry(const char *name, const char *type, int array)
{
	struct tap_param_s *new;

	if (0 == strcmp(name, "_tap_skip")) {
		return;
	}

	if (tap_param_name_len < strlen(name)) {
		tap_param_name_len = strlen(name);
	}
       
	new = malloc(sizeof *new + strlen(name) + strlen(type) + 8 + 2 + 1 + 1);

	new->override = (char*)new + sizeof *new;
	new->name = (char*)new->override + 8;
	new->type = new->name + strlen(name) + 1;
	new->overriden = 0;

	strcpy(new->name, name);
	strcpy(new->type, type);

	if (array) {
		strcat(new->type, "[]");
	}

	TAILQ_INSERT_TAIL(&tap_param_list, new, entries);
}

void tap_params_init(const char *params_def)
{
	char *parsed = malloc(strlen(params_def) + 1);
	char *parsed_ptr, *token_ptr, *token;
	int i, nested, pos, in_space;

	TAILQ_INIT(&tap_param_list);

	// Remove nested
	i = 0, nested = 0, pos = 0, in_space = 1;
	do {
		switch (params_def[i]) {
			case '{':
				nested++;
				break;
			case '}':
				nested--;
			case ' ':
			case '\t':
			case '\n':
				if (nested == 0 && !in_space) {
					parsed[pos++] = ' ';
					in_space = 1;
				}
				break;
			default:
				if (nested == 0) {
					if (params_def[i] == ';') {
						if (in_space) {
							pos--;
						}
						in_space = 1;
					} else {
						in_space = 0;
					}
					parsed[pos++] = params_def[i];
				}
		}
	} while (params_def[i++] != 0);

	parsed_ptr = parsed;
	while (NULL != (token = strsep(&parsed_ptr, ";"))) {
		char *tmp;

		if (*token == 0) continue;
		
		token_ptr = rindex(token, '[');
		if (token_ptr) {
			if (token_ptr[-1] == ' ') token_ptr--;
			*token_ptr = '\0';
		}

		tmp = rindex(token, ' ');
		for (*tmp++ = '\0'; tmp[0] == '*'; tmp++) {
			tmp[-1] = '*';
			tmp[0] = '\0';
		}
		add_entry(tmp, token, token_ptr != NULL);
	}
}

void *tap_get_override(const char *name, void *fallback)
{
	struct tap_param_s *param;

	TAILQ_FOREACH(param, &tap_param_list, entries) {
		if (0 == strcmp(param->name, name)) {
			return param->overriden ? param->override : fallback;
		}
	}
}

void tap_param_override(const char *name, const char *value)
{
	struct tap_param_s *param;
	int i;
	char buf[10];

	TAILQ_FOREACH(param, &tap_param_list, entries) {
		if (0 == strcmp(name, param->name)) {
			for (i = 0; i < sizeof numbers/sizeof numbers[0]; i++) {
				if (0 == strcmp(param->type, numbers[i].type)) {
					char none;

					strcpy(buf, "%");
					strcat(buf, numbers[i].fmt);
					strcat(buf, "%c");

					if (1 != sscanf(value, buf, param->override, &none)) {
						fprintf(stderr, "'%s' is not a valid number\n", value);
						exit(255);
					}

					param->overriden = 1;

					return;
				}
			}
			if (0 == strcmp(param->type, "const char*") ||
			    0 == strcmp(param->type, "const char *") ||
			    0 == strcmp(param->type, "char*") ||
			    0 == strcmp(param->type, "char *")) {
				*(char**)param->override = malloc(strlen(value) + 1);
				strcpy(*(char**)param->override, value);
				param->overriden = 1;
				return;
			}
		}
	}

	fprintf(stderr, "Parameter '%s' does not exist\n", name);
	exit(255);
}

static void tc_skip_string(const char **string)
{
	int escape = 0;

	while (*++(*string)) switch (**string) {
		case '\\':
			escape = 1;
			break;
		case '"':
			if (escape == 0) {
				return;
			}
		default:
			escape = 0;
	}
}

static void tap_dump_def_indent(int level)
{
	level = level * 4 + 2;
	while (level-- > 0) {
		putchar(' ');
	}
}

static void tap_params_dump_val(const char *name, const char *val,
		const char **val_str, int *val_len)
{
	char *start;
	int found = 0;
	int nested = 0;

	while (*++val) switch (*val) {
		case ')':
			goto end;
		case '.':
			if (  0 == nested &&
			      0 == strncmp(name, val + 1, strlen(name)) &&
			      0 == isalnum(val[1 + strlen(name)]) &&
			    '_' != val[1 + strlen(name)]) {
				found = 1;
				start = index(val + 1 + strlen(name), '=') + 1;
				while (isblank(*start)) start++;
				val = start - 1;
			}
			break;
		case '"':
			tc_skip_string(&val);
			break;
		case '{':
			nested++;
			break;
		case '}':
			nested--;
			break;
		case ',':
			if (nested == 0 && found) goto end;
			break;
	}

end:
	if (found) {
		*val_str = start;
		*val_len = val - start;
	} else {
		*val_str = "0";
		*val_len = 1;
	}
}


static void tap_dump_def(char **definition, int level)
{
	int in_space = 1;
	int in_nl = 1;

	while (*++(*definition)) switch (**definition) {
		case '{':
			if (in_space) {
				printf("{\n");
			} else {
				printf(" {\n");
			}
			//++*definition;
			tap_dump_def(definition, level + 1);
			in_space = 1;
			break;
		case '}':
			if (in_nl == 0) {
				putchar('\n');
			}
			tap_dump_def_indent(level - 1);
			printf("} ");
			return;
		case ';':
			printf(";\n");
			in_nl = 1;
			in_space = 1;
			break;
		case ' ':
		case '\n':
		case '\t':
			if (in_space == 0) {
				putchar(' ');
			}
			in_space = 1;
			break;
		default:
			if (in_nl) {
				tap_dump_def_indent(level);
				in_nl = 0;
			}
			in_space = 0;
			putchar(**definition);
	}
}

static void tap_verbose_print(const char *fmt, ...)
{
	va_list ap;

	if (tap_verbose == 0) return;

	fputs("# ", stderr);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	fputs("\n", stderr);
}

static void tap_params_dump_vals(const char *vals_def, 
		int num)
{
	struct tap_param_s *param;
	const char *start;
	const char *val;
	char buf[64], *end;
	int val_len;
	int i;
	
	start = vals_def; num++;
	while (num--) {
		start = strstr(start, "TAP_PARAMS_VALUES");
		if (start == NULL) {
			start = "TAP_PARAMS_VALUES ()";
			break;
		}
		start++;
	}

	// Dump
	start = index(start, '(') + 1;
	TAILQ_FOREACH(param, &tap_param_list, entries) {
		tap_params_dump_val(param->name, start, &val, &val_len);

		if (tap_params_override_active && param->overriden) {
			for (i = 0; i < sizeof numbers/sizeof numbers[0]; i++) {
				if (0 == strcmp(param->type, numbers[i].type)) {
					switch (numbers[i].size) {
						case 8:
							sprintf(buf, "0x%llX (default: ", *(long long*)param->override);
							break;
						case 4:
							sprintf(buf, "0x%X (default: ", *(int*)param->override);
							break;
						case 2:
							sprintf(buf, "0x%X (default: ", *(short*)param->override);
							break;
						case 1:
							sprintf(buf, "0x%X (default: ", *(char*)param->override);
							break;
					}
					break;
				}
			}
			if (0 == strcmp(param->type, "const char*") ||
			    0 == strcmp(param->type, "const char *") ||
			    0 == strcmp(param->type, "char*") ||
			    0 == strcmp(param->type, "char *")) {
				snprintf(buf, sizeof buf, "\"%s\" (default: ", *(char**)param->override);
			}
			end = ")";
		} else {
			buf[0] = 0;
			end = "";
		}	

		tap_verbose_print("% *s: %s%.*s%s", tap_param_name_len, 
				param->name, buf, val_len, val, end);
	}
}

int tap_param_skip(char *range, void *vals, unsigned long vals_size, 
		unsigned long vals_nmemb)
{
	char *token;
	int start, end, tmp, i;
	static int disabled = 0;

	if (disabled == 0) {
		disabled = 1;
		for (i = 0; i < vals_nmemb; i++) {
			*(int*)((char*)vals + i * vals_size) = 1;
		}
	}

	while (NULL != (token = strsep(&range, ","))) {
		if (*token == 0) continue;
		
		if (index(token, '-')) {
			if (2 != sscanf(token, "%d-%d%c", &start, &end, &tmp)) {
				start = -1;
			} else if (end < start) {
				start = -1;
			}
		} else {
			if (1 != sscanf(token, "%d%c", &start, &tmp)) {
				start = -1;
			} else {
				end = start;
			}
		}

		if (start < 0) {
			return -1;
		}

		for (i = start; i < vals_nmemb && i <= end; i++) {
			*(int*)((char*)vals + i * vals_size) = 0;
		}
	}

	return 0;
}

void tap_params_main(char *params_def, char *vals_def, 
		void *vals, void **current, 
		unsigned long vals_size, unsigned long vals_nmemb, 
		int count)
{
	int i, j;
	int tc_count = 0;

	for (i = 0; i < vals_nmemb; i++) {
		tap_params_header_t *hdr = 
				(tap_params_header_t *)((char*)vals + i * vals_size);
		if (hdr->skip == 0) {
			tc_count += hdr->plan;
		}
	}
	
	tap_init(tap_flags);

	plan_tests(tc_count * count);

	for (i = 0; i < vals_nmemb; i++) {
		*current = (char*)vals + i * vals_size;

		if (((tap_params_header_t*)*current)->skip) {
			tap_verbose_print("Skipping round %d", i);
			continue;
		} else {
			tap_verbose_print("Starting round %d", i);
			tap_params_dump_vals(vals_def, i);
		}

		for (j = 0; j < count; j++) {
			tap_main(i);
		}
	}
}

void tap_params_info(void)
{
	if (0 == strlen(tap_params_def)) {
		printf("This test case does not have any parameters defined.\n");
	} else {
		char *params_def = (char*)tap_params_def - 1;
		printf("Parameters:\n");
		tap_dump_def(&params_def, 0);
	}
}
