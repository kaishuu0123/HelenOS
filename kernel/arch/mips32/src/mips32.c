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

/** @addtogroup mips32
 * @{
 */
/** @file
 */

#include <arch.h>
#include <typedefs.h>
#include <errno.h>
#include <interrupt.h>
#include <macros.h>
#include <str.h>
#include <memstr.h>
#include <userspace.h>
#include <console/console.h>
#include <syscall/syscall.h>
#include <sysinfo/sysinfo.h>
#include <arch/debug.h>
#include <arch/debugger.h>
#include <arch/drivers/msim.h>
#include <genarch/fb/fb.h>
#include <genarch/drivers/dsrln/dsrlnin.h>
#include <genarch/drivers/dsrln/dsrlnout.h>
#include <genarch/srln/srln.h>

/* Size of the code jumping to the exception handler code
 * - J+NOP
 */
#define EXCEPTION_JUMP_SIZE  8

#define TLB_EXC    ((char *) 0x80000000)
#define NORM_EXC   ((char *) 0x80000180)
#define CACHE_EXC  ((char *) 0x80000100)


/* Why the linker moves the variable 64K away in assembler
 * when not in .text section?
 */

/* Stack pointer saved when entering user mode */
uintptr_t supervisor_sp __attribute__ ((section (".text")));

size_t cpu_count = 0;

/** Performs mips32-specific initialization before main_bsp() is called. */
void arch_pre_main(void *entry __attribute__((unused)), bootinfo_t *bootinfo)
{
	init.cnt = min3(bootinfo->cnt, TASKMAP_MAX_RECORDS, CONFIG_INIT_TASKS);
	
	size_t i;
	for (i = 0; i < init.cnt; i++) {
		init.tasks[i].paddr = KA2PA(bootinfo->tasks[i].addr);
		init.tasks[i].size = bootinfo->tasks[i].size;
		str_cpy(init.tasks[i].name, CONFIG_TASK_NAME_BUFLEN,
		    bootinfo->tasks[i].name);
	}
	
	for (i = 0; i < CPUMAP_MAX_RECORDS; i++) {
		if ((bootinfo->cpumap & (1 << i)) != 0)
			cpu_count++;
	}
}

void arch_pre_mm_init(void)
{
	/* It is not assumed by default */
	interrupts_disable();
	
	/* Initialize dispatch table */
	exception_init();

	/* Copy the exception vectors to the right places */
	memcpy(TLB_EXC, (char *) tlb_refill_entry, EXCEPTION_JUMP_SIZE);
	smc_coherence_block(TLB_EXC, EXCEPTION_JUMP_SIZE);
	memcpy(NORM_EXC, (char *) exception_entry, EXCEPTION_JUMP_SIZE);
	smc_coherence_block(NORM_EXC, EXCEPTION_JUMP_SIZE);
	memcpy(CACHE_EXC, (char *) cache_error_entry, EXCEPTION_JUMP_SIZE);
	smc_coherence_block(CACHE_EXC, EXCEPTION_JUMP_SIZE);
	
	/*
	 * Switch to BEV normal level so that exception vectors point to the
	 * kernel. Clear the error level.
	 */
	cp0_status_write(cp0_status_read() &
	    ~(cp0_status_bev_bootstrap_bit | cp0_status_erl_error_bit));
	
	/*
	 * Mask all interrupts
	 */
	cp0_mask_all_int();
	
	debugger_init();
}

void arch_post_mm_init(void)
{
	interrupt_init();
	
#ifdef CONFIG_FB
	/* GXemul framebuffer */
	fb_properties_t gxemul_prop = {
		.addr = 0x12000000,
		.offset = 0,
		.x = 640,
		.y = 480,
		.scan = 1920,
		.visual = VISUAL_RGB_8_8_8,
	};
	
	outdev_t *fbdev = fb_init(&gxemul_prop);
	if (fbdev)
		stdout_wire(fbdev);
#endif

#ifdef CONFIG_MIPS_PRN
	outdev_t *dsrlndev = dsrlnout_init((ioport8_t *) MSIM_KBD_ADDRESS);
	if (dsrlndev)
		stdout_wire(dsrlndev);
#endif
}

void arch_post_cpu_init(void)
{
}

void arch_pre_smp_init(void)
{
}

void arch_post_smp_init(void)
{
	static const char *platform;

	/* Set platform name. */
#ifdef MACHINE_msim
	platform = "msim";
#endif
#ifdef MACHINE_bgxemul
	platform = "gxemul";
#endif
#ifdef MACHINE_lgxemul
	platform = "gxemul";
#endif
	sysinfo_set_item_data("platform", NULL, (void *) platform,
	    str_size(platform));

#ifdef CONFIG_MIPS_KBD
	/*
	 * Initialize the msim/GXemul keyboard port. Then initialize the serial line
	 * module and connect it to the msim/GXemul keyboard. Enable keyboard interrupts.
	 */
	dsrlnin_instance_t *dsrlnin_instance
	    = dsrlnin_init((dsrlnin_t *) MSIM_KBD_ADDRESS, MSIM_KBD_IRQ);
	if (dsrlnin_instance) {
		srln_instance_t *srln_instance = srln_init();
		if (srln_instance) {
			indev_t *sink = stdin_wire();
			indev_t *srln = srln_wire(srln_instance, sink);
			dsrlnin_wire(dsrlnin_instance, srln);
			cp0_unmask_int(MSIM_KBD_IRQ);
		}
	}
	
	/*
	 * This is the necessary evil until the userspace driver is entirely
	 * self-sufficient.
	 */
	sysinfo_set_item_val("kbd", NULL, true);
	sysinfo_set_item_val("kbd.inr", NULL, MSIM_KBD_IRQ);
	sysinfo_set_item_val("kbd.address.physical", NULL,
	    PA2KA(MSIM_KBD_ADDRESS));
#endif
}

void calibrate_delay_loop(void)
{
}

void userspace(uspace_arg_t *kernel_uarg)
{
	/* EXL = 1, UM = 1, IE = 1 */
	cp0_status_write(cp0_status_read() | (cp0_status_exl_exception_bit |
	    cp0_status_um_bit | cp0_status_ie_enabled_bit));
	cp0_epc_write((uintptr_t) kernel_uarg->uspace_entry);
	userspace_asm(((uintptr_t) kernel_uarg->uspace_stack +
	    kernel_uarg->uspace_stack_size),
	    (uintptr_t) kernel_uarg->uspace_uarg,
	    (uintptr_t) kernel_uarg->uspace_entry);
	
	while (1);
}

/** Perform mips32 specific tasks needed before the new task is run. */
void before_task_runs_arch(void)
{
}

/** Perform mips32 specific tasks needed before the new thread is scheduled. */
void before_thread_runs_arch(void)
{
	supervisor_sp =
	    (uintptr_t) &THREAD->kstack[STACK_SIZE - SP_DELTA];
}

void after_thread_ran_arch(void)
{
}

/** Set thread-local-storage pointer
 *
 * We have it currently in K1, it is
 * possible to have it separately in the future.
 */
sysarg_t sys_tls_set(uintptr_t addr)
{
	return EOK;
}

void arch_reboot(void)
{
	___halt();
	while (1);
}

/** Construct function pointer
 *
 * @param fptr   function pointer structure
 * @param addr   function address
 * @param caller calling function address
 *
 * @return address of the function pointer
 *
 */
void *arch_construct_function(fncptr_t *fptr, void *addr, void *caller)
{
	return addr;
}

void irq_initialize_arch(irq_t *irq)
{
	(void) irq;
}

/** @}
 */
