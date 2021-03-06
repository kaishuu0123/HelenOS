#
# Copyright (c) 2003-2004 Jakub Jermar
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

#include <arch/asm/regname.h>
#include <arch/mm/page.h>
#include <arch/asm/boot.h>
#include <arch/context_offset.h>
#include <arch/stack.h>

.text

.set noat
.set noreorder
.set nomacro

.global kernel_image_start
.global tlb_refill_entry
.global cache_error_entry
.global exception_entry
.global userspace_asm

/*
 * Which status bits are thread-local:
 * KSU(UM), EXL, ERL, IE
 */
#define REG_SAVE_MASK 0x1f

#define ISTATE_OFFSET_A0         0
#define ISTATE_OFFSET_A1         8
#define ISTATE_OFFSET_A2         16
#define ISTATE_OFFSET_A3         24
#define ISTATE_OFFSET_T0         32
#define ISTATE_OFFSET_T1         40
#define ISTATE_OFFSET_V0         48
#define ISTATE_OFFSET_V1         56
#define ISTATE_OFFSET_AT         64
#define ISTATE_OFFSET_T2         72
#define ISTATE_OFFSET_T3         80
#define ISTATE_OFFSET_T4         88
#define ISTATE_OFFSET_T5         96
#define ISTATE_OFFSET_T6         104
#define ISTATE_OFFSET_T7         112
#define ISTATE_OFFSET_S0         120
#define ISTATE_OFFSET_S1         128
#define ISTATE_OFFSET_S2         136
#define ISTATE_OFFSET_S3         144
#define ISTATE_OFFSET_S4         152
#define ISTATE_OFFSET_S5         160
#define ISTATE_OFFSET_S6         168
#define ISTATE_OFFSET_S7         176
#define ISTATE_OFFSET_T8         184
#define ISTATE_OFFSET_T9         192
#define ISTATE_OFFSET_KT0        200
#define ISTATE_OFFSET_KT1        208
#define ISTATE_OFFSET_GP         216
#define ISTATE_OFFSET_SP         224
#define ISTATE_OFFSET_S8         232
#define ISTATE_OFFSET_RA         240
#define ISTATE_OFFSET_LO         248
#define ISTATE_OFFSET_HI         252
#define ISTATE_OFFSET_STATUS     256
#define ISTATE_OFFSET_EPC        264
#define ISTATE_OFFSET_ALIGNMENT  272

#define ISTATE_SOFT_SIZE         280

/*
 * The fake ABI prologue is never executed and may not be part of the
 * procedure's body. Instead, it should be immediately preceding the procedure's
 * body. Its only purpose is to trick the stack trace walker into thinking that
 * the exception is more or less just a normal function call.
 */
.macro FAKE_ABI_PROLOGUE
	sub $sp, ISTATE_SOFT_SIZE
	sd $ra, ISTATE_OFFSET_EPC($sp)
.endm

/*
 * Save registers to space defined by \r
 * We will change status: Disable ERL, EXL, UM, IE
 * These changes will be automatically reversed in REGISTER_LOAD
 * %sp is NOT saved as part of these registers
 */
.macro REGISTERS_STORE_AND_EXC_RESET r
	sd $at, ISTATE_OFFSET_AT(\r)
	sd $v0, ISTATE_OFFSET_V0(\r)
	sd $v1, ISTATE_OFFSET_V1(\r)
	sd $a0, ISTATE_OFFSET_A0(\r)
	sd $a1, ISTATE_OFFSET_A1(\r)
	sd $a2, ISTATE_OFFSET_A2(\r)
	sd $a3, ISTATE_OFFSET_A3(\r)
	sd $t0, ISTATE_OFFSET_T0(\r)
	sd $t1, ISTATE_OFFSET_T1(\r)
	sd $t2, ISTATE_OFFSET_T2(\r)
	sd $t3, ISTATE_OFFSET_T3(\r)
	sd $t4, ISTATE_OFFSET_T4(\r)
	sd $t5, ISTATE_OFFSET_T5(\r)
	sd $t6, ISTATE_OFFSET_T6(\r)
	sd $t7, ISTATE_OFFSET_T7(\r)
	sd $t8, ISTATE_OFFSET_T8(\r)
	sd $t9, ISTATE_OFFSET_T9(\r)
	sd $s0, ISTATE_OFFSET_S0(\r)
	sd $s1, ISTATE_OFFSET_S1(\r)
	sd $s2, ISTATE_OFFSET_S2(\r)
	sd $s3, ISTATE_OFFSET_S3(\r)
	sd $s4, ISTATE_OFFSET_S4(\r)
	sd $s5, ISTATE_OFFSET_S5(\r)
	sd $s6, ISTATE_OFFSET_S6(\r)
	sd $s7, ISTATE_OFFSET_S7(\r)
	sd $s8, ISTATE_OFFSET_S8(\r)
	
	mflo $at
	sw $at, ISTATE_OFFSET_LO(\r)
	mfhi $at
	sw $at, ISTATE_OFFSET_HI(\r)
	
	sd $gp, ISTATE_OFFSET_GP(\r)
	sd $ra, ISTATE_OFFSET_RA(\r)
	sd $k0, ISTATE_OFFSET_KT0(\r)
	sd $k1, ISTATE_OFFSET_KT1(\r)
	
	dmfc0 $t0, $status
	dmfc0 $t1, $epc
	
	/* save only KSU, EXL, ERL, IE */
	and $t2, $t0, REG_SAVE_MASK
	
	/* clear KSU, EXL, ERL, IE */
	li $t3, ~(REG_SAVE_MASK)
	and $t0, $t0, $t3
	
	sd $t2, ISTATE_OFFSET_STATUS(\r)
	sd $t1, ISTATE_OFFSET_EPC(\r)
	dmtc0 $t0, $status
.endm

.macro REGISTERS_LOAD r
	/*
	 * Update only UM, EXR, IE from status, the rest
	 * is controlled by OS and not bound to task.
	 */
	dmfc0 $t0, $status
	ld $t1, ISTATE_OFFSET_STATUS(\r)
	
	/* mask UM, EXL, ERL, IE */
	li $t2, ~REG_SAVE_MASK
	and $t0, $t0, $t2
	
	/* copy UM, EXL, ERL, IE from saved status */
	or $t0, $t0, $t1
	dmtc0 $t0, $status
	
	ld $v0, ISTATE_OFFSET_V0(\r)
	ld $v1, ISTATE_OFFSET_V1(\r)
	ld $a0, ISTATE_OFFSET_A0(\r)
	ld $a1, ISTATE_OFFSET_A1(\r)
	ld $a2, ISTATE_OFFSET_A2(\r)
	ld $a3, ISTATE_OFFSET_A3(\r)
	ld $t0, ISTATE_OFFSET_T0(\r)
	ld $t1, ISTATE_OFFSET_T1(\r)
	ld $t2, ISTATE_OFFSET_T2(\r)
	ld $t3, ISTATE_OFFSET_T3(\r)
	ld $t4, ISTATE_OFFSET_T4(\r)
	ld $t5, ISTATE_OFFSET_T5(\r)
	ld $t6, ISTATE_OFFSET_T6(\r)
	ld $t7, ISTATE_OFFSET_T7(\r)
	ld $t8, ISTATE_OFFSET_T8(\r)
	ld $t9, ISTATE_OFFSET_T9(\r)
	
	ld $gp, ISTATE_OFFSET_GP(\r)
	ld $ra, ISTATE_OFFSET_RA(\r)
	ld $k1, ISTATE_OFFSET_KT1(\r)
	
	lw $at, ISTATE_OFFSET_LO(\r)
	mtlo $at
	lw $at, ISTATE_OFFSET_HI(\r)
	mthi $at
	
	ld $at, ISTATE_OFFSET_EPC(\r)
	dmtc0 $at, $epc
	
	ld $at, ISTATE_OFFSET_AT(\r)
	ld $sp, ISTATE_OFFSET_SP(\r)
.endm

/*
 * Move kernel stack pointer address to register $k0.
 * If we are in user mode, load the appropriate stack address.
 */
.macro KERNEL_STACK_TO_K0
	/* if we are in user mode */
	dmfc0 $k0, $status
	andi $k0, 0x10
	
	beq $k0, $0, 1f
	move $k0, $sp
	
	/* move $k0 pointer to kernel stack */
	dla $k0, supervisor_sp
	
	/* move $k0 (supervisor_sp) */
	lw $k0, ($k0)
	
	1:
.endm

.org 0x0
kernel_image_start:
	/* load temporary stack */
	lui $sp, %hi(end_stack)
	ori $sp, $sp, %lo(end_stack)
	
	/* not sure about this, but might be needed for PIC code */
	lui $gp, 0x8000
	
	/* $a1 contains physical address of bootinfo_t */
	jal arch_pre_main
	addiu $sp, -ABI_STACK_FRAME
	
	j main_bsp
	nop

.space TEMP_STACK_SIZE
end_stack:

tlb_refill_entry:
	j tlb_refill_handler
	nop

cache_error_entry:
	j cache_error_handler
	nop

exception_entry:
	j exception_handler
	nop

	FAKE_ABI_PROLOGUE
exception_handler:
	KERNEL_STACK_TO_K0
	
	sub $k0, ISTATE_SOFT_SIZE
	sw $sp, ISTATE_OFFSET_SP($k0)
	move $sp, $k0
	
	mfc0 $k0, $cause
	
	sra $k0, $k0, 0x2    /* cp0_exc_cause() part 1 */
	andi $k0, $k0, 0x1f  /* cp0_exc_cause() part 2 */
	sub $k0, 8           /* 8 = SYSCALL */
	
	beqz $k0, syscall_shortcut
	add $k0, 8           /* revert $k0 back to correct exc number */
	
	REGISTERS_STORE_AND_EXC_RESET $sp
	
	move $a1, $sp
	move $a0, $k0
	jal exc_dispatch     /* exc_dispatch(excno, register_space) */
	addiu $sp, -ABI_STACK_FRAME
	addiu $sp, ABI_STACK_FRAME
	
	REGISTERS_LOAD $sp
	/* the $sp is automatically restored to former value */
	eret

/** Syscall entry
 *
 * Registers:
 *
 * @param $v0 Syscall number.
 * @param $a0 1st argument.
 * @param $a1 2nd argument.
 * @param $a2 3rd argument.
 * @param $a3 4th argument.
 * @param $t0 5th argument.
 * @param $t1 6th argument.
 *
 * @return The return value will be stored in $v0.
 *
 */
syscall_shortcut:
	mfc0 $t3, $epc
	mfc0 $t2, $status
	sw $t3, ISTATE_OFFSET_EPC($sp)  /* save EPC */
	sw $k1, ISTATE_OFFSET_KT1($sp)  /* save $k1 not saved on context switch */
	
	and $t4, $t2, REG_SAVE_MASK  /* save only KSU, EXL, ERL, IE */
	li $t5, ~(0x1f)
	and $t2, $t2, $t5  /* clear KSU, EXL, ERL */
	ori $t2, $t2, 0x1  /* set IE */
	
	sw $t4, ISTATE_OFFSET_STATUS($sp)
	mtc0 $t2, $status
	
	/*
	 * Call the higher level system call handler.
	 *
	 */
	sw $t0, ISTATE_OFFSET_T0($sp)  /* save the 5th argument on the stack */
	sw $t1, ISTATE_OFFSET_T1($sp)  /* save the 6th argument on the stack */
	
	jal syscall_handler
	sw $v0, ISTATE_OFFSET_V0($sp)  /* save the syscall number on the stack */
	
	/* restore status */
	mfc0 $t2, $status
	lw $t3, ISTATE_OFFSET_STATUS($sp)
	
	/*
	 * Change back to EXL = 1 (from last exception), otherwise
	 * an interrupt could rewrite the CP0 - EPC.
	 *
	 */
	li $t4, ~REG_SAVE_MASK  /* mask UM, EXL, ERL, IE */
	and $t2, $t2, $t4
	or $t2, $t2, $t3  /* copy saved UM, EXL, ERL, IE */
	mtc0 $t2, $status
	
	/* restore epc + 4 */
	lw $t2, ISTATE_OFFSET_EPC($sp)
	lw $k1, ISTATE_OFFSET_KT1($sp)
	addi $t2, $t2, 4
	mtc0 $t2, $epc
	
	lw $sp, ISTATE_OFFSET_SP($sp)  /* restore $sp */
	eret

	FAKE_ABI_PROLOGUE
tlb_refill_handler:
	KERNEL_STACK_TO_K0
	sub $k0, ISTATE_SOFT_SIZE
	REGISTERS_STORE_AND_EXC_RESET $k0
	sw $sp, ISTATE_OFFSET_SP($k0)
	move $sp, $k0
	
	move $a0, $sp
	jal tlb_refill
	addiu $sp, -ABI_STACK_FRAME
	addiu $sp, ABI_STACK_FRAME
	
	REGISTERS_LOAD $sp
	eret

	FAKE_ABI_PROLOGUE
cache_error_handler:
	KERNEL_STACK_TO_K0
	sub $k0, ISTATE_SOFT_SIZE
	REGISTERS_STORE_AND_EXC_RESET $k0
	sw $sp, ISTATE_OFFSET_SP($k0)
	move $sp, $k0
	
	move $a0, $sp
	jal cache_error
	addiu $sp, -ABI_STACK_FRAME
	addiu $sp, ABI_STACK_FRAME
	
	REGISTERS_LOAD $sp
	eret

userspace_asm:
	move $sp, $a0
	move $v0, $a1
	move $t9, $a2      /* set up correct entry into PIC code */
	xor $a0, $a0, $a0  /* $a0 is defined to hold pcb_ptr */
	                   /* set it to 0 */
	eret
