/*
 * Copyright (c) 2008 Jakub Jermar
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

/** @addtogroup fs
 * @{
 */

/**
 * @file vfs.c
 * @brief VFS service for HelenOS.
 */

#include <vfs/vfs.h>
#include <ipc/services.h>
#include <abi/ipc/event.h>
#include <event.h>
#include <ns.h>
#include <async.h>
#include <errno.h>
#include <stdio.h>
#include <bool.h>
#include <str.h>
#include <as.h>
#include <atomic.h>
#include <macros.h>
#include "vfs.h"

#define NAME  "vfs"

enum {
	VFS_TASK_STATE_CHANGE
};

static void vfs_connection(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	bool cont = true;
	
	/*
	 * The connection was opened via the IPC_CONNECT_ME_TO call.
	 * This call needs to be answered.
	 */
	async_answer_0(iid, EOK);
	
	while (cont) {
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);
		
		if (!IPC_GET_IMETHOD(call))
			break;
		
		switch (IPC_GET_IMETHOD(call)) {
		case VFS_IN_REGISTER:
			vfs_register(callid, &call);
			cont = false;
			break;
		case VFS_IN_MOUNT:
			vfs_mount(callid, &call);
			break;
		case VFS_IN_UNMOUNT:
			vfs_unmount(callid, &call);
			break;
		case VFS_IN_OPEN:
			vfs_open(callid, &call);
			break;
		case VFS_IN_CLOSE:
			vfs_close(callid, &call);
			break;
		case VFS_IN_READ:
			vfs_read(callid, &call);
			break;
		case VFS_IN_WRITE:
			vfs_write(callid, &call);
			break;
		case VFS_IN_SEEK:
			vfs_seek(callid, &call);
			break;
		case VFS_IN_TRUNCATE:
			vfs_truncate(callid, &call);
			break;
		case VFS_IN_FSTAT:
			vfs_fstat(callid, &call);
			break;
		case VFS_IN_STAT:
			vfs_stat(callid, &call);
			break;
		case VFS_IN_MKDIR:
			vfs_mkdir(callid, &call);
			break;
		case VFS_IN_UNLINK:
			vfs_unlink(callid, &call);
			break;
		case VFS_IN_RENAME:
			vfs_rename(callid, &call);
			break;
		case VFS_IN_SYNC:
			vfs_sync(callid, &call);
			break;
		case VFS_IN_DUP:
			vfs_dup(callid, &call);
			break;
		case VFS_IN_WAIT_HANDLE:
			vfs_wait_handle(callid, &call);
			break;
		case VFS_IN_MTAB_GET:
			vfs_get_mtab(callid, &call);
			break;
		default:
			async_answer_0(callid, ENOTSUP);
			break;
		}
	}
	
	/*
	 * Open files for this client will be cleaned up when its last
	 * connection fibril terminates.
	 */
}

static void notification_received(ipc_callid_t callid, ipc_call_t *call)
{
	switch (IPC_GET_IMETHOD(*call)) {
	case VFS_TASK_STATE_CHANGE:
		if (IPC_GET_ARG1(*call) == VFS_PASS_HANDLE)
			vfs_pass_handle(
			    (task_id_t) MERGE_LOUP32(IPC_GET_ARG4(*call),
			    IPC_GET_ARG5(*call)), call->in_task_id,
			    (int) IPC_GET_ARG2(*call));
		break;
	default:
		break;
	}
}

int main(int argc, char **argv)
{
	printf("%s: HelenOS VFS server\n", NAME);
	
	/*
	 * Initialize VFS node hash table.
	 */
	if (!vfs_nodes_init()) {
		printf("%s: Failed to initialize VFS node hash table\n",
		    NAME);
		return ENOMEM;
	}
	
	/*
	 * Allocate and initialize the Path Lookup Buffer.
	 */
	plb = as_area_create(AS_AREA_ANY, PLB_SIZE,
	    AS_AREA_READ | AS_AREA_WRITE | AS_AREA_CACHEABLE);
	if (plb == AS_MAP_FAILED) {
		printf("%s: Cannot create address space area\n", NAME);
		return ENOMEM;
	}
	memset(plb, 0, PLB_SIZE);
	
	/*
	 * Set client data constructor and destructor.
	 */
	async_set_client_data_constructor(vfs_client_data_create);
	async_set_client_data_destructor(vfs_client_data_destroy);

	/*
	 * Set a connection handling function/fibril.
	 */
	async_set_client_connection(vfs_connection);

	/*
	 * Set notification handler and subscribe to notifications.
	 */
	async_set_interrupt_received(notification_received);
	event_task_subscribe(EVENT_TASK_STATE_CHANGE, VFS_TASK_STATE_CHANGE);
	
	/*
	 * Register at the naming service.
	 */
	int rc = service_register(SERVICE_VFS);
	if (rc != EOK) {
		printf("%s: Cannot register VFS service\n", NAME);
		return rc;
	}
	
	/*
	 * Start accepting connections.
	 */
	printf("%s: Accepting connections\n", NAME);
	async_manager();
	return 0;
}

/**
 * @}
 */
