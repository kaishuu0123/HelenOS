/*
 * Copyright (c) 2009 Jakub Jermar 
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
 * @}
 */

/** @addtogroup libc
 */
/** @file
 */

#include <libc.h>
#include <event.h>

/** Subscribe event notifications.
 *
 * @param evno    Event type to subscribe.
 * @param imethod Use this interface and method for notifying me.
 *
 * @return Value returned by the kernel.
 *
 */
int event_subscribe(event_type_t evno, sysarg_t imethod)
{
	return __SYSCALL2(SYS_EVENT_SUBSCRIBE, (sysarg_t) evno,
	    (sysarg_t) imethod);
}

int event_task_subscribe(event_task_type_t evno, sysarg_t imethod)
{
	return __SYSCALL2(SYS_EVENT_SUBSCRIBE, (sysarg_t) evno,
	    (sysarg_t) imethod);
}

/** Unmask event notifications.
 *
 * @param evno Event type to unmask.
 *
 * @return Value returned by the kernel.
 *
 */
int event_unmask(event_type_t evno)
{
	return __SYSCALL1(SYS_EVENT_UNMASK, (sysarg_t) evno);
}

int event_task_unmask(event_task_type_t evno)
{
	return __SYSCALL1(SYS_EVENT_UNMASK, (sysarg_t) evno);
}

/** @}
 */
