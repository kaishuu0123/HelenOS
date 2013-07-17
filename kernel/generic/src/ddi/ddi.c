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

/** @addtogroup genericddi
 * @{
 */

/**
 * @file
 * @brief Device Driver Interface functions.
 *
 * This file contains functions that comprise the Device Driver Interface.
 * These are the functions for mapping physical memory and enabling I/O
 * space to tasks.
 */

#include <ddi/ddi.h>
#include <proc/task.h>
#include <security/cap.h>
#include <mm/frame.h>
#include <mm/as.h>
#include <mm/page.h>
#include <synch/mutex.h>
#include <syscall/copy.h>
#include <adt/btree.h>
#include <arch.h>
#include <align.h>
#include <errno.h>
#include <trace.h>
#include <bitops.h>

/** This lock protects the parea_btree. */
static mutex_t parea_lock;

/** B+tree with enabled physical memory areas. */
static btree_t parea_btree;

/** Initialize DDI.
 *
 */
void ddi_init(void)
{
	btree_create(&parea_btree);
	mutex_initialize(&parea_lock, MUTEX_PASSIVE);
}

/** Enable piece of physical memory for mapping by physmem_map().
 *
 * @param parea Pointer to physical area structure.
 *
 */
void ddi_parea_register(parea_t *parea)
{
	mutex_lock(&parea_lock);
	
	/*
	 * We don't check for overlaps here as the kernel is pretty sane.
	 */
	btree_insert(&parea_btree, (btree_key_t) parea->pbase, parea, NULL);
	
	mutex_unlock(&parea_lock);
}

/** Map piece of physical memory into virtual address space of current task.
 *
 * @param phys  Physical address of the starting frame.
 * @param pages Number of pages to map.
 * @param flags Address space area flags for the mapping.
 * @param virt  Virtual address of the starting page.
 * @param bound Lowest virtual address bound.
 *
 * @return EOK on success.
 * @return EPERM if the caller lacks capabilities to use this syscall.
 * @return EBADMEM if phys is not page aligned.
 * @return ENOENT if there is no task matching the specified ID or
 *         the physical address space is not enabled for mapping.
 * @return ENOMEM if there was a problem in creating address space area.
 *
 */
NO_TRACE static int physmem_map(uintptr_t phys, size_t pages,
    unsigned int flags, uintptr_t *virt, uintptr_t bound)
{
	ASSERT(TASK);
	
	if ((phys % FRAME_SIZE) != 0)
		return EBADMEM;
	
	/*
	 * Unprivileged tasks are only allowed to map pareas
	 * which are explicitly marked as such.
	 */
	bool priv =
	    ((cap_get(TASK) & CAP_MEM_MANAGER) == CAP_MEM_MANAGER);
	
	mem_backend_data_t backend_data;
	backend_data.base = phys;
	backend_data.frames = pages;
	
	/*
	 * Check if the memory region is explicitly enabled
	 * for mapping by any parea structure.
	 */
	
	mutex_lock(&parea_lock);
	btree_node_t *nodep;
	parea_t *parea = (parea_t *) btree_search(&parea_btree,
	    (btree_key_t) phys, &nodep);
	
	if ((parea != NULL) && (parea->frames >= pages)) {
		if ((!priv) && (!parea->unpriv)) {
			mutex_unlock(&parea_lock);
			return EPERM;
		}
		
		goto map;
	}
	
	parea = NULL;
	mutex_unlock(&parea_lock);
	
	/*
	 * Check if the memory region is part of physical
	 * memory generally enabled for mapping.
	 */
	
	irq_spinlock_lock(&zones.lock, true);
	size_t znum = find_zone(ADDR2PFN(phys), pages, 0);
	
	if (znum == (size_t) -1) {
		/*
		 * Frames not found in any zone
		 * -> assume it is a hardware device and allow mapping
		 *    for privileged tasks.
		 */
		irq_spinlock_unlock(&zones.lock, true);
		
		if (!priv)
			return EPERM;
		
		goto map;
	}
	
	if (zones.info[znum].flags & (ZONE_FIRMWARE | ZONE_RESERVED)) {
		/*
		 * Frames are part of firmware or reserved zone
		 * -> allow mapping for privileged tasks.
		 */
		irq_spinlock_unlock(&zones.lock, true);
		
		if (!priv)
			return EPERM;
		
		goto map;
	}
	
	irq_spinlock_unlock(&zones.lock, true);
	return ENOENT;
	
map:
	if (!as_area_create(TASK->as, flags, FRAMES2SIZE(pages),
	    AS_AREA_ATTR_NONE, &phys_backend, &backend_data, virt, bound)) {
		/*
		 * The address space area was not created.
		 * We report it using ENOMEM.
		 */
		
		if (parea != NULL)
			mutex_unlock(&parea_lock);
		
		return ENOMEM;
	}
	
	/*
	 * Mapping is created on-demand during page fault.
	 */
	
	if (parea != NULL) {
		parea->mapped = true;
		mutex_unlock(&parea_lock);
	}
	
	return EOK;
}

NO_TRACE static int physmem_unmap(uintptr_t virt)
{
	// TODO: implement unmap
	return EOK;
}

/** Wrapper for SYS_PHYSMEM_MAP syscall.
 *
 * @param phys     Physical base address to map
 * @param pages    Number of pages
 * @param flags    Flags of newly mapped pages
 * @param virt_ptr Destination virtual address
 * @param bound    Lowest virtual address bound.
 *
 * @return 0 on success, otherwise it returns error code found in errno.h
 *
 */
sysarg_t sys_physmem_map(uintptr_t phys, size_t pages, unsigned int flags,
    void *virt_ptr, uintptr_t bound)
{
	uintptr_t virt = (uintptr_t) -1;
	int rc = physmem_map(ALIGN_DOWN(phys, FRAME_SIZE), pages, flags,
	    &virt, bound);
	if (rc != EOK)
		return rc;
	
	rc = copy_to_uspace(virt_ptr, &virt, sizeof(virt));
	if (rc != EOK) {
		physmem_unmap((uintptr_t) virt);
		return rc;
	}
	
	return EOK;
}

sysarg_t sys_physmem_unmap(uintptr_t virt)
{
	return physmem_unmap(virt);
}

/** Enable range of I/O space for task.
 *
 * @param id Task ID of the destination task.
 * @param ioaddr Starting I/O address.
 * @param size Size of the enabled I/O space..
 *
 * @return 0 on success, EPERM if the caller lacks capabilities to use this
 *           syscall, ENOENT if there is no task matching the specified ID.
 *
 */
NO_TRACE static int iospace_enable(task_id_t id, uintptr_t ioaddr, size_t size)
{
	/*
	 * Make sure the caller is authorised to make this syscall.
	 */
	cap_t caps = cap_get(TASK);
	if (!(caps & CAP_IO_MANAGER))
		return EPERM;
	
	irq_spinlock_lock(&tasks_lock, true);
	
	task_t *task = task_find_by_id(id);
	
	if ((!task) || (!container_check(CONTAINER, task->container))) {
		/*
		 * There is no task with the specified ID
		 * or the task belongs to a different security
		 * context.
		 */
		irq_spinlock_unlock(&tasks_lock, true);
		return ENOENT;
	}
	
	/* Lock the task and release the lock protecting tasks_btree. */
	irq_spinlock_exchange(&tasks_lock, &task->lock);
	int rc = ddi_iospace_enable_arch(task, ioaddr, size);
	irq_spinlock_unlock(&task->lock, true);
	
	return rc;
}

/** Wrapper for SYS_ENABLE_IOSPACE syscall.
 *
 * @param uspace_io_arg User space address of DDI argument structure.
 *
 * @return 0 on success, otherwise it returns error code found in errno.h
 *
 */
sysarg_t sys_iospace_enable(ddi_ioarg_t *uspace_io_arg)
{
	ddi_ioarg_t arg;
	int rc = copy_from_uspace(&arg, uspace_io_arg, sizeof(ddi_ioarg_t));
	if (rc != 0)
		return (sysarg_t) rc;
	
	return (sysarg_t) iospace_enable((task_id_t) arg.task_id,
	    (uintptr_t) arg.ioaddr, (size_t) arg.size);
}

sysarg_t sys_iospace_disable(ddi_ioarg_t *uspace_io_arg)
{
	// TODO: implement
	return ENOTSUP;
}

NO_TRACE static int dmamem_map(uintptr_t virt, size_t size, unsigned int map_flags,
    unsigned int flags, void **phys)
{
	ASSERT(TASK);
	
	// TODO: implement locking of non-anonymous mapping
	return page_find_mapping(virt, phys);
}

NO_TRACE static int dmamem_map_anonymous(size_t size, unsigned int map_flags,
    unsigned int flags, void **phys, uintptr_t *virt, uintptr_t bound)
{
	ASSERT(TASK);
	
	size_t pages = SIZE2FRAMES(size);
	uint8_t order;
	
	/* We need the 2^order >= pages */
	if (pages == 1)
		order = 0;
	else
		order = fnzb(pages - 1) + 1;
	
	*phys = frame_alloc_noreserve(order, 0);
	if (*phys == NULL)
		return ENOMEM;
	
	mem_backend_data_t backend_data;
	backend_data.base = (uintptr_t) *phys;
	backend_data.frames = pages;
	
	if (!as_area_create(TASK->as, map_flags, size,
	    AS_AREA_ATTR_NONE, &phys_backend, &backend_data, virt, bound)) {
		frame_free_noreserve((uintptr_t) *phys);
		return ENOMEM;
	}
	
	return EOK;
}

NO_TRACE static int dmamem_unmap(uintptr_t virt, size_t size)
{
	// TODO: implement unlocking & unmap
	return EOK;
}

NO_TRACE static int dmamem_unmap_anonymous(uintptr_t virt)
{
	// TODO: implement unlocking & unmap
	return EOK;
}

sysarg_t sys_dmamem_map(size_t size, unsigned int map_flags, unsigned int flags,
    void *phys_ptr, void *virt_ptr, uintptr_t bound)
{
	if ((flags & DMAMEM_FLAGS_ANONYMOUS) == 0) {
		/*
		 * Non-anonymous DMA mapping
		 */
		
		void *phys;
		int rc = dmamem_map((uintptr_t) virt_ptr, size, map_flags,
		    flags, &phys);
		
		if (rc != EOK)
			return rc;
		
		rc = copy_to_uspace(phys_ptr, &phys, sizeof(phys));
		if (rc != EOK) {
			dmamem_unmap((uintptr_t) virt_ptr, size);
			return rc;
		}
	} else {
		/*
		 * Anonymous DMA mapping
		 */
		
		void *phys;
		uintptr_t virt = (uintptr_t) -1;
		int rc = dmamem_map_anonymous(size, map_flags, flags,
		    &phys, &virt, bound);
		if (rc != EOK)
			return rc;
		
		rc = copy_to_uspace(phys_ptr, &phys, sizeof(phys));
		if (rc != EOK) {
			dmamem_unmap_anonymous((uintptr_t) virt);
			return rc;
		}
		
		rc = copy_to_uspace(virt_ptr, &virt, sizeof(virt));
		if (rc != EOK) {
			dmamem_unmap_anonymous((uintptr_t) virt);
			return rc;
		}
	}
	
	return EOK;
}

sysarg_t sys_dmamem_unmap(uintptr_t virt, size_t size, unsigned int flags)
{
	if ((flags & DMAMEM_FLAGS_ANONYMOUS) == 0)
		return dmamem_unmap(virt, size);
	else
		return dmamem_unmap_anonymous(virt);
}

/** @}
 */
