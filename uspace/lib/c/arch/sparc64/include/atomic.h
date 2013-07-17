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

/** @addtogroup libcsparc64
 * @{
 */
/** @file
 */

#ifndef LIBC_sparc64_ATOMIC_H_
#define LIBC_sparc64_ATOMIC_H_

#define LIBC_ARCH_ATOMIC_H_

#include <atomicdflt.h>
#include <sys/types.h>

/** Atomic add operation.
 *
 * Use atomic compare and swap operation to atomically add signed value.
 *
 * @param val Atomic variable.
 * @param i   Signed value to be added.
 *
 * @return Value of the atomic variable as it existed before addition.
 *
 */
static inline atomic_count_t atomic_add(atomic_t *val, atomic_count_t i)
{
	atomic_count_t a;
	atomic_count_t b;
	
	do {
		volatile uintptr_t ptr = (uintptr_t) &val->count;
		
		a = *((atomic_count_t *) ptr);
		b = a + i;
		
		asm volatile (
			"casx %0, %2, %1\n"
			: "+m" (*((atomic_count_t *) ptr)),
			  "+r" (b)
			: "r" (a)
		);
	} while (a != b);
	
	return a;
}

static inline atomic_count_t atomic_preinc(atomic_t *val)
{
	return atomic_add(val, 1) + 1;
}

static inline atomic_count_t atomic_postinc(atomic_t *val)
{
	return atomic_add(val, 1);
}

static inline atomic_count_t atomic_predec(atomic_t *val)
{
	return atomic_add(val, -1) - 1;
}

static inline atomic_count_t atomic_postdec(atomic_t *val)
{
	return atomic_add(val, -1);
}

static inline void atomic_inc(atomic_t *val)
{
	(void) atomic_add(val, 1);
}

static inline void atomic_dec(atomic_t *val)
{
	(void) atomic_add(val, -1);
}

#endif

/** @}
 */
