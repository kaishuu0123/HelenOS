/*
 * Copyright (c) 2011 Jiri Zarevucky
 * Copyright (c) 2011 Petr Koupy
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

/** @addtogroup libposix
 * @{
 */
/** @file Additional string manipulation.
 */

#ifndef POSIX_STRINGS_H_
#define POSIX_STRINGS_H_

/* Search Functions */
#ifndef POSIX_STRING_H_
extern int posix_ffs(int i);
#endif

/* String/Array Comparison */
#ifndef POSIX_STRING_H_
extern int posix_strcasecmp(const char *s1, const char *s2);
extern int posix_strncasecmp(const char *s1, const char *s2, size_t n);
#endif

/* TODO: not implemented due to missing locale support
 *
 * int strcasecmp_l(const char *, const char *, locale_t);
 * int strncasecmp_l(const char *, const char *, size_t, locale_t);
 */

/* Legacy Functions */
extern int posix_bcmp(const void *mem1, const void *mem2, size_t n);
extern void posix_bcopy(const void *src, void *dest, size_t n);
extern void posix_bzero(void *mem, size_t n);
extern char *posix_index(const char *s, int c);
extern char *posix_rindex(const char *s, int c);

#ifndef LIBPOSIX_INTERNAL
	#define ffs posix_ffs

	#define strcasecmp posix_strcasecmp
	#define strncasecmp posix_strncasecmp

	#define bcmp posix_bcmp
	#define bcopy posix_bcopy
	#undef bzero
	#define bzero posix_bzero
	#define index posix_index
	#define rindex posix_rindex
#endif

#endif  // POSIX_STRINGS_H_

/** @}
 */
