/*
 * Copyright (c) 2001-2004 Jakub Jermar
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

/** @addtogroup ia32
 * @{
 */
/** @file
 */

#ifndef KERN_ia32_CPUID_H_
#define KERN_ia32_CPUID_H_

#define INTEL_CPUID_LEVEL     0x00000000
#define INTEL_CPUID_STANDARD  0x00000001
#define INTEL_PSE             3
#define INTEL_SEP             11

#ifndef __ASM__

#include <typedefs.h>

typedef struct {
	uint32_t cpuid_eax;
	uint32_t cpuid_ebx;
	uint32_t cpuid_ecx;
	uint32_t cpuid_edx;
} __attribute__ ((packed)) cpu_info_t;

struct __cpuid_extended_feature_info {
	unsigned sse3 :  1;
	unsigned      : 31;
} __attribute__ ((packed));

typedef union cpuid_extended_feature_info {
	struct __cpuid_extended_feature_info bits;
	uint32_t word;
} cpuid_extended_feature_info;

struct __cpuid_feature_info {
	unsigned      : 11;
	unsigned sep  :  1;
	unsigned      : 11;
	unsigned mmx  :  1;
	unsigned fxsr :  1;
	unsigned sse  :  1;
	unsigned sse2 :  1;
	unsigned      :  5;
} __attribute__ ((packed));

typedef union cpuid_feature_info {
	struct __cpuid_feature_info bits;
	uint32_t word;
} cpuid_feature_info;


static inline uint32_t has_cpuid(void)
{
	uint32_t val, ret;
	
	asm volatile (
		"pushf\n"                    /* read flags */
		"popl %[ret]\n"
		"movl %[ret], %[val]\n"
		
		"btcl $21, %[val]\n"         /* swap the ID bit */
		
		"pushl %[val]\n"             /* propagate the change into flags */
		"popf\n"
		"pushf\n"
		"popl %[val]\n"
		
		"andl $(1 << 21), %[ret]\n"  /* interrested only in ID bit */
		"andl $(1 << 21), %[val]\n"
		"xorl %[val], %[ret]\n"
		: [ret] "=r" (ret), [val] "=r" (val)
	);
	
	return ret;
}

static inline void cpuid(uint32_t cmd, cpu_info_t *info)
{
	asm volatile (
		"cpuid\n"
		: "=a" (info->cpuid_eax), "=b" (info->cpuid_ebx),
		  "=c" (info->cpuid_ecx), "=d" (info->cpuid_edx)
		: "a" (cmd)
	);
}

#endif /* !def __ASM__ */
#endif

/** @}
 */
