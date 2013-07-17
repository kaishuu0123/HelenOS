/*
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

/** @addtogroup trace
 * @{
 */
/** @file
 */

#include <stdio.h>
#include <stdlib.h>
#include <adt/hash_table.h>
#include <sys/typefmt.h>
#include <abi/ipc/methods.h>
#include "ipc_desc.h"
#include "proto.h"
#include "trace.h"
#include "ipcp.h"

#define IPCP_CALLID_SYNC 0

typedef struct {
	sysarg_t phone_hash;
	ipc_call_t question;
	oper_t *oper;

	ipc_callid_t call_hash;

	link_t link;
} pending_call_t;

typedef struct {
	int server;
	proto_t *proto;
} connection_t;

#define MAX_PHONE 64
connection_t connections[MAX_PHONE];
int have_conn[MAX_PHONE];

#define PCALL_TABLE_CHAINS 32
hash_table_t pending_calls;

/*
 * Pseudo-protocols
 */
proto_t *proto_system;		/**< Protocol describing system IPC methods. */
proto_t	*proto_unknown;		/**< Protocol with no known methods. */

static hash_index_t pending_call_hash(unsigned long key[]);
static int pending_call_compare(unsigned long key[], hash_count_t keys,
    link_t *item);
static void pending_call_remove_callback(link_t *item);

hash_table_operations_t pending_call_ops = {
	.hash = pending_call_hash,
	.compare = pending_call_compare,
	.remove_callback = pending_call_remove_callback
};


static hash_index_t pending_call_hash(unsigned long key[])
{
//	printf("pending_call_hash\n");
	return key[0] % PCALL_TABLE_CHAINS;
}

static int pending_call_compare(unsigned long key[], hash_count_t keys,
    link_t *item)
{
	pending_call_t *hs;

//	printf("pending_call_compare\n");
	hs = hash_table_get_instance(item, pending_call_t, link);

	// FIXME: this will fail if sizeof(long) < sizeof(void *).
	return key[0] == hs->call_hash;
}

static void pending_call_remove_callback(link_t *item)
{
//	printf("pending_call_remove_callback\n");
}


void ipcp_connection_set(int phone, int server, proto_t *proto)
{
	if (phone <0 || phone >= MAX_PHONE) return;
	connections[phone].server = server;
	connections[phone].proto = proto;
	have_conn[phone] = 1;
}

void ipcp_connection_clear(int phone)
{
	have_conn[phone] = 0;
	connections[phone].server = 0;
	connections[phone].proto = NULL;
}

static void ipc_m_print(proto_t *proto, sysarg_t method)
{
	oper_t *oper;

	/* Try system methods first */
	oper = proto_get_oper(proto_system, method);

	if (oper == NULL && proto != NULL) {
		/* Not a system method, try the user protocol. */
		oper = proto_get_oper(proto, method);
	}

	if (oper != NULL) {
		printf("%s (%" PRIun ")", oper->name, method);
		return;
	}

	printf("%" PRIun, method);
}

void ipcp_init(void)
{
	ipc_m_desc_t *desc;
	oper_t *oper;

	val_type_t arg_def[OPER_MAX_ARGS] = {
		V_INTEGER,
		V_INTEGER,
		V_INTEGER,
		V_INTEGER,
		V_INTEGER		
	};

	/*
	 * Create a pseudo-protocol 'unknown' that has no known methods.
	 */
	proto_unknown = proto_new("unknown");

	/*
	 * Create a pseudo-protocol 'system' defining names of system IPC
	 * methods.
	 */
	proto_system = proto_new("system");

	desc = ipc_methods;
	while (desc->number != 0) {
		oper = oper_new(desc->name, OPER_MAX_ARGS, arg_def, V_INTEGER,
			OPER_MAX_ARGS, arg_def);
		proto_add_oper(proto_system, desc->number, oper);

		++desc;
	}

	hash_table_create(&pending_calls, PCALL_TABLE_CHAINS, 1, &pending_call_ops);
}

void ipcp_cleanup(void)
{
	proto_delete(proto_system);
	hash_table_destroy(&pending_calls);
}

void ipcp_call_out(int phone, ipc_call_t *call, ipc_callid_t hash)
{
	pending_call_t *pcall;
	proto_t *proto;
	unsigned long key[1];
	oper_t *oper;
	sysarg_t *args;
	int i;

	if (have_conn[phone]) proto = connections[phone].proto;
	else proto = NULL;

	args = call->args;

	if ((display_mask & DM_IPC) != 0) {
		printf("Call ID: %p, phone: %d, proto: %s, method: ",
		    (void *) hash, phone,
		    (proto ? proto->name : "n/a"));
		ipc_m_print(proto, IPC_GET_IMETHOD(*call));
		printf(" args: (%" PRIun ", %" PRIun ", %" PRIun ", "
		    "%" PRIun ", %" PRIun ")\n",
		    args[1], args[2], args[3], args[4], args[5]);
	}


	if ((display_mask & DM_USER) != 0) {

		if (proto != NULL) {
			oper = proto_get_oper(proto, IPC_GET_IMETHOD(*call));
		} else {
			oper = NULL;
		}

		if (oper != NULL) {

			printf("%s(%d).%s", (proto ? proto->name : "n/a"),
			    phone, (oper ? oper->name : "unknown"));

			putchar('(');
			for (i = 1; i <= oper->argc; ++i) {
				if (i > 1) printf(", ");
				val_print(args[i], oper->arg_type[i - 1]);
			}
			putchar(')');

			if (oper->rv_type == V_VOID && oper->respc == 0) {
				/*
				 * No response data (typically the task will
				 * not be interested in the response).
				 * We will not display response.
				 */
				putchar('.');
			}

			putchar('\n');
		}
	} else {
		oper = NULL;
	}

	/* Store call in hash table for response matching */

	pcall = malloc(sizeof(pending_call_t));
	pcall->phone_hash = phone;
	pcall->question = *call;
	pcall->call_hash = hash;
	pcall->oper = oper;

	key[0] = hash;

	hash_table_insert(&pending_calls, key, &pcall->link);
}

static void parse_answer(ipc_callid_t hash, pending_call_t *pcall,
    ipc_call_t *answer)
{
	sysarg_t phone;
	sysarg_t method;
	sysarg_t service;
	sysarg_t retval;
	proto_t *proto;
	int cphone;
	
	sysarg_t *resp;
	oper_t *oper;
	int i;
	
	phone = pcall->phone_hash;
	method = IPC_GET_IMETHOD(pcall->question);
	retval = IPC_GET_RETVAL(*answer);
	
	resp = answer->args;
	
	if ((display_mask & DM_IPC) != 0) {
		printf("Response to %p: retval=%" PRIdn ", args = (%" PRIun ", "
		    "%" PRIun ", %" PRIun ", %" PRIun ", %" PRIun ")\n",
		    (void *) hash, retval, IPC_GET_ARG1(*answer),
		    IPC_GET_ARG2(*answer), IPC_GET_ARG3(*answer),
		    IPC_GET_ARG4(*answer), IPC_GET_ARG5(*answer));
	}
	
	if ((display_mask & DM_USER) != 0) {
		oper = pcall->oper;
		
		if ((oper != NULL) &&
		    ((oper->rv_type != V_VOID) || (oper->respc > 0))) {
			printf("->");
			
			if (oper->rv_type != V_VOID) {
				putchar(' ');
				val_print(retval, oper->rv_type);
			}
			
			if (oper->respc > 0) {
				putchar(' ');
				putchar('(');
				for (i = 1; i <= oper->respc; ++i) {
					if (i > 1)
						printf(", ");
					val_print(resp[i], oper->resp_type[i - 1]);
				}
				putchar(')');
			}
			
			putchar('\n');
		}
	}
	
	if ((phone == PHONE_NS) && (method == IPC_M_CONNECT_ME_TO) &&
	    (retval == 0)) {
		/* Connected to a service (through NS) */
		service = IPC_GET_ARG1(pcall->question);
		proto = proto_get_by_srv(service);
		if (proto == NULL)
			proto = proto_unknown;
		
		cphone = IPC_GET_ARG5(*answer);
		if ((display_mask & DM_SYSTEM) != 0) {
			printf("Registering connection (phone %d, protocol: %s)\n", cphone,
		    proto->name);
		}
		
		ipcp_connection_set(cphone, 0, proto);
	}
}

void ipcp_call_in(ipc_call_t *call, ipc_callid_t hash)
{
	link_t *item;
	pending_call_t *pcall;
	unsigned long key[1];
	
	if ((hash & IPC_CALLID_ANSWERED) == 0 && hash != IPCP_CALLID_SYNC) {
		/* Not a response */
		if ((display_mask & DM_IPC) != 0) {
			printf("Not a response (hash %p)\n", (void *) hash);
		}
		return;
	}
	
	hash = hash & ~IPC_CALLID_ANSWERED;
	key[0] = hash;
	
	item = hash_table_find(&pending_calls, key);
	if (item == NULL)
		return; /* No matching question found */
	
	/*
	 * Response matched to question.
	 */
	
	pcall = hash_table_get_instance(item, pending_call_t, link);
	hash_table_remove(&pending_calls, key, 1);
	
	parse_answer(hash, pcall, call);
	free(pcall);
}

void ipcp_call_sync(int phone, ipc_call_t *call, ipc_call_t *answer)
{
	ipcp_call_out(phone, call, IPCP_CALLID_SYNC);
	ipcp_call_in(answer, IPCP_CALLID_SYNC);
}

void ipcp_hangup(int phone, int rc)
{
	if ((display_mask & DM_SYSTEM) != 0) {
		printf("Hang phone %d up -> %d\n", phone, rc);
		ipcp_connection_clear(phone);
	}
}

/** @}
 */
