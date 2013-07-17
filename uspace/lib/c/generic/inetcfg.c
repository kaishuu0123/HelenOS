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
#include <inet/inetcfg.h>
#include <ipc/inet.h>
#include <ipc/services.h>
#include <loc.h>
#include <stdlib.h>
#include <str.h>

static async_sess_t *inetcfg_sess = NULL;

static int inetcfg_get_ids_once(sysarg_t method, sysarg_t arg1,
    sysarg_t *id_buf, size_t buf_size, size_t *act_size)
{
	async_exch_t *exch = async_exchange_begin(inetcfg_sess);

	ipc_call_t answer;
	aid_t req = async_send_1(exch, method, arg1, &answer);
	int rc = async_data_read_start(exch, id_buf, buf_size);

	async_exchange_end(exch);

	if (rc != EOK) {
		async_forget(req);
		return rc;
	}

	sysarg_t retval;
	async_wait_for(req, &retval);

	if (retval != EOK) {
		return retval;
	}

	*act_size = IPC_GET_ARG1(answer);
	return EOK;
}

/** Get list of IDs.
 *
 * Returns an allocated array of service IDs.
 *
 * @param method	IPC method
 * @param arg1		IPC argument 1
 * @param data		Place to store pointer to array of IDs
 * @param count		Place to store number of IDs
 * @return 		EOK on success or negative error code
 */
static int inetcfg_get_ids_internal(sysarg_t method, sysarg_t arg1,
    sysarg_t **data, size_t *count)
{
	*data = NULL;
	*count = 0;

	size_t act_size = 0;
	int rc = inetcfg_get_ids_once(method, arg1, NULL, 0,
	    &act_size);
	if (rc != EOK)
		return rc;

	size_t alloc_size = act_size;
	service_id_t *ids = malloc(alloc_size);
	if (ids == NULL)
		return ENOMEM;

	while (true) {
		rc = inetcfg_get_ids_once(method, arg1, ids, alloc_size,
		    &act_size);
		if (rc != EOK)
			return rc;

		if (act_size <= alloc_size)
			break;

		alloc_size = act_size;
		ids = realloc(ids, alloc_size);
		if (ids == NULL)
			return ENOMEM;
	}

	*count = act_size / sizeof(sysarg_t);
	*data = ids;
	return EOK;
}

int inetcfg_init(void)
{
	service_id_t inet_svc;
	int rc;

	assert(inetcfg_sess == NULL);
	
	rc = loc_service_get_id(SERVICE_NAME_INETCFG, &inet_svc,
	    IPC_FLAG_BLOCKING);
	if (rc != EOK)
		return ENOENT;
	
	inetcfg_sess = loc_service_connect(EXCHANGE_SERIALIZE, inet_svc,
	    IPC_FLAG_BLOCKING);
	if (inetcfg_sess == NULL)
		return ENOENT;
	
	return EOK;
}

int inetcfg_addr_create_static(const char *name, inet_naddr_t *naddr,
    sysarg_t link_id, sysarg_t *addr_id)
{
	async_exch_t *exch = async_exchange_begin(inetcfg_sess);

	ipc_call_t answer;
	aid_t req = async_send_3(exch, INETCFG_ADDR_CREATE_STATIC, naddr->ipv4,
	    naddr->bits, link_id, &answer);
	sysarg_t retval = async_data_write_start(exch, name, str_size(name));

	async_exchange_end(exch);

	if (retval != EOK) {
		async_forget(req);
		return retval;
	}

	async_wait_for(req, &retval);
	*addr_id = IPC_GET_ARG1(answer);

	return retval;
}

int inetcfg_addr_delete(sysarg_t addr_id)
{
	async_exch_t *exch = async_exchange_begin(inetcfg_sess);

	int rc = async_req_1_0(exch, INETCFG_ADDR_DELETE, addr_id);
	async_exchange_end(exch);

	return rc;
}

int inetcfg_addr_get(sysarg_t addr_id, inet_addr_info_t *ainfo)
{
	ipc_call_t dreply;
	sysarg_t dretval;
	size_t act_size;
	char name_buf[LOC_NAME_MAXLEN + 1];

	async_exch_t *exch = async_exchange_begin(inetcfg_sess);

	ipc_call_t answer;
	aid_t req = async_send_1(exch, INETCFG_ADDR_GET, addr_id, &answer);
	aid_t dreq = async_data_read(exch, name_buf, LOC_NAME_MAXLEN, &dreply);
	async_wait_for(dreq, &dretval);

	async_exchange_end(exch);

	if (dretval != EOK) {
		async_forget(req);
		return dretval;
	}

	sysarg_t retval;
	async_wait_for(req, &retval);

	if (retval != EOK)
		return retval;

	act_size = IPC_GET_ARG2(dreply);
	assert(act_size <= LOC_NAME_MAXLEN);
	name_buf[act_size] = '\0';

	ainfo->naddr.ipv4 = IPC_GET_ARG1(answer);
	ainfo->naddr.bits = IPC_GET_ARG2(answer);
	ainfo->ilink = IPC_GET_ARG3(answer);
	ainfo->name = str_dup(name_buf);

	return EOK;
}

int inetcfg_addr_get_id(const char *name, sysarg_t link_id, sysarg_t *addr_id)
{
	async_exch_t *exch = async_exchange_begin(inetcfg_sess);

	ipc_call_t answer;
	aid_t req = async_send_1(exch, INETCFG_ADDR_GET_ID, link_id, &answer);
	sysarg_t retval = async_data_write_start(exch, name, str_size(name));

	async_exchange_end(exch);

	if (retval != EOK) {
		async_forget(req);
		return retval;
	}

	async_wait_for(req, &retval);
	*addr_id = IPC_GET_ARG1(answer);

	return retval;
}

int inetcfg_get_addr_list(sysarg_t **addrs, size_t *count)
{
	return inetcfg_get_ids_internal(INETCFG_GET_ADDR_LIST,
	    0, addrs, count);
}

int inetcfg_get_link_list(sysarg_t **links, size_t *count)
{
	return inetcfg_get_ids_internal(INETCFG_GET_LINK_LIST,
	    0, links, count);
}

int inetcfg_get_sroute_list(sysarg_t **sroutes, size_t *count)
{
	return inetcfg_get_ids_internal(INETCFG_GET_SROUTE_LIST,
	    0, sroutes, count);
}

int inetcfg_link_get(sysarg_t link_id, inet_link_info_t *linfo)
{
	ipc_call_t dreply;
	sysarg_t dretval;
	size_t act_size;
	char name_buf[LOC_NAME_MAXLEN + 1];

	async_exch_t *exch = async_exchange_begin(inetcfg_sess);

	ipc_call_t answer;
	aid_t req = async_send_1(exch, INETCFG_LINK_GET, link_id, &answer);
	aid_t dreq = async_data_read(exch, name_buf, LOC_NAME_MAXLEN, &dreply);
	async_wait_for(dreq, &dretval);

	async_exchange_end(exch);

	if (dretval != EOK) {
		async_forget(req);
		return dretval;
	}

	sysarg_t retval;
	async_wait_for(req, &retval);

	if (retval != EOK)
		return retval;

	act_size = IPC_GET_ARG2(dreply);
	assert(act_size <= LOC_NAME_MAXLEN);
	name_buf[act_size] = '\0';

	linfo->name = str_dup(name_buf);
	linfo->def_mtu = IPC_GET_ARG1(answer);

	return EOK;
}

int inetcfg_sroute_create(const char *name, inet_naddr_t *dest,
    inet_addr_t *router, sysarg_t *sroute_id)
{
	async_exch_t *exch = async_exchange_begin(inetcfg_sess);

	ipc_call_t answer;
	aid_t req = async_send_3(exch, INETCFG_SROUTE_CREATE,
	    dest->ipv4, dest->bits, router->ipv4, &answer);
	sysarg_t retval = async_data_write_start(exch, name, str_size(name));

	async_exchange_end(exch);

	if (retval != EOK) {
		async_forget(req);
		return retval;
	}

	async_wait_for(req, &retval);
	*sroute_id = IPC_GET_ARG1(answer);

	return retval;
}

int inetcfg_sroute_delete(sysarg_t sroute_id)
{
	async_exch_t *exch = async_exchange_begin(inetcfg_sess);

	int rc = async_req_1_0(exch, INETCFG_SROUTE_DELETE, sroute_id);
	async_exchange_end(exch);

	return rc;
}

int inetcfg_sroute_get(sysarg_t sroute_id, inet_sroute_info_t *srinfo)
{
	ipc_call_t dreply;
	sysarg_t dretval;
	size_t act_size;
	char name_buf[LOC_NAME_MAXLEN + 1];

	async_exch_t *exch = async_exchange_begin(inetcfg_sess);

	ipc_call_t answer;
	aid_t req = async_send_1(exch, INETCFG_SROUTE_GET, sroute_id, &answer);
	aid_t dreq = async_data_read(exch, name_buf, LOC_NAME_MAXLEN, &dreply);
	async_wait_for(dreq, &dretval);

	async_exchange_end(exch);

	if (dretval != EOK) {
		async_forget(req);
		return dretval;
	}

	sysarg_t retval;
	async_wait_for(req, &retval);

	if (retval != EOK)
		return retval;

	act_size = IPC_GET_ARG2(dreply);
	assert(act_size <= LOC_NAME_MAXLEN);
	name_buf[act_size] = '\0';

	srinfo->dest.ipv4 = IPC_GET_ARG1(answer);
	srinfo->dest.bits = IPC_GET_ARG2(answer);
	srinfo->router.ipv4 = IPC_GET_ARG3(answer);
	srinfo->name = str_dup(name_buf);

	return EOK;
}

int inetcfg_sroute_get_id(const char *name, sysarg_t *sroute_id)
{
	async_exch_t *exch = async_exchange_begin(inetcfg_sess);

	ipc_call_t answer;
	aid_t req = async_send_0(exch, INETCFG_SROUTE_GET_ID, &answer);
	sysarg_t retval = async_data_write_start(exch, name, str_size(name));

	async_exchange_end(exch);

	if (retval != EOK) {
		async_forget(req);
		return retval;
	}

	async_wait_for(req, &retval);
	*sroute_id = IPC_GET_ARG1(answer);

	return retval;
}

/** @}
 */
