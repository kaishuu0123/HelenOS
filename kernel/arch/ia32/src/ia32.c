/*
 * Copyright (c) 2001-2004 Jakub Jermar
 * Copyright (c) 2009 Jiri Svoboda
 * Copyright (c) 2009 Martin Decky
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

#include <arch.h>
#include <typedefs.h>
#include <errno.h>
#include <memstr.h>
#include <interrupt.h>
#include <console/console.h>
#include <syscall/syscall.h>
#include <sysinfo/sysinfo.h>
#include <arch/bios/bios.h>
#include <arch/boot/boot.h>
#include <arch/debugger.h>
#include <arch/drivers/i8254.h>
#include <arch/drivers/i8259.h>
#include <genarch/acpi/acpi.h>
#include <genarch/drivers/ega/ega.h>
#include <genarch/drivers/i8042/i8042.h>
#include <genarch/drivers/legacy/ia32/io.h>
#include <genarch/fb/bfb.h>
#include <genarch/kbrd/kbrd.h>
#include <genarch/multiboot/multiboot.h>
#include <genarch/multiboot/multiboot2.h>

#ifdef CONFIG_SMP
#include <arch/smp/apic.h>
#endif

/** Perform ia32-specific initialization before main_bsp() is called.
 *
 * @param signature Multiboot signature.
 * @param info      Multiboot information structure.
 *
 */
void arch_pre_main(uint32_t signature, void *info)
{
	/* Parse multiboot information obtained from the bootloader. */
	multiboot_info_parse(signature, (multiboot_info_t *) info);
	multiboot2_info_parse(signature, (multiboot2_info_t *) info);
	
#ifdef CONFIG_SMP
	/* Copy AP bootstrap routines below 1 MB. */
	memcpy((void *) AP_BOOT_OFFSET, (void *) BOOT_OFFSET,
	    (size_t) &_hardcoded_unmapped_size);
#endif
}

void arch_pre_mm_init(void)
{
	pm_init();

	if (config.cpu_active == 1) {
		interrupt_init();
		bios_init();
		
		/* PIC */
		i8259_init();
	}
}

void arch_post_mm_init(void)
{
	if (config.cpu_active == 1) {
		/* Initialize IRQ routing */
		irq_init(IRQ_COUNT, IRQ_COUNT);
		
		/* hard clock */
		i8254_init();
		
#if (defined(CONFIG_FB) || defined(CONFIG_EGA))
		bool bfb = false;
#endif
		
#ifdef CONFIG_FB
		bfb = bfb_init();
#endif
		
#ifdef CONFIG_EGA
		if (!bfb) {
			outdev_t *egadev = ega_init(EGA_BASE, EGA_VIDEORAM);
			if (egadev)
				stdout_wire(egadev);
		}
#endif
		
		/* Enable debugger */
		debugger_init();
		/* Merge all memory zones to 1 big zone */
		zone_merge_all();
	}
}

void arch_post_cpu_init()
{
#ifdef CONFIG_SMP
	if (config.cpu_active > 1) {
		l_apic_init();
		l_apic_debug();
	}
#endif
}

void arch_pre_smp_init(void)
{
	if (config.cpu_active == 1) {
#ifdef CONFIG_SMP
		acpi_init();
#endif /* CONFIG_SMP */
	}
}

void arch_post_smp_init(void)
{
	/* Currently the only supported platform for ia32 is 'pc'. */
	static const char *platform = "pc";

	sysinfo_set_item_data("platform", NULL, (void *) platform,
	    str_size(platform));

#ifdef CONFIG_PC_KBD
	/*
	 * Initialize the i8042 controller. Then initialize the keyboard
	 * module and connect it to i8042. Enable keyboard interrupts.
	 */
	i8042_instance_t *i8042_instance = i8042_init((i8042_t *) I8042_BASE, IRQ_KBD);
	if (i8042_instance) {
		kbrd_instance_t *kbrd_instance = kbrd_init();
		if (kbrd_instance) {
			indev_t *sink = stdin_wire();
			indev_t *kbrd = kbrd_wire(kbrd_instance, sink);
			i8042_wire(i8042_instance, kbrd);
			trap_virtual_enable_irqs(1 << IRQ_KBD);
			trap_virtual_enable_irqs(1 << IRQ_MOUSE);
		}
	}
#endif
	
	if (irqs_info != NULL)
		sysinfo_set_item_val(irqs_info, NULL, true);
}

void calibrate_delay_loop(void)
{
	i8254_calibrate_delay_loop();
	if (config.cpu_active == 1) {
		/*
		 * This has to be done only on UP.
		 * On SMP, i8254 is not used for time keeping and its interrupt pin remains masked.
		 */
		i8254_normal_operation();
	}
}

/** Set thread-local-storage pointer
 *
 * TLS pointer is set in GS register. That means, the GS contains
 * selector, and the descriptor->base is the correct address.
 */
sysarg_t sys_tls_set(uintptr_t addr)
{
	THREAD->arch.tls = addr;
	set_tls_desc(addr);
	
	return EOK;
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

void arch_reboot(void)
{
#ifdef CONFIG_PC_KBD
	i8042_cpu_reset((i8042_t *) I8042_BASE);
#endif
}

void irq_initialize_arch(irq_t *irq)
{
	(void) irq;
}

/** @}
 */
