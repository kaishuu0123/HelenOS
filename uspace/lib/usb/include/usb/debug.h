/*
 * Copyright (c) 2010-2011 Vojtech Horky
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

/** @addtogroup libusb
 * @{
 */
/** @file
 * Debugging related functions.
 */
#ifndef LIBUSB_DEBUG_H_
#define LIBUSB_DEBUG_H_
#include <stdio.h>
#include <inttypes.h>
#include <usb/usb.h>
#include <assert.h>

void usb_dump_standard_descriptor(FILE *, const char *, const char *,
    const uint8_t *, size_t);

/** Logging level. */
typedef enum {
	/** Fatal, unrecoverable, error.
	 * Such error prevents the driver from working at all.
	 */
	USB_LOG_LEVEL_FATAL,

	/** Serious but recoverable error
	 * Shall be used for errors fatal for single device but not for
	 * driver itself.
	 */
	USB_LOG_LEVEL_ERROR,

	/** Warning.
	 * Problems from which the driver is able to recover gracefully.
	 */
	USB_LOG_LEVEL_WARNING,

	/** Information message.
	 * This should be the last level that is printed by default to
	 * the screen.
	 * Typical usage is to inform that new device was found and what
	 * are its capabilities.
	 * Do not use for repetitive actions (such as device polling).
	 */
	USB_LOG_LEVEL_INFO,

	/** Debugging message. */
	USB_LOG_LEVEL_DEBUG,

	/** More detailed debugging message. */
	USB_LOG_LEVEL_DEBUG2,

	/** Terminating constant for logging levels. */
	USB_LOG_LEVEL_MAX
} usb_log_level_t;

/** Default log level. */
#ifdef CONFIG_USB_VERBOSE
	#define USB_LOG_LEVEL_DEFAULT USB_LOG_LEVEL_DEBUG
#else
	#define USB_LOG_LEVEL_DEFAULT USB_LOG_LEVEL_INFO
#endif

void usb_log_enable(usb_log_level_t, const char *);

void usb_log_printf(usb_log_level_t, const char *, ...)
	PRINTF_ATTRIBUTE(2, 3);

/** Log fatal error. */
#define usb_log_fatal(format, ...) \
	usb_log_printf(USB_LOG_LEVEL_FATAL, format, ##__VA_ARGS__)

/** Log normal (recoverable) error. */
#define usb_log_error(format, ...) \
	usb_log_printf(USB_LOG_LEVEL_ERROR, format, ##__VA_ARGS__)

/** Log warning. */
#define usb_log_warning(format, ...) \
	usb_log_printf(USB_LOG_LEVEL_WARNING, format, ##__VA_ARGS__)

/** Log informational message. */
#define usb_log_info(format, ...) \
	usb_log_printf(USB_LOG_LEVEL_INFO, format, ##__VA_ARGS__)

/** Log debugging message. */
#define usb_log_debug(format, ...) \
	usb_log_printf(USB_LOG_LEVEL_DEBUG, format, ##__VA_ARGS__)

/** Log verbose debugging message. */
#define usb_log_debug2(format, ...) \
	usb_log_printf(USB_LOG_LEVEL_DEBUG2, format, ##__VA_ARGS__)

const char *usb_debug_str_buffer(const uint8_t *, size_t, size_t);


#endif
/**
 * @}
 */

