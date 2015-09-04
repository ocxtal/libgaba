
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
#define _vec_single_const(v, k)		__m128i const v = _mm_set1_epi8(k)
#define _vec_char_reg(v)			__m128i register v

/**
 * substitution to cell vectors
 */
#define vec_assign(a, b) { \
	(a) = (b); \
}

#define vec_set(v, i) { \
	(v) = _mm_set1_epi8(i); \
}

#define vec_setzero(v) { \
	(v) = _mm_setzero_si128(); \
}

#define vec_setones(v) { \
	(v) = _mm_set1_epi8(0xff); \
}

/**
 * substitution to char vectors
 */
#define vec_char_setzero(v) { \
	(v) = _mm_setzero_si128(); \
}

#define vec_char_setones(v) { \
	(v) = _mm_set1_epi8(0xff); \
}

/**
 * special substitution macros
 */
#define vec_set_lhalf(v, i) { \
	vec_set(v, i); (v) = _mm_srli_si128((v), BAND_WIDTH/2); \
}

#define vec_set_uhalf(v, i) { \
	vec_set(v, i); (v) = _mm_slli_si128((v), BAND_WIDTH/2); \
}

#define vec_setf_msb(v) { \
	vec_setzero(v); vec_insert_msb((v), 0xf0); \
}

#define vec_setf_lsb(v) { \
	vec_setzero(v); vec_insert_lsb((v), 0x0f); \
}

/**
 * insertion and extraction macros
 */
#define vec_insert_msb(v, a) { \
	(v) = _mm_insert_epi8((v), (a), sizeof(__m128i)-1); \
}

#define vec_insert_lsb(v, a) { \
	(v) = _mm_insert_epi8((v), (a), 0); \
}

#define vec_msb(v)		( (signed char)_mm_extract_epi8((v), sizeof(__m128i)-1) )
#define vec_lsb(v)		( (signed char)_mm_extract_epi8((v), 0) )
#define vec_center(v)	( (signed char)_mm_extract_epi8((v), 8) )

/**
 * arithmetic and logic operations
 */
#define vec_or(a, b, c) { \
	(a) = _mm_or_si128((b), (c)); \
}

#define vec_add(a, b, c) { \
	(a) = _mm_adds_epi8((b), (c)); \
}

#define vec_adds(a, b, c) { \
	(a) = _mm_adds_epu8((b), (c)); \
}

#define vec_sub(a, b, c) { \
	(a) = _mm_subs_epi8((b), (c)); \
}

#define vec_subs(a, b, c) { \
	(a) = _mm_subs_epu8((b), (c)); \
}

#define vec_max(a, b, c) { \
	(a) = _mm_max_epi8((b), (c)); \
}

#define vec_min(a, b, c) { \
	(a) = _mm_min_epi8((b), (c)); \
}

/**
 * shift operations
 */
#define vec_shift_r(a, b) { \
	(a) = _mm_srli_si128((b), 1); \
}

#define vec_shift_l(a, b) { \
	(a) = _mm_slli_si128((b), 1); \
}

#if 0
/**
 * compare and select
 */
#define vec_comp(a, b, c) { \
	(a) = _mm_cmpeq_epi8((b), (c)); \
}

#define vec_select(a, b, c, d) { \
	(a) = _mm_blendv_epi8((b), (c), (d)); \
}
#endif

/**
 * @macro vec_comp_sel
 * @brief compare two char vectors q1 and q2, select m if match, x otherwise
 */
#define vec_comp_sel(a, q1, q2, m, x) { \
	(a) = _mm_blendv_epi8((x), (m), _mm_cmpeq_epi8((q1), (q2))); \
}

/**
 * @macro vec_hmax
 * @brief horizontal max
 */
#define vec_hmax(val, v) { \
	__m128i t; \
	t = _mm_max_epi8(v, \
		_mm_srli_si128(v, 1)); \
	t = _mm_max_epi8(t, \
		_mm_srli_si128(t, 2)); \
	t = _mm_max_epi8(t, \
		_mm_srli_si128(t, 4)); \
	t = _mm_max_epi8(t, \
		_mm_srli_si128(t, 8)); \
	(val) = _mm_extract_epi8(t, 0); \
}

#if 0
/**
 * @macro vec_maxpos
 */
#define vec_maxpos(pos, val, v) { \
	int32_t r1, r2; \
	__m128i t1, t2; \
	__m128i const offset = _mm_set1_epi16(0x7fff); \
	/** expand vector */ \
	t1 = _mm_cvtepi8_epi16(v); \
	t2 = _mm_cvtepi8_epi16(_mm_srli_si128(v, 8)); \
	/** negate vector */ \
	t1 = _mm_sub_epu16(offset, t1); \
	t2 = _mm_sub_epu16(offset, t2); \
	/** calculate max position with phminposuw */ \
	r1 = _mm_extract_epi32(_mm_minpos_epu16(t1), 0); \
	r2 = _mm_extract_epi32(_mm_minpos_epu16(t2), 0); \
	/** extract max value and pos */ \
	int32_t val1, val2; \
	val1 = 0x7fff - (int32_t)(r1 & 0xffff); \
	val2 = 0x7fff - (int32_t)(r2 & 0xffff); \
	if(val2 > val1) { \
		(pos) = (r2>>16) + 8; (val) = val2; \
	} else { \
		(pos) = r1>>16; (val) = val1; \
	} \
}
#endif

/**
 * load and store operations
 */
#define vec_store(p, v) { \
	_mm_store_si128((__m128i *)(p), v); p += sizeof(__m128i); \
}

#define vec_store_packed(p, dv, dh) { \
	_mm_store_si128( \
		(__m128i *)(p), \
		_mm_or_si128(_mm_slli_epi64(dh, 4), dv)); \
	p += sizeof(__m128i); \
}

#define vec_load(p, v) { \
	v = _mm_load_si128((__m128i *)(p)); p += sizeof(__m128i); \
}

#define vec_load_packed(p, dv, dh) { \
	__m128i const mask = _mm_set1_epi8(0x0f); \
	dv = _mm_load_si128((__m128i *)(p)); \
	dh = _mm_and_si128(_mm_srli_epi64(dv, 4), mask); \
	dv = _mm_and_si128(dv, mask); \
	p += sizeof(__m128i); \
}

/**
 * store vector to 32-elem array of int32_t
 */
#define vec_store32(p, v) { \
	__m128i t = v; \
 	__m128i const m = _mm_set1_epi32(CELL_MIN); \
 	vec_store(p, m); vec_store(p, m);	/** margin */ \
	vec_store(p, _mm_cvtepi8_epi32(t)); \
	t = _mm_slli_si128(t, 4); \
	vec_store(p, _mm_cvtepi8_epi32(t)); \
	t = _mm_slli_si128(t, 4); \
	vec_store(p, _mm_cvtepi8_epi32(t)); \
	t = _mm_slli_si128(t, 4); \
	vec_store(p, _mm_cvtepi8_epi32(t)); \
 	vec_store(p, m); vec_store(p, m);	/** margin */ \
}

/**
 * load vector from 32-elem array of int32_t
 */
#define vec_load32(p, v) { \
	__m128i t1, t2, t3, t4; \
	p += 2*sizeof(__m128i);	/** skip margin */ \
	vec_load(p, t1); \
	vec_load(p, t2); \
	vec_load(p, t3); \
	vec_load(p, t4); \
	p += 2*sizeof(__m128i);	/** skip margin */ \
	v = _mm_packs_epi32( \
		_mm_packs_epi16(t1, t2), \
		_mm_packs_epi16(t3, t4)); \
}

/**
 * print vector
 */
#define vec_print(s, v) { \
	int8_t b[16]; \
	void *p = (void *)b; \
	vec_store(p, v); \
	fprintf(s, \
		"[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]\n", \
		b[15], b[14], b[13], b[12], b[11], b[10], b[9], b[8], \
		b[7], b[6], b[5], b[4], b[3], b[2], b[1], b[0]); \
}

/**
 * char vector operations
 */
#define vec_char_shift_r(a, b) { \
	vec_shift_r(a, b); \
}

#define vec_char_shift_l(a, b) { \
	vec_shift_l(a, b); \
}

#define vec_char_insert_msb(x, y) { \
	vec_insert_msb(x, y); \
}

#define vec_char_insert_lsb(x, y) { \
	vec_insert_lsb(x, y); \
}

#define vec_char_print(s, v) { \
	vec_print(s, v); \
}

#endif /* #ifndef _SIMD_INCLUDED */

#endif /* #ifndef _SSE_B8_R1_H_INCLUDED */
/**
 * end of sse_b8_r1.h
 */
