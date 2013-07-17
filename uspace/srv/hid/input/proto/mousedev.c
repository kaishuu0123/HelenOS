/*
 * Copyright (c) 2011 Martin Decky
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

/** @addtogroup mouse_proto
 * @ingroup input
 * @{
 */
/**
 * @file
 * @brief Mouse device connector controller driver.
 */

#include <stdio.h>
#include <fcntl.h>
#include <vfs/vfs_sess.h>
#include <malloc.h>
#include <async.h>
#include <errno.h>
#include <ipc/mouseev.h>
#include <input.h>
#include <loc.h>
#include <mouse.h>
#include <mouse_port.h>
#include <mouse_proto.h>
#include <sys/typefmt.h>

/** Mousedev softstate */
typedef struct {
	/** Link to generic mouse device */
	mouse_dev_t *mouse_dev;
} mousedev_t;

static mousedev_t *mousedev_new(mouse_dev_t *mdev)
{
	mousedev_t *mousedev = calloc(1, sizeof(mousedev_t));
	if (mousedev == NULL)
		return NULL;
	
	mousedev->mouse_dev = mdev;
	
	return mousedev;
}

static void mousedev_destroy(mousedev_t *mousedev)
{
	free(mousedev);
}

static void mousedev_callback_conn(ipc_callid_t iid, ipc_call_t *icall,
    void *arg)
{
	/* Mousedev device structure */
	mousedev_t *mousedev = (mousedev_t *) arg;
	
	while (true) {
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);
		
		if (!IPC_GET_IMETHOD(call)) {
			mousedev_destroy(mousedev);
			return;
		}
		
		int retval;
		
		switch (IPC_GET_IMETHOD(call)) {
		case MOUSEEV_MOVE_EVENT:
			mouse_push_event_move(mousedev->mouse_dev,
			    IPC_GET_ARG1(call), IPC_GET_ARG2(call),
			    IPC_GET_ARG3(call));
			retval = EOK;
			break;
		case MOUSEEV_BUTTON_EVENT:
			mouse_push_event_button(mousedev->mouse_dev,
			    IPC_GET_ARG1(call), IPC_GET_ARG2(call));
			retval = EOK;
			break;
		default:
			retval = ENOTSUP;
			break;
		}
		
		async_answer_0(callid, retval);
	}
}

static int mousedev_proto_init(mouse_dev_t *mdev)
{
	async_sess_t *sess = loc_service_connect(EXCHANGE_SERIALIZE,
	    mdev->svc_id, 0);
	if (sess == NULL) {
		printf("%s: Failed starting session with '%s'\n", NAME,
		    mdev->svc_name);
		return ENOENT;
	}
	
	mousedev_t *mousedev = mousedev_new(mdev);
	if (mousedev == NULL) {
		printf("%s: Failed allocating device structure for '%s'.\n",
		    NAME, mdev->svc_name);
		async_hangup(sess);
		return ENOMEM;
	}
	
	async_exch_t *exch = async_exchange_begin(sess);
	if (exch == NULL) {
		printf("%s: Failed starting exchange with '%s'.\n", NAME,
		    mdev->svc_name);
		mousedev_destroy(mousedev);
		async_hangup(sess);
		return ENOENT;
	}
	
	int rc = async_connect_to_me(exch, 0, 0, 0, mousedev_callback_conn, mousedev);
	async_exchange_end(exch);
	async_hangup(sess);
	
	if (rc != EOK) {
		printf("%s: Failed creating callback connection from '%s'.\n",
		    NAME, mdev->svc_name);
		mousedev_destroy(mousedev);
		return rc;
	}
	
	return EOK;
}

mouse_proto_ops_t mousedev_proto = {
	.init = mousedev_proto_init
};

/**
 * @}
 */
