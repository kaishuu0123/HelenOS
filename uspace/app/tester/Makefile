#
# Copyright (c) 2005 Martin Decky
# Copyright (c) 2007 Jakub Jermar
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# - Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
# - Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
# - The name of the author may not be used to endorse or promote products
#   derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

USPACE_PREFIX = ../..
LIBS = $(LIBEXT2_PREFIX)/libext2.a $(LIBBLOCK_PREFIX)/libblock.a
EXTRA_CFLAGS = -I$(LIBBLOCK_PREFIX) -I$(LIBEXT2_PREFIX)
BINARY = tester

SOURCES = \
	tester.c \
	util.c \
	thread/thread1.c \
	print/print1.c \
	print/print2.c \
	print/print3.c \
	print/print4.c \
	print/print5.c \
	console/console1.c \
	stdio/stdio1.c \
	stdio/stdio2.c \
	fault/fault1.c \
	fault/fault2.c \
	fault/fault3.c \
	vfs/vfs1.c \
	ipc/ping_pong.c \
	ipc/starve.c \
	loop/loop1.c \
	mm/common.c \
	mm/malloc1.c \
	mm/malloc2.c \
	mm/malloc3.c \
	mm/mapping1.c \
	hw/misc/virtchar1.c \
	hw/serial/serial1.c \
	libext2/libext2_1.c

include $(USPACE_PREFIX)/Makefile.common
