/*
  * Copyright (c) 2012 Petr Jerman
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

/** @addtogroup libdrv
 * @{
 */
/** @file
 */

#include <async.h>
#include <errno.h>
#include <stdio.h>
#include "ahci_iface.h"
#include "ddf/driver.h"

typedef enum {
	IPC_M_AHCI_GET_SATA_DEVICE_NAME,
	IPC_M_AHCI_GET_NUM_BLOCKS,
	IPC_M_AHCI_GET_BLOCK_SIZE,
	IPC_M_AHCI_READ_BLOCKS,
	IPC_M_AHCI_WRITE_BLOCKS
} ahci_iface_funcs_t;

#define LO(ptr) \
	((uint32_t) (((uint64_t) ((uintptr_t) (ptr))) & 0xffffffff))

#define HI(ptr) \
	((uint32_t) (((uint64_t) ((uintptr_t) (ptr))) >> 32))

static void remote_ahci_get_sata_device_name(ddf_fun_t *, void *, ipc_callid_t,
    ipc_call_t *);
static void remote_ahci_get_num_blocks(ddf_fun_t *, void *, ipc_callid_t,
    ipc_call_t *);
static void remote_ahci_get_block_size(ddf_fun_t *, void *, ipc_callid_t,
    ipc_call_t *);
static void remote_ahci_read_blocks(ddf_fun_t *, void *, ipc_callid_t,
    ipc_call_t *);
static void remote_ahci_write_blocks(ddf_fun_t *, void *, ipc_callid_t,
    ipc_call_t *);

/** Remote AHCI interface operations. */
static remote_iface_func_ptr_t remote_ahci_iface_ops [] = {
	[IPC_M_AHCI_GET_SATA_DEVICE_NAME] = remote_ahci_get_sata_device_name,
	[IPC_M_AHCI_GET_NUM_BLOCKS] = remote_ahci_get_num_blocks,
	[IPC_M_AHCI_GET_BLOCK_SIZE] = remote_ahci_get_block_size,
	[IPC_M_AHCI_READ_BLOCKS] = remote_ahci_read_blocks,
	[IPC_M_AHCI_WRITE_BLOCKS] = remote_ahci_write_blocks
};

/** Remote AHCI interface structure.
 */
remote_iface_t remote_ahci_iface = {
	.method_count = sizeof(remote_ahci_iface_ops) /
	    sizeof(remote_ahci_iface_ops[0]),
	.methods = remote_ahci_iface_ops
};

void remote_ahci_get_sata_device_name(ddf_fun_t *fun, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	const ahci_iface_t *ahci_iface = (ahci_iface_t *) iface;
	
	if (ahci_iface->get_sata_device_name == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}
	
	const size_t sata_dev_name_length =
	    (size_t) DEV_IPC_GET_ARG1(*call);
	
	char* sata_dev_name = malloc(sata_dev_name_length);
	
	const int ret = ahci_iface->get_sata_device_name(fun,
	    sata_dev_name_length, sata_dev_name);
	
	size_t real_size;
	ipc_callid_t cid;
	if ((async_data_read_receive(&cid, &real_size)) &&
	    (real_size == sata_dev_name_length))
		async_data_read_finalize(cid, sata_dev_name, sata_dev_name_length);
	
	async_answer_0(callid, ret);
}

static void remote_ahci_get_num_blocks(ddf_fun_t *fun, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	const ahci_iface_t *ahci_iface = (ahci_iface_t *) iface;
	
	if (ahci_iface->get_num_blocks == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}
	
	uint64_t blocks;
	const int ret = ahci_iface->get_num_blocks(fun, &blocks);
	
	if (ret != EOK)
		async_answer_0(callid, ret);
	else
		async_answer_2(callid, EOK, HI(blocks), LO(blocks));
}

static void remote_ahci_get_block_size(ddf_fun_t *fun, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	const ahci_iface_t *ahci_iface = (ahci_iface_t *) iface;
	
	if (ahci_iface->get_block_size == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}
	
	size_t blocks;
	const int ret = ahci_iface->get_block_size(fun, &blocks);
	
	if (ret != EOK)
		async_answer_0(callid, ret);
	else
		async_answer_1(callid, EOK, blocks);
}

void remote_ahci_read_blocks(ddf_fun_t *fun, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	const ahci_iface_t *ahci_iface = (ahci_iface_t *) iface;
	
	if (ahci_iface->read_blocks == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}
	
	size_t maxblock_size;
	unsigned int flags;
	
	ipc_callid_t cid;
	async_share_out_receive(&cid, &maxblock_size, &flags);
	
	void *buf;
	async_share_out_finalize(cid, &buf);
	
	const uint64_t blocknum =
	    (((uint64_t) (DEV_IPC_GET_ARG1(*call))) << 32) |
	    (((uint64_t) (DEV_IPC_GET_ARG2(*call))) & 0xffffffff);
	const size_t cnt = (size_t) DEV_IPC_GET_ARG3(*call);
	
	const int ret = ahci_iface->read_blocks(fun, blocknum, cnt, buf);
	
	async_answer_0(callid, ret);
}

void remote_ahci_write_blocks(ddf_fun_t *fun, void *iface, ipc_callid_t callid,
    ipc_call_t *call)
{
	const ahci_iface_t *ahci_iface = (ahci_iface_t *) iface;
	
	if (ahci_iface->read_blocks == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}
	
	size_t maxblock_size;
	unsigned int flags;
	
	ipc_callid_t cid;
	async_share_out_receive(&cid, &maxblock_size, &flags);
	
	void *buf;
	async_share_out_finalize(cid, &buf);
	
	const uint64_t blocknum =
	    (((uint64_t)(DEV_IPC_GET_ARG1(*call))) << 32) |
	    (((uint64_t)(DEV_IPC_GET_ARG2(*call))) & 0xffffffff);
	const size_t cnt = (size_t) DEV_IPC_GET_ARG3(*call);
	
	const int ret = ahci_iface->write_blocks(fun, blocknum, cnt, buf);
	
	async_answer_0(callid, ret);
}

/**
 * @}
 */
