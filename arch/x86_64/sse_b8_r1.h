
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

#include <smmintrin.h>
#include <stdint.h>

/**
 * register declarations. 
 */
#define b8c16_size()				( sizeof(__m128i) )
#define _b8c16_cell(v)				__m128i v##1
#define _b8c16_cell_const(v, k)		__m128i const v##1 = _mm_set1_epi8(k)
#define _b8c16_cell_reg(v)			__m128i register v##1
#define _b8c16_single_const(v, k)	__m128i const v = _mm_set1_epi8(k)
#define _b8c16_char_reg(v)			__m128i register v##1

/**
 * load and store operations
 */
#define b8c16_store(p, v) { \
	_mm_store_si128((__m128i *)(p), (v##1)); \
}

#define b8c16_store_packed(p, dv, dh) { \
	_mm_store_si128( \
		(__m128i *)(p), \
		_mm_or_si128(_mm_slli_epi64((dh##1), 4), (dv##1))); \
}

#define b8c16_load(p, v) { \
	(v##1) = _mm_load_si128((__m128i *)(p)); \
}

#define b8c16_load_packed(p, dv, dh) { \
	__m128i const mask = _mm_set1_epi8(0x0f); \
	(dv##1) = _mm_load_si128((__m128i *)(p)); \
	(dh##1) = _mm_and_si128(_mm_srli_epi64((dv##1), 4), mask); \
	(dv##1) = _mm_and_si128((dv##1), mask); \
}

#define b8c16_load8(p, v) { \
	(v##1) = _mm_load_si128((__m128i *)(p)); \
}

#define b8c16_char_store(p, v)	b8c16_store(p, v)
#define b8c16_char_load(p, v)	b8c16_load(p, v)

#if 0
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
#endif

#ifndef _SIMD_INCLUDED
#define _SIMD_INCLUDED

#define vec_size					b8c16_size
#define _vec_cell					_b8c16_cell
#define _vec_cell_const				_b8c16_cell_const
#define _vec_cell_reg				_b8c16_cell_reg
#define _vec_single_const			_b8c16_single_const
#define _vec_char_reg				_b8c16_char_reg

/**
 * load and store operations
 */
#define vec_store 					b8c16_store
#define vec_store_packed			b8c16_store_packed
#define vec_load 					b8c16_load
#define vec_load_packed 			b8c16_load_packed
#define vec_load8 					b8c16_load8

/**
 * assign to cell vectors
 */
#define vec_assign(a, b) { \
	(a##1) = (b##1); \
}

#define vec_set(v, i) { \
	(v##1) = _mm_set1_epi8(i); \
}

#define vec_setzero(v) { \
	(v##1) = _mm_setzero_si128(); \
}

#define vec_setones(v) { \
	(v##1) = _mm_set1_epi8(0xff); \
}

/**
 * substitution to char vectors
 */
#define vec_char_setzero(v) { \
	(v##1) = _mm_setzero_si128(); \
}

#define vec_char_setones(v) { \
	(v##1) = _mm_set1_epi8(0xff); \
}

/**
 * special substitution macros
 */
#define vec_set_lhalf(v, i) { \
	vec_set(v, i); (v##1) = _mm_srli_si128((v##1), BAND_WIDTH/2); \
}

#define vec_set_uhalf(v, i) { \
	vec_set(v, i); (v##1) = _mm_slli_si128((v##1), BAND_WIDTH/2); \
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
	(v##1) = _mm_insert_epi8((v##1), (a), sizeof(__m128i)-1); \
}

#define vec_insert_lsb(v, a) { \
	(v##1) = _mm_insert_epi8((v##1), (a), 0); \
}

#define vec_msb(v)		( (signed char)_mm_extract_epi8((v##1), sizeof(__m128i)-1) )
#define vec_lsb(v)		( (signed char)_mm_extract_epi8((v##1), 0) )
#define vec_center(v)	( (signed char)_mm_extract_epi8((v##1), 8) )

/**
 * arithmetic and logic operations
 */
#define vec_or(a, b, c) { \
	(a##1) = _mm_or_si128((b##1), (c##1)); \
}

#define vec_add(a, b, c) { \
	(a##1) = _mm_adds_epi8((b##1), (c##1)); \
}

#define vec_adds(a, b, c) { \
	(a##1) = _mm_adds_epu8((b##1), (c##1)); \
}

#define vec_sub(a, b, c) { \
	(a##1) = _mm_subs_epi8((b##1), (c##1)); \
}

#define vec_subs(a, b, c) { \
	(a##1) = _mm_subs_epu8((b##1), (c##1)); \
}

#define vec_max(a, b, c) { \
	(a##1) = _mm_max_epi8((b##1), (c##1)); \
}

#define vec_min(a, b, c) { \
	(a##1) = _mm_min_epi8((b##1), (c##1)); \
}

/**
 * shift operations
 */
#define vec_shift_r(a, b) { \
	(a##1) = _mm_srli_si128((b##1), 1); \
}

#define vec_shift_l(a, b) { \
	(a##1) = _mm_slli_si128((b##1), 1); \
}

#if 0
/**
 * compare and select
 */
#define vec_comp(a, b, c) { \
	(a##1) = _mm_cmpeq_epi8((b##1), (c##1)); \
}
#endif

/**
 * @macro vec_comp_sel
 * @brief compare two char vectors q1 and q2, select m if match, x otherwise
 */
#define vec_comp_sel(a, q1, q2, m, x) { \
	(a##1) = _mm_blendv_epi8((x##1), (m##1), _mm_cmpeq_epi8((q1##1), (q2##1))); \
}

/**
 * @macro vec_sel
 */
#define vec_sel(a, mask, m, x) { \
	(a##1) = _mm_blendv_epi8((x), (m), (mask##1)); \
}

/**
 * @macro vec_comp_mask
 * @brief compare two vectors a and b, make int mask
 */
#define vec_comp_mask(mask, a, b) { \
	__m128i t = _mm_cmpeq_epi8((a##1), (b##1)); \
	(mask) = _mm_movemask_epi8(t); \
}

/**
 * @macro vec_hmax
 * @brief horizontal max
 */
#define vec_hmax(val, v) { \
	__m128i t; \
	t = _mm_max_epi8((v##1), \
		_mm_srli_si128((v##1), 1)); \
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
 * print vector
 */
#ifdef DEBUG

#define vec_print(v) { \
	int8_t b[16]; \
	void *p = (void *)b; \
	vec_store(p, v); \
	fprintf(stderr, \
/*		"[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]\n",*/ \
		"[%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x]\n", \
		(uint8_t)b[15], (uint8_t)b[14], (uint8_t)b[13], (uint8_t)b[12], (uint8_t)b[11], (uint8_t)b[10], (uint8_t)b[9], (uint8_t)b[8], \
		(uint8_t)b[7], (uint8_t)b[6], (uint8_t)b[5], (uint8_t)b[4], (uint8_t)b[3], (uint8_t)b[2], (uint8_t)b[1], (uint8_t)b[0]); \
}

#else

#define vec_print(v)		;

#endif

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

#define vec_char_print(v) { \
	vec_print(v); \
}

#endif /* #ifndef _SIMD_INCLUDED */

#endif /* #ifndef _SSE_B8_R1_H_INCLUDED */
/**
 * end of sse_b8_r1.h
 */
