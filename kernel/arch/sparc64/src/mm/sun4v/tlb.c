/*
 * Copyright (c) 2005 Jakub Jermar
 * Copyright (c) 2008 Pavel Rimsky
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

/** @addtogroup sparc64mm
 * @{
 */
/** @file
 */

#include <mm/tlb.h>
#include <mm/as.h>
#include <mm/asid.h>
#include <arch/sun4v/hypercall.h>
#include <arch/mm/frame.h>
#include <arch/mm/page.h>
#include <arch/mm/tte.h>
#include <arch/mm/tlb.h>
#include <arch/interrupt.h>
#include <interrupt.h>
#include <arch.h>
#include <print.h>
#include <typedefs.h>
#include <config.h>
#include <arch/trap/trap.h>
#include <arch/trap/exception.h>
#include <panic.h>
#include <arch/asm.h>
#include <arch/cpu.h>
#include <arch/mm/pagesize.h>
#include <genarch/mm/page_ht.h>

#ifdef CONFIG_TSB
#include <arch/mm/tsb.h>
#endif

static void itlb_pte_copy(pte_t *);
static void dtlb_pte_copy(pte_t *, bool);
static void do_fast_instruction_access_mmu_miss_fault(istate_t *, uintptr_t,
    const char *);
static void do_fast_data_access_mmu_miss_fault(istate_t *, uint64_t,
    const char *);
static void do_fast_data_access_protection_fault(istate_t *,
    uint64_t, const char *);

/*
 * The assembly language routine passes a 64-bit parameter to the Data Access
 * MMU Miss and Data Access protection handlers, the parameter encapsulates
 * a virtual address of the faulting page and the faulting context. The most
 * significant 51 bits represent the VA of the faulting page and the least
 * significant 13 vits represent the faulting context. The following macros
 * extract the page and context out of the 64-bit parameter:
 */

/* extracts the VA of the faulting page */
#define DMISS_ADDRESS(page_and_ctx)	(((page_and_ctx) >> 13) << 13)

/* extracts the faulting context */
#define DMISS_CONTEXT(page_and_ctx)	((page_and_ctx) & 0x1fff)

/**
 * Descriptions of fault types from the MMU Fault status area.
 *
 * fault_type[i] contains description of error for which the IFT or DFT
 * field of the MMU fault status area is i.
 */
static const char *fault_types[] = {
	"unknown",
	"fast miss",
	"fast protection",
	"MMU miss",
	"invalid RA",
	"privileged violation",
	"protection violation",
	"NFO access",
	"so page/NFO side effect",
	"invalid VA",
	"invalid ASI",
	"nc atomic",
	"privileged action",
	"unknown",
	"unaligned access",
	"invalid page size"
	};

/** Array of MMU fault status areas. */
extern mmu_fault_status_area_t mmu_fsas[MAX_NUM_STRANDS];

/*
 * Invalidate all non-locked DTLB and ITLB entries.
 */
void tlb_arch_init(void)
{
	tlb_invalidate_all();
}

/** Insert privileged mapping into DMMU TLB.
 *
 * @param page		Virtual page address.
 * @param frame		Physical frame address.
 * @param pagesize	Page size.
 * @param locked	True for permanent mappings, false otherwise.
 * @param cacheable	True if the mapping is cacheable, false otherwise.
 */
void dtlb_insert_mapping(uintptr_t page, uintptr_t frame, int pagesize,
    bool locked, bool cacheable)
{
	tte_data_t data;
	
	data.value = 0;
	data.v = true;
	data.nfo = false;
	data.ra = frame >> FRAME_WIDTH;
	data.ie = false;
	data.e = false;
	data.cp = cacheable;
#ifdef CONFIG_VIRT_IDX_DCACHE
	data.cv = cacheable;
#endif
	data.p = true;
	data.x = false;
	data.w = true;
	data.size = pagesize;
	
	if (locked) {
		__hypercall_fast4(
			MMU_MAP_PERM_ADDR, page, 0, data.value, MMU_FLAG_DTLB);
	} else {
		__hypercall_hyperfast(
			page, ASID_KERNEL, data.value, MMU_FLAG_DTLB, 0,
			MMU_MAP_ADDR);
	}
}

/** Copy PTE to TLB.
 *
 * @param t 		Page Table Entry to be copied.
 * @param ro		If true, the entry will be created read-only, regardless
 * 			of its w field.
 */
void dtlb_pte_copy(pte_t *t, bool ro)
{
	tte_data_t data;
	
	data.value = 0;
	data.v = true;
	data.nfo = false;
	data.ra = (t->frame) >> FRAME_WIDTH;
	data.ie = false;
	data.e = false;
	data.cp = t->c;
#ifdef CONFIG_VIRT_IDX_DCACHE
	data.cv = t->c;
#endif
	data.p = t->k;
	data.x = false;
	data.w = ro ? false : t->w;
	data.size = PAGESIZE_8K;
	
	__hypercall_hyperfast(
		t->page, t->as->asid, data.value, MMU_FLAG_DTLB, 0, MMU_MAP_ADDR);
}

/** Copy PTE to ITLB.
 *
 * @param t		Page Table Entry to be copied.
 */
void itlb_pte_copy(pte_t *t)
{
	tte_data_t data;
	
	data.value = 0;
	data.v = true;
	data.nfo = false;
	data.ra = (t->frame) >> FRAME_WIDTH;
	data.ie = false;
	data.e = false;
	data.cp = t->c;
	data.cv = false;
	data.p = t->k;
	data.x = true;
	data.w = false;
	data.size = PAGESIZE_8K;
	
	__hypercall_hyperfast(
		t->page, t->as->asid, data.value, MMU_FLAG_ITLB, 0, MMU_MAP_ADDR);
}

/** ITLB miss handler. */
void fast_instruction_access_mmu_miss(sysarg_t unused, istate_t *istate)
{
	uintptr_t va = ALIGN_DOWN(istate->tpc, PAGE_SIZE);
	pte_t *t;

	t = page_mapping_find(AS, va, true);

	if (t && PTE_EXECUTABLE(t)) {
		/*
		 * The mapping was found in the software page hash table.
		 * Insert it into ITLB.
		 */
		t->a = true;
		itlb_pte_copy(t);
#ifdef CONFIG_TSB
		itsb_pte_copy(t);
#endif
	} else {
		/*
		 * Forward the page fault to the address space page fault
		 * handler.
		 */
		if (as_page_fault(va, PF_ACCESS_EXEC, istate) == AS_PF_FAULT) {
			do_fast_instruction_access_mmu_miss_fault(istate,
			    istate->tpc, __func__);
		}
	}
}

/** DTLB miss handler.
 *
 * Note that some faults (e.g. kernel faults) were already resolved by the
 * low-level, assembly language part of the fast_data_access_mmu_miss handler.
 *
 * @param page_and_ctx	A 64-bit value describing the fault. The most
 * 			significant 51 bits of the value contain the virtual
 * 			address which caused the fault truncated to the page
 * 			boundary. The least significant 13 bits of the value
 * 			contain the number of the context in which the fault
 * 			occurred.
 * @param istate	Interrupted state saved on the stack.
 */
void fast_data_access_mmu_miss(uint64_t page_and_ctx, istate_t *istate)
{
	pte_t *t;
	uintptr_t va = DMISS_ADDRESS(page_and_ctx);
	uint16_t ctx = DMISS_CONTEXT(page_and_ctx);

	if (ctx == ASID_KERNEL) {
		if (va == 0) {
			/* NULL access in kernel */
			do_fast_data_access_mmu_miss_fault(istate, page_and_ctx,
			    __func__);
		}
		do_fast_data_access_mmu_miss_fault(istate, page_and_ctx, "Unexpected "
		    "kernel page fault.");
	}

	t = page_mapping_find(AS, va, true);
	if (t) {
		/*
		 * The mapping was found in the software page hash table.
		 * Insert it into DTLB.
		 */
		t->a = true;
		dtlb_pte_copy(t, true);
#ifdef CONFIG_TSB
		dtsb_pte_copy(t, true);
#endif
	} else {
		/*
		 * Forward the page fault to the address space page fault
		 * handler.
		 */		
		if (as_page_fault(va, PF_ACCESS_READ, istate) == AS_PF_FAULT) {
			do_fast_data_access_mmu_miss_fault(istate, page_and_ctx,
			    __func__);
		}
	}
}

/** DTLB protection fault handler.
 *
 * @param page_and_ctx	A 64-bit value describing the fault. The most
 * 			significant 51 bits of the value contain the virtual
 * 			address which caused the fault truncated to the page
 * 			boundary. The least significant 13 bits of the value
 * 			contain the number of the context in which the fault
 * 			occurred.
 * @param istate	Interrupted state saved on the stack.
 */
void fast_data_access_protection(uint64_t page_and_ctx, istate_t *istate)
{
	pte_t *t;
	uintptr_t va = DMISS_ADDRESS(page_and_ctx);
	uint16_t ctx = DMISS_CONTEXT(page_and_ctx);

	t = page_mapping_find(AS, va, true);
	if (t && PTE_WRITABLE(t)) {
		/*
		 * The mapping was found in the software page hash table and is
		 * writable. Demap the old mapping and insert an updated mapping
		 * into DTLB.
		 */
		t->a = true;
		t->d = true;
		mmu_demap_page(va, ctx, MMU_FLAG_DTLB);
		dtlb_pte_copy(t, false);
#ifdef CONFIG_TSB
		dtsb_pte_copy(t, false);
#endif
	} else {
		/*
		 * Forward the page fault to the address space page fault
		 * handler.
		 */		
		if (as_page_fault(va, PF_ACCESS_WRITE, istate) == AS_PF_FAULT) {
			do_fast_data_access_protection_fault(istate, page_and_ctx,
			    __func__);
		}
	}
}

/*
 * On Niagara this function does not work, as supervisor software is isolated
 * from the TLB by the hypervisor and has no chance to investigate the TLB
 * entries.
 */
void tlb_print(void)
{
	printf("Operation not possible on Niagara.\n");
}

void do_fast_instruction_access_mmu_miss_fault(istate_t *istate, uintptr_t va,
    const char *str)
{
	fault_if_from_uspace(istate, "%s, address=%p.", str,
	    (void *) va);
	panic_memtrap(istate, PF_ACCESS_EXEC, va, str);
}

void do_fast_data_access_mmu_miss_fault(istate_t *istate,
    uint64_t page_and_ctx, const char *str)
{
	fault_if_from_uspace(istate, "%s, page=%p (asid=%" PRId64 ").", str,
	    (void *) DMISS_ADDRESS(page_and_ctx), DMISS_CONTEXT(page_and_ctx));
	panic_memtrap(istate, PF_ACCESS_UNKNOWN, DMISS_ADDRESS(page_and_ctx),
	    str);
}

void do_fast_data_access_protection_fault(istate_t *istate,
    uint64_t page_and_ctx, const char *str)
{
	fault_if_from_uspace(istate, "%s, page=%p (asid=%" PRId64 ").", str,
	    (void *) DMISS_ADDRESS(page_and_ctx), DMISS_CONTEXT(page_and_ctx));
	panic_memtrap(istate, PF_ACCESS_WRITE, DMISS_ADDRESS(page_and_ctx),
	    str);
}

/**
 * Describes the exact condition which caused the last DMMU fault.
 */
void describe_dmmu_fault(void)
{
	uint64_t myid;
	__hypercall_fast_ret1(0, 0, 0, 0, 0, CPU_MYID, &myid);

	ASSERT(mmu_fsas[myid].dft < 16);

	printf("condition which caused the fault: %s\n",
		fault_types[mmu_fsas[myid].dft]);
}

/** Invalidate all unlocked ITLB and DTLB entries. */
void tlb_invalidate_all(void)
{
	uint64_t errno =  __hypercall_fast3(MMU_DEMAP_ALL, 0, 0,
		MMU_FLAG_DTLB | MMU_FLAG_ITLB);
	if (errno != HV_EOK)
		panic("Error code = %" PRIu64 ".\n", errno);
}

/** Invalidate all ITLB and DTLB entries that belong to specified ASID
 * (Context).
 *
 * @param asid Address Space ID.
 */
void tlb_invalidate_asid(asid_t asid)
{
	/* switch to nucleus because we are mapped by the primary context */
	nucleus_enter();

	__hypercall_fast4(MMU_DEMAP_CTX, 0, 0, asid,
		MMU_FLAG_ITLB | MMU_FLAG_DTLB);

	nucleus_leave();
}

/** Invalidate all ITLB and DTLB entries for specified page range in specified
 * address space.
 *
 * @param asid		Address Space ID.
 * @param page		First page which to sweep out from ITLB and DTLB.
 * @param cnt		Number of ITLB and DTLB entries to invalidate.
 */
void tlb_invalidate_pages(asid_t asid, uintptr_t page, size_t cnt)
{
	unsigned int i;
	
	/* switch to nucleus because we are mapped by the primary context */
	nucleus_enter();

	for (i = 0; i < cnt; i++) {
		__hypercall_fast5(MMU_DEMAP_PAGE, 0, 0, page, asid,
			MMU_FLAG_DTLB | MMU_FLAG_ITLB);
	}

	nucleus_leave();
}

/** @}
 */
