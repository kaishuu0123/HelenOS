/*
 * Copyright (c) 2006 Jakub Jermar
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

#include <arch/mm/tsb.h>
#include <arch/mm/tlb.h>
#include <arch/mm/page.h>
#include <arch/barrier.h>
#include <mm/as.h>
#include <typedefs.h>
#include <macros.h>
#include <debug.h>

#define TSB_INDEX_MASK	((1 << (21 + 1 + TSB_SIZE - MMU_PAGE_WIDTH)) - 1)

/** Invalidate portion of TSB.
 *
 * We assume that the address space is already locked. Note that respective
 * portions of both TSBs are invalidated at a time.
 *
 * @param as Address space.
 * @param page First page to invalidate in TSB.
 * @param pages Number of pages to invalidate. Value of (size_t) -1 means the
 * 	whole TSB.
 */
void tsb_invalidate(as_t *as, uintptr_t page, size_t pages)
{
	size_t i0;
	size_t i;
	size_t cnt;
	
	ASSERT(as->arch.itsb && as->arch.dtsb);
	
	i0 = (page >> MMU_PAGE_WIDTH) & TSB_INDEX_MASK;
	ASSERT(i0 < ITSB_ENTRY_COUNT && i0 < DTSB_ENTRY_COUNT);

	if (pages == (size_t) -1 || (pages * 2) > ITSB_ENTRY_COUNT)
		cnt = ITSB_ENTRY_COUNT;
	else
		cnt = pages * 2;
	
	for (i = 0; i < cnt; i++) {
		as->arch.itsb[(i0 + i) & (ITSB_ENTRY_COUNT - 1)].tag.invalid =
		    true;
		as->arch.dtsb[(i0 + i) & (DTSB_ENTRY_COUNT - 1)].tag.invalid =
		    true;
	}
}

/** Copy software PTE to ITSB.
 *
 * @param t 	Software PTE.
 * @param index	Zero if lower 8K-subpage, one if higher 8K subpage.
 */
void itsb_pte_copy(pte_t *t, size_t index)
{
	as_t *as;
	tsb_entry_t *tsb;
	size_t entry;

	ASSERT(index <= 1);
	
	as = t->as;
	entry = ((t->page >> MMU_PAGE_WIDTH) + index) & TSB_INDEX_MASK; 
	ASSERT(entry < ITSB_ENTRY_COUNT);
	tsb = &as->arch.itsb[entry];

	/*
	 * We use write barriers to make sure that the TSB load
	 * won't use inconsistent data or that the fault will
	 * be repeated.
	 */

	tsb->tag.invalid = true;	/* invalidate the entry
					 * (tag target has this
					 * set to 0) */

	write_barrier();

	tsb->tag.context = as->asid;
	/* the shift is bigger than PAGE_WIDTH, do not bother with index  */
	tsb->tag.va_tag = t->page >> VA_TAG_PAGE_SHIFT;
	tsb->data.value = 0;
	tsb->data.size = PAGESIZE_8K;
	tsb->data.pfn = (t->frame >> MMU_FRAME_WIDTH) + index;
	tsb->data.cp = t->c;	/* cp as cache in phys.-idxed, c as cacheable */
	tsb->data.p = t->k;	/* p as privileged, k as kernel */
	tsb->data.v = t->p;	/* v as valid, p as present */
	
	write_barrier();
	
	tsb->tag.invalid = false;	/* mark the entry as valid */
}

/** Copy software PTE to DTSB.
 *
 * @param t	Software PTE.
 * @param index	Zero if lower 8K-subpage, one if higher 8K-subpage.
 * @param ro	If true, the mapping is copied read-only.
 */
void dtsb_pte_copy(pte_t *t, size_t index, bool ro)
{
	as_t *as;
	tsb_entry_t *tsb;
	size_t entry;
	
	ASSERT(index <= 1);

	as = t->as;
	entry = ((t->page >> MMU_PAGE_WIDTH) + index) & TSB_INDEX_MASK;
	ASSERT(entry < DTSB_ENTRY_COUNT);
	tsb = &as->arch.dtsb[entry];

	/*
	 * We use write barriers to make sure that the TSB load
	 * won't use inconsistent data or that the fault will
	 * be repeated.
	 */

	tsb->tag.invalid = true;	/* invalidate the entry
					 * (tag target has this
					 * set to 0) */

	write_barrier();

	tsb->tag.context = as->asid;
	/* the shift is bigger than PAGE_WIDTH, do not bother with index */
	tsb->tag.va_tag = t->page >> VA_TAG_PAGE_SHIFT;
	tsb->data.value = 0;
	tsb->data.size = PAGESIZE_8K;
	tsb->data.pfn = (t->frame >> MMU_FRAME_WIDTH) + index;
	tsb->data.cp = t->c;
#ifdef CONFIG_VIRT_IDX_DCACHE
	tsb->data.cv = t->c;
#endif /* CONFIG_VIRT_IDX_DCACHE */
	tsb->data.p = t->k;		/* p as privileged */
	tsb->data.w = ro ? false : t->w;
	tsb->data.v = t->p;
	
	write_barrier();
	
	tsb->tag.invalid = false;	/* mark the entry as valid */
}

/** @}
 */

