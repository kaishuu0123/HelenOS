#
# Copyright (c) 2006 Martin Decky
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

BFD_NAME = elf64-ia64-little
BFD_OUTPUT = $(BFD_NAME)
BFD_ARCH = ia64

#
# FIXME:
#
# The -fno-selective-scheduling and -fno-selective-scheduling2 options
# should be removed as soon as a bug in GCC concerning unchecked
# speculative loads is fixed.
#
# See http://gcc.gnu.org/bugzilla/show_bug.cgi?id=53975 for reference.
#

BITS = 64
ENDIANESS = LE
EXTRA_CFLAGS = -fno-unwind-tables -mfixed-range=f32-f127 -mno-pic -mno-sdata -fno-selective-scheduling -fno-selective-scheduling2

RD_SRVS_NON_ESSENTIAL +=

RD_SRVS_ESSENTIAL +=

SOURCES = \
	arch/$(BARCH)/src/boot.S \
	arch/$(BARCH)/src/asm.S \
	arch/$(BARCH)/src/main.c \
	arch/$(BARCH)/src/sal.c \
	arch/$(BARCH)/src/sal_asm.S \
	arch/$(BARCH)/src/pal.c \
	arch/$(BARCH)/src/pal_asm.S \
	arch/$(BARCH)/src/putchar.c \
	$(COMPS_C) \
	genarch/src/efi.c \
	genarch/src/division.c \
	generic/src/balloc.c \
	generic/src/memstr.c \
	generic/src/printf_core.c \
	generic/src/vprintf.c \
	generic/src/printf.c \
	generic/src/str.c \
	generic/src/version.c \
	generic/src/inflate.c

ifeq ($(MACHINE),ski)
SOURCES += \
	arch/$(BARCH)/src/ski.c
endif
