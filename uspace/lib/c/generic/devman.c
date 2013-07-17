/*
 * Copyright (c) 2007 Josef Cejka
 * Copyright (c) 2011 Jiri Svoboda
 * Copyright (c) 2010 Lenka Trochtova
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

#include <adt/list.h>
#include <str.h>
#include <ipc/services.h>
#include <ns.h>
#include <ipc/devman.h>
#include <devman.h>
#include <fibril_synch.h>
#include <async.h>
#include <errno.h>
#include <malloc.h>
#include <bool.h>

static FIBRIL_MUTEX_INITIALIZE(devman_driver_block_mutex);
static FIBRIL_MUTEX_INITIALIZE(devman_client_block_mutex);

static FIBRIL_MUTEX_INITIALIZE(devman_driver_mutex);
static FIBRIL_MUTEX_INITIALIZE(devman_client_mutex);

static async_sess_t *devman_driver_block_sess = NULL;
static async_sess_t *devman_client_block_sess = NULL;

static async_sess_t *devman_driver_sess = NULL;
static async_sess_t *devman_client_sess = NULL;

static void clone_session(fibril_mutex_t *mtx, async_sess_t *src,
    async_sess_t **dst)
{
	fibril_mutex_lock(mtx);
	
	if ((*dst == NULL) && (src != NULL))
		*dst = src;
	
	fibril_mutex_unlock(mtx);
}

/** Start an async exchange on the devman session (blocking).
 *
 * @param iface Device manager interface to choose
 *
 * @return New exchange.
 *
 */
async_exch_t *devman_exchange_begin_blocking(devman_interface_t iface)
{
	switch (iface) {
	case DEVMAN_DRIVER:
		fibril_mutex_lock(&devman_driver_block_mutex);
		
		while (devman_driver_block_sess == NULL) {
			clone_session(&devman_driver_mutex, devman_driver_sess,
			    &devman_driver_block_sess);
			
			if (devman_driver_block_sess == NULL)
				devman_driver_block_sess =
				    service_connect_blocking(EXCHANGE_PARALLEL,
				    SERVICE_DEVMAN, DEVMAN_DRIVER, 0);
		}
		
		fibril_mutex_unlock(&devman_driver_block_mutex);
		
		clone_session(&devman_driver_mutex, devman_driver_block_sess,
		    &devman_driver_sess);
		
		return async_exchange_begin(devman_driver_block_sess);
	case DEVMAN_CLIENT:
		fibril_mutex_lock(&devman_client_block_mutex);
		
		while (devman_client_block_sess == NULL) {
			clone_session(&devman_client_mutex, devman_client_sess,
			    &devman_client_block_sess);
			
			if (devman_client_block_sess == NULL)
				devman_client_block_sess =
				    service_connect_blocking(EXCHANGE_SERIALIZE,
				    SERVICE_DEVMAN, DEVMAN_CLIENT, 0);
		}
		
		fibril_mutex_unlock(&devman_client_block_mutex);
		
		clone_session(&devman_client_mutex, devman_client_block_sess,
		    &devman_client_sess);
		
		return async_exchange_begin(devman_client_block_sess);
	default:
		return NULL;
	}
}

/** Start an async exchange on the devman session.
 *
 * @param iface Device manager interface to choose
 *
 * @return New exchange.
 *
 */
async_exch_t *devman_exchange_begin(devman_interface_t iface)
{
	switch (iface) {
	case DEVMAN_DRIVER:
		fibril_mutex_lock(&devman_driver_mutex);
		
		if (devman_driver_sess == NULL)
			devman_driver_sess =
			    service_connect(EXCHANGE_PARALLEL, SERVICE_DEVMAN,
			    DEVMAN_DRIVER, 0);
		
		fibril_mutex_unlock(&devman_driver_mutex);
		
		if (devman_driver_sess == NULL)
			return NULL;
		
		return async_exchange_begin(devman_driver_sess);
	case DEVMAN_CLIENT:
		fibril_mutex_lock(&devman_client_mutex);
		
		if (devman_client_sess == NULL)
			devman_client_sess =
			    service_connect(EXCHANGE_SERIALIZE, SERVICE_DEVMAN,
			    DEVMAN_CLIENT, 0);
		
		fibril_mutex_unlock(&devman_client_mutex);
		
		if (devman_client_sess == NULL)
			return NULL;
		
		return async_exchange_begin(devman_client_sess);
	default:
		return NULL;
	}
}

/** Finish an async exchange on the devman session.
 *
 * @param exch Exchange to be finished.
 *
 */
void devman_exchange_end(async_exch_t *exch)
{
	async_exchange_end(exch);
}

/** Register running driver with device manager. */
int devman_driver_register(const char *name)
{
	async_exch_t *exch = devman_exchange_begin_blocking(DEVMAN_DRIVER);
	
	ipc_call_t answer;
	aid_t req = async_send_2(exch, DEVMAN_DRIVER_REGISTER, 0, 0, &answer);
	sysarg_t retval = async_data_write_start(exch, name, str_size(name));
	
	devman_exchange_end(exch);
	
	if (retval != EOK) {
		async_forget(req);
		return retval;
	}
	
	exch = devman_exchange_begin(DEVMAN_DRIVER);
	async_connect_to_me(exch, 0, 0, 0, NULL, NULL);
	devman_exchange_end(exch);
	
	async_wait_for(req, &retval);
	return retval;
}

/** Add function to a device.
 *
 * Request devman to add a new function to the specified device owned by
 * this driver task.
 *
 * @param name      Name of the new function
 * @param ftype     Function type, fun_inner or fun_exposed
 * @param match_ids Match IDs (should be empty for fun_exposed)
 * @param devh      Devman handle of the device
 * @param funh      Place to store handle of the new function
 *
 * @return EOK on success or negative error code.
 *
 */
int devman_add_function(const char *name, fun_type_t ftype,
    match_id_list_t *match_ids, devman_handle_t devh, devman_handle_t *funh)
{
	int match_count = list_count(&match_ids->ids);
	async_exch_t *exch = devman_exchange_begin_blocking(DEVMAN_DRIVER);
	
	ipc_call_t answer;
	aid_t req = async_send_3(exch, DEVMAN_ADD_FUNCTION, (sysarg_t) ftype,
	    devh, match_count, &answer);
	sysarg_t retval = async_data_write_start(exch, name, str_size(name));
	if (retval != EOK) {
		devman_exchange_end(exch);
		async_forget(req);
		return retval;
	}
	
	match_id_t *match_id = NULL;
	
	list_foreach(match_ids->ids, link) {
		match_id = list_get_instance(link, match_id_t, link);
		
		ipc_call_t answer2;
		aid_t req2 = async_send_1(exch, DEVMAN_ADD_MATCH_ID,
		    match_id->score, &answer2);
		retval = async_data_write_start(exch, match_id->id,
		    str_size(match_id->id));
		if (retval != EOK) {
			devman_exchange_end(exch);
			async_forget(req2);
			async_forget(req);
			return retval;
		}
		
		async_wait_for(req2, &retval);
		if (retval != EOK) {
			devman_exchange_end(exch);
			async_forget(req);
			return retval;
		}
	}
	
	devman_exchange_end(exch);
	
	async_wait_for(req, &retval);
	if (retval == EOK) {
		if (funh != NULL)
			*funh = (int) IPC_GET_ARG1(answer);
	} else {
		if (funh != NULL)
			*funh = -1;
	}
	
	return retval;
}

int devman_add_device_to_category(devman_handle_t devman_handle,
    const char *cat_name)
{
	async_exch_t *exch = devman_exchange_begin_blocking(DEVMAN_DRIVER);
	
	ipc_call_t answer;
	aid_t req = async_send_1(exch, DEVMAN_ADD_DEVICE_TO_CATEGORY,
	    devman_handle, &answer);
	sysarg_t retval = async_data_write_start(exch, cat_name,
	    str_size(cat_name));
	
	devman_exchange_end(exch);
	
	if (retval != EOK) {
		async_forget(req);
		return retval;
	}
	
	async_wait_for(req, &retval);
	return retval;
}

async_sess_t *devman_device_connect(exch_mgmt_t mgmt, devman_handle_t handle,
    unsigned int flags)
{
	async_sess_t *sess;
	
	if (flags & IPC_FLAG_BLOCKING)
		sess = service_connect_blocking(mgmt, SERVICE_DEVMAN,
			    DEVMAN_CONNECT_TO_DEVICE, handle);
	else
		sess = service_connect(mgmt, SERVICE_DEVMAN,
			    DEVMAN_CONNECT_TO_DEVICE, handle);
	
	return sess;
}

/** Remove function from device.
 *
 * Request devman to remove function owned by this driver task.
 * @param funh      Devman handle of the function
 *
 * @return EOK on success or negative error code.
 */
int devman_remove_function(devman_handle_t funh)
{
	async_exch_t *exch;
	sysarg_t retval;
	
	exch = devman_exchange_begin_blocking(DEVMAN_DRIVER);
	retval = async_req_1_0(exch, DEVMAN_REMOVE_FUNCTION, (sysarg_t) funh);
	devman_exchange_end(exch);
	
	return (int) retval;
}

int devman_drv_fun_online(devman_handle_t funh)
{
	async_exch_t *exch = devman_exchange_begin(DEVMAN_DRIVER);
	if (exch == NULL)
		return ENOMEM;
	
	sysarg_t retval = async_req_1_0(exch, DEVMAN_DRV_FUN_ONLINE, funh);
	
	devman_exchange_end(exch);
	return (int) retval;
}

int devman_drv_fun_offline(devman_handle_t funh)
{
	async_exch_t *exch = devman_exchange_begin(DEVMAN_DRIVER);
	if (exch == NULL)
		return ENOMEM;
	
	sysarg_t retval = async_req_1_0(exch, DEVMAN_DRV_FUN_OFFLINE, funh);
	
	devman_exchange_end(exch);
	return (int) retval;
}

async_sess_t *devman_parent_device_connect(exch_mgmt_t mgmt,
    devman_handle_t handle, unsigned int flags)
{
	async_sess_t *sess;
	
	if (flags & IPC_FLAG_BLOCKING)
		sess = service_connect_blocking(mgmt, SERVICE_DEVMAN,
			    DEVMAN_CONNECT_TO_PARENTS_DEVICE, handle);
	else
		sess = service_connect(mgmt, SERVICE_DEVMAN,
			    DEVMAN_CONNECT_TO_PARENTS_DEVICE, handle);
	
	return sess;
}

int devman_fun_get_handle(const char *pathname, devman_handle_t *handle,
    unsigned int flags)
{
	async_exch_t *exch;
	
	if (flags & IPC_FLAG_BLOCKING)
		exch = devman_exchange_begin_blocking(DEVMAN_CLIENT);
	else {
		exch = devman_exchange_begin(DEVMAN_CLIENT);
		if (exch == NULL)
			return ENOMEM;
	}
	
	ipc_call_t answer;
	aid_t req = async_send_2(exch, DEVMAN_DEVICE_GET_HANDLE, flags, 0,
	    &answer);
	sysarg_t retval = async_data_write_start(exch, pathname,
	    str_size(pathname));
	
	devman_exchange_end(exch);
	
	if (retval != EOK) {
		async_forget(req);
		return retval;
	}
	
	async_wait_for(req, &retval);
	
	if (retval != EOK) {
		if (handle != NULL)
			*handle = (devman_handle_t) -1;
		
		return retval;
	}
	
	if (handle != NULL)
		*handle = (devman_handle_t) IPC_GET_ARG1(answer);
	
	return retval;
}

static int devman_get_str_internal(sysarg_t method, sysarg_t arg1, char *buf,
    size_t buf_size)
{
	async_exch_t *exch;
	ipc_call_t dreply;
	size_t act_size;
	sysarg_t dretval;
	
	exch = devman_exchange_begin_blocking(LOC_PORT_CONSUMER);
	
	ipc_call_t answer;
	aid_t req = async_send_1(exch, method, arg1, &answer);
	aid_t dreq = async_data_read(exch, buf, buf_size - 1, &dreply);
	async_wait_for(dreq, &dretval);
	
	devman_exchange_end(exch);
	
	if (dretval != EOK) {
		async_forget(req);
		return dretval;
	}
	
	sysarg_t retval;
	async_wait_for(req, &retval);
	
	if (retval != EOK) {
		return retval;
	}
	
	act_size = IPC_GET_ARG2(dreply);
	assert(act_size <= buf_size - 1);
	buf[act_size] = '\0';
	
	return EOK;
}

int devman_fun_get_path(devman_handle_t handle, char *buf, size_t buf_size)
{
	return devman_get_str_internal(DEVMAN_FUN_GET_PATH, handle, buf,
	    buf_size);
}

int devman_fun_get_name(devman_handle_t handle, char *buf, size_t buf_size)
{
	return devman_get_str_internal(DEVMAN_FUN_GET_NAME, handle, buf,
	    buf_size);
}

int devman_fun_get_driver_name(devman_handle_t handle, char *buf, size_t buf_size)
{
	return devman_get_str_internal(DEVMAN_FUN_GET_DRIVER_NAME, handle, buf,
	    buf_size);
}

int devman_fun_online(devman_handle_t funh)
{
	async_exch_t *exch = devman_exchange_begin(DEVMAN_CLIENT);
	if (exch == NULL)
		return ENOMEM;
	
	sysarg_t retval = async_req_1_0(exch, DEVMAN_FUN_ONLINE, funh);
	
	devman_exchange_end(exch);
	return (int) retval;
}

int devman_fun_offline(devman_handle_t funh)
{
	async_exch_t *exch = devman_exchange_begin(DEVMAN_CLIENT);
	if (exch == NULL)
		return ENOMEM;
	
	sysarg_t retval = async_req_1_0(exch, DEVMAN_FUN_OFFLINE, funh);
	
	devman_exchange_end(exch);
	return (int) retval;
}

static int devman_get_handles_once(sysarg_t method, sysarg_t arg1,
    devman_handle_t *handle_buf, size_t buf_size, size_t *act_size)
{
	async_exch_t *exch = devman_exchange_begin_blocking(DEVMAN_CLIENT);

	ipc_call_t answer;
	aid_t req = async_send_1(exch, method, arg1, &answer);
	int rc = async_data_read_start(exch, handle_buf, buf_size);
	
	devman_exchange_end(exch);
	
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

/** Get list of handles.
 *
 * Returns an allocated array of handles.
 *
 * @param method	IPC method
 * @param arg1		IPC argument 1
 * @param data		Place to store pointer to array of handles
 * @param count		Place to store number of handles
 * @return 		EOK on success or negative error code
 */
static int devman_get_handles_internal(sysarg_t method, sysarg_t arg1,
    devman_handle_t **data, size_t *count)
{
	devman_handle_t *handles;
	size_t act_size;
	size_t alloc_size;
	int rc;

	*data = NULL;
	act_size = 0;	/* silence warning */

	rc = devman_get_handles_once(method, arg1, NULL, 0,
	    &act_size);
	if (rc != EOK)
		return rc;

	alloc_size = act_size;
	handles = malloc(alloc_size);
	if (handles == NULL)
		return ENOMEM;

	while (true) {
		rc = devman_get_handles_once(method, arg1, handles, alloc_size,
		    &act_size);
		if (rc != EOK)
			return rc;

		if (act_size <= alloc_size)
			break;

		alloc_size *= 2;
		free(handles);

		handles = malloc(alloc_size);
		if (handles == NULL)
			return ENOMEM;
	}

	*count = act_size / sizeof(devman_handle_t);
	*data = handles;
	return EOK;
}

int devman_fun_get_child(devman_handle_t funh, devman_handle_t *devh)
{
	async_exch_t *exch = devman_exchange_begin(DEVMAN_CLIENT);
	if (exch == NULL)
		return ENOMEM;
	
	sysarg_t retval = async_req_1_1(exch, DEVMAN_FUN_GET_CHILD,
	    funh, devh);
	
	devman_exchange_end(exch);
	return (int) retval;
}

int devman_dev_get_functions(devman_handle_t devh, devman_handle_t **funcs,
    size_t *count)
{
	return devman_get_handles_internal(DEVMAN_DEV_GET_FUNCTIONS,
	    devh, funcs, count);
}

int devman_fun_sid_to_handle(service_id_t sid, devman_handle_t *handle)
{
	async_exch_t *exch = devman_exchange_begin(DEVMAN_CLIENT);
	if (exch == NULL)
		return ENOMEM;
	
	sysarg_t retval = async_req_1_1(exch, DEVMAN_FUN_SID_TO_HANDLE,
	    sid, handle);
	
	devman_exchange_end(exch);
	return (int) retval;
}

/** @}
 */
