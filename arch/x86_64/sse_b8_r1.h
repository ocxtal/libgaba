
/**
 * @file sse_b8_r1.h
 *
 * @brief a header for macros of packed 8-bit SSE4 instructions.
 *
 * @detail
 * This is a collection of wrapper macros of SSE4.1 SIMD intrinsics.
 * Each macro corresponds to a set of several intrinsics defined in
 * smmintrin.h. The details of the intrinsics are found in the intel's
 * website: https://software.intel.com/sites/landingpage/IntrinsicsGuide/
 * The required set of macros are documented in porting section of 
 * README.md in the top directory of the library.
 *
 * @sa sse.h
 */
#ifndef _SSE_B8_R1_H_INCLUDED
#define _SSE_B8_R1_H_INCLUDED

#ifndef _SIMD_INCLUDED
#define _SIMD_INCLUDED

#include <smmintrin.h>
#include <stdint.h>

/**
 * register declarations. 
 */
#define _vec_cell(v)				__m128i v
#define _vec_cell_const(v, k)		__m128i const v = _mm_set1_epi8(k)
#define _vec_cell_reg(v)			__m128i register v
#define _vec_char_reg(v)			__m128i register v

/**
 * substitution to cell vectors
 */
#define VEC_ASSIGN(a, b) { \
	(a) = (b); \
}

#define VEC_SET(v, i) { \
	(v) = _mm_set1_epi8(i); \
}

#define VEC_SETZERO(v) { \
	(v) = _mm_setzero_si128(); \
}

#define VEC_SETONES(v) { \
	(v) = _mm_set1_epi8(0xff); \
}

/**
 * substitution to char vectors
 */
#define VEC_CHAR_SETZERO(v) { \
	(v) = _mm_setzero_si128(); \
}

#define VEC_CHAR_SETONES(v) { \
	(v) = _mm_set1_epi8(0xff); \
}

/**
 * special substitution macros
 */
#define VEC_SET_LHALF(v, i) { \
	VEC_SET(v, i); (v) = _mm_srli_si128((v), BAND_WIDTH/2); \
}

#define VEC_SET_UHALF(v, i) { \
	VEC_SET(v, i); (v) = _mm_slli_si128((v), BAND_WIDTH/2); \
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
	(v) = _mm_insert_epi8((v), (a), sizeof(__m128i)-1); \
}

#define VEC_INSERT_LSB(v, a) { \
	(v) = _mm_insert_epi8((v), (a), 0); \
}

#define VEC_MSB(v)		( (signed char)_mm_extract_epi8((v), sizeof(__m128i)-1) )
#define VEC_LSB(v)		( (signed char)_mm_extract_epi8((v), 0) )
#define VEC_CENTER(v)	( (signed char)_mm_extract_epi8((v), 8) )

/**
 * arithmetic and logic operations
 */
#define VEC_OR(a, b, c) { \
	(a) = _mm_or_si128((b), (c)); \
}

#define VEC_ADD(a, b, c) { \
	(a) = _mm_adds_epi8((b), (c)); \
}

#define VEC_ADDS(a, b, c) { \
	(a) = _mm_adds_epu8((b), (c)); \
}

#define VEC_SUB(a, b, c) { \
	(a) = _mm_subs_epi8((b), (c)); \
}

#define VEC_SUBS(a, b, c) { \
	(a) = _mm_subs_epu8((b), (c)); \
}

#define VEC_MAX(a, b, c) { \
	(a) = _mm_max_epi8((b), (c)); \
}

#define VEC_MIN(a, b, c) { \
	(a) = _mm_min_epi8((b), (c)); \
}

/**
 * shift operations
 */
#define VEC_SHIFT_R(a, b) { \
	(a) = _mm_srli_si128((b), 1); \
}

#define VEC_SHIFT_L(a, b) { \
	(a) = _mm_slli_si128((b), 1); \
}

/**
 * compare and select
 */
#define VEC_COMPARE(a, b, c) { \
	(a) = _mm_cmpeq_epi8((b), (c)); \
}

#define VEC_SELECT(a, b, c, d) { \
	(a) = _mm_blendv_epi8((b), (c), (d)); \
}

/**
 * load and store operations
 */
#define VEC_STORE(p, v) { \
	_mm_store_si128((__m128i *)(p), v); p += sizeof(__m128i); \
}

#define VEC_STORE_PACKED(p, dv, dh) { \
	_mm_store_si128( \
		(__m128i *)(p), \
		_mm_or_si128(_mm_slli_epi64(dh, 4), dv)); \
	p += sizeof(__m128i); \
}

#define VEC_LOAD(p, v) { \
	v = _mm_load_si128((__m128i *)(p)); p += sizeof(__m128i); \
}

#define VEC_LOAD_PACKED(p, dv, dh) { \
	__m128i const mask = _mm_set1_epi8(0x0f); \
	dv = _mm_load_si128((__m128i *)(p)); \
	dh = _mm_and_si128(_mm_srli_epi64(dv, 4), mask); \
	dv = _mm_and_si128(dv, mask); \
	p += sizeof(__m128i); \
}

/**
 * store vector to 32-elem array of int32_t
 */
#define VEC_STORE32(p, v) { \
	__m128i t = v; \
 	__m128i const m = _mm_set1_epi32(CELL_MIN); \
 	VEC_STORE(p, m); VEC_STORE(p, m);	/** margin */ \
	VEC_STORE(p, _mm_cvtepi8_epi32(t)); \
	t = _mm_slli_si128(t, 4); \
	VEC_STORE(p, _mm_cvtepi8_epi32(t)); \
	t = _mm_slli_si128(t, 4); \
	VEC_STORE(p, _mm_cvtepi8_epi32(t)); \
	t = _mm_slli_si128(t, 4); \
	VEC_STORE(p, _mm_cvtepi8_epi32(t)); \
 	VEC_STORE(p, m); VEC_STORE(p, m);	/** margin */ \
}

/**
 * load vector from 32-elem array of int32_t
 */
#define VEC_LOAD32(p, v) { \
	__m128i t1, t2, t3, t4; \
	p += 2*sizeof(__m128i);	/** skip margin */ \
	VEC_LOAD(p, t1); \
	VEC_LOAD(p, t2); \
	VEC_LOAD(p, t3); \
	VEC_LOAD(p, t4); \
	p += 2*sizeof(__m128i);	/** skip margin */ \
	v = _mm_packs_epi32( \
		_mm_packs_epi16(t1, t2), \
		_mm_packs_epi16(t3, t4)); \
}

/**
 * print vector
 */
#define VEC_PRINT(s, v) { \
	int8_t b[16]; \
	void *p = (void *)b; \
	VEC_STORE(p, v); \
	fprintf(s, \
		"[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]\n", \
		b[15], b[14], b[13], b[12], b[11], b[10], b[9], b[8], \
		b[7], b[6], b[5], b[4], b[3], b[2], b[1], b[0]); \
}

/**
 * char vector operations
 */
#define VEC_CHAR_SHIFT_R(a, b) { \
	VEC_SHIFT_R(a, b); \
}

#define VEC_CHAR_SHIFT_L(a, b) { \
	VEC_SHIFT_L(a, b); \
}

#define VEC_CHAR_INSERT_MSB(x, y) { \
	VEC_INSERT_MSB(x, y); \
}

#define VEC_CHAR_INSERT_LSB(x, y) { \
	VEC_INSERT_LSB(x, y); \
}

#define VEC_CHAR_PRINT(s, v) { \
	VEC_PRINT(s, v); \
}

#endif /* #ifndef _SIMD_INCLUDED */

#endif /* #ifndef _SSE_B8_R1_H_INCLUDED */
/**
 * end of sse_b8_r1.h
 */
