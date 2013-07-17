/*
 * Copyright (c) 2010 Lenka Trochtova
 * Copyright (c) 2011 Jiri Svoboda
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

/** @addtogroup pciintel
 * @{
 */
/** @file
 */

#ifndef PCI_H_
#define PCI_H_

#include <ddf/driver.h>
#include "pci_regs.h"

#define PCI_MAX_HW_RES 8

typedef struct pciintel_bus {
	/** DDF device node */
	ddf_dev_t *dnode;
	uint32_t conf_io_addr;
	uint32_t conf_io_data;
	void *conf_data_port;
	void *conf_addr_port;
	fibril_mutex_t conf_mutex;
} pci_bus_t;

typedef struct pci_fun_data {
	pci_bus_t *busptr;
	ddf_fun_t *fnode;

	int bus;
	int dev;
	int fn;
	int vendor_id;
	int device_id;
	uint8_t class_code;
	uint8_t subclass_code;
	uint8_t prog_if;
	uint8_t revision;
	hw_resource_list_t hw_resources;
	hw_resource_t resources[PCI_MAX_HW_RES];
} pci_fun_t;

extern void pci_fun_create_match_ids(pci_fun_t *);

extern uint8_t pci_conf_read_8(pci_fun_t *, int);
extern uint16_t pci_conf_read_16(pci_fun_t *, int);
extern uint32_t pci_conf_read_32(pci_fun_t *, int);
extern void pci_conf_write_8(pci_fun_t *, int, uint8_t);
extern void pci_conf_write_16(pci_fun_t *, int, uint16_t);
extern void pci_conf_write_32(pci_fun_t *, int, uint32_t);

extern void pci_add_range(pci_fun_t *, uint64_t, size_t, bool);
extern int pci_read_bar(pci_fun_t *, int);
extern void pci_read_interrupt(pci_fun_t *);
extern void pci_add_interrupt(pci_fun_t *, int);

extern pci_fun_t *pci_fun_new(pci_bus_t *);
extern void pci_fun_init(pci_fun_t *, int, int, int);
extern void pci_fun_delete(pci_fun_t *);
extern char *pci_fun_create_name(pci_fun_t *);

extern void pci_bus_scan(pci_bus_t *, int);

extern bool pci_alloc_resource_list(pci_fun_t *);
extern void pci_clean_resource_list(pci_fun_t *);

extern void pci_read_bars(pci_fun_t *);
extern size_t pci_bar_mask_to_size(uint32_t);

#endif

/**
 * @}
 */
