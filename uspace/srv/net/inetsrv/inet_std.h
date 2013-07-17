/*
 * Copyright (c) 2012 Jiri Svoboda
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

/** @addtogroup inet
 * @{
 */
/**
 * @file IP header definitions
 *
 */

#ifndef INET_STD_H_
#define INET_STD_H_

#include <sys/types.h>

/** Internet Datagram header (fixed part) */
typedef struct {
	/** Version, Internet Header Length */
	uint8_t ver_ihl;
	/* Type of Service */
	uint8_t tos;
	/** Total Length */
	uint16_t tot_len;
	/** Identification */
	uint16_t id;
	/** Flags, Fragment Offset */
	uint16_t flags_foff;
	/** Time to Live */
	uint8_t ttl;
	/** Protocol */
	uint8_t proto;
	/** Header Checksum */
	uint16_t chksum;
	/** Source Address */
	uint32_t src_addr;
	/** Destination Address */
	uint32_t dest_addr;
} ip_header_t;

/** Bits in ip_header_t.ver_ihl */
enum ver_ihl_bits {
	/** Version, highest bit */
	VI_VERSION_h = 7,
	/** Version, lowest bit */
	VI_VERSION_l = 4,
	/** Internet Header Length, highest bit */
	VI_IHL_h     = 3,
	/** Internet Header Length, lowest bit */
	VI_IHL_l     = 0
};

/** Bits in ip_header_t.flags_foff */
enum flags_foff_bits {
	/** Reserved, must be zero */
	FF_FLAG_RSVD = 15,
	/** Don't Fragment */
	FF_FLAG_DF = 14,
	/** More Fragments */
	FF_FLAG_MF = 13,
	/** Fragment Offset, highest bit */
	FF_FRAGOFF_h = 12,
	/** Fragment Offset, lowest bit */
	FF_FRAGOFF_l = 0
};

/** Fragment offset is expressed in units of 8 bytes */
#define FRAG_OFFS_UNIT 8

#endif

/** @}
 */
