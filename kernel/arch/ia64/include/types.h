/*
 * Copyright (c) 2005 Jakub Jermar
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

/** @addtogroup ia64
 * @{
 */
/** @file
 */

#ifndef KERN_ia64_TYPES_H_
#define KERN_ia64_TYPES_H_

typedef uint64_t size_t;
typedef int64_t ssize_t;

typedef uint64_t uintptr_t;
typedef uint64_t pfn_t;

typedef uint64_t ipl_t;

typedef uint64_t sysarg_t;
typedef int64_t native_t;
typedef uint64_t atomic_count_t;

typedef struct {
	sysarg_t fnc;
	sysarg_t gp;
} __attribute__((may_alias)) fncptr_t;

#define INTN_C(c)   INT64_C(c)
#define UINTN_C(c)  UINT64_C(c)

#define PRIdn  PRId64  /**< Format for native_t. */
#define PRIun  PRIu64  /**< Format for sysarg_t. */
#define PRIxn  PRIx64  /**< Format for hexadecimal sysarg_t. */
#define PRIua  PRIu64  /**< Format for atomic_count_t. */

#endif

/** @}
 */
