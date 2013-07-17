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

/** @addtogroup sparc64
 * @{
 */
/** @file
 */

#ifndef KERN_sparc64_BOOT_H_
#define KERN_sparc64_BOOT_H_

#define VMA  0x400000
#define LMA  VMA

#ifndef __ASM__
#ifndef __LINKER__

#include <config.h>
#include <typedefs.h>
#include <genarch/ofw/ofw_tree.h>

#define TASKMAP_MAX_RECORDS  32
#define MEMMAP_MAX_RECORDS   32

#define BOOTINFO_TASK_NAME_BUFLEN  32

typedef struct {
	void *addr;
	size_t size;
	char name[BOOTINFO_TASK_NAME_BUFLEN];
} utask_t;

typedef struct {
	size_t cnt;
	utask_t tasks[TASKMAP_MAX_RECORDS];
} taskmap_t;

typedef struct {
	void *start;
	size_t size;
} memzone_t;

typedef struct {
	uint64_t total;
	size_t cnt;
	memzone_t zones[MEMMAP_MAX_RECORDS];
} memmap_t;

/** Bootinfo structure.
 *
 * Must be in sync with bootinfo structure used by the boot loader.
 *
 */
typedef struct {
	uintptr_t physmem_start;
	taskmap_t taskmap;
	memmap_t memmap;
	ballocs_t ballocs;
	ofw_tree_node_t *ofw_root;
} bootinfo_t;

extern memmap_t memmap;

#endif
#endif

#endif

/** @}
 */
