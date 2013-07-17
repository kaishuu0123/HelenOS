/*
 * Copyright (c) 2009 Jakub Jermar
 * Copyright (c) 2010 Jiri Svoboda
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

#include <stacktrace.h>
#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>

static int stacktrace_read_uintptr(void *arg, uintptr_t addr, uintptr_t *data);

void stacktrace_print_fp_pc(uintptr_t fp, uintptr_t pc)
{
	stacktrace_t st;
	uintptr_t nfp;

	st.op_arg = NULL;
	st.read_uintptr = stacktrace_read_uintptr;

	while (stacktrace_fp_valid(&st, fp)) {
		printf("%p: %p()\n", (void *) fp, (void *) pc);
		(void) stacktrace_ra_get(&st, fp, &pc);
		(void) stacktrace_fp_prev(&st, fp, &nfp);
		fp = nfp;
	}
}

void stacktrace_print(void)
{
	stacktrace_prepare();
	stacktrace_print_fp_pc(stacktrace_fp_get(), stacktrace_pc_get());
	
	/*
	 * Prevent the tail call optimization of the previous call by
	 * making it a non-tail call.
	 */
	
	printf("-- end of stack trace --\n");
}

static int stacktrace_read_uintptr(void *arg, uintptr_t addr, uintptr_t *data)
{
	(void) arg;
	*data = *((uintptr_t *) addr);
	return EOK;
}

/** @}
 */
