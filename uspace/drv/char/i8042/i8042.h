/*
 * Copyright (c) 2006 Josef Cejka
 * Copyright (c) 2011 Jan Vesely
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

/** @addtogroup kbd_port
 * @ingroup  kbd
 * @{
 */

/** @file
 * @brief i8042 port driver.
 */

#ifndef i8042_H_
#define i8042_H_

#include <sys/types.h>
#include <fibril_synch.h>
#include <ddf/driver.h>
#include "buffer.h"

#define NAME  "i8042"

#define BUFFER_SIZE  12

/** i8042 HW I/O interface */
typedef struct {
	ioport8_t data;
	uint8_t pad[3];
	ioport8_t status;
} __attribute__ ((packed)) i8042_regs_t;

/** i8042 driver structure. */
typedef struct i8042 {
	i8042_regs_t *regs;             /**< I/O registers. */
	ddf_fun_t *kbd_fun;             /**< Pirmary port device function. */
	ddf_fun_t *aux_fun;             /**< Auxiliary port device function. */
	buffer_t kbd_buffer;            /**< Primary port buffer. */
	buffer_t aux_buffer;            /**< Aux. port buffer. */
	uint8_t aux_data[BUFFER_SIZE];  /**< Primary port buffer space. */
	uint8_t kbd_data[BUFFER_SIZE];  /**< Aux. port buffer space. */
	fibril_mutex_t write_guard;     /**< Prevents simultanous port writes.*/
} i8042_t;

int i8042_init(i8042_t *, void *, size_t, int, int, ddf_dev_t *);

#endif

/**
 * @}
 */
