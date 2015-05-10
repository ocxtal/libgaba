
/**
 * @file avx_b8_r1.h
 *
 * @brief a header for macros of packed 8-bit AVX2 instructions.
 *
 * @detail
 * This is a collection of wrapper macros of SSE4.1 SIMD intrinsics.
 * Each macro corresponds to a set of several intrinsics defined in
 * smmintrin.h. The details of the intrinsics are found in the intel's
 * website: https://software.intel.com/sites/landingpage/IntrinsicsGuide/
 * The required set of macros are documented in porting section of 
 * README.md in the top directory of the library.
 *
 * @sa avx.h
 */
#ifndef _AVX_B8_R1_H_INCLUDED
#define _AVX_B8_R1_H_INCLUDED

#include <immintrin.h>

/**
 * register declarations. 
 */
#define DECLARE_VEC_CELL(v)			__m256i v
#define DECLARE_VEC_CELL_REG(v)		__m256i register v
#define DECLARE_VEC_CHAR_REG(v)		__m256i register v

/**
 * substitution to cell vectors
 */
#define VEC_ASSIGN(a, b) { \
	(a) = (b); \
}

#define VEC_SET(v, i) { \
	(v) = _mm256_set1_epi8(i); \
}

#define VEC_SETZERO(v) { \
	(v) = _mm256_setzero_si256(); \
}

#define VEC_SETONES(v) { \
	(v) = _mm256_set1_epi8(0xff); \
}

/**
 * substitution to char vectors
 */
#define VEC_CHAR_SETZERO(v) { \
	(v) = _mm256_setzero_si256(); \
}

#define VEC_CHAR_SETONES(v) { \
	(v) = _mm256_set1_epi8(0xff); \
}

/**
 * special substitution macros
 */
#define VEC_SET_LHALF(v, i) { \
	VEC_SET(v, i); (v) = _mm256_permute2x128_si256((v), (v), 0x80); \
}

#define VEC_SET_UHALF(v, i) { \
	VEC_SET(v, i); (v) = _mm256_permute2x128_si256((v), (v), 0x08); \
}

#define VEC_SETF_MSB(v) { \
	VEC_SETZERO(v); VEC_INSERT_MSB((v), 0xf0); \
}

#define VEC_SETF_LSB(v) { \
	VEC_SETZERO(v); VEC_INSERT_LSB((v), 0x0f); \
}

/**
 * insertion and extraction macros
 */
#define VEC_INSERT_MSB(v, a) { \
	(v) = _mm256_inserti128_si256( \
		(v), \
		_mm_insert_epi8(_mm256_extracti128_si256((v), 1), (a), sizeof(__m128i)-1), \
		1); \
}

#define VEC_INSERT_LSB(v, a) { \
	(v) = _mm256_inserti128_si256( \
		(v), \
		_mm_insert_epi8(_mm256_extracti128_si256((v), 0), (a), 0), \
		0); \
}

#define VEC_MSB(v)		( (signed char)_mm_extract_epi8(_mm256_extracti128_si256((v), 1), sizeof(__m128i)/sizeof(char)-1) )
#define VEC_LSB(v)		( (signed char)_mm_extract_epi8(_mm256_extracti128_si256((v), 0), 0) )
#define VEC_CENTER(v)	( (signed char)_mm_extract_epi8(_mm256_extracti128_si256((v), 1), 0) )

/**
 * arithmetic and logic operations
 */
#define VEC_OR(a, b, c) { \
	(a) = _mm256_or_si256((b), (c)); \
}

#define VEC_ADD(a, b, c) { \
	(a) = _mm256_adds_epi8((b), (c)); \
}

#define VEC_ADDS(a, b, c) { \
	(a) = _mm256_adds_epu8((b), (c)); \
}

#define VEC_SUB(a, b, c) { \
	(a) = _mm256_subs_epi8((b), (c)); \
}

#define VEC_SUBS(a, b, c) { \
	(a) = _mm256_subs_epu8((b), (c)); \
}

#define VEC_MAX(a, b, c) { \
	(a) = _mm256_max_epi8((b), (c)); \
}

#define VEC_MIN(a, b, c) { \
	(a) = _mm256_min_epi8((b), (c)); \
}

/**
 * shift operations
 *
 * Since AVX2 instruction sets does not have 256-bit through shift operations,
 * VEC_SHIFT_R and VEC_SHIFT_L macro need tricky implementation.
 *
 * SHIFT_R:
 *     First, _mm256_permute2x128_si256((a), (a), 0x81) moves the upper half of the
 *     register to the lower half, filling the upper zero:
 *         +------------+------------+
 * input:  | a[255:128] |  a[127:0]  |
 *         +------------+------------+
 *    v
 *         +------------+------------+
 * output: |     0      | a[255:128] |
 *         +------------+------------+
 *     Then, _mm256_alignr_epi8 concatenates two inputs, with former vector divided
 *     128-bits and moved to the upper half of the temporary, and lattar vector 
 *     to the lower half of the temporary. The two temporary register is then shifted 
 *     right by 8-bit respectively, and the lower half of the temporary registers are
 *     concatenated into 256-bit result.
 *         +------------+------------+  +------------+------------+
 * input:  |     0      | a[255:128] |  | a[255:128] |  a[127:0]  |
 *         +------------+------------+  +------------+------------+
 *    v
 *         +----------------------------------+----------------------------------+
 * output: |     (0<<128 | a[255:128])>>8     |  (a[255:128]<<128 | a[127:0])>>8 |
 *         +----------------------------------+----------------------------------+
 *
 *
 * SHIFT_L:
 *     SHIFT_L operation consists of the same elements as the SHIFT_R operation.
 *     We show only the diagrams of the movement of the data.
 *     _mm256_permute2x128_si256:
 *         +------------+------------+
 * input:  | a[255:128] |  a[127:0]  |
 *         +------------+------------+
 *    v
 *         +------------+------------+
 * output: |  a[127:0]  |     0      |
 *         +------------+------------+
 *     _mm256_alignr_epi8:
 *         +------------+------------+  +------------+------------+
 * input:  | a[255:128] |  a[127:0]  |  |  a[127:0]  |     0      |
 *         +------------+------------+  +------------+------------+
 *    v
 *         +-----------------------------------+----------------------------------+
 * output: | (a[255:128]<<128 | a[127:0])>>120 |     (a[127:0]<<128 | 0)>>120     |
 *         +-----------------------------------+----------------------------------+
 */
#define VEC_SHIFT_R(a) { \
	(a) = _mm256_alignr_epi8( \
		_mm256_permute2x128_si256((a), (a), 0x81), \
		(a), \
		1); \
}

#define VEC_SHIFT_L(a) { \
	(a) = _mm256_alignr_epi8( \
		(a), \
		_mm256_permute2x128_si256((a), (a), 0x08), \
		sizeof(__m128i)-1); \
}

/**
 * compare and select
 */
#define VEC_COMPARE(a, b, c) { \
	(a) = _mm256_cmpeq_epi8((b), (c)); \
}

#define VEC_SELECT(a, b, c, d) { \
	(a) = _mm256_blendv_epi8((b), (c), (d)); \
}

/**
 * load and store operations
 */
#define VEC_STORE(p, v) { \
	_mm256_store_si256((__m256i *)(p), v); p += sizeof(__m256i); \
}

#define VEC_STORE_PACKED(p, dv, dh) { \
	_mm256_store_si256( \
		(__m256i *)(p), \
		_mm256_or_si256(_mm256_slli_epi64(dh, 4), dv)); \
	p += sizeof(__m256i); \
}

/**
 * char vector operations
 */
#define VEC_CHAR_LOAD(p, v) { \
	VEC_LOAD(p, v); \
}

#define VEC_CHAR_STORE(p, v) { \
	VEC_STORE(p, v); \
}

#define VEC_CHAR_SHIFT_R(a) { \
	VEC_SHIFT_R(a); \
}

#define VEC_CHAR_SHIFT_L(a) { \
	VEC_SHIFT_L(a); \
}

#define VEC_CHAR_INSERT_MSB(x, y) { \
	VEC_INSERT_MSB(x, y); \
}

#define VEC_CHAR_INSERT_LSB(x, y) { \
	VEC_INSERT_LSB(x, y); \
}

#endif /* #ifndef _AVX_B8_R1_H_INCLUDED */
/**
 * end of avx_b8_r1.h
 */
