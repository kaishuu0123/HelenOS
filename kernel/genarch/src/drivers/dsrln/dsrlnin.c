/*
 * Copyright (c) 2009 Martin Decky
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

/** @addtogroup genarch
 * @{
 */
/**
 * @file
 * @brief Dummy serial line input.
 */

#include <genarch/drivers/dsrln/dsrlnin.h>
#include <console/chardev.h>
#include <mm/slab.h>
#include <arch/asm.h>
#include <ddi/device.h>

static irq_ownership_t dsrlnin_claim(irq_t *irq)
{
	return IRQ_ACCEPT;
}

static void dsrlnin_irq_handler(irq_t *irq)
{
	dsrlnin_instance_t *instance = irq->instance;
	dsrlnin_t *dev = instance->dsrlnin;
	
	indev_push_character(instance->srlnin, pio_read_8(&dev->data));
}

dsrlnin_instance_t *dsrlnin_init(dsrlnin_t *dev, inr_t inr)
{
	dsrlnin_instance_t *instance
	    = malloc(sizeof(dsrlnin_instance_t), FRAME_ATOMIC);
	if (instance) {
		instance->dsrlnin = dev;
		instance->srlnin = NULL;
		
		irq_initialize(&instance->irq);
		instance->irq.devno = device_assign_devno();
		instance->irq.inr = inr;
		instance->irq.claim = dsrlnin_claim;
		instance->irq.handler = dsrlnin_irq_handler;
		instance->irq.instance = instance;
	}
	
	return instance;
}

void dsrlnin_wire(dsrlnin_instance_t *instance, indev_t *srlnin)
{
	ASSERT(instance);
	ASSERT(srlnin);
	
	instance->srlnin = srlnin;
	irq_register(&instance->irq);
}

/** @}
 */
