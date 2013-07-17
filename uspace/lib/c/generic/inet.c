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
#include <inet/inet.h>
#include <ipc/inet.h>
#include <ipc/services.h>
#include <loc.h>

static void inet_cb_conn(ipc_callid_t iid, ipc_call_t *icall, void *arg);

static async_sess_t *inet_sess = NULL;
static inet_ev_ops_t *inet_ev_ops = NULL;
static uint8_t inet_protocol = 0;

static int inet_callback_create(void)
{
	async_exch_t *exch = async_exchange_begin(inet_sess);
	
	ipc_call_t answer;
	aid_t req = async_send_0(exch, INET_CALLBACK_CREATE, &answer);
	int rc = async_connect_to_me(exch, 0, 0, 0, inet_cb_conn, NULL);
	async_exchange_end(exch);
	
	if (rc != EOK)
		return rc;
	
	sysarg_t retval;
	async_wait_for(req, &retval);
	
	return retval;
}

static int inet_set_proto(uint8_t protocol)
{
	async_exch_t *exch = async_exchange_begin(inet_sess);
	int rc = async_req_1_0(exch, INET_SET_PROTO, protocol);
	async_exchange_end(exch);
	
	return rc;
}

int inet_init(uint8_t protocol, inet_ev_ops_t *ev_ops)
{
	service_id_t inet_svc;
	int rc;

	assert(inet_sess == NULL);
	assert(inet_ev_ops == NULL);
	assert(inet_protocol == 0);
	
	rc = loc_service_get_id(SERVICE_NAME_INET, &inet_svc,
	    IPC_FLAG_BLOCKING);
	if (rc != EOK)
		return ENOENT;
	
	inet_sess = loc_service_connect(EXCHANGE_SERIALIZE, inet_svc,
	    IPC_FLAG_BLOCKING);
	if (inet_sess == NULL)
		return ENOENT;
	
	if (inet_set_proto(protocol) != EOK) {
		async_hangup(inet_sess);
		inet_sess = NULL;
		return EIO;
	}
	
	if (inet_callback_create() != EOK) {
		async_hangup(inet_sess);
		inet_sess = NULL;
		return EIO;
	}
	
	inet_protocol = protocol;
	inet_ev_ops = ev_ops;

	return EOK;
}

int inet_send(inet_dgram_t *dgram, uint8_t ttl, inet_df_t df)
{
	async_exch_t *exch = async_exchange_begin(inet_sess);

	ipc_call_t answer;
	aid_t req = async_send_5(exch, INET_SEND, dgram->src.ipv4,
	    dgram->dest.ipv4, dgram->tos, ttl, df, &answer);
	int rc = async_data_write_start(exch, dgram->data, dgram->size);
	async_exchange_end(exch);

	if (rc != EOK) {
		async_forget(req);
		return rc;
	}

	sysarg_t retval;
	async_wait_for(req, &retval);
	if (retval != EOK)
		return retval;

	return EOK;
}

int inet_get_srcaddr(inet_addr_t *remote, uint8_t tos, inet_addr_t *local)
{
	sysarg_t local_addr;
	async_exch_t *exch = async_exchange_begin(inet_sess);

	int rc = async_req_2_1(exch, INET_GET_SRCADDR, remote->ipv4,
	    tos, &local_addr);
	async_exchange_end(exch);

	if (rc != EOK)
		return rc;

	local->ipv4 = local_addr;
	return EOK;
}

static void inet_ev_recv(ipc_callid_t callid, ipc_call_t *call)
{
	int rc;
	inet_dgram_t dgram;

	dgram.src.ipv4 = IPC_GET_ARG1(*call);
	dgram.dest.ipv4 = IPC_GET_ARG2(*call);
	dgram.tos = IPC_GET_ARG3(*call);

	rc = async_data_write_accept(&dgram.data, false, 0, 0, 0, &dgram.size);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		return;
	}

	rc = inet_ev_ops->recv(&dgram);
	async_answer_0(callid, rc);
}

static void inet_cb_conn(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	while (true) {
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);

		if (!IPC_GET_IMETHOD(call)) {
			/* TODO: Handle hangup */
			return;
		}

		switch (IPC_GET_IMETHOD(call)) {
		case INET_EV_RECV:
			inet_ev_recv(callid, &call);
			break;
		default:
			async_answer_0(callid, ENOTSUP);
		}
	}
}

/** @}
 */
