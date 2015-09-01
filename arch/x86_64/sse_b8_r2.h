
/**
 * @file sse_b8_r2.h
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
#ifndef _SSE_B8_R2_H_INCLUDED
#define _SSE_B8_R2_H_INCLUDED

#ifndef _SIMD_INCLUDED
#define _SIMD_INCLUDED

#include <smmintrin.h>
#include <stdint.h>

/**
 * register declarations. 
 */
#define _vec_cell(v)				__m128i v##1, v##2
#define _vec_cell_const(v, k)		__m128i const v##1 = _mm_set1_epi8(k), v##2 = _mm_set1_epi8(k)
#define _vec_cell_reg(v)			__m128i register v##1, v##2
#define _vec_char_reg(v)			__m128i register v##1, v##2

/**
 * substitution to cell vectors
 */
#define VEC_ASSIGN(a, b) { \
	(a##1) = (b##1); (a##2) = (b##2); \
}

#define VEC_SET(v, i) { \
	(v##1) = _mm_set1_epi8(i); \
	(v##2) = _mm_set1_epi8(i); \
}

#define VEC_SETZERO(v) { \
	(v##1) = _mm_setzero_si128(); \
	(v##2) = _mm_setzero_si128(); \
}

#define VEC_SETONES(v) { \
	(v##1) = _mm_set1_epi8(0xff); \
	(v##2) = _mm_set1_epi8(0xff); \
}

/**
 * substitution to char vectors
 */
#define VEC_CHAR_SETZERO(v) { \
	VEC_SETZERO(v); \
}

#define VEC_CHAR_SETONES(v) { \
	VEC_SETONES(v); \
}

/**
 * special substitution macros
 */
#define VEC_SET_LHALF(v, i) { \
	(v##1) = _mm_set1_epi8(i); \
	(v##2) = _mm_setzero_si128(); \
}

#define VEC_SET_UHALF(v, i) { \
	(v##1) = _mm_setzero_si128(); \
	(v##2) = _mm_set1_epi8(i); \
}

#define VEC_SETF_MSB(v) { \
	VEC_SETZERO(v); VEC_INSERT_MSB(v, 0xf0); \
}

#define VEC_SETF_LSB(v) { \
	VEC_SETZERO(v); VEC_INSERT_LSB(v, 0x0f); \
}

/**
 * insertion and extraction macros
 */
#define VEC_INSERT_MSB(v, a) { \
	(v##2) = _mm_insert_epi8((v##2), (a), sizeof(__m128i)-1); \
}

#define VEC_INSERT_LSB(v, a) { \
	(v##1) = _mm_insert_epi8((v##1), (a), 0); \
}

#define VEC_MSB(v)		( (signed char)_mm_extract_epi8((v##2), sizeof(__m128i)-1) )
#define VEC_LSB(v)		( (signed char)_mm_extract_epi8((v##1), 0) )
#define VEC_CENTER(v)	( (signed char)_mm_extract_epi8((v##2), 0) )

/**
 * arithmetic and logic operations
 */
#define VEC_OR(a, b, c) { \
	(a##1) = _mm_or_si128((b##1), (c##1)); \
	(a##2) = _mm_or_si128((b##2), (c##2)); \
}

#define VEC_ADD(a, b, c) { \
	(a##1) = _mm_adds_epi8((b##1), (c##1)); \
	(a##2) = _mm_adds_epi8((b##2), (c##2)); \
}

#define VEC_ADDS(a, b, c) { \
	(a##1) = _mm_adds_epu8((b##1), (c##1)); \
	(a##2) = _mm_adds_epu8((b##2), (c##2)); \
}

#define VEC_SUB(a, b, c) { \
	(a##1) = _mm_subs_epi8((b##1), (c##1)); \
	(a##2) = _mm_subs_epi8((b##2), (c##2)); \
}

#define VEC_SUBS(a, b, c) { \
	(a##1) = _mm_subs_epu8((b##1), (c##1)); \
	(a##2) = _mm_subs_epu8((b##2), (c##2)); \
}

#define VEC_MAX(a, b, c) { \
	(a##1) = _mm_max_epi8((b##1), (c##1)); \
	(a##2) = _mm_max_epi8((b##2), (c##2)); \
}

#define VEC_MIN(a, b, c) { \
	(a##1) = _mm_min_epi8((b##1), (c##1)); \
	(a##2) = _mm_min_epi8((b##2), (c##2)); \
}

/**
 * shift operations
 */
#define VEC_SHIFT_R(a, b) { \
	(a##1) = _mm_or_si128( \
		_mm_srli_si128((b##1), 1), \
		_mm_slli_si128((b##2), sizeof(__m128i)-1)); \
	(a##2) = _mm_srli_si128((b##2), 1); \
}

#define VEC_SHIFT_L(a, b) { \
	(a##2) = _mm_or_si128( \
		_mm_slli_si128((b##2), 1), \
		_mm_srli_si128((b##1), sizeof(__m128i)-1)); \
	(a##1) = _mm_slli_si128((b##1), 1); \
}

/**
 * compare and select
 */
#define VEC_COMPARE(a, b, c) { \
	(a##1) = _mm_cmpeq_epi8((b##1), (c##1)); \
	(a##2) = _mm_cmpeq_epi8((b##2), (c##2)); \
}

#define VEC_SELECT(a, b, c, d) { \
	(a##1) = _mm_blendv_epi8((b##1), (c##1), (d##1)); \
	(a##2) = _mm_blendv_epi8((b##2), (c##2), (d##2)); \
}

/**
 * load and store operations
 */
#define VEC_STORE(p, v) { \
	_mm_store_si128((__m128i *)(p), v##1); p += sizeof(__m128i); \
	_mm_store_si128((__m128i *)(p), v##2); p += sizeof(__m128i); \
}

#define VEC_STORE_PACKED(p, dv, dh) { \
	_mm_store_si128( \
		(__m128i *)(p), \
		_mm_or_si128(_mm_slli_epi64((dh##1), 4), (dv##1))); \
	p += sizeof(__m128i); \
	_mm_store_si128( \
		(__m128i *)(p), \
		_mm_or_si128(_mm_slli_epi64((dh##2), 4), (dv##2))); \
	p += sizeof(__m128i); \
}

#define VEC_STORE_DVDH(p, dv, dh) { \
	VEC_STORE_PACKED(p, dv, dh); \
}

#define VEC_LOAD(p, v) { \
	(v##1) = _mm_load_si128((__m128i *)(p)); p += sizeof(__m128i); \
	(v##2) = _mm_load_si128((__m128i *)(p)); p += sizeof(__m128i); \
}

#define VEC_LOAD_PACKED(p, dv, dh) { \
	__m128i const mask = _mm_set1_epi8(0x0f); \
	(dv##1) = _mm_load_si128((__m128i *)(p)); \
	(dh##1) = _mm_and_si128(_mm_srli_epi64((dv##1), 4), mask); \
	(dv##1) = _mm_and_si128((dv##1), mask); \
	p += sizeof(__m128i); \
	(dv##2) = _mm_load_si128((__m128i *)(p)); \
	(dh##2) = _mm_and_si128(_mm_srli_epi64((dv##2), 4), mask); \
	(dv##2) = _mm_and_si128((dv##2), mask); \
	p += sizeof(__m128i); \
}

#define VEC_LOAD_DVDH(p, dv, dh) { \
	VEC_LOAD_PACKED(p, dv, dh); \
}

/**
 * store vector to int32_t array
 */
#define VEC_STORE32(p, v) { \
	__m128i t = v##1; \
	VEC_STORE(p, _mm_cvtepi8_epi32(t)); \
	t = _mm_slli_si128(t, 4); \
	VEC_STORE(p, _mm_cvtepi8_epi32(t)); \
	t = _mm_slli_si128(t, 4); \
	VEC_STORE(p, _mm_cvtepi8_epi32(t)); \
	t = _mm_slli_si128(t, 4); \
	VEC_STORE(p, _mm_cvtepi8_epi32(t)); \
	t = v##2; \
	VEC_STORE(p, _mm_cvtepi8_epi32(t)); \
	t = _mm_slli_si128(t, 4); \
	VEC_STORE(p, _mm_cvtepi8_epi32(t)); \
	t = _mm_slli_si128(t, 4); \
	VEC_STORE(p, _mm_cvtepi8_epi32(t)); \
	t = _mm_slli_si128(t, 4); \
	VEC_STORE(p, _mm_cvtepi8_epi32(t)); \
}

/**
 * load vector from int32_t array
 */
#define VEC_LOAD32(p, v) { \
	__m128i t1, t2, t3, t4; \
	VEC_LOAD(p, t1); \
	VEC_LOAD(p, t2); \
	VEC_LOAD(p, t3); \
	VEC_LOAD(p, t4); \
	v##1 = _mm_packs_epi32( \
		_mm_packs_epi16(t1, t2), \
		_mm_packs_epi16(t3, t4)); \
	VEC_LOAD(p, t1); \
	VEC_LOAD(p, t2); \
	VEC_LOAD(p, t3); \
	VEC_LOAD(p, t4); \
	v##2 = _mm_packs_epi32( \
		_mm_packs_epi16(t1, t2), \
		_mm_packs_epi16(t3, t4)); \
}

#define DH(c, g)					( (*((pack_t *)(c))>>4) + g )
#define DV(c, g)					( (*((pack_t *)(c)) & 0x0f) + g )
#define DE(c, g)					( (*(((pack_t *)(c) + BW))>>4) + g )
#define DF(c, g)					( (*(((pack_t *)(c) + BW)) & 0x0f) + g )

/*
#define VEC_STORE_DVDH(p, dv, dh) { \
	VEC_STORE(p, dv); \
	VEC_STORE(p, dh); \
}

#define DH(c, g)					( (*((cell_t *)(c) + BW)) + g )
#define DV(c, g)					( (*(cell_t *)(c)) + g )
#define DE(c, g)					( (*((cell_t *)(c) + 3*BW)) + g )
#define DF(c, g)					( (*((cell_t *)(c) + 2*BW)) + g )
*/

/**
 * print vector
 */
#define VEC_PRINT(s, v) { \
	int8_t b[32]; \
	void *p = (void *)b; \
	VEC_STORE(p, v); \
	fprintf(s, \
		"[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d," \
		"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]\n", \
		b[31], b[30], b[29], b[28], b[27], b[26], b[25], b[24], \
		b[23], b[22], b[21], b[20], b[19], b[18], b[17], b[16], \
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

#endif /* #ifndef _SSE_B8_R2_H_INCLUDED */
/**
 * end of sse_b8_r2.h
 */
