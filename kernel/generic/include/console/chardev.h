/*
 * Copyright (c) 2005 Jakub Jermar
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

/** @addtogroup genericconsole
 * @{
 */
/** @file
 */

#ifndef KERN_CHARDEV_H_
#define KERN_CHARDEV_H_

#include <adt/list.h>
#include <typedefs.h>
#include <synch/waitq.h>
#include <synch/spinlock.h>

#define INDEV_BUFLEN  512

struct indev;

/* Input character device operations interface. */
typedef struct {
	/** Read character directly from device, assume interrupts disabled. */
	wchar_t (* poll)(struct indev *);
} indev_operations_t;

/** Character input device. */
typedef struct indev {
	const char *name;
	waitq_t wq;
	
	/** Protects everything below. */
	IRQ_SPINLOCK_DECLARE(lock);
	wchar_t buffer[INDEV_BUFLEN];
	size_t counter;
	
	/** Implementation of indev operations. */
	indev_operations_t *op;
	size_t index;
	void *data;
} indev_t;


struct outdev;

/* Output character device operations interface. */
typedef struct {
	/** Write character to output. */
	void (* write)(struct outdev *, wchar_t);
	
	/** Redraw any previously cached characters. */
	void (* redraw)(struct outdev *);
} outdev_operations_t;

/** Character output device. */
typedef struct outdev {
	const char *name;
	
	/** Protects everything below. */
	SPINLOCK_DECLARE(lock);
	
	/** Fields suitable for multiplexing. */
	link_t link;
	list_t list;
	
	/** Implementation of outdev operations. */
	outdev_operations_t *op;
	void *data;
} outdev_t;

extern void indev_initialize(const char *, indev_t *,
    indev_operations_t *);
extern void indev_push_character(indev_t *, wchar_t);
extern wchar_t indev_pop_character(indev_t *);

extern void outdev_initialize(const char *, outdev_t *,
    outdev_operations_t *);

extern bool check_poll(indev_t *);

#endif /* KERN_CHARDEV_H_ */

/** @}
 */
