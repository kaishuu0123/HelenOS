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

/** @addtogroup sparc64interrupt
 * @{
 */
/**
 * @file
 * @brief This file contains register window trap handlers.
 */

#ifndef KERN_sparc64_REGWIN_H_
#define KERN_sparc64_REGWIN_H_

#include <arch/stack.h>
#include <arch/arch.h>
#include <align.h>

#define TT_CLEAN_WINDOW			0x24
#define TT_SPILL_0_NORMAL		0x80	/* kernel spills */
#define TT_SPILL_1_NORMAL		0x84	/* userspace spills */
#define TT_SPILL_2_NORMAL		0x88	/* spills to userspace window buffer */
#define TT_SPILL_0_OTHER		0xa0	/* spills to userspace window buffer */
#define TT_FILL_0_NORMAL		0xc0	/* kernel fills */
#define TT_FILL_1_NORMAL		0xc4	/* userspace fills */

#define REGWIN_HANDLER_SIZE		128

#define CLEAN_WINDOW_HANDLER_SIZE	REGWIN_HANDLER_SIZE
#define SPILL_HANDLER_SIZE		REGWIN_HANDLER_SIZE
#define FILL_HANDLER_SIZE		REGWIN_HANDLER_SIZE

/* Window Save Area offsets. */
#define L0_OFFSET	0
#define L1_OFFSET	8
#define L2_OFFSET	16
#define L3_OFFSET	24
#define L4_OFFSET	32
#define L5_OFFSET	40
#define L6_OFFSET	48
#define L7_OFFSET	56
#define I0_OFFSET	64
#define I1_OFFSET	72
#define I2_OFFSET	80
#define I3_OFFSET	88
#define I4_OFFSET	96
#define I5_OFFSET	104
#define I6_OFFSET	112
#define I7_OFFSET	120

/* Uspace Window Buffer constants. */
#define UWB_SIZE	((NWINDOWS - 1) * STACK_WINDOW_SAVE_AREA_SIZE)
#define UWB_ALIGNMENT	1024
#define UWB_ASIZE	ALIGN_UP(UWB_SIZE, UWB_ALIGNMENT)

#ifdef __ASM__

/*
 * Macro used by the nucleus and the primary context 0 during normal and other spills.
 */
.macro SPILL_NORMAL_HANDLER_KERNEL
	stx %l0, [%sp + STACK_BIAS + L0_OFFSET]	
	stx %l1, [%sp + STACK_BIAS + L1_OFFSET]
	stx %l2, [%sp + STACK_BIAS + L2_OFFSET]
	stx %l3, [%sp + STACK_BIAS + L3_OFFSET]
	stx %l4, [%sp + STACK_BIAS + L4_OFFSET]
	stx %l5, [%sp + STACK_BIAS + L5_OFFSET]
	stx %l6, [%sp + STACK_BIAS + L6_OFFSET]
	stx %l7, [%sp + STACK_BIAS + L7_OFFSET]
	stx %i0, [%sp + STACK_BIAS + I0_OFFSET]
	stx %i1, [%sp + STACK_BIAS + I1_OFFSET]
	stx %i2, [%sp + STACK_BIAS + I2_OFFSET]
	stx %i3, [%sp + STACK_BIAS + I3_OFFSET]
	stx %i4, [%sp + STACK_BIAS + I4_OFFSET]
	stx %i5, [%sp + STACK_BIAS + I5_OFFSET]
	stx %i6, [%sp + STACK_BIAS + I6_OFFSET]
	stx %i7, [%sp + STACK_BIAS + I7_OFFSET]
	saved
	retry
.endm

/*
 * Macro used by the userspace during normal spills.
 */
.macro SPILL_NORMAL_HANDLER_USERSPACE
	wr %g0, ASI_AIUP, %asi
	stxa %l0, [%sp + STACK_BIAS + L0_OFFSET] %asi
	stxa %l1, [%sp + STACK_BIAS + L1_OFFSET] %asi
	stxa %l2, [%sp + STACK_BIAS + L2_OFFSET] %asi
	stxa %l3, [%sp + STACK_BIAS + L3_OFFSET] %asi
	stxa %l4, [%sp + STACK_BIAS + L4_OFFSET] %asi
	stxa %l5, [%sp + STACK_BIAS + L5_OFFSET] %asi
	stxa %l6, [%sp + STACK_BIAS + L6_OFFSET] %asi
	stxa %l7, [%sp + STACK_BIAS + L7_OFFSET] %asi
	stxa %i0, [%sp + STACK_BIAS + I0_OFFSET] %asi
	stxa %i1, [%sp + STACK_BIAS + I1_OFFSET] %asi
	stxa %i2, [%sp + STACK_BIAS + I2_OFFSET] %asi
	stxa %i3, [%sp + STACK_BIAS + I3_OFFSET] %asi
	stxa %i4, [%sp + STACK_BIAS + I4_OFFSET] %asi
	stxa %i5, [%sp + STACK_BIAS + I5_OFFSET] %asi
	stxa %i6, [%sp + STACK_BIAS + I6_OFFSET] %asi
	stxa %i7, [%sp + STACK_BIAS + I7_OFFSET] %asi
	saved
	retry
.endm

/*
 * Macro used by the nucleus and the primary context 0 during normal fills.
 */
.macro FILL_NORMAL_HANDLER_KERNEL
	ldx [%sp + STACK_BIAS + L0_OFFSET], %l0
	ldx [%sp + STACK_BIAS + L1_OFFSET], %l1
	ldx [%sp + STACK_BIAS + L2_OFFSET], %l2
	ldx [%sp + STACK_BIAS + L3_OFFSET], %l3
	ldx [%sp + STACK_BIAS + L4_OFFSET], %l4
	ldx [%sp + STACK_BIAS + L5_OFFSET], %l5
	ldx [%sp + STACK_BIAS + L6_OFFSET], %l6
	ldx [%sp + STACK_BIAS + L7_OFFSET], %l7
	ldx [%sp + STACK_BIAS + I0_OFFSET], %i0
	ldx [%sp + STACK_BIAS + I1_OFFSET], %i1
	ldx [%sp + STACK_BIAS + I2_OFFSET], %i2
	ldx [%sp + STACK_BIAS + I3_OFFSET], %i3
	ldx [%sp + STACK_BIAS + I4_OFFSET], %i4
	ldx [%sp + STACK_BIAS + I5_OFFSET], %i5
	ldx [%sp + STACK_BIAS + I6_OFFSET], %i6
	ldx [%sp + STACK_BIAS + I7_OFFSET], %i7
	restored
	retry
.endm

/*
 * Macro used by the userspace during normal fills.
 */
.macro FILL_NORMAL_HANDLER_USERSPACE
	wr %g0, ASI_AIUP, %asi
	ldxa [%sp + STACK_BIAS + L0_OFFSET] %asi, %l0
	ldxa [%sp + STACK_BIAS + L1_OFFSET] %asi, %l1
	ldxa [%sp + STACK_BIAS + L2_OFFSET] %asi, %l2
	ldxa [%sp + STACK_BIAS + L3_OFFSET] %asi, %l3
	ldxa [%sp + STACK_BIAS + L4_OFFSET] %asi, %l4
	ldxa [%sp + STACK_BIAS + L5_OFFSET] %asi, %l5
	ldxa [%sp + STACK_BIAS + L6_OFFSET] %asi, %l6
	ldxa [%sp + STACK_BIAS + L7_OFFSET] %asi, %l7
	ldxa [%sp + STACK_BIAS + I0_OFFSET] %asi, %i0
	ldxa [%sp + STACK_BIAS + I1_OFFSET] %asi, %i1
	ldxa [%sp + STACK_BIAS + I2_OFFSET] %asi, %i2
	ldxa [%sp + STACK_BIAS + I3_OFFSET] %asi, %i3
	ldxa [%sp + STACK_BIAS + I4_OFFSET] %asi, %i4
	ldxa [%sp + STACK_BIAS + I5_OFFSET] %asi, %i5
	ldxa [%sp + STACK_BIAS + I6_OFFSET] %asi, %i6
	ldxa [%sp + STACK_BIAS + I7_OFFSET] %asi, %i7
	restored
	retry
.endm

.macro CLEAN_WINDOW_HANDLER
	rdpr %cleanwin, %l0
	add %l0, 1, %l0
	wrpr %l0, 0, %cleanwin
#if defined(SUN4U)
	mov %r0, %l0
	mov %r0, %l1
	mov %r0, %l2
	mov %r0, %l3
	mov %r0, %l4
	mov %r0, %l5
	mov %r0, %l6
	mov %r0, %l7
	mov %r0, %o0
	mov %r0, %o1
	mov %r0, %o2
	mov %r0, %o3
	mov %r0, %o4
	mov %r0, %o5
	mov %r0, %o6
	mov %r0, %o7
#endif
	retry
.endm
#endif /* __ASM__ */

#if defined(SUN4U)
#include <arch/trap/sun4u/regwin.h>
#elif defined(SUN4V)
#include <arch/trap/sun4v/regwin.h>
#endif

#endif

/** @}
 */
