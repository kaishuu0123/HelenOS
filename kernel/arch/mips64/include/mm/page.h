/*
 * Copyright (c) 2003-2004 Jakub Jermar
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

/** @addtogroup mips64mm
 * @{
 */
/** @file
 */

#ifndef KERN_mips64_PAGE_H_
#define KERN_mips64_PAGE_H_

#include <arch/mm/frame.h>
#include <trace.h>

#define PAGE_WIDTH  FRAME_WIDTH
#define PAGE_SIZE   FRAME_SIZE

#ifndef __ASM__
	#define KA2PA(addr)  (((uintptr_t) (addr)) - 0xffffffff80000000)
	#define PA2KA(addr)  (((uintptr_t) (addr)) + 0xffffffff80000000)
#else
	#define KA2PA(addr)  ((addr) - 0xffffffff80000000)
	#define PA2KA(addr)  ((addr) + 0xffffffff80000000)
#endif

#ifndef __ASM__

extern void page_arch_init(void);

#endif /* __ASM__ */

#endif

/** @}
 */
