/*
 * Copyright (c) 2011 Petr Koupy
 * Copyright (c) 2011 Jiri Zarevucky
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
/** @file String manipulation.
 */

#define LIBPOSIX_INTERNAL

#include "internal/common.h"
#include "string.h"

#include "assert.h"
#include "errno.h"
#include "limits.h"
#include "stdlib.h"
#include "signal.h"

#include "libc/str_error.h"

/**
 * The same as strpbrk, except it returns pointer to the nul terminator
 * if no occurence is found.
 *
 * @param s1 String in which to look for the bytes.
 * @param s2 String of bytes to look for.
 * @return Pointer to the found byte on success, pointer to the
 *     string terminator otherwise.
 */
static char *strpbrk_null(const char *s1, const char *s2)
{
	while (!posix_strchr(s2, *s1)) {
		++s1;
	}
	
	return (char *) s1;
}

/**
 * Copy a string.
 *
 * @param dest Destination pre-allocated buffer.
 * @param src Source string to be copied.
 * @return Pointer to the destination buffer.
 */
char *posix_strcpy(char *restrict dest, const char *restrict src)
{
	posix_stpcpy(dest, src);
	return dest;
}

/**
 * Copy fixed length string.
 *
 * @param dest Destination pre-allocated buffer.
 * @param src Source string to be copied.
 * @param n Number of bytes to be stored into destination buffer.
 * @return Pointer to the destination buffer.
 */
char *posix_strncpy(char *restrict dest, const char *restrict src, size_t n)
{
	posix_stpncpy(dest, src, n);
	return dest;
}

/**
 * Copy a string.
 *
 * @param dest Destination pre-allocated buffer.
 * @param src Source string to be copied.
 * @return Pointer to the nul character in the destination string.
 */
char *posix_stpcpy(char *restrict dest, const char *restrict src)
{
	assert(dest != NULL);
	assert(src != NULL);

	for (size_t i = 0; ; ++i) {
		dest[i] = src[i];
		
		if (src[i] == '\0') {
			/* pointer to the terminating nul character */
			return &dest[i];
		}
	}
	
	/* unreachable */
	return NULL;
}

/**
 * Copy fixed length string.
 *
 * @param dest Destination pre-allocated buffer.
 * @param src Source string to be copied.
 * @param n Number of bytes to be stored into destination buffer.
 * @return Pointer to the first written nul character or &dest[n].
 */
char *posix_stpncpy(char *restrict dest, const char *restrict src, size_t n)
{
	assert(dest != NULL);
	assert(src != NULL);

	for (size_t i = 0; i < n; ++i) {
		dest[i] = src[i];
	
		/* the standard requires that nul characters
		 * are appended to the length of n, in case src is shorter
		 */
		if (src[i] == '\0') {
			char *result = &dest[i];
			for (++i; i < n; ++i) {
				dest[i] = '\0';
			}
			return result;
		}
	}
	
	return &dest[n];
}

/**
 * Concatenate two strings.
 *
 * @param dest String to which src shall be appended.
 * @param src String to be appended after dest.
 * @return Pointer to destination buffer.
 */
char *posix_strcat(char *restrict dest, const char *restrict src)
{
	assert(dest != NULL);
	assert(src != NULL);

	posix_strcpy(posix_strchr(dest, '\0'), src);
	return dest;
}

/**
 * Concatenate a string with part of another.
 *
 * @param dest String to which part of src shall be appended.
 * @param src String whose part shall be appended after dest.
 * @param n Number of bytes to append after dest.
 * @return Pointer to destination buffer.
 */
char *posix_strncat(char *restrict dest, const char *restrict src, size_t n)
{
	assert(dest != NULL);
	assert(src != NULL);

	char *zeroptr = posix_strncpy(posix_strchr(dest, '\0'), src, n);
	/* strncpy doesn't append the nul terminator, so we do it here */
	zeroptr[n] = '\0';
	return dest;
}

/**
 * Copy limited number of bytes in memory.
 *
 * @param dest Destination buffer.
 * @param src Source buffer.
 * @param c Character after which the copying shall stop.
 * @param n Number of bytes that shall be copied if not stopped earlier by c.
 * @return Pointer to the first byte after c in dest if found, NULL otherwise.
 */
void *posix_memccpy(void *restrict dest, const void *restrict src, int c, size_t n)
{
	assert(dest != NULL);
	assert(src != NULL);
	
	unsigned char* bdest = dest;
	const unsigned char* bsrc = src;
	
	for (size_t i = 0; i < n; ++i) {
		bdest[i] = bsrc[i];
	
		if (bsrc[i] == (unsigned char) c) {
			/* pointer to the next byte */
			return &bdest[i + 1];
		}
	}
	
	return NULL;
}

/**
 * Duplicate a string.
 *
 * @param s String to be duplicated.
 * @return Newly allocated copy of the string.
 */
char *posix_strdup(const char *s)
{
	return posix_strndup(s, SIZE_MAX);
}

/**
 * Duplicate a specific number of bytes from a string.
 *
 * @param s String to be duplicated.
 * @param n Maximum length of the resulting string..
 * @return Newly allocated string copy of length at most n.
 */
char *posix_strndup(const char *s, size_t n)
{
	assert(s != NULL);

	size_t len = posix_strnlen(s, n);
	char *dup = malloc(len + 1);
	if (dup == NULL) {
		return NULL;
	}

	memcpy(dup, s, len);
	dup[len] = '\0';

	return dup;
}

/**
 * Compare bytes in memory.
 *
 * @param mem1 First area of memory to be compared.
 * @param mem2 Second area of memory to be compared.
 * @param n Maximum number of bytes to be compared.
 * @return Difference of the first pair of inequal bytes,
 *     or 0 if areas have the same content.
 */
int posix_memcmp(const void *mem1, const void *mem2, size_t n)
{
	assert(mem1 != NULL);
	assert(mem2 != NULL);

	const unsigned char *s1 = mem1;
	const unsigned char *s2 = mem2;
	
	for (size_t i = 0; i < n; ++i) {
		if (s1[i] != s2[i]) {
			return s1[i] - s2[i];
		}
	}
	
	return 0;
}

/**
 * Compare two strings.
 *
 * @param s1 First string to be compared.
 * @param s2 Second string to be compared.
 * @return Difference of the first pair of inequal characters,
 *     or 0 if strings have the same content.
 */
int posix_strcmp(const char *s1, const char *s2)
{
	assert(s1 != NULL);
	assert(s2 != NULL);

	return posix_strncmp(s1, s2, STR_NO_LIMIT);
}

/**
 * Compare part of two strings.
 *
 * @param s1 First string to be compared.
 * @param s2 Second string to be compared.
 * @param n Maximum number of characters to be compared.
 * @return Difference of the first pair of inequal characters,
 *     or 0 if strings have the same content.
 */
int posix_strncmp(const char *s1, const char *s2, size_t n)
{
	assert(s1 != NULL);
	assert(s2 != NULL);

	for (size_t i = 0; i < n; ++i) {
		if (s1[i] != s2[i]) {
			return s1[i] - s2[i];
		}
		if (s1[i] == '\0') {
			break;
		}
	}

	return 0;
}

/**
 * Find byte in memory.
 *
 * @param mem Memory area in which to look for the byte.
 * @param c Byte to look for.
 * @param n Maximum number of bytes to be inspected.
 * @return Pointer to the specified byte on success,
 *     NULL pointer otherwise.
 */
void *posix_memchr(const void *mem, int c, size_t n)
{
	assert(mem != NULL);
	
	const unsigned char *s = mem;
	
	for (size_t i = 0; i < n; ++i) {
		if (s[i] == (unsigned char) c) {
			return (void *) &s[i];
		}
	}
	return NULL;
}

/**
 * Scan string for a first occurence of a character.
 *
 * @param s String in which to look for the character.
 * @param c Character to look for.
 * @return Pointer to the specified character on success,
 *     NULL pointer otherwise.
 */
char *posix_strchr(const char *s, int c)
{
	assert(s != NULL);
	
	char *res = gnu_strchrnul(s, c);
	return (*res == c) ? res : NULL;
}

/**
 * Scan string for a last occurence of a character.
 *
 * @param s String in which to look for the character.
 * @param c Character to look for.
 * @return Pointer to the specified character on success,
 *     NULL pointer otherwise.
 */
char *posix_strrchr(const char *s, int c)
{
	assert(s != NULL);
	
	const char *ptr = posix_strchr(s, '\0');
	
	/* the same as in strchr, except it loops in reverse direction */
	while (*ptr != (char) c) {
		if (ptr == s) {
			return NULL;
		}

		ptr--;
	}

	return (char *) ptr;
}

/**
 * Scan string for a first occurence of a character.
 *
 * @param s String in which to look for the character.
 * @param c Character to look for.
 * @return Pointer to the specified character on success, pointer to the
 *     string terminator otherwise.
 */
char *gnu_strchrnul(const char *s, int c)
{
	assert(s != NULL);
	
	while (*s != c && *s != '\0') {
		s++;
	}
	
	return (char *) s;
}

/**
 * Scan a string for a first occurence of one of provided bytes.
 *
 * @param s1 String in which to look for the bytes.
 * @param s2 String of bytes to look for.
 * @return Pointer to the found byte on success,
 *     NULL pointer otherwise.
 */
char *posix_strpbrk(const char *s1, const char *s2)
{
	assert(s1 != NULL);
	assert(s2 != NULL);

	char *ptr = strpbrk_null(s1, s2);
	return (*ptr == '\0') ? NULL : ptr;
}

/**
 * Get the length of a complementary substring.
 *
 * @param s1 String that shall be searched for complementary prefix.
 * @param s2 String of bytes that shall not occur in the prefix.
 * @return Length of the prefix.
 */
size_t posix_strcspn(const char *s1, const char *s2)
{
	assert(s1 != NULL);
	assert(s2 != NULL);

	char *ptr = strpbrk_null(s1, s2);
	return (size_t) (ptr - s1);
}

/**
 * Get length of a substring.
 *
 * @param s1 String that shall be searched for prefix.
 * @param s2 String of bytes that the prefix must consist of.
 * @return Length of the prefix.
 */
size_t posix_strspn(const char *s1, const char *s2)
{
	assert(s1 != NULL);
	assert(s2 != NULL);

	const char *ptr;
	for (ptr = s1; *ptr != '\0'; ++ptr) {
		if (!posix_strchr(s2, *ptr)) {
			break;
		}
	}
	return ptr - s1;
}

/**
 * Find a substring. Uses Knuth-Morris-Pratt algorithm.
 *
 * @param s1 String in which to look for a substring.
 * @param s2 Substring to look for.
 * @return Pointer to the first character of the substring in s1, or NULL if
 *     not found.
 */
char *posix_strstr(const char *haystack, const char *needle)
{
	assert(haystack != NULL);
	assert(needle != NULL);
	
	/* Special case - needle is an empty string. */
	if (needle[0] == '\0') {
		return (char *) haystack;
	}
	
	/* Preprocess needle. */
	size_t nlen = posix_strlen(needle);
	size_t prefix_table[nlen + 1];
	
	{
		size_t i = 0;
		ssize_t j = -1;
		
		prefix_table[i] = j;
		
		while (i < nlen) {
			while (j >= 0 && needle[i] != needle[j]) {
				j = prefix_table[j];
			}
			i++; j++;
			prefix_table[i] = j;
		}
	}
	
	/* Search needle using the precomputed table. */
	size_t npos = 0;
	
	for (size_t hpos = 0; haystack[hpos] != '\0'; ++hpos) {
		while (npos != 0 && haystack[hpos] != needle[npos]) {
			npos = prefix_table[npos];
		}
		
		if (haystack[hpos] == needle[npos]) {
			npos++;
			
			if (npos == nlen) {
				return (char *) (haystack + hpos - nlen + 1);
			}
		}
	}
	
	return NULL;
}

/**
 * String comparison using collating information.
 *
 * Currently ignores locale and just calls strcmp.
 *
 * @param s1 First string to be compared.
 * @param s2 Second string to be compared.
 * @return Difference of the first pair of inequal characters,
 *     or 0 if strings have the same content.
 */
int posix_strcoll(const char *s1, const char *s2)
{
	assert(s1 != NULL);
	assert(s2 != NULL);

	return posix_strcmp(s1, s2);
}

/**
 * Transform a string in such a way that the resulting string yields the same
 * results when passed to the strcmp as if the original string is passed to
 * the strcoll.
 *
 * Since strcoll is equal to strcmp here, this just makes a copy.
 *
 * @param s1 Transformed string.
 * @param s2 Original string.
 * @param n Maximum length of the transformed string.
 * @return Length of the transformed string.
 */
size_t posix_strxfrm(char *restrict s1, const char *restrict s2, size_t n)
{
	assert(s1 != NULL || n == 0);
	assert(s2 != NULL);

	size_t len = posix_strlen(s2);

	if (n > len) {
		posix_strcpy(s1, s2);
	}

	return len;
}

/**
 * Get error message string.
 *
 * @param errnum Error code for which to obtain human readable string.
 * @return Error message.
 */
char *posix_strerror(int errnum)
{
	static const char *error_msgs[] = {
		[E2BIG] = "[E2BIG] Argument list too long",
		[EACCES] = "[EACCES] Permission denied",
		[EADDRINUSE] = "[EADDRINUSE] Address in use",
		[EADDRNOTAVAIL] = "[EADDRNOTAVAIL] Address not available",
		[EAFNOSUPPORT] = "[EAFNOSUPPORT] Address family not supported",
		[EAGAIN] = "[EAGAIN] Resource unavailable, try again",
		[EALREADY] = "[EALREADY] Connection already in progress",
		[EBADF] = "[EBADF] Bad file descriptor",
		[EBADMSG] = "[EBADMSG] Bad message",
		[EBUSY] = "[EBUSY] Device or resource busy",
		[ECANCELED] = "[ECANCELED] Operation canceled",
		[ECHILD] = "[ECHILD] No child processes",
		[ECONNABORTED] = "[ECONNABORTED] Connection aborted",
		[ECONNREFUSED] = "[ECONNREFUSED] Connection refused",
		[ECONNRESET] = "[ECONNRESET] Connection reset",
		[EDEADLK] = "[EDEADLK] Resource deadlock would occur",
		[EDESTADDRREQ] = "[EDESTADDRREQ] Destination address required",
		[EDOM] = "[EDOM] Mathematics argument out of domain of function",
		[EDQUOT] = "[EDQUOT] Reserved",
		[EEXIST] = "[EEXIST] File exists",
		[EFAULT] = "[EFAULT] Bad address",
		[EFBIG] = "[EFBIG] File too large",
		[EHOSTUNREACH] = "[EHOSTUNREACH] Host is unreachable",
		[EIDRM] = "[EIDRM] Identifier removed",
		[EILSEQ] = "[EILSEQ] Illegal byte sequence",
		[EINPROGRESS] = "[EINPROGRESS] Operation in progress",
		[EINTR] = "[EINTR] Interrupted function",
		[EINVAL] = "[EINVAL] Invalid argument",
		[EIO] = "[EIO] I/O error",
		[EISCONN] = "[EISCONN] Socket is connected",
		[EISDIR] = "[EISDIR] Is a directory",
		[ELOOP] = "[ELOOP] Too many levels of symbolic links",
		[EMFILE] = "[EMFILE] File descriptor value too large",
		[EMLINK] = "[EMLINK] Too many links",
		[EMSGSIZE] = "[EMSGSIZE] Message too large",
		[EMULTIHOP] = "[EMULTIHOP] Reserved",
		[ENAMETOOLONG] = "[ENAMETOOLONG] Filename too long",
		[ENETDOWN] = "[ENETDOWN] Network is down",
		[ENETRESET] = "[ENETRESET] Connection aborted by network",
		[ENETUNREACH] = "[ENETUNREACH] Network unreachable",
		[ENFILE] = "[ENFILE] Too many files open in system",
		[ENOBUFS] = "[ENOBUFS] No buffer space available",
		[ENODATA] = "[ENODATA] No message is available on the STREAM head read queue",
		[ENODEV] = "[ENODEV] No such device",
		[ENOENT] = "[ENOENT] No such file or directory",
		[ENOEXEC] = "[ENOEXEC] Executable file format error",
		[ENOLCK] = "[ENOLCK] No locks available",
		[ENOLINK] = "[ENOLINK] Reserved",
		[ENOMEM] = "[ENOMEM] Not enough space",
		[ENOMSG] = "[ENOMSG] No message of the desired type",
		[ENOPROTOOPT] = "[ENOPROTOOPT] Protocol not available",
		[ENOSPC] = "[ENOSPC] No space left on device",
		[ENOSR] = "[ENOSR] No STREAM resources.",
		[ENOSTR] = "[ENOSTR] Not a STREAM",
		[ENOSYS] = "[ENOSYS] Function not supported",
		[ENOTCONN] = "[ENOTCONN] The socket is not connected",
		[ENOTDIR] = "[ENOTDIR] Not a directory",
		[ENOTEMPTY] = "[ENOTEMPTY] Directory not empty",
		[ENOTRECOVERABLE] = "[ENOTRECOVERABLE] State not recoverable",
		[ENOTSOCK] = "[ENOTSOCK] Not a socket",
		[ENOTSUP] = "[ENOTSUP] Not supported",
		[ENOTTY] = "[ENOTTY] Inappropriate I/O control operation",
		[ENXIO] = "[ENXIO] No such device or address",
		[EOPNOTSUPP] = "[EOPNOTSUPP] Operation not supported",
		[EOVERFLOW] = "[EOVERFLOW] Value too large to be stored in data type",
		[EOWNERDEAD] = "[EOWNERDEAD] Previous owned died",
		[EPERM] = "[EPERM] Operation not permitted",
		[EPIPE] = "[EPIPE] Broken pipe",
		[EPROTO] = "[EPROTO] Protocol error",
		[EPROTONOSUPPORT] = "[EPROTONOSUPPORT] Protocol not supported",
		[EPROTOTYPE] = "[EPROTOTYPE] Protocol wrong type for socket",
		[ERANGE] = "[ERANGE] Result too large",
		[EROFS] = "[EROFS] Read-only file system",
		[ESPIPE] = "[ESPIPE] Invalid seek",
		[ESRCH] = "[ESRCH] No such process",
		[ESTALE] = "[ESTALE] Reserved",
		[ETIME] = "[ETIME] Stream ioctl() timeout",
		[ETIMEDOUT] = "[ETIMEDOUT] Connection timed out",
		[ETXTBSY] = "[ETXTBSY] Text file busy",
		[EWOULDBLOCK] = "[EWOULDBLOCK] Operation would block",
		[EXDEV] = "[EXDEV] Cross-device link",
	};

	return (char *) error_msgs[posix_abs(errnum)];
}

/**
 * Get error message string.
 *
 * @param errnum Error code for which to obtain human readable string.
 * @param buf Buffer to store a human readable string to.
 * @param bufsz Size of buffer pointed to by buf.
 * @return Zero on success, errno otherwise.
 */
int posix_strerror_r(int errnum, char *buf, size_t bufsz)
{
	assert(buf != NULL);
	
	char *errstr = posix_strerror(errnum);
	
	if (posix_strlen(errstr) + 1 > bufsz) {
		return ERANGE;
	} else {
		posix_strcpy(buf, errstr);
	}

	return 0;
}

/**
 * Get length of the string.
 *
 * @param s String which length shall be determined.
 * @return Length of the string.
 */
size_t posix_strlen(const char *s)
{
	assert(s != NULL);
	
	return (size_t) (posix_strchr(s, '\0') - s);
}

/**
 * Get limited length of the string.
 *
 * @param s String which length shall be determined.
 * @param n Maximum number of bytes that can be examined to determine length.
 * @return The lower of either string length or n limit.
 */
size_t posix_strnlen(const char *s, size_t n)
{
	assert(s != NULL);
	
	for (size_t sz = 0; sz < n; ++sz) {
		
		if (s[sz] == '\0') {
			return sz;
		}
	}
	
	return n;
}

/**
 * Get description of a signal.
 *
 * @param signum Signal number.
 * @return Human readable signal description.
 */
char *posix_strsignal(int signum)
{
	static const char *const sigstrings[] = {
		[SIGABRT] = "SIGABRT (Process abort signal)",
		[SIGALRM] = "SIGALRM (Alarm clock)",
		[SIGBUS] = "SIGBUS (Access to an undefined portion of a memory object)",
		[SIGCHLD] = "SIGCHLD (Child process terminated, stopped, or continued)",
		[SIGCONT] = "SIGCONT (Continue executing, if stopped)",
		[SIGFPE] = "SIGFPE (Erroneous arithmetic operation)",
		[SIGHUP] = "SIGHUP (Hangup)",
		[SIGILL] = "SIGILL (Illegal instruction)",
		[SIGINT] = "SIGINT (Terminal interrupt signal)",
		[SIGKILL] = "SIGKILL (Kill process)",
		[SIGPIPE] = "SIGPIPE (Write on a pipe with no one to read it)",
		[SIGQUIT] = "SIGQUIT (Terminal quit signal)",
		[SIGSEGV] = "SIGSEGV (Invalid memory reference)",
		[SIGSTOP] = "SIGSTOP (Stop executing)",
		[SIGTERM] = "SIGTERM (Termination signal)",
		[SIGTSTP] = "SIGTSTP (Terminal stop signal)",
		[SIGTTIN] = "SIGTTIN (Background process attempting read)",
		[SIGTTOU] = "SIGTTOU (Background process attempting write)",
		[SIGUSR1] = "SIGUSR1 (User-defined signal 1)",
		[SIGUSR2] = "SIGUSR2 (User-defined signal 2)",
		[SIGPOLL] = "SIGPOLL (Pollable event)",
		[SIGPROF] = "SIGPROF (Profiling timer expired)",
		[SIGSYS] = "SIGSYS (Bad system call)",
		[SIGTRAP] = "SIGTRAP (Trace/breakpoint trap)",
		[SIGURG] = "SIGURG (High bandwidth data is available at a socket)",
		[SIGVTALRM] = "SIGVTALRM (Virtual timer expired)",
		[SIGXCPU] = "SIGXCPU (CPU time limit exceeded)",
		[SIGXFSZ] = "SIGXFSZ (File size limit exceeded)"
	};

	if (signum <= _TOP_SIGNAL) {
		return (char *) sigstrings[signum];
	}

	return (char *) "ERROR, Invalid signal number";
}

/** @}
 */
