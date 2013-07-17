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

/** @addtogroup tcp
 * @{
 */

/**
 * @file TCP (Transmission Control Protocol) network module
 */

#include <async.h>
#include <bitops.h>
#include <byteorder.h>
#include <errno.h>
#include <inet/inet.h>
#include <io/log.h>
#include <stdio.h>
#include <task.h>

#include "ncsim.h"
#include "pdu.h"
#include "rqueue.h"
#include "sock.h"
#include "std.h"
#include "tcp.h"
#include "test.h"

#define NAME       "tcp"

#define IP_PROTO_TCP 6

static int tcp_inet_ev_recv(inet_dgram_t *dgram);
static void tcp_received_pdu(tcp_pdu_t *pdu);

static inet_ev_ops_t tcp_inet_ev_ops = {
	.recv = tcp_inet_ev_recv
};

/** Received datagram callback */
static int tcp_inet_ev_recv(inet_dgram_t *dgram)
{
	uint8_t *pdu_raw;
	size_t pdu_raw_size;

	log_msg(LVL_DEBUG, "tcp_inet_ev_recv()");

	pdu_raw = dgram->data;
	pdu_raw_size = dgram->size;

	/* Split into header and payload. */

	log_msg(LVL_DEBUG, "tcp_inet_ev_recv() - split header/payload");

	tcp_pdu_t *pdu;
	size_t hdr_size;
	tcp_header_t *hdr;
	uint32_t data_offset;

	if (pdu_raw_size < sizeof(tcp_header_t)) {
		log_msg(LVL_WARN, "pdu_raw_size = %zu < sizeof(tcp_header_t) = %zu",
		    pdu_raw_size, sizeof(tcp_header_t));
		return EINVAL;
	}

	hdr = (tcp_header_t *)pdu_raw;
	data_offset = BIT_RANGE_EXTRACT(uint32_t, DF_DATA_OFFSET_h, DF_DATA_OFFSET_l,
	    uint16_t_be2host(hdr->doff_flags));

	hdr_size = sizeof(uint32_t) * data_offset;

	if (pdu_raw_size < hdr_size) {
		log_msg(LVL_WARN, "pdu_raw_size = %zu < hdr_size = %zu",
		    pdu_raw_size, hdr_size);
		return EINVAL;
	}

	if (hdr_size < sizeof(tcp_header_t)) {
		log_msg(LVL_WARN, "hdr_size = %zu < sizeof(tcp_header_t) = %zu",
		    hdr_size, sizeof(tcp_header_t));		return EINVAL;
	}

	log_msg(LVL_DEBUG, "pdu_raw_size=%zu, hdr_size=%zu",
	    pdu_raw_size, hdr_size);
	pdu = tcp_pdu_create(pdu_raw, hdr_size, pdu_raw + hdr_size,
	    pdu_raw_size - hdr_size);
	if (pdu == NULL) {
		log_msg(LVL_WARN, "Failed creating PDU. Dropped.");
		return ENOMEM;
	}

	pdu->src_addr.ipv4 = dgram->src.ipv4;
	pdu->dest_addr.ipv4 = dgram->dest.ipv4;
	log_msg(LVL_DEBUG, "src: 0x%08x, dest: 0x%08x",
	    pdu->src_addr.ipv4, pdu->dest_addr.ipv4);

	tcp_received_pdu(pdu);
	tcp_pdu_delete(pdu);

	return EOK;
}

/** Transmit PDU over network layer. */
void tcp_transmit_pdu(tcp_pdu_t *pdu)
{
	int rc;
	uint8_t *pdu_raw;
	size_t pdu_raw_size;
	inet_dgram_t dgram;

	pdu_raw_size = pdu->header_size + pdu->text_size;
	pdu_raw = malloc(pdu_raw_size);
	if (pdu_raw == NULL) {
		log_msg(LVL_ERROR, "Failed to transmit PDU. Out of memory.");
		return;
	}

	memcpy(pdu_raw, pdu->header, pdu->header_size);
	memcpy(pdu_raw + pdu->header_size, pdu->text,
	    pdu->text_size);

	dgram.src.ipv4 = pdu->src_addr.ipv4;
	dgram.dest.ipv4 = pdu->dest_addr.ipv4;
	dgram.tos = 0;
	dgram.data = pdu_raw;
	dgram.size = pdu_raw_size;

	rc = inet_send(&dgram, INET_TTL_MAX, 0);
	if (rc != EOK)
		log_msg(LVL_ERROR, "Failed to transmit PDU.");
}

/** Process received PDU. */
static void tcp_received_pdu(tcp_pdu_t *pdu)
{
	tcp_segment_t *dseg;
	tcp_sockpair_t rident;

	log_msg(LVL_DEBUG, "tcp_received_pdu()");

	if (tcp_pdu_decode(pdu, &rident, &dseg) != EOK) {
		log_msg(LVL_WARN, "Not enough memory. PDU dropped.");
		return;
	}

	/* Insert decoded segment into rqueue */
	tcp_rqueue_insert_seg(&rident, dseg);
}

static int tcp_init(void)
{
	int rc;

	log_msg(LVL_DEBUG, "tcp_init()");

	tcp_rqueue_init();
	tcp_rqueue_fibril_start();

	tcp_ncsim_init();
	tcp_ncsim_fibril_start();

	if (0) tcp_test();

	rc = inet_init(IP_PROTO_TCP, &tcp_inet_ev_ops);
	if (rc != EOK) {
		log_msg(LVL_ERROR, "Failed connecting to internet service.");
		return ENOENT;
	}

	rc = tcp_sock_init();
	if (rc != EOK) {
		log_msg(LVL_ERROR, "Failed initializing socket service.");
		return ENOENT;
	}

	return EOK;
}

int main(int argc, char **argv)
{
	int rc;

	printf(NAME ": TCP (Transmission Control Protocol) network module\n");

	rc = log_init(NAME, LVL_WARN);
	if (rc != EOK) {
		printf(NAME ": Failed to initialize log.\n");
		return 1;
	}

	rc = tcp_init();
	if (rc != EOK)
		return 1;

	printf(NAME ": Accepting connections.\n");
	task_retval(0);
	async_manager();

	/* Not reached */
	return 0;
}

/**
 * @}
 */
