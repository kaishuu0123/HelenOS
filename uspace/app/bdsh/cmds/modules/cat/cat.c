/*
 * Copyright (c) 2008 Tim Post
 * Copyright (c) 2011, Martin Sucha
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <str.h>
#include <fcntl.h>
#include <io/console.h>
#include <io/color.h>
#include <io/style.h>
#include <io/keycode.h>
#include <errno.h>
#include <vfs/vfs.h>
#include <assert.h>

#include "config.h"
#include "util.h"
#include "errors.h"
#include "entry.h"
#include "cat.h"
#include "cmds.h"

static const char *cmdname = "cat";
#define CAT_VERSION "0.0.1"
#define CAT_DEFAULT_BUFLEN 1024
#define CAT_FULL_FILE 0

static const char *hexchars = "0123456789abcdef";

static bool paging_enabled = false;
static size_t chars_remaining = 0;
static size_t lines_remaining = 0;
static sysarg_t console_cols = 0;
static sysarg_t console_rows = 0;
static bool should_quit = false;
static bool dash_represents_stdin = false;

static console_ctrl_t *console = NULL;

static struct option const long_options[] = {
	{ "help", no_argument, 0, 'h' },
	{ "version", no_argument, 0, 'v' },
	{ "head", required_argument, 0, 'H' },
	{ "tail", required_argument, 0, 't' },
	{ "buffer", required_argument, 0, 'b' },
	{ "more", no_argument, 0, 'm' },
	{ "hex", no_argument, 0, 'x' },
	{ "stdin", no_argument, 0, 's' },
	{ 0, 0, 0, 0 }
};

/* Dispays help for cat in various levels */
void help_cmd_cat(unsigned int level)
{
	if (level == HELP_SHORT) {
		printf("`%s' shows the contents of files\n", cmdname);
	} else {
		help_cmd_cat(HELP_SHORT);
		printf(
		"Usage:  %s [options] <file1> [file2] [...]\n"
		"Options:\n"
		"  -h, --help       A short option summary\n"
		"  -v, --version    Print version information and exit\n"
		"  -H, --head ##    Print only the first ## bytes\n"
		"  -t, --tail ##    Print only the last ## bytes\n"
		"  -b, --buffer ##  Set the read buffer size to ##\n"
		"  -m, --more       Pause after each screen full\n"
		"  -x, --hex        Print bytes as hex values\n"
		"  -s  --stdin      Treat `-' in file list as standard input\n"
		"Currently, %s is under development, some options don't work.\n",
		cmdname, cmdname);
	}

	return;
}

static void waitprompt()
{
	console_set_pos(console, 0, console_rows-1);
	console_set_color(console, COLOR_WHITE, COLOR_BLUE, 0);
	
	printf("ENTER/SPACE/PAGE DOWN - next page, "
	       "ESC/Q - quit, C - continue unpaged");
	fflush(stdout);
	
	console_set_style(console, STYLE_NORMAL);
}

static void waitkey()
{
	kbd_event_t ev;
	
	while (true) {
		if (!console_get_kbd_event(console, &ev)) {
			return;
		}
		if (ev.type == KEY_PRESS) {
			if (ev.key == KC_ESCAPE || ev.key == KC_Q) {
				should_quit = true;
				return;
			}
			if (ev.key == KC_C) {
				paging_enabled = false;
				return;
			}
			if (ev.key == KC_ENTER || ev.key == KC_SPACE ||
			    ev.key == KC_PAGE_DOWN) {
				return;
			}
		}
	}
	assert(false);
}

static void newpage()
{
	console_clear(console);
	chars_remaining = console_cols;
	lines_remaining = console_rows - 1;
}

static void paged_char(wchar_t c)
{
	putchar(c);
	if (paging_enabled) {
		chars_remaining--;
		if (c == '\n' || chars_remaining == 0) {
			chars_remaining = console_cols;
			lines_remaining--;
		}
		if (lines_remaining == 0) {
			fflush(stdout);
			waitprompt();
			waitkey();
			newpage();
		}
	}
}

static unsigned int cat_file(const char *fname, size_t blen, bool hex,
    off64_t head, off64_t tail, bool tail_first)
{
	int fd, bytes = 0, count = 0, reads = 0;
	char *buff = NULL;
	int i;
	size_t offset = 0, copied_bytes = 0;
	off64_t file_size = 0, length = 0;

	bool reading_stdin = dash_represents_stdin && (str_cmp(fname, "-") == 0);
	
	if (reading_stdin) {
		fd = fileno(stdin);
		/* Allow storing the whole UTF-8 character. */
		blen = STR_BOUNDS(1);
	} else
		fd = open(fname, O_RDONLY);
	
	if (fd < 0) {
		printf("Unable to open %s\n", fname);
		return 1;
	}

	if (NULL == (buff = (char *) malloc(blen + 1))) {
		close(fd);
		printf("Unable to allocate enough memory to read %s\n",
			fname);
		return 1;
	}

	if (tail != CAT_FULL_FILE) {
		file_size = lseek(fd, 0, SEEK_END);
		if (head == CAT_FULL_FILE) {
			head = file_size;
			length = tail;
		} else if (tail_first) {
			length = head;
		} else {
			if (tail > head)
				tail = head;
			length = tail;
		}

		if (tail_first) {
			lseek(fd, (tail >= file_size) ? 0 : (file_size - tail), SEEK_SET);
		} else {
			lseek(fd, ((head - tail) >= file_size) ? 0 : (head - tail), SEEK_SET);
		}
	} else
		length = head;

	do {
		size_t bytes_to_read;
		if (reading_stdin) {
			bytes_to_read = 1;
		} else {
			if ((length != CAT_FULL_FILE) &&
			    (length - (off64_t)count <= (off64_t)(blen - copied_bytes))) {
				bytes_to_read = (size_t) (length - count);
			} else {
				bytes_to_read = blen - copied_bytes;
			}
		}
		
		bytes = read(fd, buff + copied_bytes, bytes_to_read);
		bytes += copied_bytes;
		copied_bytes = 0;

		if (bytes > 0) {
			buff[bytes] = '\0';
			offset = 0;
			for (i = 0; i < bytes && !should_quit; i++) {
				if (hex) {
					paged_char(hexchars[((uint8_t)buff[i])/16]);
					paged_char(hexchars[((uint8_t)buff[i])%16]);
					paged_char(((count+i+1) & 0xf) == 0 ? '\n' : ' ');
				}
				else {
					wchar_t c = str_decode(buff, &offset, bytes);
					if (c == 0) {
						/* Reached end of string */
						break;
					} else if (c == U_SPECIAL && offset + 2 >= (size_t)bytes) {
						/* If an extended character is cut off due to the size of the buffer,
						   we will copy it over to the next buffer so it can be read correctly. */
						copied_bytes = bytes - offset + 1;
						memcpy(buff, buff + offset - 1, copied_bytes);
						break;
					}
					paged_char(c);
				}
				
			}
			count += bytes;
			reads++;
		}
		
		if (reading_stdin)
			fflush(stdout);
	} while (bytes > 0 && !should_quit && (count < length || length == CAT_FULL_FILE));

	close(fd);
	if (bytes == -1) {
		printf("Error reading %s\n", fname);
		free(buff);
		return 1;
	}

	free(buff);

	return 0;
}

/* Main entry point for cat, accepts an array of arguments */
int cmd_cat(char **argv)
{
	unsigned int argc, i, ret = 0;
	size_t buffer = 0;
	int c, opt_ind;
	aoff64_t head = CAT_FULL_FILE, tail = CAT_FULL_FILE;
	bool hex = false;
	bool more = false;
	bool tailFirst = false;
	sysarg_t rows, cols;
	int rc;
	
	/*
	 * reset global state
	 * TODO: move to structure?
	 */
	paging_enabled = false;
	chars_remaining = 0;
	lines_remaining = 0;
	console_cols = 0;
	console_rows = 0;
	should_quit = false;
	console = console_init(stdin, stdout);

	argc = cli_count_args(argv);

	for (c = 0, optind = 0, opt_ind = 0; c != -1;) {
		c = getopt_long(argc, argv, "xhvmH:t:b:s", long_options, &opt_ind);
		switch (c) {
		case 'h':
			help_cmd_cat(HELP_LONG);
			return CMD_SUCCESS;
		case 'v':
			printf("%s\n", CAT_VERSION);
			return CMD_SUCCESS;
		case 'H':
			if (!optarg || str_uint64_t(optarg, NULL, 10, false, &head) != EOK ) {
				puts("Invalid head size\n");
				return CMD_FAILURE;
			}
			break;
		case 't':
			if (!optarg || str_uint64_t(optarg, NULL, 10, false, &tail) != EOK ) {
				puts("Invalid tail size\n");
				return CMD_FAILURE;
			}
			if (head == CAT_FULL_FILE)
				tailFirst = true;
			break;
		case 'b':
			if (!optarg || str_size_t(optarg, NULL, 10, false, &buffer) != EOK ) {
				puts("Invalid buffer size\n");
				return CMD_FAILURE;
			}
			break;
		case 'm':
			more = true;
			break;
		case 'x':
			hex = true;
			break;
		case 's':
			dash_represents_stdin = true;
			break;
		}
	}

	argc -= optind;

	if (argc < 1) {
		printf("%s - incorrect number of arguments. Try `%s --help'\n",
			cmdname, cmdname);
		return CMD_FAILURE;
	}

	if (buffer < 4)
		buffer = CAT_DEFAULT_BUFLEN;
	
	if (more) {
		rc = console_get_size(console, &cols, &rows);
		if (rc != EOK) {
			printf("%s - cannot get console size\n", cmdname);
			return CMD_FAILURE;
		}
		console_cols = cols;
		console_rows = rows;
		paging_enabled = true;
		newpage();
	}

	for (i = optind; argv[i] != NULL && !should_quit; i++)
		ret += cat_file(argv[i], buffer, hex, head, tail, tailFirst);

	if (ret)
		return CMD_FAILURE;
	else
		return CMD_SUCCESS;
}

