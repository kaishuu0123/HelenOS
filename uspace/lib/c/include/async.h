/*
 * Copyright (c) 2006 Ondrej Palkovsky
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

#if ((defined(LIBC_IPC_H_)) && (!defined(LIBC_ASYNC_C_)))
	#error Do not intermix low-level IPC interface and async framework
#endif

#ifndef LIBC_ASYNC_H_
#define LIBC_ASYNC_H_

#include <ipc/common.h>
#include <fibril.h>
#include <sys/time.h>
#include <atomic.h>
#include <bool.h>
#include <task.h>

typedef ipc_callid_t aid_t;

typedef void *(*async_client_data_ctor_t)(void);
typedef void (*async_client_data_dtor_t)(void *);

/** Client connection handler
 *
 * @param callid ID of incoming call or 0 if connection initiated from
 *               inside using async_connect_to_me()
 * @param call   Incoming call or 0 if connection initiated from inside
 * @param arg    Local argument passed from async_new_connection() or
 *               async_connect_to_me()
 */
typedef void (*async_client_conn_t)(ipc_callid_t, ipc_call_t *, void *);

/** Interrupt handler */
typedef void (*async_interrupt_handler_t)(ipc_callid_t, ipc_call_t *);

/** Exchange management style
 *
 */
typedef enum {
	/** No explicit exchange management
	 *
	 * Suitable for protocols which use a single
	 * IPC message per exchange only.
	 *
	 */
	EXCHANGE_ATOMIC = 0,
	
	/** Exchange management via phone cloning
	 *
	 * Suitable for servers which support client
	 * data tracking by task hashes and do not
	 * mind cloned phones.
	 *
	 */
	EXCHANGE_PARALLEL,
	
	/** Exchange management via mutual exclusion
	 *
	 * Suitable for any kind of client/server communication,
	 * but can limit parallelism.
	 *
	 */
	EXCHANGE_SERIALIZE
} exch_mgmt_t;

/** Forward declarations */
struct async_exch;
struct async_sess;

typedef struct async_sess async_sess_t;
typedef struct async_exch async_exch_t;

extern atomic_t threads_in_ipc_wait;

#define async_manager() \
	fibril_switch(FIBRIL_TO_MANAGER)

#define async_get_call(data) \
	async_get_call_timeout(data, 0)

extern ipc_callid_t async_get_call_timeout(ipc_call_t *, suseconds_t);

/*
 * User-friendly wrappers for async_send_fast() and async_send_slow(). The
 * macros are in the form async_send_m(), where m denotes the number of payload
 * arguments. Each macros chooses between the fast and the slow version based
 * on m.
 */

#define async_send_0(exch, method, dataptr) \
	async_send_fast(exch, method, 0, 0, 0, 0, dataptr)
#define async_send_1(exch, method, arg1, dataptr) \
	async_send_fast(exch, method, arg1, 0, 0, 0, dataptr)
#define async_send_2(exch, method, arg1, arg2, dataptr) \
	async_send_fast(exch, method, arg1, arg2, 0, 0, dataptr)
#define async_send_3(exch, method, arg1, arg2, arg3, dataptr) \
	async_send_fast(exch, method, arg1, arg2, arg3, 0, dataptr)
#define async_send_4(exch, method, arg1, arg2, arg3, arg4, dataptr) \
	async_send_fast(exch, method, arg1, arg2, arg3, arg4, dataptr)
#define async_send_5(exch, method, arg1, arg2, arg3, arg4, arg5, dataptr) \
	async_send_slow(exch, method, arg1, arg2, arg3, arg4, arg5, dataptr)

extern aid_t async_send_fast(async_exch_t *, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t, ipc_call_t *);
extern aid_t async_send_slow(async_exch_t *, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t, sysarg_t, ipc_call_t *);

extern void async_wait_for(aid_t, sysarg_t *);
extern int async_wait_timeout(aid_t, sysarg_t *, suseconds_t);
extern void async_forget(aid_t);

extern fid_t async_new_connection(task_id_t, sysarg_t, ipc_callid_t,
    ipc_call_t *, async_client_conn_t, void *);

extern void async_usleep(suseconds_t);
extern void async_create_manager(void);
extern void async_destroy_manager(void);

extern void async_set_client_data_constructor(async_client_data_ctor_t);
extern void async_set_client_data_destructor(async_client_data_dtor_t);
extern void *async_get_client_data(void);
extern void *async_get_client_data_by_id(task_id_t);
extern void async_put_client_data_by_id(task_id_t);

extern void async_set_client_connection(async_client_conn_t);
extern void async_set_interrupt_received(async_interrupt_handler_t);

/*
 * Wrappers for simple communication.
 */

extern void async_msg_0(async_exch_t *, sysarg_t);
extern void async_msg_1(async_exch_t *, sysarg_t, sysarg_t);
extern void async_msg_2(async_exch_t *, sysarg_t, sysarg_t, sysarg_t);
extern void async_msg_3(async_exch_t *, sysarg_t, sysarg_t, sysarg_t, sysarg_t);
extern void async_msg_4(async_exch_t *, sysarg_t, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t);
extern void async_msg_5(async_exch_t *, sysarg_t, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t);

/*
 * Wrappers for answer routines.
 */

extern sysarg_t async_answer_0(ipc_callid_t, sysarg_t);
extern sysarg_t async_answer_1(ipc_callid_t, sysarg_t, sysarg_t);
extern sysarg_t async_answer_2(ipc_callid_t, sysarg_t, sysarg_t, sysarg_t);
extern sysarg_t async_answer_3(ipc_callid_t, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t);
extern sysarg_t async_answer_4(ipc_callid_t, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t);
extern sysarg_t async_answer_5(ipc_callid_t, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t, sysarg_t);

/*
 * Wrappers for forwarding routines.
 */

extern int async_forward_fast(ipc_callid_t, async_exch_t *, sysarg_t, sysarg_t,
    sysarg_t, unsigned int);
extern int async_forward_slow(ipc_callid_t, async_exch_t *, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t, sysarg_t, sysarg_t, unsigned int);

/*
 * User-friendly wrappers for async_req_fast() and async_req_slow(). The macros
 * are in the form async_req_m_n(), where m is the number of payload arguments
 * and n is the number of return arguments. The macros decide between the fast
 * and slow verion based on m.
 */

#define async_req_0_0(exch, method) \
	async_req_fast(exch, method, 0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL)
#define async_req_0_1(exch, method, r1) \
	async_req_fast(exch, method, 0, 0, 0, 0, r1, NULL, NULL, NULL, NULL)
#define async_req_0_2(exch, method, r1, r2) \
	async_req_fast(exch, method, 0, 0, 0, 0, r1, r2, NULL, NULL, NULL)
#define async_req_0_3(exch, method, r1, r2, r3) \
	async_req_fast(exch, method, 0, 0, 0, 0, r1, r2, r3, NULL, NULL)
#define async_req_0_4(exch, method, r1, r2, r3, r4) \
	async_req_fast(exch, method, 0, 0, 0, 0, r1, r2, r3, r4, NULL)
#define async_req_0_5(exch, method, r1, r2, r3, r4, r5) \
	async_req_fast(exch, method, 0, 0, 0, 0, r1, r2, r3, r4, r5)

#define async_req_1_0(exch, method, arg1) \
	async_req_fast(exch, method, arg1, 0, 0, 0, NULL, NULL, NULL, NULL, \
	    NULL)
#define async_req_1_1(exch, method, arg1, rc1) \
	async_req_fast(exch, method, arg1, 0, 0, 0, rc1, NULL, NULL, NULL, \
	    NULL)
#define async_req_1_2(exch, method, arg1, rc1, rc2) \
	async_req_fast(exch, method, arg1, 0, 0, 0, rc1, rc2, NULL, NULL, \
	    NULL)
#define async_req_1_3(exch, method, arg1, rc1, rc2, rc3) \
	async_req_fast(exch, method, arg1, 0, 0, 0, rc1, rc2, rc3, NULL, \
	    NULL)
#define async_req_1_4(exch, method, arg1, rc1, rc2, rc3, rc4) \
	async_req_fast(exch, method, arg1, 0, 0, 0, rc1, rc2, rc3, rc4, \
	    NULL)
#define async_req_1_5(exch, method, arg1, rc1, rc2, rc3, rc4, rc5) \
	async_req_fast(exch, method, arg1, 0, 0, 0, rc1, rc2, rc3, rc4, \
	    rc5)

#define async_req_2_0(exch, method, arg1, arg2) \
	async_req_fast(exch, method, arg1, arg2, 0, 0, NULL, NULL, NULL, \
	    NULL, NULL)
#define async_req_2_1(exch, method, arg1, arg2, rc1) \
	async_req_fast(exch, method, arg1, arg2, 0, 0, rc1, NULL, NULL, \
	    NULL, NULL)
#define async_req_2_2(exch, method, arg1, arg2, rc1, rc2) \
	async_req_fast(exch, method, arg1, arg2, 0, 0, rc1, rc2, NULL, NULL, \
	    NULL)
#define async_req_2_3(exch, method, arg1, arg2, rc1, rc2, rc3) \
	async_req_fast(exch, method, arg1, arg2, 0, 0, rc1, rc2, rc3, NULL, \
	    NULL)
#define async_req_2_4(exch, method, arg1, arg2, rc1, rc2, rc3, rc4) \
	async_req_fast(exch, method, arg1, arg2, 0, 0, rc1, rc2, rc3, rc4, \
	    NULL)
#define async_req_2_5(exch, method, arg1, arg2, rc1, rc2, rc3, rc4, rc5) \
	async_req_fast(exch, method, arg1, arg2, 0, 0, rc1, rc2, rc3, rc4, \
	    rc5)

#define async_req_3_0(exch, method, arg1, arg2, arg3) \
	async_req_fast(exch, method, arg1, arg2, arg3, 0, NULL, NULL, NULL, \
	    NULL, NULL)
#define async_req_3_1(exch, method, arg1, arg2, arg3, rc1) \
	async_req_fast(exch, method, arg1, arg2, arg3, 0, rc1, NULL, NULL, \
	    NULL, NULL)
#define async_req_3_2(exch, method, arg1, arg2, arg3, rc1, rc2) \
	async_req_fast(exch, method, arg1, arg2, arg3, 0, rc1, rc2, NULL, \
	    NULL, NULL)
#define async_req_3_3(exch, method, arg1, arg2, arg3, rc1, rc2, rc3) \
	async_req_fast(exch, method, arg1, arg2, arg3, 0, rc1, rc2, rc3, \
	    NULL, NULL)
#define async_req_3_4(exch, method, arg1, arg2, arg3, rc1, rc2, rc3, rc4) \
	async_req_fast(exch, method, arg1, arg2, arg3, 0, rc1, rc2, rc3, \
	    rc4, NULL)
#define async_req_3_5(exch, method, arg1, arg2, arg3, rc1, rc2, rc3, rc4, \
    rc5) \
	async_req_fast(exch, method, arg1, arg2, arg3, 0, rc1, rc2, rc3, \
	    rc4, rc5)

#define async_req_4_0(exch, method, arg1, arg2, arg3, arg4) \
	async_req_fast(exch, method, arg1, arg2, arg3, arg4, NULL, NULL, \
	    NULL, NULL, NULL)
#define async_req_4_1(exch, method, arg1, arg2, arg3, arg4, rc1) \
	async_req_fast(exch, method, arg1, arg2, arg3, arg4, rc1, NULL, \
	    NULL, NULL, NULL)
#define async_req_4_2(exch, method, arg1, arg2, arg3, arg4, rc1, rc2) \
	async_req_fast(exch, method, arg1, arg2, arg3, arg4, rc1, rc2, NULL, \
	    NULL, NULL)
#define async_req_4_3(exch, method, arg1, arg2, arg3, arg4, rc1, rc2, rc3) \
	async_req_fast(exch, method, arg1, arg2, arg3, arg4, rc1, rc2, rc3, \
	    NULL, NULL)
#define async_req_4_4(exch, method, arg1, arg2, arg3, arg4, rc1, rc2, rc3, \
    rc4) \
	async_req_fast(exch, method, arg1, arg2, arg3, arg4, rc1, rc2, rc3, \
	    rc4, NULL)
#define async_req_4_5(exch, method, arg1, arg2, arg3, arg4, rc1, rc2, rc3, \
    rc4, rc5) \
	async_req_fast(exch, method, arg1, arg2, arg3, arg4, rc1, rc2, rc3, \
	    rc4, rc5)

#define async_req_5_0(exch, method, arg1, arg2, arg3, arg4, arg5) \
	async_req_slow(exch, method, arg1, arg2, arg3, arg4, arg5, NULL, \
	    NULL, NULL, NULL, NULL)
#define async_req_5_1(exch, method, arg1, arg2, arg3, arg4, arg5, rc1) \
	async_req_slow(exch, method, arg1, arg2, arg3, arg4, arg5, rc1, \
	    NULL, NULL, NULL, NULL)
#define async_req_5_2(exch, method, arg1, arg2, arg3, arg4, arg5, rc1, rc2) \
	async_req_slow(exch, method, arg1, arg2, arg3, arg4, arg5, rc1, rc2, \
	    NULL, NULL, NULL)
#define async_req_5_3(exch, method, arg1, arg2, arg3, arg4, arg5, rc1, rc2, \
    rc3) \
	async_req_slow(exch, method, arg1, arg2, arg3, arg4, arg5, rc1, rc2, \
	    rc3, NULL, NULL)
#define async_req_5_4(exch, method, arg1, arg2, arg3, arg4, arg5, rc1, rc2, \
    rc3, rc4) \
	async_req_slow(exch, method, arg1, arg2, arg3, arg4, arg5, rc1, rc2, \
	    rc3, rc4, NULL)
#define async_req_5_5(exch, method, arg1, arg2, arg3, arg4, arg5, rc1, rc2, \
    rc3, rc4, rc5) \
	async_req_slow(exch, method, arg1, arg2, arg3, arg4, arg5, rc1, rc2, \
	    rc3, rc4, rc5)

extern sysarg_t async_req_fast(async_exch_t *, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t, sysarg_t *, sysarg_t *, sysarg_t *, sysarg_t *,
    sysarg_t *);
extern sysarg_t async_req_slow(async_exch_t *, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t, sysarg_t, sysarg_t *, sysarg_t *, sysarg_t *,
    sysarg_t *, sysarg_t *);

extern async_sess_t *async_clone_establish(exch_mgmt_t, async_exch_t *);
extern async_sess_t *async_connect_me_to(exch_mgmt_t, async_exch_t *, sysarg_t,
    sysarg_t, sysarg_t);
extern async_sess_t *async_connect_me_to_blocking(exch_mgmt_t, async_exch_t *,
    sysarg_t, sysarg_t, sysarg_t);
extern async_sess_t *async_connect_kbox(task_id_t);

extern int async_connect_to_me(async_exch_t *, sysarg_t, sysarg_t, sysarg_t,
    async_client_conn_t, void *);

extern int async_hangup(async_sess_t *);
extern void async_poke(void);

extern async_exch_t *async_exchange_begin(async_sess_t *);
extern void async_exchange_end(async_exch_t *);

/*
 * FIXME These functions just work around problems with parallel exchange
 * management. Proper solution needs to be implemented.
 */
void async_sess_args_set(async_sess_t *sess, sysarg_t, sysarg_t, sysarg_t);

/*
 * User-friendly wrappers for async_share_in_start().
 */

#define async_share_in_start_0_0(exch, size, dst) \
	async_share_in_start(exch, size, 0, NULL, dst)
#define async_share_in_start_0_1(exch, size, flags, dst) \
	async_share_in_start(exch, size, 0, flags, dst)
#define async_share_in_start_1_0(exch, size, arg, dst) \
	async_share_in_start(exch, size, arg, NULL, dst)
#define async_share_in_start_1_1(exch, size, arg, flags, dst) \
	async_share_in_start(exch, size, arg, flags, dst)

extern int async_share_in_start(async_exch_t *, size_t, sysarg_t,
    unsigned int *, void **);
extern bool async_share_in_receive(ipc_callid_t *, size_t *);
extern int async_share_in_finalize(ipc_callid_t, void *, unsigned int);

extern int async_share_out_start(async_exch_t *, void *, unsigned int);
extern bool async_share_out_receive(ipc_callid_t *, size_t *, unsigned int *);
extern int async_share_out_finalize(ipc_callid_t, void **);

/*
 * User-friendly wrappers for async_data_read_forward_fast().
 */

#define async_data_read_forward_0_0(exch, method, answer) \
	async_data_read_forward_fast(exch, method, 0, 0, 0, 0, NULL)
#define async_data_read_forward_0_1(exch, method, answer) \
	async_data_read_forward_fast(exch, method, 0, 0, 0, 0, answer)
#define async_data_read_forward_1_0(exch, method, arg1, answer) \
	async_data_read_forward_fast(exch, method, arg1, 0, 0, 0, NULL)
#define async_data_read_forward_1_1(exch, method, arg1, answer) \
	async_data_read_forward_fast(exch, method, arg1, 0, 0, 0, answer)
#define async_data_read_forward_2_0(exch, method, arg1, arg2, answer) \
	async_data_read_forward_fast(exch, method, arg1, arg2, 0, 0, NULL)
#define async_data_read_forward_2_1(exch, method, arg1, arg2, answer) \
	async_data_read_forward_fast(exch, method, arg1, arg2, 0, 0, answer)
#define async_data_read_forward_3_0(exch, method, arg1, arg2, arg3, answer) \
	async_data_read_forward_fast(exch, method, arg1, arg2, arg3, 0, NULL)
#define async_data_read_forward_3_1(exch, method, arg1, arg2, arg3, answer) \
	async_data_read_forward_fast(exch, method, arg1, arg2, arg3, 0, \
	    answer)
#define async_data_read_forward_4_0(exch, method, arg1, arg2, arg3, arg4, \
    answer) \
	async_data_read_forward_fast(exch, method, arg1, arg2, arg3, arg4, \
	    NULL)
#define async_data_read_forward_4_1(exch, method, arg1, arg2, arg3, arg4, \
    answer) \
	async_data_read_forward_fast(exch, method, arg1, arg2, arg3, arg4, \
	    answer)

extern aid_t async_data_read(async_exch_t *, void *, size_t, ipc_call_t *);
extern int async_data_read_start(async_exch_t *, void *, size_t);
extern bool async_data_read_receive(ipc_callid_t *, size_t *);
extern int async_data_read_finalize(ipc_callid_t, const void *, size_t);

extern int async_data_read_forward_fast(async_exch_t *, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t, sysarg_t, ipc_call_t *);

/*
 * User-friendly wrappers for async_data_write_forward_fast().
 */

#define async_data_write_forward_0_0(exch, method, answer) \
	async_data_write_forward_fast(exch, method, 0, 0, 0, 0, NULL)
#define async_data_write_forward_0_1(exch, method, answer) \
	async_data_write_forward_fast(exch, method, 0, 0, 0, 0, answer)
#define async_data_write_forward_1_0(exch, method, arg1, answer) \
	async_data_write_forward_fast(exch, method, arg1, 0, 0, 0, NULL)
#define async_data_write_forward_1_1(exch, method, arg1, answer) \
	async_data_write_forward_fast(exch, method, arg1, 0, 0, 0, answer)
#define async_data_write_forward_2_0(exch, method, arg1, arg2, answer) \
	async_data_write_forward_fast(exch, method, arg1, arg2, 0, 0, NULL)
#define async_data_write_forward_2_1(exch, method, arg1, arg2, answer) \
	async_data_write_forward_fast(exch, method, arg1, arg2, 0, 0, answer)
#define async_data_write_forward_3_0(exch, method, arg1, arg2, arg3, answer) \
	async_data_write_forward_fast(exch, method, arg1, arg2, arg3, 0, \
	    NULL)
#define async_data_write_forward_3_1(exch, method, arg1, arg2, arg3, answer) \
	async_data_write_forward_fast(exch, method, arg1, arg2, arg3, 0, \
	    answer)
#define async_data_write_forward_4_0(exch, method, arg1, arg2, arg3, arg4, \
    answer) \
	async_data_write_forward_fast(exch, method, arg1, arg2, arg3, arg4, \
	    NULL)
#define async_data_write_forward_4_1(exch, method, arg1, arg2, arg3, arg4, \
    answer) \
	async_data_write_forward_fast(exch, method, arg1, arg2, arg3, arg4, \
	    answer)

extern int async_data_write_start(async_exch_t *, const void *, size_t);
extern bool async_data_write_receive(ipc_callid_t *, size_t *);
extern int async_data_write_finalize(ipc_callid_t, void *, size_t);

extern int async_data_write_accept(void **, const bool, const size_t,
    const size_t, const size_t, size_t *);
extern void async_data_write_void(sysarg_t);

extern int async_data_write_forward_fast(async_exch_t *, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t, sysarg_t, ipc_call_t *);

extern int async_exchange_clone(async_exch_t *, async_exch_t *);
extern async_sess_t *async_clone_receive(exch_mgmt_t);
extern async_sess_t *async_callback_receive(exch_mgmt_t);
extern async_sess_t *async_callback_receive_start(exch_mgmt_t, ipc_call_t *);

extern int async_state_change_start(async_exch_t *, sysarg_t, sysarg_t,
    sysarg_t, async_exch_t *);
extern bool async_state_change_receive(ipc_callid_t *, sysarg_t *, sysarg_t *,
    sysarg_t *);
extern int async_state_change_finalize(ipc_callid_t, async_exch_t *);

extern void *async_remote_state_acquire(async_sess_t *);
extern void async_remote_state_update(async_sess_t *, void *);
extern void async_remote_state_release(async_sess_t *);
extern void async_remote_state_release_exchange(async_exch_t *);

#endif

/** @}
 */
