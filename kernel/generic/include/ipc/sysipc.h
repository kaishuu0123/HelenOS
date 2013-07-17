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

/** @addtogroup genericipc
 * @{
 */
/** @file
 */

#ifndef KERN_SYSIPC_H_
#define KERN_SYSIPC_H_

#include <ipc/ipc.h>
#include <ipc/irq.h>
#include <typedefs.h>

extern sysarg_t sys_ipc_call_sync_fast(sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t, ipc_data_t *);
extern sysarg_t sys_ipc_call_sync_slow(sysarg_t, ipc_data_t *, ipc_data_t *);
extern sysarg_t sys_ipc_call_async_fast(sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t, sysarg_t);
extern sysarg_t sys_ipc_call_async_slow(sysarg_t, ipc_data_t *);
extern sysarg_t sys_ipc_answer_fast(sysarg_t, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t);
extern sysarg_t sys_ipc_answer_slow(sysarg_t, ipc_data_t *);
extern sysarg_t sys_ipc_wait_for_call(ipc_data_t *, uint32_t, unsigned int);
extern sysarg_t sys_ipc_poke(void);
extern sysarg_t sys_ipc_forward_fast(sysarg_t, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, unsigned int);
extern sysarg_t sys_ipc_forward_slow(sysarg_t, sysarg_t, ipc_data_t *,
    unsigned int);
extern sysarg_t sys_ipc_hangup(sysarg_t);
extern sysarg_t sys_irq_register(inr_t, devno_t, sysarg_t, irq_code_t *);
extern sysarg_t sys_irq_unregister(inr_t, devno_t);

#ifdef __32_BITS__

extern sysarg_t sys_ipc_connect_kbox(sysarg64_t *);

#endif  /* __32_BITS__ */

#ifdef __64_BITS__

extern sysarg_t sys_ipc_connect_kbox(sysarg_t);

#endif  /* __64_BITS__ */

#endif

/** @}
 */
