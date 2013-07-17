/*
 * Copyright (c) 2011 Jiri Michalec
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

/** @addtogroup libc
 * @{
 */
/** @file
 */

#include <ipc/dev_iface.h>
#include <assert.h>
#include <device/pci.h>
#include <errno.h>
#include <async.h>
#include <ipc/services.h>

int pci_config_space_read_8(async_sess_t *sess, uint32_t address, uint8_t *val)
{
	sysarg_t res = 0;
	
	async_exch_t *exch = async_exchange_begin(sess);
	int rc = async_req_2_1(exch, DEV_IFACE_ID(PCI_DEV_IFACE),
	    IPC_M_CONFIG_SPACE_READ_8, address, &res);
	async_exchange_end(exch);
	
	*val = (uint8_t) res;
	return rc;
}

int pci_config_space_read_16(async_sess_t *sess, uint32_t address,
    uint16_t *val)
{
	sysarg_t res = 0;
	
	async_exch_t *exch = async_exchange_begin(sess);
	int rc = async_req_2_1(exch, DEV_IFACE_ID(PCI_DEV_IFACE),
	    IPC_M_CONFIG_SPACE_READ_16, address, &res);
	async_exchange_end(exch);
	
	*val = (uint16_t) res;
	return rc;
}

int pci_config_space_read_32(async_sess_t *sess, uint32_t address,
    uint32_t *val)
{
	sysarg_t res = 0;
	
	async_exch_t *exch = async_exchange_begin(sess);
	int rc = async_req_2_1(exch, DEV_IFACE_ID(PCI_DEV_IFACE),
	    IPC_M_CONFIG_SPACE_READ_32, address, &res);
	async_exchange_end(exch);
	
	*val = (uint32_t) res;
	return rc;
}

int pci_config_space_write_8(async_sess_t *sess, uint32_t address, uint8_t val)
{
	async_exch_t *exch = async_exchange_begin(sess);
	int rc = async_req_3_0(exch, DEV_IFACE_ID(PCI_DEV_IFACE),
	    IPC_M_CONFIG_SPACE_WRITE_8, address, val);
	async_exchange_end(exch);
	
	return rc;
}

int pci_config_space_write_16(async_sess_t *sess, uint32_t address,
    uint16_t val)
{
	async_exch_t *exch = async_exchange_begin(sess);
	int rc = async_req_3_0(exch, DEV_IFACE_ID(PCI_DEV_IFACE),
	    IPC_M_CONFIG_SPACE_WRITE_16, address, val);
	async_exchange_end(exch);
	
	return rc;
}

int pci_config_space_write_32(async_sess_t *sess, uint32_t address,
    uint32_t val)
{
	async_exch_t *exch = async_exchange_begin(sess);
	int rc = async_req_3_0(exch, DEV_IFACE_ID(PCI_DEV_IFACE),
	    IPC_M_CONFIG_SPACE_WRITE_32, address, val);
	async_exchange_end(exch);
	
	return rc;
}

/** @}
 */
