/*
 * Copyright (c) 2010 Vojtech Horky
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

/** @addtogroup libdrv
 * @addtogroup usb
 * @{
 */

/** @file
 * @brief USB host controller interface definition.
 */

#ifndef LIBDRV_USBHC_IFACE_H_
#define LIBDRV_USBHC_IFACE_H_

#include "ddf/driver.h"
#include <usb/usb.h>
#include <bool.h>

int usbhc_request_address(async_exch_t *, usb_address_t *, bool, usb_speed_t);
int usbhc_bind_address(async_exch_t *, usb_address_t, devman_handle_t);
int usbhc_get_handle(async_exch_t *, usb_address_t, devman_handle_t *);
int usbhc_release_address(async_exch_t *, usb_address_t);
int usbhc_register_endpoint(async_exch_t *, usb_address_t, usb_endpoint_t,
    usb_transfer_type_t, usb_direction_t, size_t, unsigned int);
int usbhc_unregister_endpoint(async_exch_t *, usb_address_t, usb_endpoint_t,
    usb_direction_t);
int usbhc_read(async_exch_t *, usb_address_t, usb_endpoint_t,
    uint64_t, void *, size_t, size_t *);
int usbhc_write(async_exch_t *, usb_address_t, usb_endpoint_t,
    uint64_t, const void *, size_t);

/** Callback for outgoing transfer. */
typedef void (*usbhc_iface_transfer_out_callback_t)(ddf_fun_t *, int, void *);

/** Callback for incoming transfer. */
typedef void (*usbhc_iface_transfer_in_callback_t)(ddf_fun_t *,
    int, size_t, void *);

/** USB host controller communication interface. */
typedef struct {
	int (*request_address)(ddf_fun_t *, usb_address_t *, bool, usb_speed_t);
	int (*bind_address)(ddf_fun_t *, usb_address_t, devman_handle_t);
	int (*get_handle)(ddf_fun_t *, usb_address_t,
	    devman_handle_t *);
	int (*release_address)(ddf_fun_t *, usb_address_t);

	int (*register_endpoint)(ddf_fun_t *,
	    usb_address_t, usb_endpoint_t,
	    usb_transfer_type_t, usb_direction_t, size_t, unsigned int);
	int (*unregister_endpoint)(ddf_fun_t *, usb_address_t, usb_endpoint_t,
	    usb_direction_t);

	int (*read)(ddf_fun_t *, usb_target_t, uint64_t, uint8_t *, size_t,
	    usbhc_iface_transfer_in_callback_t, void *);

	int (*write)(ddf_fun_t *, usb_target_t, uint64_t, const uint8_t *,
	    size_t, usbhc_iface_transfer_out_callback_t, void *);
} usbhc_iface_t;


#endif
/**
 * @}
 */
