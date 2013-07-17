/*
 * Copyright (c) 2005 Jakub Jermar
 * Copyright (c) 2009 Pavel Rimsky
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

/** @addtogroup sparc64interrupt
 * @{
 */
/** @file
 */

#include <arch/interrupt.h>
#include <arch/trap/interrupt.h>
#include <arch/sparc64.h>
#include <interrupt.h>
#include <ddi/irq.h>
#include <typedefs.h>
#include <debug.h>
#include <arch/asm.h>
#include <arch/barrier.h>
#include <print.h>
#include <arch.h>
#include <mm/tlb.h>
#include <config.h>
#include <synch/spinlock.h>

/** Register Interrupt Level Handler.
 *
 * @param n       Interrupt Level (1 - 15).
 * @param name    Short descriptive string.
 * @param handler Handler.
 *
 */
void interrupt_register(unsigned int n, const char *name, iroutine_t handler)
{
	ASSERT(n >= IVT_FIRST);
	ASSERT(n <= IVT_ITEMS);
	
	exc_register(n - IVT_FIRST, name, true, handler);
}

/** @}
 */
