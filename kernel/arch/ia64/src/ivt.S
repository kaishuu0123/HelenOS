#
# Copyright (c) 2005 Jakub Vana
# Copyright (c) 2005 Jakub Jermar
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# - Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
# - Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
# - The name of the author may not be used to endorse or promote products
#   derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

#include <arch/stack.h>
#include <arch/register.h>
#include <arch/mm/page.h>
#include <align.h>

#define FRS_TO_SAVE 30
#define STACK_ITEMS		(21 + FRS_TO_SAVE * 2)
#define STACK_FRAME_SIZE	ALIGN_UP((STACK_ITEMS*STACK_ITEM_SIZE) + STACK_SCRATCH_AREA_SIZE, STACK_ALIGNMENT)

#if (STACK_ITEMS % 2 == 0)
#	define STACK_FRAME_BIAS	8
#else
#	define STACK_FRAME_BIAS 16
#endif

/** Partitioning of bank 0 registers. */
#define R_OFFS 		r16
#define R_HANDLER	r17
#define R_RET		r18
#define R_TMP		r19
#define R_KSTACK_BSP	r22	/* keep in sync with before_thread_runs_arch() */
#define R_KSTACK	r23	/* keep in sync with before_thread_runs_arch() */

/* Speculation vector handler */
.macro SPECULATION_VECTOR_HANDLER offs
    .org ivt + \offs

    /* 1. Save predicates, IIM, IIP, IPSR and ISR CR's in bank 0 registers. */
	mov r16 = pr
	mov r17 = cr.iim
	mov r18 = cr.iip
	mov r19 = cr.ipsr
	mov r20 = cr.isr ;;
	
    /* 2. Move IIP to IIPA. */
	mov cr.iipa = r18
	
    /* 3. Sign extend IIM[20:0], shift left by 4 and add to IIP. */
	shl r17 = r17, 43 ;;	/* shift bit 20 to bit 63 */
	shr r17 = r17, 39 ;;	/* signed shift right to bit 24 */
	add r18 = r18, r17 ;;
	mov cr.iip = r18
	
    /* 4. Set IPSR.ri to 0. */
	dep r19 = 0, r19, PSR_RI_SHIFT, PSR_RI_LEN ;;
	mov cr.ipsr = r19
	
    /* 5. Check whether IPSR.tb or IPSR.ss is set. */

	/* TODO:
	 * Implement this when Taken Branch and Single Step traps can occur.
	 */
    
    /* 6. Restore predicates and return from interruption. */
	mov pr = r16 ;;
	rfi
.endm

/** Heavyweight interrupt handler
 *
 * This macro roughly follows steps from 1 to 19 described in
 * Intel Itanium Architecture Software Developer's Manual, Chapter 3.4.2.
 *
 * HEAVYWEIGHT_HANDLER macro must cram into 16 bundles (48 instructions).
 * This goal is achieved by using procedure calls after RSE becomes operational.
 *
 * Some steps are skipped (enabling and disabling interrupts).
 *
 * @param offs Offset from the beginning of IVT.
 * @param handler Interrupt handler address.
 */
.macro HEAVYWEIGHT_HANDLER offs, handler=universal_handler
    .org ivt + \offs
	mov R_OFFS = \offs
	movl R_HANDLER = \handler ;;
	br heavyweight_handler
.endm

.global heavyweight_handler
heavyweight_handler:
    /* 1. copy interrupt registers into bank 0 */
    
	/*
	 * Note that r24-r31 from bank 0 can be used only as long as PSR.ic = 0.
	 */
	
	/* Set up FPU as in interrupted context. */
	mov r24 = psr
	mov r25 = cr.ipsr 
	mov r26 = PSR_DFH_MASK
	mov r27 = ~PSR_DFH_MASK ;;
	and r26 = r25, r26
	and r24 = r24, r27;;
	or r24 = r24, r26;;
	mov psr.l = r24;;
	srlz.i
	srlz.d;;

	mov r24 = cr.iip
	mov r25 = cr.ipsr
	mov r26 = cr.iipa
	mov r27 = cr.isr
	mov r28 = cr.ifa
	
    /* 2. preserve predicate register into bank 0 */
	mov r29 = pr ;;
	
    /* 3. switch to kernel memory stack */
    	mov r30 = cr.ipsr
	shr.u r31 = r12, VRN_SHIFT ;;

	shr.u r30 = r30, PSR_CPL_SHIFT ;;
	and r30 = PSR_CPL_MASK_SHIFTED, r30 ;;

	/*
	 * Set p3 to true if the interrupted context executed in kernel mode.
	 * Set p4 to false if the interrupted context didn't execute in kernel mode.
	 */
	cmp.eq p3, p4 = r30, r0 ;;
	cmp.eq p1, p2 = r30, r0 ;;	/* remember IPSR setting in p1 and p2 */

	/*
	 * Set p3 to true if the stack register references kernel address space.
	 * Set p4 to false if the stack register doesn't reference kernel address space.
	 */
(p3)	cmp.eq p3, p4 = VRN_KERNEL, r31 ;;
	
	/*
	 * Now, p4 is true iff the stack needs to be switched to kernel stack.
	 */
	mov r30 = r12
(p4)	mov r12 = R_KSTACK ;;
	
	add r31 = -STACK_FRAME_BIAS, r12 ;;
	add r12 = -STACK_FRAME_SIZE, r12

    /* 4. save registers in bank 0 into memory stack */

	/*
	 * If this is break_instruction handler,
	 * copy input parameters to stack.
	 */
    	mov R_TMP = 0x2c00 ;;
	cmp.eq p6, p5 = R_OFFS, R_TMP ;;
	
	/*
	 * From now on, if this is break_instruction handler, p6 is true and p5
	 * is false. Otherwise p6 is false and p5 is true.
	 * Note that p5 is a preserved predicate register and we make use of it.
	 */

(p6)	st8 [r31] = r38, -8 ;;		/* save in6 */
(p6)	st8 [r31] = r37, -8 ;;		/* save in5 */
(p6)	st8 [r31] = r36, -8 ;;		/* save in4 */	
(p6)	st8 [r31] = r35, -8 ;;		/* save in3 */
(p6) 	st8 [r31] = r34, -8 ;;		/* save in2 */
(p6)	st8 [r31] = r33, -8 ;;		/* save in1 */
(p6)	st8 [r31] = r32, -8 ;;		/* save in0 */
(p5)	add r31 = -56, r31 ;;
    
	st8 [r31] = r30, -8 ;;		/* save old stack pointer */ 
	
	st8 [r31] = r29, -8 ;;		/* save predicate registers */

	st8 [r31] = r24, -8 ;;		/* save cr.iip */
	st8 [r31] = r25, -8 ;;		/* save cr.ipsr */
	st8 [r31] = r26, -8 ;;		/* save cr.iipa */
	st8 [r31] = r27, -8 ;;		/* save cr.isr */
	st8 [r31] = r28, -8 ;;		/* save cr.ifa */

    /* 5. RSE switch from interrupted context */
	mov r24 = ar.rsc
	mov r25 = ar.pfs
	cover
	mov r26 = cr.ifs
	
	st8 [r31] = r24, -8 ;;		/* save ar.rsc */
	st8 [r31] = r25, -8 ;;		/* save ar.pfs */
	st8 [r31] = r26, -8		/* save ar.ifs */
	
	and r24 = ~(RSC_PL_MASK), r24 ;;
	and r30 = ~(RSC_MODE_MASK), r24 ;;
	mov ar.rsc = r30 ;;		/* update RSE state */
	
	mov r27 = ar.rnat
	mov r28 = ar.bspstore ;;
	
	/*
	 * Inspect BSPSTORE to figure out whether it is necessary to switch to
	 * kernel BSPSTORE.
	 */
(p1)	shr.u r30 = r28, VRN_SHIFT ;;
(p1)	cmp.eq p1, p2 = VRN_KERNEL, r30 ;;
	
	/*
	 * If BSPSTORE needs to be switched, p1 is false and p2 is true.
	 */
(p1)	mov r30 = r28
(p2)	mov r30 = R_KSTACK_BSP ;;
(p2)	mov ar.bspstore = r30 ;;
	
	mov r29 = ar.bsp
	
	st8 [r31] = r27, -8 ;;		/* save ar.rnat */
	st8 [r31] = r30, -8 ;;		/* save new value written to ar.bspstore */
	st8 [r31] = r28, -8 ;;		/* save ar.bspstore */
	st8 [r31] = r29, -8 		/* save ar.bsp */
	
	mov ar.rsc = r24		/* restore RSE's setting + kernel privileges */
	
    /* steps 6 - 15 are done by heavyweight_handler_inner() */
	mov R_RET = b0 			/* save b0 belonging to interrupted context */
	br.call.sptk.many b0 = heavyweight_handler_inner
0:	mov b0 = R_RET			/* restore b0 belonging to the interrupted context */

    /* 16. RSE switch to interrupted context */
	cover				/* allocate zero size frame (step 1 (from Intel Docs)) */

	add r31 = (STACK_SCRATCH_AREA_SIZE + (FRS_TO_SAVE * 2 * 8)), r12 ;;

	ld8 r30 = [r31], +8 ;;		/* load ar.bsp */
	ld8 r29 = [r31], +8 ;;   	/* load ar.bspstore */
	ld8 r28 = [r31], +8 ;;   	/* load ar.bspstore_new */
	sub r27 = r30 , r28 ;;		/* calculate loadrs (step 2) */
	shl r27 = r27, 16

	mov r24 = ar.rsc ;;
	and r30 = ~3, r24 ;;
	or  r24 = r30 , r27 ;;     
	mov ar.rsc = r24 ;;		/* place RSE in enforced lazy mode */

	loadrs 				/* (step 3) */

	ld8 r27 = [r31], +8 ;;		/* load ar.rnat */
	ld8 r26 = [r31], +8 ;;		/* load cr.ifs */
	ld8 r25 = [r31], +8 ;;		/* load ar.pfs */
	ld8 r24 = [r31], +8 ;;		/* load ar.rsc */

	mov ar.bspstore = r29 ;;	/* (step 4) */
	mov ar.rnat = r27		/* (step 5) */

	mov ar.pfs = r25		/* (step 6) */
	mov cr.ifs = r26	

	mov ar.rsc = r24		/* (step 7) */

    /* 17. restore interruption state from memory stack */
	ld8 r28 = [r31], +8 ;;		/* load cr.ifa */		
	ld8 r27 = [r31], +8 ;;		/* load cr.isr */
	ld8 r26 = [r31], +8 ;;		/* load cr.iipa */
	ld8 r25 = [r31], +8 ;;		/* load cr.ipsr */
	ld8 r24 = [r31], +8 ;;		/* load cr.iip */

	mov cr.iip = r24;;
	mov cr.iipa = r26
	mov cr.isr = r27
	mov cr.ifa = r28

	/* Set up FPU as in exception. */
	mov r24 = psr
	mov r26 = PSR_DFH_MASK
	mov r27 = ~PSR_DFH_MASK ;;
	and r25 = r25, r27
	and r24 = r24, r26 ;;
	or r25 = r25, r24;;
	mov cr.ipsr = r25

    /* 18. restore predicate registers from memory stack */
	ld8 r29 = [r31], +8 ;;		/* load predicate registers */
	mov pr = r29
	
    /* 19. return from interruption */
    	ld8 r12 = [r31]			/* load stack pointer */ 
	rfi ;;

.global heavyweight_handler_inner
heavyweight_handler_inner:
	/*
	 * From this point, the rest of the interrupted context
	 * will be preserved in stacked registers and backing store.
	 */
	alloc loc0 = ar.pfs, 0, 48, 2, 0 ;;
	
	/* bank 0 is going to be shadowed, copy essential data from there */
	mov loc1 = R_RET	/* b0 belonging to interrupted context */
	mov loc2 = R_HANDLER
	mov out0 = R_OFFS
	
	add out1 = STACK_SCRATCH_AREA_SIZE, r12

    /* 6. switch to bank 1 and reenable PSR.ic */
	ssm PSR_IC_MASK
	bsw.1 ;;
	srlz.d
	
    /* 7. preserve branch and application registers */
    	mov loc3 = ar.unat
	mov loc4 = ar.lc
	mov loc5 = ar.ec
	mov loc6 = ar.ccv
	mov loc7 = ar.csd
	mov loc8 = ar.ssd
	
	mov loc9 = b0
	mov loc10 = b1
	mov loc11 = b2
	mov loc12 = b3
	mov loc13 = b4
	mov loc14 = b5
	mov loc15 = b6
	mov loc16 = b7
	
    /* 8. preserve general and floating-point registers */
	mov loc17 = r1
	mov loc18 = r2
	mov loc19 = r3
	mov loc20 = r4
	mov loc21 = r5
	mov loc22 = r6
	mov loc23 = r7
(p5)	mov loc24 = r8		/* only if not in break_instruction handler */
	mov loc25 = r9
	mov loc26 = r10
	mov loc27 = r11
	/* skip r12 (stack pointer) */
	mov loc28 = r13
	mov loc29 = r14
	mov loc30 = r15
	mov loc31 = r16
	mov loc32 = r17
	mov loc33 = r18
	mov loc34 = r19
	mov loc35 = r20
	mov loc36 = r21
	mov loc37 = r22
	mov loc38 = r23
	mov loc39 = r24
	mov loc40 = r25
	mov loc41 = r26
	mov loc42 = r27
	mov loc43 = r28
	mov loc44 = r29
	mov loc45 = r30
	mov loc46 = r31

	add r24 = 96 + STACK_SCRATCH_AREA_SIZE, r12
	add r25 = 112 + STACK_SCRATCH_AREA_SIZE, r12
	add r26 = 0 + STACK_SCRATCH_AREA_SIZE, r12
	add r27 = 16 + STACK_SCRATCH_AREA_SIZE, r12
	add r28 = 32 + STACK_SCRATCH_AREA_SIZE, r12
	add r29 = 48 + STACK_SCRATCH_AREA_SIZE, r12
	add r30 = 64 + STACK_SCRATCH_AREA_SIZE, r12
	add r31 = 80 + STACK_SCRATCH_AREA_SIZE, r12 ;;
	
	stf.spill [r26] = f2, 0x80
	stf.spill [r27] = f3, 0x80
	stf.spill [r28] = f4, 0x80
	stf.spill [r29] = f5, 0x80
	stf.spill [r30] = f6, 0x80
	stf.spill [r31] = f7, 0x80 ;;

	stf.spill [r24] = f8, 0x80
	stf.spill [r25] = f9, 0x80
	stf.spill [r26] = f10, 0x80
	stf.spill [r27] = f11, 0x80
	stf.spill [r28] = f12, 0x80
	stf.spill [r29] = f13, 0x80
	stf.spill [r30] = f14, 0x80
	stf.spill [r31] = f15, 0x80 ;;

	stf.spill [r24] = f16, 0x80
	stf.spill [r25] = f17, 0x80
	stf.spill [r26] = f18, 0x80
	stf.spill [r27] = f19, 0x80
	stf.spill [r28] = f20, 0x80
	stf.spill [r29] = f21, 0x80
	stf.spill [r30] = f22, 0x80
	stf.spill [r31] = f23, 0x80 ;;

	stf.spill [r24] = f24, 0x80
	stf.spill [r25] = f25, 0x80
	stf.spill [r26] = f26, 0x80
	stf.spill [r27] = f27, 0x80
	stf.spill [r28] = f28, 0x80
	stf.spill [r29] = f29, 0x80
	stf.spill [r30] = f30, 0x80
	stf.spill [r31] = f31, 0x80 ;;

	mov loc47 = ar.fpsr	/* preserve floating point status register */
    
    /* 9. skipped (will not enable interrupts) */
	/*
    	 * ssm PSR_I_MASK
	 * ;;
	 * srlz.d
	 */

    /* 10. call handler */
    	movl r1 = __gp
    
    	mov b1 = loc2
	br.call.sptk.many b0 = b1

    /* 11. return from handler */
0:
	
    /* 12. skipped (will not disable interrupts) */
	/*
    	 * rsm PSR_I_MASK
	 * ;;
	 * srlz.d
	 */

    /* 13. restore general and floating-point registers */
	add r24 = 96 + STACK_SCRATCH_AREA_SIZE, r12
	add r25 = 112 + STACK_SCRATCH_AREA_SIZE, r12
	add r26 = 0 + STACK_SCRATCH_AREA_SIZE, r12
	add r27 = 16 + STACK_SCRATCH_AREA_SIZE, r12
	add r28 = 32 + STACK_SCRATCH_AREA_SIZE, r12
	add r29 = 48 + STACK_SCRATCH_AREA_SIZE, r12
	add r30 = 64 + STACK_SCRATCH_AREA_SIZE, r12
	add r31 = 80 + STACK_SCRATCH_AREA_SIZE, r12 ;;

	ldf.fill f2 = [r26], 0x80
	ldf.fill f3 = [r27], 0x80
	ldf.fill f4 = [r28], 0x80
	ldf.fill f5 = [r29], 0x80
	ldf.fill f6 = [r30], 0x80
	ldf.fill f7 = [r31], 0x80 ;;

	ldf.fill f8 = [r24], 0x80
	ldf.fill f9 = [r25], 0x80
	ldf.fill f10 = [r26], 0x80
	ldf.fill f11 = [r27], 0x80
	ldf.fill f12 = [r28], 0x80
	ldf.fill f13 = [r29], 0x80
	ldf.fill f14 = [r30], 0x80
	ldf.fill f15 = [r31], 0x80 ;;

	ldf.fill f16 = [r24], 0x80
	ldf.fill f17 = [r25], 0x80
	ldf.fill f18 = [r26], 0x80
	ldf.fill f19 = [r27], 0x80
	ldf.fill f20 = [r28], 0x80
	ldf.fill f21 = [r29], 0x80
	ldf.fill f22 = [r30], 0x80
	ldf.fill f23 = [r31], 0x80 ;;

	ldf.fill f24 = [r24], 0x80
	ldf.fill f25 = [r25], 0x80
	ldf.fill f26 = [r26], 0x80
	ldf.fill f27 = [r27], 0x80
	ldf.fill f28 = [r28], 0x80
	ldf.fill f29 = [r29], 0x80
	ldf.fill f30 = [r30], 0x80
	ldf.fill f31 = [r31], 0x80 ;;
	
	mov r1 = loc17
	mov r2 = loc18
	mov r3 = loc19
	mov r4 = loc20
	mov r5 = loc21
	mov r6 = loc22
	mov r7 = loc23
(p5)	mov r8 = loc24		/* only if not in break_instruction handler */
	mov r9 = loc25
	mov r10 = loc26
	mov r11 = loc27
	/* skip r12 (stack pointer) */
	mov r13 = loc28
	mov r14 = loc29
	mov r15 = loc30
	mov r16 = loc31
	mov r17 = loc32
	mov r18 = loc33
	mov r19 = loc34
	mov r20 = loc35
	mov r21 = loc36
	mov r22 = loc37
	mov r23 = loc38
	mov r24 = loc39
	mov r25 = loc40
	mov r26 = loc41 
	mov r27 = loc42
	mov r28 = loc43
	mov r29 = loc44
	mov r30 = loc45
	mov r31 = loc46

	mov ar.fpsr = loc47	/* restore floating point status register */
	
    /* 14. restore branch and application registers */
    	mov ar.unat = loc3
	mov ar.lc = loc4
	mov ar.ec = loc5
	mov ar.ccv = loc6
	mov ar.csd = loc7
	mov ar.ssd = loc8
	
	mov b0 = loc9
	mov b1 = loc10
	mov b2 = loc11
	mov b3 = loc12
	mov b4 = loc13
	mov b5 = loc14
	mov b6 = loc15
	mov b7 = loc16
	
    /* 15. disable PSR.ic and switch to bank 0 */
	rsm PSR_IC_MASK
	bsw.0 ;;
	srlz.d

	mov R_RET = loc1
	mov ar.pfs = loc0
	br.ret.sptk.many b0

.global ivt
.align 32768
ivt:
	HEAVYWEIGHT_HANDLER 0x0000
	HEAVYWEIGHT_HANDLER 0x0400
	HEAVYWEIGHT_HANDLER 0x0800
	HEAVYWEIGHT_HANDLER 0x0c00 alternate_instruction_tlb_fault
	HEAVYWEIGHT_HANDLER 0x1000 alternate_data_tlb_fault
	HEAVYWEIGHT_HANDLER 0x1400 data_nested_tlb_fault
	HEAVYWEIGHT_HANDLER 0x1800
	HEAVYWEIGHT_HANDLER 0x1c00
	HEAVYWEIGHT_HANDLER 0x2000 data_dirty_bit_fault
	HEAVYWEIGHT_HANDLER 0x2400 instruction_access_bit_fault
	HEAVYWEIGHT_HANDLER 0x2800 data_access_bit_fault
	HEAVYWEIGHT_HANDLER 0x2c00 break_instruction
	HEAVYWEIGHT_HANDLER 0x3000 external_interrupt	/* For external interrupt, heavyweight handler is used. */
	HEAVYWEIGHT_HANDLER 0x3400
	HEAVYWEIGHT_HANDLER 0x3800
	HEAVYWEIGHT_HANDLER 0x3c00
	HEAVYWEIGHT_HANDLER 0x4000
	HEAVYWEIGHT_HANDLER 0x4400
	HEAVYWEIGHT_HANDLER 0x4800
	HEAVYWEIGHT_HANDLER 0x4c00

	HEAVYWEIGHT_HANDLER 0x5000 page_not_present
	HEAVYWEIGHT_HANDLER 0x5100
	HEAVYWEIGHT_HANDLER 0x5200
	HEAVYWEIGHT_HANDLER 0x5300 data_access_rights_fault
	HEAVYWEIGHT_HANDLER 0x5400 general_exception
	HEAVYWEIGHT_HANDLER 0x5500 disabled_fp_register
	HEAVYWEIGHT_HANDLER 0x5600
	SPECULATION_VECTOR_HANDLER 0x5700
	HEAVYWEIGHT_HANDLER 0x5800
	HEAVYWEIGHT_HANDLER 0x5900
	HEAVYWEIGHT_HANDLER 0x5a00
	HEAVYWEIGHT_HANDLER 0x5b00
	HEAVYWEIGHT_HANDLER 0x5c00
	HEAVYWEIGHT_HANDLER 0x5d00 
	HEAVYWEIGHT_HANDLER 0x5e00
	HEAVYWEIGHT_HANDLER 0x5f00
	
	HEAVYWEIGHT_HANDLER 0x6000
	HEAVYWEIGHT_HANDLER 0x6100
	HEAVYWEIGHT_HANDLER 0x6200
	HEAVYWEIGHT_HANDLER 0x6300
	HEAVYWEIGHT_HANDLER 0x6400
	HEAVYWEIGHT_HANDLER 0x6500
	HEAVYWEIGHT_HANDLER 0x6600
	HEAVYWEIGHT_HANDLER 0x6700
	HEAVYWEIGHT_HANDLER 0x6800
	HEAVYWEIGHT_HANDLER 0x6900
	HEAVYWEIGHT_HANDLER 0x6a00
	HEAVYWEIGHT_HANDLER 0x6b00
	HEAVYWEIGHT_HANDLER 0x6c00
	HEAVYWEIGHT_HANDLER 0x6d00
	HEAVYWEIGHT_HANDLER 0x6e00
	HEAVYWEIGHT_HANDLER 0x6f00

	HEAVYWEIGHT_HANDLER 0x7000
	HEAVYWEIGHT_HANDLER 0x7100
	HEAVYWEIGHT_HANDLER 0x7200
	HEAVYWEIGHT_HANDLER 0x7300
	HEAVYWEIGHT_HANDLER 0x7400
	HEAVYWEIGHT_HANDLER 0x7500
	HEAVYWEIGHT_HANDLER 0x7600
	HEAVYWEIGHT_HANDLER 0x7700
	HEAVYWEIGHT_HANDLER 0x7800
	HEAVYWEIGHT_HANDLER 0x7900
	HEAVYWEIGHT_HANDLER 0x7a00
	HEAVYWEIGHT_HANDLER 0x7b00
	HEAVYWEIGHT_HANDLER 0x7c00
	HEAVYWEIGHT_HANDLER 0x7d00
	HEAVYWEIGHT_HANDLER 0x7e00
	HEAVYWEIGHT_HANDLER 0x7f00
