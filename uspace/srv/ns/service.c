/*
 * Copyright (c) 2009 Martin Decky
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

/** @addtogroup ns
 * @{
 */

#include <ipc/ipc.h>
#include <adt/hash_table.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <malloc.h>
#include "service.h"
#include "ns.h"

#define SERVICE_HASH_TABLE_CHAINS  20

/** Service hash table item. */
typedef struct {
	link_t link;
	sysarg_t service;        /**< Service ID. */
	sysarg_t phone;          /**< Phone registered with the service. */
	sysarg_t in_phone_hash;  /**< Incoming phone hash. */
} hashed_service_t;

/** Compute hash index into service hash table.
 *
 * @param key Pointer keys. However, only the first key (i.e. service number)
 *            is used to compute the hash index.
 *
 * @return Hash index corresponding to key[0].
 *
 */
static hash_index_t service_hash(unsigned long key[])
{
	assert(key);
	return (key[0] % SERVICE_HASH_TABLE_CHAINS);
}

/** Compare a key with hashed item.
 *
 * This compare function always ignores the third key.
 * It exists only to make it possible to remove records
 * originating from connection with key[1] in_phone_hash
 * value. Note that this is close to being classified
 * as a nasty hack.
 *
 * @param key  Array of keys.
 * @param keys Must be lesser or equal to 3.
 * @param item Pointer to a hash table item.
 *
 * @return Non-zero if the key matches the item, zero otherwise.
 *
 */
static int service_compare(unsigned long key[], hash_count_t keys, link_t *item)
{
	assert(key);
	assert(keys <= 3);
	assert(item);
	
	hashed_service_t *hs = hash_table_get_instance(item, hashed_service_t, link);
	
	if (keys == 2)
		return ((key[0] == hs->service) && (key[1] == hs->in_phone_hash));
	else
		return (key[0] == hs->service);
}

/** Perform actions after removal of item from the hash table.
 *
 * @param item Item that was removed from the hash table.
 *
 */
static void service_remove(link_t *item)
{
	assert(item);
	free(hash_table_get_instance(item, hashed_service_t, link));
}

/** Operations for service hash table. */
static hash_table_operations_t service_hash_table_ops = {
	.hash = service_hash,
	.compare = service_compare,
	.remove_callback = service_remove
};

/** Service hash table structure. */
static hash_table_t service_hash_table;

/** Pending connection structure. */
typedef struct {
	link_t link;
	sysarg_t service;        /**< Number of the service. */
	ipc_callid_t callid;     /**< Call ID waiting for the connection */
	sysarg_t arg2;           /**< Second argument */
	sysarg_t arg3;           /**< Third argument */
} pending_conn_t;

static list_t pending_conn;

int service_init(void)
{
	if (!hash_table_create(&service_hash_table, SERVICE_HASH_TABLE_CHAINS,
	    3, &service_hash_table_ops)) {
		printf(NAME ": No memory available for services\n");
		return ENOMEM;
	}
	
	list_initialize(&pending_conn);
	
	return EOK;
}

/** Process pending connection requests */
void process_pending_conn(void)
{
loop:
	list_foreach(pending_conn, cur) {
		pending_conn_t *pr = list_get_instance(cur, pending_conn_t, link);
		
		unsigned long keys[3] = {
			pr->service,
			0,
			0
		};
		
		link_t *link = hash_table_find(&service_hash_table, keys);
		if (!link)
			continue;
		
		hashed_service_t *hs = hash_table_get_instance(link, hashed_service_t, link);
		(void) ipc_forward_fast(pr->callid, hs->phone, pr->arg2,
		    pr->arg3, 0, IPC_FF_NONE);
		
		list_remove(cur);
		free(pr);
		goto loop;
	}
}

/** Register service.
 *
 * @param service Service to be registered.
 * @param phone   Phone to be used for connections to the service.
 * @param call    Pointer to call structure.
 *
 * @return Zero on success or a value from @ref errno.h.
 *
 */
int register_service(sysarg_t service, sysarg_t phone, ipc_call_t *call)
{
	unsigned long keys[3] = {
		service,
		call->in_phone_hash,
		0
	};
	
	if (hash_table_find(&service_hash_table, keys))
		return EEXISTS;
	
	hashed_service_t *hs = (hashed_service_t *) malloc(sizeof(hashed_service_t));
	if (!hs)
		return ENOMEM;
	
	link_initialize(&hs->link);
	hs->service = service;
	hs->phone = phone;
	hs->in_phone_hash = call->in_phone_hash;
	hash_table_insert(&service_hash_table, keys, &hs->link);
	
	return EOK;
}

/** Connect client to service.
 *
 * @param service Service to be connected to.
 * @param call    Pointer to call structure.
 * @param callid  Call ID of the request.
 *
 * @return Zero on success or a value from @ref errno.h.
 *
 */
void connect_to_service(sysarg_t service, ipc_call_t *call, ipc_callid_t callid)
{
	sysarg_t retval;
	unsigned long keys[3] = {
		service,
		0,
		0
	};
	
	link_t *link = hash_table_find(&service_hash_table, keys);
	if (!link) {
		if (IPC_GET_ARG4(*call) & IPC_FLAG_BLOCKING) {
			/* Blocking connection, add to pending list */
			pending_conn_t *pr =
			    (pending_conn_t *) malloc(sizeof(pending_conn_t));
			if (!pr) {
				retval = ENOMEM;
				goto out;
			}
			
			link_initialize(&pr->link);
			pr->service = service;
			pr->callid = callid;
			pr->arg2 = IPC_GET_ARG2(*call);
			pr->arg3 = IPC_GET_ARG3(*call);
			list_append(&pr->link, &pending_conn);
			return;
		}
		retval = ENOENT;
		goto out;
	}
	
	hashed_service_t *hs = hash_table_get_instance(link, hashed_service_t, link);
	(void) ipc_forward_fast(callid, hs->phone, IPC_GET_ARG2(*call),
	    IPC_GET_ARG3(*call), 0, IPC_FF_NONE);
	return;
	
out:
	if (!(callid & IPC_CALLID_NOTIFICATION))
		ipc_answer_0(callid, retval);
}

/**
 * @}
 */
