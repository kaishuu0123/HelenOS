/*
 * Copyright (c) 2007 Pavel Jancik, Michal Kebrt
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

/** @addtogroup arm32mm
 * @{
 */
/** @file
 *  @brief Page fault related declarations.
 */

#ifndef KERN_arm32_PAGE_FAULT_H_
#define KERN_arm32_PAGE_FAULT_H_

#include <typedefs.h>


/** Decribes CP15 "fault status register" (FSR). */
typedef struct {
	unsigned status : 3;
	unsigned domain : 4;
	unsigned zero : 1;
	unsigned should_be_zero : 24;
} ATTRIBUTE_PACKED fault_status_t;


/** Help union used for casting integer value into #fault_status_t. */
typedef union {
	fault_status_t fs;
	uint32_t dummy;
} fault_status_union_t;


/** Simplified description of instruction code.
 *
 * @note Used for recognizing memory access instructions.
 * @see ARM architecture reference (chapter 3.1)
 */
typedef struct {
	unsigned dummy1 : 4;
	unsigned bit4 : 1;
	unsigned bits567 : 3;
	unsigned dummy : 12;
	unsigned access : 1;
	unsigned opcode : 4;
	unsigned type : 3;
	unsigned condition : 4;
} ATTRIBUTE_PACKED instruction_t;


/** Help union used for casting pc register (uint_32_t) value into
 *  #instruction_t pointer.
 */
typedef union {
	instruction_t *instr;
	uint32_t pc;
} instruction_union_t;

extern void prefetch_abort(unsigned int, istate_t *);
extern void data_abort(unsigned int, istate_t *);

#endif

/** @}
 */
