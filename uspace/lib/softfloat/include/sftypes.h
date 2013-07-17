/*
 * Copyright (c) 2005 Josef Cejka
 * Copyright (c) 2011 Petr Koupy
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

/** @addtogroup softfloat
 * @{
 */
/** @file Floating point types and constants.
 */

#ifndef __SFTYPES_H__
#define __SFTYPES_H__

#include <byteorder.h>
#include <stdint.h>

/*
 * For recognizing NaNs or infinity use specialized comparison
 * functions, comparing with these constants is not sufficient.
 */

#define FLOAT32_NAN     UINT32_C(0x7FC00001)
#define FLOAT32_SIGNAN  UINT32_C(0x7F800001)
#define FLOAT32_INF     UINT32_C(0x7F800000)

#define FLOAT64_NAN     UINT64_C(0x7FF8000000000001)
#define FLOAT64_SIGNAN  UINT64_C(0x7FF0000000000001)
#define FLOAT64_INF     UINT64_C(0x7FF0000000000000)

#define FLOAT96_NAN_HI     UINT64_C(0x7FFF80000000)
#define FLOAT96_NAN_LO     UINT32_C(0x00010000)
#define FLOAT96_SIGNAN_HI  UINT64_C(0x7FFF00000000)
#define FLOAT96_SIGNAN_LO  UINT32_C(0x00010000)

#define FLOAT128_NAN_HI     UINT64_C(0x7FFF800000000000)
#define FLOAT128_NAN_LO     UINT64_C(0x0000000000000001)
#define FLOAT128_SIGNAN_HI  UINT64_C(0x7FFF000000000000)
#define FLOAT128_SIGNAN_LO  UINT64_C(0x0000000000000001)
#define FLOAT128_INF_HI     UINT64_C(0x7FFF000000000000)
#define FLOAT128_INF_LO     UINT64_C(0x0000000000000000)

#define FLOAT32_FRACTION_SIZE   23
#define FLOAT64_FRACTION_SIZE   52
#define FLOAT96_FRACTION_SIZE   64
#define FLOAT128_FRACTION_SIZE  112
#define FLOAT128_FRAC_HI_SIZE   48
#define FLOAT128_FRAC_LO_SIZE   64

#define FLOAT32_HIDDEN_BIT_MASK      UINT32_C(0x800000)
#define FLOAT64_HIDDEN_BIT_MASK      UINT64_C(0x10000000000000)
#define FLOAT128_HIDDEN_BIT_MASK_HI  UINT64_C(0x1000000000000)
#define FLOAT128_HIDDEN_BIT_MASK_LO  UINT64_C(0x0000000000000000)

#define FLOAT32_MAX_EXPONENT   0xFF
#define FLOAT64_MAX_EXPONENT   0x7FF
#define FLOAT96_MAX_EXPONENT   0x7FFF
#define FLOAT128_MAX_EXPONENT  0x7FFF

#define FLOAT32_BIAS   0x7F
#define FLOAT64_BIAS   0x3FF
#define FLOAT96_BIAS   0x3FFF
#define FLOAT128_BIAS  0x3FFF

#if defined(__BE__)

typedef union {
	uint32_t bin;
	
	struct {
		uint32_t sign : 1;
		uint32_t exp : 8;
		uint32_t fraction : 23;
	} parts __attribute__((packed));
} float32;

typedef union {
	uint64_t bin;
	
	struct {
		uint64_t sign : 1;
		uint64_t exp : 11;
		uint64_t fraction : 52;
	} parts __attribute__((packed));
} float64;

typedef union {
	struct {
		uint64_t hi;
		uint32_t lo;
	} bin __attribute__((packed));
	
	struct {
		uint64_t padding : 16;
		uint64_t sign : 1;
		uint64_t exp : 15;
		uint64_t fraction : 64;
	} parts __attribute__((packed));
} float96;

typedef union {
	struct {
		uint64_t hi;
		uint64_t lo;
	} bin __attribute__((packed));
	
	struct {
		uint64_t sign : 1;
		uint64_t exp : 15;
		uint64_t frac_hi : 48;
		uint64_t frac_lo : 64;
	} parts __attribute__((packed));
} float128;

#elif defined(__LE__)

typedef union {
	uint32_t bin;
	
	struct {
		uint32_t fraction : 23;
		uint32_t exp : 8;
		uint32_t sign : 1;
	} parts __attribute__((packed));
} float32;

typedef union {
	uint64_t bin;
	
	struct {
		uint64_t fraction : 52;
		uint64_t exp : 11;
		uint64_t sign : 1;
	} parts __attribute__((packed));
} float64;

typedef union {
	struct {
		uint32_t lo;
		uint64_t hi;
	} bin __attribute__((packed));
	
	struct {
		uint64_t fraction : 64;
		uint64_t exp : 15;
		uint64_t sign : 1;
		uint64_t padding : 16;
	} parts __attribute__((packed));
} float96;

typedef union {
	struct {
		uint64_t lo;
		uint64_t hi;
	} bin __attribute__((packed));
	
	struct {
		uint64_t frac_lo : 64;
		uint64_t frac_hi : 48;
		uint64_t exp : 15;
		uint64_t sign : 1;
	} parts __attribute__((packed));
} float128;

#else
	#error Unknown endianess
#endif

typedef union {
	float val;
	
#if defined(FLOAT_SIZE_32)
	float32 data;
#elif defined(FLOAT_SIZE_64)
	float64 data;
#elif defined(FLOAT_SIZE_96)
	float96 data;
#elif defined(FLOAT_SIZE_128)
	float128 data;
#else
	#error Unsupported float size
#endif
} float_t;

typedef union {
	double val;
	
#if defined(DOUBLE_SIZE_32)
	float32 data;
#elif defined(DOUBLE_SIZE_64)
	float64 data;
#elif defined(DOUBLE_SIZE_96)
	float96 data;
#elif defined(DOUBLE_SIZE_128)
	float128 data;
#else
	#error Unsupported double size
#endif
} double_t;

typedef union {
	long double val;
	
#if defined(LONG_DOUBLE_SIZE_32)
	float32 data;
#elif defined(LONG_DOUBLE_SIZE_64)
	float64 data;
#elif defined(LONG_DOUBLE_SIZE_96)
	float96 data;
#elif defined(LONG_DOUBLE_SIZE_128)
	float128 data;
#else
	#error Unsupported long double size
#endif
} long_double_t;


#if defined(INT_SIZE_8)

#define _to_int   _to_int8
#define from_int  int8

#elif defined(INT_SIZE_16)

#define _to_int   _to_int16
#define from_int  int16

#elif defined(INT_SIZE_32)

#define _to_int   _to_int32
#define from_int  int32

#elif defined(INT_SIZE_64)

#define _to_int   _to_int64
#define from_int  int64

#endif


#if defined(UINT_SIZE_8)

#define _to_uint   _to_uint8
#define from_uint  uint8

#elif defined(UINT_SIZE_16)

#define _to_uint   _to_uint16
#define from_uint  uint16

#elif defined(UINT_SIZE_32)

#define _to_uint   _to_uint32
#define from_uint  uint32

#elif defined(UINT_SIZE_64)

#define _to_uint   _to_uint64
#define from_uint  uint64

#endif


#if defined(LONG_SIZE_8)

#define _to_long   _to_int8
#define from_long  int8

#elif defined(LONG_SIZE_16)

#define _to_long   _to_int16
#define from_long  int16

#elif defined(LONG_SIZE_32)

#define _to_long   _to_int32
#define from_long  int32

#elif defined(LONG_SIZE_64)

#define _to_long   _to_int64
#define from_long  int64

#endif


#if defined(ULONG_SIZE_8)

#define _to_ulong   _to_uint8
#define from_ulong  uint8

#elif defined(ULONG_SIZE_16)

#define _to_ulong   _to_uint16
#define from_ulong  uint16

#elif defined(ULONG_SIZE_32)

#define _to_ulong   _to_uint32
#define from_ulong  uint32

#elif defined(ULONG_SIZE_64)

#define _to_ulong   _to_uint64
#define from_ulong  uint64

#endif


#if defined(LLONG_SIZE_8)

#define _to_llong   _to_int8
#define from_llong  int8

#elif defined(LLONG_SIZE_16)

#define _to_llong   _to_int16
#define from_llong  int16

#elif defined(LLONG_SIZE_32)

#define _to_llong   _to_int32
#define from_llong  int32

#elif defined(LLONG_SIZE_64)

#define _to_llong   _to_int64
#define from_llong  int64

#endif


#if defined(ULLONG_SIZE_8)

#define _to_ullong   _to_uint8
#define from_ullong  uint8

#elif defined(ULLONG_SIZE_16)

#define _to_ullong   _to_uint16
#define from_ullong  uint16

#elif defined(ULLONG_SIZE_32)

#define _to_ullong   _to_uint32
#define from_ullong  uint32

#elif defined(ULLONG_SIZE_64)

#define _to_ullong   _to_uint64
#define from_ullong  uint64

#endif


#if defined(FLOAT_SIZE_32)

#define add_float     add_float32
#define sub_float     sub_float32
#define mul_float     mul_float32
#define div_float     div_float32
#define _to_float     _to_float32
#define from_float    float32
#define is_float_nan  is_float32_nan
#define is_float_eq   is_float32_eq
#define is_float_lt   is_float32_lt
#define is_float_gt   is_float32_gt

#elif defined(FLOAT_SIZE_64)

#define add_float     add_float64
#define sub_float     sub_float64
#define mul_float     mul_float64
#define div_float     div_float64
#define _to_float     _to_float64
#define from_float    float64
#define is_float_nan  is_float64_nan
#define is_float_eq   is_float64_eq
#define is_float_lt   is_float64_lt
#define is_float_gt   is_float64_gt

#elif defined(FLOAT_SIZE_96)

#define add_float     add_float96
#define sub_float     sub_float96
#define mul_float     mul_float96
#define div_float     div_float96
#define _to_float     _to_float96
#define from_float    float96
#define is_float_nan  is_float96_nan
#define is_float_eq   is_float96_eq
#define is_float_lt   is_float96_lt
#define is_float_gt   is_float96_gt

#elif defined(FLOAT_SIZE_128)

#define add_float     add_float128
#define sub_float     sub_float128
#define mul_float     mul_float128
#define div_float     div_float128
#define _to_float     _to_float128
#define from_float    float128
#define is_float_nan  is_float128_nan
#define is_float_eq   is_float128_eq
#define is_float_lt   is_float128_lt
#define is_float_gt   is_float128_gt

#endif


#if defined(DOUBLE_SIZE_32)

#define add_double     add_float32
#define sub_double     sub_float32
#define mul_double     mul_float32
#define div_double     div_float32
#define _to_double     _to_float32
#define from_double    float32
#define is_double_nan  is_float32_nan
#define is_double_eq   is_float32_eq
#define is_double_lt   is_float32_lt
#define is_double_gt   is_float32_gt

#elif defined(DOUBLE_SIZE_64)

#define add_double     add_float64
#define sub_double     sub_float64
#define mul_double     mul_float64
#define div_double     div_float64
#define _to_double     _to_float64
#define from_double    float64
#define is_double_nan  is_float64_nan
#define is_double_eq   is_float64_eq
#define is_double_lt   is_float64_lt
#define is_double_gt   is_float64_gt

#elif defined(DOUBLE_SIZE_96)

#define add_double     add_float96
#define sub_double     sub_float96
#define mul_double     mul_float96
#define div_double     div_float96
#define _to_double     _to_float96
#define from_double    float96
#define is_double_nan  is_float96_nan
#define is_double_eq   is_float96_eq
#define is_double_lt   is_float96_lt
#define is_double_gt   is_float96_gt

#elif defined(DOUBLE_SIZE_128)

#define add_double     add_float128
#define sub_double     sub_float128
#define mul_double     mul_float128
#define div_double     div_float128
#define _to_double     _to_float128
#define from_double    float128
#define is_double_nan  is_float128_nan
#define is_double_eq   is_float128_eq
#define is_double_lt   is_float128_lt
#define is_double_gt   is_float128_gt

#endif


#if defined(LONG_DOUBLE_SIZE_32)

#define add_long_double     add_float32
#define sub_long_double     sub_float32
#define mul_long_double     mul_float32
#define div_long_double     div_float32
#define _to_long_double     _to_float32
#define from_long_double    float32
#define is_long_double_nan  is_float32_nan
#define is_long_double_eq   is_float32_eq
#define is_long_double_lt   is_float32_lt
#define is_long_double_gt   is_float32_gt

#elif defined(LONG_DOUBLE_SIZE_64)

#define add_long_double     add_float64
#define sub_long_double     sub_float64
#define mul_long_double     mul_float64
#define div_long_double     div_float64
#define _to_long_double     _to_float64
#define from_long_double    float64
#define is_long_double_nan  is_float64_nan
#define is_long_double_eq   is_float64_eq
#define is_long_double_lt   is_float64_lt
#define is_long_double_gt   is_float64_gt

#elif defined(LONG_DOUBLE_SIZE_96)

#define add_long_double     add_float96
#define sub_long_double     sub_float96
#define mul_long_double     mul_float96
#define div_long_double     div_float96
#define _to_long_double     _to_float96
#define from_long_double    float96
#define is_long_double_nan  is_float96_nan
#define is_long_double_eq   is_float96_eq
#define is_long_double_lt   is_float96_lt
#define is_long_double_gt   is_float96_gt

#elif defined(LONG_DOUBLE_SIZE_128)

#define add_long_double     add_float128
#define sub_long_double     sub_float128
#define mul_long_double     mul_float128
#define div_long_double     div_float128
#define _to_long_double     _to_float128
#define from_long_double    float128
#define is_long_double_nan  is_float128_nan
#define is_long_double_eq   is_float128_eq
#define is_long_double_lt   is_float128_lt
#define is_long_double_gt   is_float128_gt

#endif


#define CONCAT(a, b)       CONCAT_ARGS(a, b)
#define CONCAT_ARGS(a, b)  a ## b

#define float32_to_float32(arg)    (arg)
#define float64_to_float64(arg)    (arg)
#define float96_to_float96(arg)    (arg)
#define float128_to_float128(arg)  (arg)

#define float_to_double       CONCAT(from_float, _to_double)
#define float_to_long_double  CONCAT(from_float, _to_long_double)
#define float_to_int          CONCAT(from_float, _to_int)
#define float_to_uint         CONCAT(from_float, _to_uint)
#define float_to_long         CONCAT(from_float, _to_long)
#define float_to_ulong        CONCAT(from_float, _to_ulong)
#define float_to_llong        CONCAT(from_float, _to_llong)
#define float_to_ullong       CONCAT(from_float, _to_ullong)

#define double_to_float        CONCAT(from_double, _to_float)
#define double_to_long_double  CONCAT(from_double, _to_long_double)
#define double_to_int          CONCAT(from_double, _to_int)
#define double_to_uint         CONCAT(from_double, _to_uint)
#define double_to_long         CONCAT(from_double, _to_long)
#define double_to_ulong        CONCAT(from_double, _to_ulong)
#define double_to_llong        CONCAT(from_double, _to_llong)
#define double_to_ullong       CONCAT(from_double, _to_ullong)

#define long_double_to_float   CONCAT(from_long_double, _to_float)
#define long_double_to_double  CONCAT(from_long_double, _to_double)
#define long_double_to_int     CONCAT(from_long_double, _to_int)
#define long_double_to_uint    CONCAT(from_long_double, _to_uint)
#define long_double_to_long    CONCAT(from_long_double, _to_long)
#define long_double_to_ulong   CONCAT(from_long_double, _to_ulong)
#define long_double_to_llong   CONCAT(from_long_double, _to_llong)
#define long_double_to_ullong  CONCAT(from_long_double, _to_ullong)

#define int_to_float        CONCAT(from_int, _to_float)
#define int_to_double       CONCAT(from_int, _to_double)
#define int_to_long_double  CONCAT(from_int, _to_long_double)

#define uint_to_float        CONCAT(from_uint, _to_float)
#define uint_to_double       CONCAT(from_uint, _to_double)
#define uint_to_long_double  CONCAT(from_uint, _to_long_double)

#define long_to_float        CONCAT(from_long, _to_float)
#define long_to_double       CONCAT(from_long, _to_double)
#define long_to_long_double  CONCAT(from_long, _to_long_double)

#define ulong_to_float        CONCAT(from_ulong, _to_float)
#define ulong_to_double       CONCAT(from_ulong, _to_double)
#define ulong_to_long_double  CONCAT(from_ulong, _to_long_double)

#define llong_to_float        CONCAT(from_llong, _to_float)
#define llong_to_double       CONCAT(from_llong, _to_double)
#define llong_to_long_double  CONCAT(from_llong, _to_long_double)

#define ullong_to_float        CONCAT(from_ullong, _to_float)
#define ullong_to_double       CONCAT(from_ullong, _to_double)
#define ullong_to_long_double  CONCAT(from_ullong, _to_long_double)


#endif

/** @}
 */
