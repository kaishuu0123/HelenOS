/*
 * Copyright (c) 2006 Sergey Bondari
 * Copyright (c) 2008 Jiri Svoboda
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

/** @addtogroup generic
 * @{
 */
/** @file
 * @brief ELF loader structures and public functions.
 */

#ifndef ELF_LOAD_H_
#define ELF_LOAD_H_

#include <elf/elf.h>
#include <sys/types.h>
#include <loader/pcb.h>

/**
 * ELF error return codes
 */
#define EE_OK			0	/* No error */
#define EE_INVALID		1	/* Invalid ELF image */
#define EE_MEMORY		2	/* Cannot allocate address space */
#define EE_INCOMPATIBLE		3	/* ELF image is not compatible with current architecture */
#define EE_UNSUPPORTED		4	/* Non-supported ELF (e.g. dynamic ELFs) */
#define EE_LOADER		5	/* The image is actually a program loader. */
#define EE_IRRECOVERABLE	6

typedef enum {
	/** Leave all segments in RW access mode. */
	ELDF_RW = 1
} eld_flags_t;

/**
 * Some data extracted from the headers are stored here
 */
typedef struct {
	/** Entry point */
	entry_point_t entry;

	/** ELF interpreter name or NULL if statically-linked */
	const char *interp;

	/** Pointer to the dynamic section */
	void *dynamic;
} elf_info_t;

/**
 * Holds information about an ELF binary being loaded.
 */
typedef struct {
	/** Filedescriptor of the file from which we are loading */
	int fd;

	/** Difference between run-time addresses and link-time addresses */
	uintptr_t bias;

	/** Flags passed to the ELF loader. */
	eld_flags_t flags;

	/** A copy of the ELF file header */
	elf_header_t *header;

	/** Store extracted info here */
	elf_info_t *info;
} elf_ld_t;

extern const char *elf_error(unsigned int);
extern int elf_load_file(const char *, size_t, eld_flags_t, elf_info_t *);
extern void elf_create_pcb(elf_info_t *, pcb_t *);

#endif

/** @}
 */
