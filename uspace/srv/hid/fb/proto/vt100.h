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

/** @file
 */

#ifndef FB_PROTO_VT100_H_
#define FB_PROTO_VT100_H_

#include <sys/types.h>
#include <screenbuffer.h>

typedef void (* vt100_putchar_t)(wchar_t ch);
typedef void (* vt100_control_puts_t)(const char *str);

/** Forward declaration */
struct vt100_state;
typedef struct vt100_state vt100_state_t;

extern vt100_state_t *vt100_state_create(sysarg_t, sysarg_t, vt100_putchar_t,
    vt100_control_puts_t);
extern void vt100_get_resolution(vt100_state_t *, sysarg_t *, sysarg_t *);
extern int vt100_yield(vt100_state_t *);
extern int vt100_claim(vt100_state_t *);

extern void vt100_putchar(vt100_state_t *, wchar_t);

extern void vt100_set_attr(vt100_state_t *, char_attrs_t);
extern void vt100_goto(vt100_state_t *, sysarg_t, sysarg_t);
extern void vt100_cursor_visibility(vt100_state_t *, bool);

#endif

/** @}
 */
