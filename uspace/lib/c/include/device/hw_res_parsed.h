/*
 * Copyright (c) 2011 Jiri Michalec
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

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef LIBC_DEVICE_HW_RES_PARSED_H_
#define LIBC_DEVICE_HW_RES_PARSED_H_

#include <device/hw_res.h>
#include <str.h>

/** Keep areas of the zero size in the list */
#define HW_RES_KEEP_ZERO_AREA  0x1

/** Keep duplicit entries */
#define HW_RES_KEEP_DUPLICIT   0x2

/** Address range structure */
typedef struct addr_range {
	/** Start address */
	uint64_t address;
	
	/** Endianness */
	endianness_t endianness;
	
	/** Area size */
	size_t size;
} addr_range_t;

/** IO range type */
typedef addr_range_t io_range_t;

/** Memory range type */
typedef addr_range_t mem_range_t;

/** List of IRQs */
typedef struct irq_list {
	/** Irq count */
	size_t count;
	
	/** Array of IRQs */
	int *irqs;
} irq_list_t;

/** List of ISA DMA channels */
typedef struct dma_list {
	/** Channel count */
	size_t count;

	/** Array of channels */
	unsigned int *channels;
} dma_list_t;

/** List of memory areas */
typedef struct addr_range_list {
	/** Areas count */
	size_t count;
	
	/** Array of areas */
	addr_range_t *ranges;
} addr_range_list_t;

/** List of IO mapped areas */
typedef addr_range_list_t io_range_list_t;

/** Memory range type */
typedef addr_range_list_t mem_range_list_t;

/** HW resources parsed according to resource type */
typedef struct hw_resource_list_parsed {
	/** List of IRQs */
	irq_list_t irqs;
	
	/** List of DMA channels */
	dma_list_t dma_channels;
	
	/** List of memory areas */
	mem_range_list_t mem_ranges;
	
	/** List of IO areas */
	io_range_list_t io_ranges;
} hw_res_list_parsed_t;

/** Clean hw_resource_list_parsed_t structure
 *
 * All allocated memory will be released, data amd pointers set to 0.
 *
 * @param list The structure to clear
 */
static inline void hw_res_list_parsed_clean(hw_res_list_parsed_t *list)
{
	if (list == NULL)
		return;
	
	free(list->irqs.irqs);
	free(list->io_ranges.ranges);
	free(list->mem_ranges.ranges);
	free(list->dma_channels.channels);
	
	bzero(list, sizeof(hw_res_list_parsed_t));
}

/** Initialize the hw_resource_list_parsed_t structure
 *
 *  @param list The structure to initialize
 */
static inline void hw_res_list_parsed_init(hw_res_list_parsed_t *list)
{
	bzero(list, sizeof(hw_res_list_parsed_t));
}

extern int hw_res_list_parse(hw_resource_list_t *, hw_res_list_parsed_t *, int);
extern int hw_res_get_list_parsed(async_sess_t *, hw_res_list_parsed_t *, int);

#endif

/** @}
 */
