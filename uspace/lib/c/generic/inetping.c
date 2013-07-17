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

#include <async.h>
#include <assert.h>
#include <errno.h>
#include <inet/inetping.h>
#include <ipc/inet.h>
#include <ipc/services.h>
#include <loc.h>
#include <stdlib.h>
#include <str.h>

static void inetping_cb_conn(ipc_callid_t, ipc_call_t *, void *);
static void inetping_ev_recv(ipc_callid_t, ipc_call_t *);

static async_sess_t *inetping_sess = NULL;
static inetping_ev_ops_t *inetping_ev_ops;

int inetping_init(inetping_ev_ops_t *ev_ops)
{
	service_id_t inetping_svc;
	int rc;

	assert(inetping_sess == NULL);
	
	inetping_ev_ops = ev_ops;
	
	rc = loc_service_get_id(SERVICE_NAME_INETPING, &inetping_svc,
	    IPC_FLAG_BLOCKING);
	if (rc != EOK)
		return ENOENT;
	
	inetping_sess = loc_service_connect(EXCHANGE_SERIALIZE, inetping_svc,
	    IPC_FLAG_BLOCKING);
	if (inetping_sess == NULL)
		return ENOENT;
	
	async_exch_t *exch = async_exchange_begin(inetping_sess);

	rc = async_connect_to_me(exch, 0, 0, 0, inetping_cb_conn, NULL);
	async_exchange_end(exch);
	
	if (rc != EOK) {
		async_hangup(inetping_sess);
		inetping_sess = NULL;
		return rc;
	}
	
	return EOK;
}

int inetping_send(inetping_sdu_t *sdu)
{
	async_exch_t *exch = async_exchange_begin(inetping_sess);

	ipc_call_t answer;
	aid_t req = async_send_3(exch, INETPING_SEND, sdu->src.ipv4,
	    sdu->dest.ipv4, sdu->seq_no, &answer);
	sysarg_t retval = async_data_write_start(exch, sdu->data, sdu->size);

	async_exchange_end(exch);

	if (retval != EOK) {
		async_forget(req);
		return retval;
	}

	async_wait_for(req, &retval);
	return retval;
}

int inetping_get_srcaddr(inet_addr_t *remote, inet_addr_t *local)
{
	sysarg_t local_addr;
	async_exch_t *exch = async_exchange_begin(inetping_sess);

	int rc = async_req_1_1(exch, INETPING_GET_SRCADDR, remote->ipv4,
	    &local_addr);
	async_exchange_end(exch);

	if (rc != EOK)
		return rc;

	local->ipv4 = local_addr;
	return EOK;
}

static void inetping_ev_recv(ipc_callid_t callid, ipc_call_t *call)
{
	int rc;
	inetping_sdu_t sdu;

	sdu.src.ipv4 = IPC_GET_ARG1(*call);
	sdu.dest.ipv4 = IPC_GET_ARG2(*call);
	sdu.seq_no = IPC_GET_ARG3(*call);

	rc = async_data_write_accept(&sdu.data, false, 0, 0, 0, &sdu.size);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		return;
	}

	rc = inetping_ev_ops->recv(&sdu);
	free(sdu.data);
	async_answer_0(callid, rc);
}

static void inetping_cb_conn(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	while (true) {
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);

		if (!IPC_GET_IMETHOD(call)) {
			/* TODO: Handle hangup */
			return;
		}

		switch (IPC_GET_IMETHOD(call)) {
		case INETPING_EV_RECV:
			inetping_ev_recv(callid, &call);
			break;
		default:
			async_answer_0(callid, ENOTSUP);
		}
	}
}

/** @}
 */
