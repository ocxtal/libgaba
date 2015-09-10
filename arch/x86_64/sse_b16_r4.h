
/**
 * @file sse_b16_r4.h
 *
 * @brief a header for macros of packed 16-bit SSE4 instructions.
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
#ifndef _SSE_B16_R4_H_INCLUDED
#define _SSE_B16_R4_H_INCLUDED

#ifndef _SIMD_INCLUDED
#define _SIMD_INCLUDED

#include <smmintrin.h>
#include <stdint.h>

/**
 * register declarations. 
 */
#define vec_size()			( 4 * sizeof(__m128i) )

#define _vec_cell(v) \
	__m128i v##1, v##2, v##3, v##4;

#define _vec_cell_const(v, k) \
	__m128i const v##1 = _mm_set1_epi16(k); \
	__m128i const v##2 = _mm_set1_epi16(k); \
	__m128i const v##3 = _mm_set1_epi16(k); \
	__m128i const v##4 = _mm_set1_epi16(k);

#define _vec_cell_reg(v) \
	__m128i register v##1, v##2, v##3, v##4;

#define _vec_single_const(v, k) \
	__m128i const v = _mm_set1_epi8(k);

#define _vec_char_reg(v) \
	__m128i register v##1, v##2;

/**
 * substitution to cell vectors
 */
#define vec_assign(a, b) { \
	(a##1) = (b##1); (a##2) = (b##2); \
	(a##3) = (b##3); (a##4) = (b##4); \
}

#define vec_set(v, i) { \
	(v##1) = _mm_set1_epi16(i); \
	(v##2) = _mm_set1_epi16(i); \
	(v##3) = _mm_set1_epi16(i); \
	(v##4) = _mm_set1_epi16(i); \
}

#define vec_setzero(v) { \
	(v##1) = _mm_setzero_si128(); \
	(v##2) = _mm_setzero_si128(); \
	(v##3) = _mm_setzero_si128(); \
	(v##4) = _mm_setzero_si128(); \
}

#define vec_setones(v) { \
	(v##1) = _mm_set1_epi8(0xff); \
	(v##2) = _mm_set1_epi8(0xff); \
	(v##3) = _mm_set1_epi8(0xff); \
	(v##4) = _mm_set1_epi8(0xff); \
}

/**
 * substitution to char vectors
 */
#define vec_char_setzero(v) { \
	(v##1) = _mm_setzero_si128(); \
	(v##2) = _mm_setzero_si128(); \
}

#define vec_char_setones(v) { \
	(v##1) = _mm_set1_epi8(0xff); \
	(v##2) = _mm_set1_epi8(0xff); \
}

/**
 * special substitution macros
 */
#define vec_set_lhalf(v, i) { \
	(v##1) = _mm_set1_epi16(i); \
	(v##2) = _mm_set1_epi16(i); \
	(v##3) = _mm_setzero_si128(); \
	(v##4) = _mm_setzero_si128(); \
}

#define vec_set_uhalf(v, i) { \
	(v##1) = _mm_setzero_si128(); \
	(v##2) = _mm_setzero_si128(); \
	(v##3) = _mm_set1_epi16(i); \
	(v##4) = _mm_set1_epi16(i); \
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
	(v##4) = _mm_insert_epi16( \
		(v##4), (a), sizeof(__m128i)/sizeof(short)-1); \
}

#define vec_insert_lsb(v, a) { \
	(v##1) = _mm_insert_epi16((v##1), (a), 0); \
}

#define vec_msb(v)		( (signed short)_mm_extract_epi16((v##4), sizeof(__m128i)/sizeof(short)-1) )
#define vec_lsb(v)		( (signed short)_mm_extract_epi16((v##1), 0) )
#define vec_center(v) 	( (signed short)_mm_extract_epi16((v##3), 0) )

/**
 * arithmetic and logic operations
 */
#define vec_or(a, b, c) { \
	(a##1) = _mm_or_si128((b##1), (c##1)); \
	(a##2) = _mm_or_si128((b##2), (c##2)); \
	(a##3) = _mm_or_si128((b##3), (c##3)); \
	(a##4) = _mm_or_si128((b##4), (c##4)); \
}

#define vec_add(a, b, c) { \
	(a##1) = _mm_adds_epi16((b##1), (c##1)); \
	(a##2) = _mm_adds_epi16((b##2), (c##2)); \
	(a##3) = _mm_adds_epi16((b##3), (c##3)); \
	(a##4) = _mm_adds_epi16((b##4), (c##4)); \
}

#define vec_adds(a, b, c) { \
	(a##1) = _mm_adds_epu16((b##1), (c##1)); \
	(a##2) = _mm_adds_epu16((b##2), (c##2)); \
	(a##3) = _mm_adds_epu16((b##3), (c##3)); \
	(a##4) = _mm_adds_epu16((b##4), (c##4)); \
}

#define vec_sub(a, b, c) { \
	(a##1) = _mm_subs_epi16((b##1), (c##1)); \
	(a##2) = _mm_subs_epi16((b##2), (c##2)); \
	(a##3) = _mm_subs_epi16((b##3), (c##3)); \
	(a##4) = _mm_subs_epi16((b##4), (c##4)); \
}

#define vec_subs(a, b, c) { \
	(a##1) = _mm_subs_epu16((b##1), (c##1)); \
	(a##2) = _mm_subs_epu16((b##2), (c##2)); \
	(a##3) = _mm_subs_epu16((b##3), (c##3)); \
	(a##4) = _mm_subs_epu16((b##4), (c##4)); \
}

#define vec_max(a, b, c) { \
	(a##1) = _mm_max_epi16((b##1), (c##1)); \
	(a##2) = _mm_max_epi16((b##2), (c##2)); \
	(a##3) = _mm_max_epi16((b##3), (c##3)); \
	(a##4) = _mm_max_epi16((b##4), (c##4)); \
}

#define vec_min(a, b, c) { \
	(a##1) = _mm_min_epi16((b##1), (c##1)); \
	(a##2) = _mm_min_epi16((b##2), (c##2)); \
	(a##3) = _mm_min_epi16((b##3), (c##3)); \
	(a##4) = _mm_min_epi16((b##4), (c##4)); \
}

/**
 * shift operations
 */
#define vec_shift_r(a, b) { \
	(a##1) = _mm_or_si128( \
		_mm_srli_si128((b##1), sizeof(short)), \
		_mm_slli_si128((b##2), sizeof(__m128i)-sizeof(short))); \
	(a##2) = _mm_or_si128( \
		_mm_srli_si128((b##2), sizeof(short)), \
		_mm_slli_si128((b##3), sizeof(__m128i)-sizeof(short))); \
	(a##3) = _mm_or_si128( \
		_mm_srli_si128((b##3), sizeof(short)), \
		_mm_slli_si128((b##4), sizeof(__m128i)-sizeof(short))); \
	(a##4) = _mm_srli_si128((b##4), sizeof(short)); \
}

#define vec_shift_l(a, b) { \
	(a##4) = _mm_or_si128( \
		_mm_slli_si128((b##4), sizeof(short)), \
		_mm_srli_si128((b##3), sizeof(__m128i)-sizeof(short))); \
	(a##3) = _mm_or_si128( \
		_mm_slli_si128((b##3), sizeof(short)), \
		_mm_srli_si128((b##2), sizeof(__m128i)-sizeof(short))); \
	(a##2) = _mm_or_si128( \
		_mm_slli_si128((b##2), sizeof(short)), \
		_mm_srli_si128((b##1), sizeof(__m128i)-sizeof(short))); \
	(a##1) = _mm_slli_si128((b##1), sizeof(short)); \
}

#if 0
/**
 * compare and select
 */
#define vec_comp(a, b, c) { \
	(a##2) = _mm_cmpeq_epi8((b##1), (c##1)); \
	(a##1) = _mm_cvtepi8_epi16((a##2)); \
	(a##2) = _mm_srli_si128((a##2), 8); \
	(a##2) = _mm_cvtepi8_epi16((a##2)); \
	(a##4) = _mm_cmpeq_epi8((b##2), (c##2)); \
	(a##3) = _mm_cvtepi8_epi16((a##4)); \
	(a##4) = _mm_srli_si128((a##4), 8); \
	(a##4) = _mm_cvtepi8_epi16((a##4)); \
}

#define vec_select(a, b, c, d) { \
	(a##1) = _mm_blendv_epi8((b##1), (c##1), (d##1)); \
	(a##2) = _mm_blendv_epi8((b##2), (c##2), (d##2)); \
	(a##3) = _mm_blendv_epi8((b##3), (c##3), (d##3)); \
	(a##4) = _mm_blendv_epi8((b##4), (c##4), (d##4)); \
}
#endif

/**
 * @macro vec_comp_sel
 * @brief compare two char vectors q1 and q2, select m (single vector) if match, x (single vector) otherwise
 */
#define vec_comp_sel(a, q1, q2, m, x) { \
	__m128i t1, t2; \
	t1 = _mm_cmpeq_epi8((q1##1), (q2##1)); \
	t2 = _mm_cmpeq_epi8((q1##2), (q2##2)); \
	(a##1) = _mm_blendv_epi8((x), (m), t1); \
	(a##3) = _mm_blendv_epi8((x), (m), t2); \
	(a##2) = _mm_cvtepi8_epi16(_mm_srli_si128((a##1), 8)); \
	(a##4) = _mm_cvtepi8_epi16(_mm_srli_si128((a##3), 8)); \
	(a##1) = _mm_cvtepi8_epi16((a##1)); \
	(a##3) = _mm_cvtepi8_epi16((a##3)); \
}

/**
 * @macro vec_comp_mask
 * @brief compare two vectors a and b, make int mask
 */
#define vec_comp_mask(mask, a, b) { \
	_vec_cell_reg(t); \
	t1 = _mm_cmpeq_epi16((a##1), (b##1)); \
	t2 = _mm_cmpeq_epi16((a##2), (b##2)); \
	t3 = _mm_cmpeq_epi16((a##3), (b##3)); \
	t4 = _mm_cmpeq_epi16((a##4), (b##4)); \
	(mask) = _mm_movemask_epi8(_mm_packs_epi16(t1, t2)) \
		  | (_mm_movemask_epi8(_mm_packs_epi16(t3, t4))<<16); \
}

/**
 * @macro vec_hmax
 * @brief horizontal max
 */
#define vec_hmax(val, v) { \
	__m128i t1, t2; \
	t1 = _mm_max_epi16(v##1, v##2); \
	t2 = _mm_max_epi16(v##3, v##4); \
	t1 = _mm_max_epi16(t1, t2); \
	t1 = _mm_max_epi16(t1, \
		_mm_srli_si128(t1, 2)); \
	t1 = _mm_max_epi16(t1, \
		_mm_srli_si128(t1, 4)); \
	t1 = _mm_max_epi16(t1, \
		_mm_srli_si128(t1, 8)); \
	(val) = _mm_extract_epi16(t1, 0); \
}

#if 0
/**
 * @macro vec_maxpos
 */
#define vec_maxpos(pos, val, v) { \
	int32_t r1, r2, r3, r4; \
	__m128i t1, t2, t3, t4; \
	__m128i const offset = _mm_set1_epi16(0x7fff); \
	/** negate vector */ \
	t1 = _mm_sub_epu16(offset, v##1); \
	t2 = _mm_sub_epu16(offset, v##2); \
	t3 = _mm_sub_epu16(offset, v##3); \
	t4 = _mm_sub_epu16(offset, v##4); \
	/** calculate max position with phminposuw */ \
	r1 = _mm_extract_epi32(_mm_minpos_epu16(t1), 0); \
	r2 = _mm_extract_epi32(_mm_minpos_epu16(t2), 0); \
	r3 = _mm_extract_epi32(_mm_minpos_epu16(t3), 0); \
	r4 = _mm_extract_epi32(_mm_minpos_epu16(t4), 0); \
	/** extract max value and pos */ \
	int32_t val1, val2, val3, val4, pos1, pos3; \
	val1 = 0x7fff - (int32_t)(r1 & 0xffff); \
	val2 = 0x7fff - (int32_t)(r2 & 0xffff); \
	val3 = 0x7fff - (int32_t)(r3 & 0xffff); \
	val4 = 0x7fff - (int32_t)(r4 & 0xffff); \
	pos1 = r1>>16; \
	pos3 = (r3>>16) + 16; \
	if(val2 > val1) { \
		pos1 = (r2>>16) + 8; val1 = val2; \
	} \
	if(val4 > val3) { \
		pos3 = (r4>>16) + 24; val3 = val4; \
	} \
	if(val3 > val1) { \
		pos1 = pos3; val1 = val3; \
	} \
	(pos) = pos1; (val) = val1; \
}
#endif

/**
 * load and store operations
 */
#define vec_store(p, v) { \
	_mm_store_si128((__m128i *)(p),     v##1); \
	_mm_store_si128((__m128i *)(p) + 1, v##2); \
	_mm_store_si128((__m128i *)(p) + 2, v##3); \
	_mm_store_si128((__m128i *)(p) + 3, v##4); \
}

#define vec_load(p, v) { \
	(v##1) = _mm_load_si128((__m128i *)(p)    ); \
	(v##2) = _mm_load_si128((__m128i *)(p) + 1); \
	(v##3) = _mm_load_si128((__m128i *)(p) + 2); \
	(v##4) = _mm_load_si128((__m128i *)(p) + 3); \
}

/**
 * load vector from 16bytes int8_t array
 */
#define vec_load8(p, v) { \
	__m128i l = _mm_load_si128((__m128i *)(p)); \
	__m128i u = _mm_srli_si128(l, 8); \
	(v##1) = _mm_set1_epi16(CELL_MIN); \
	(v##2) = _mm_cvtepi8_epi16(l); \
	(v##3) = _mm_cvtepi8_epi16(u); \
	(v##4) = _mm_set1_epi16(CELL_MIN); \
}

#if 0
/**
 * store vector to int32_t array
 */
#define vec_store32(p, v) { \
	_vec_cell_reg(t); \
	t1 = _mm_cvtepi16_epi32(v##1); \
	t2 = _mm_cvtepi16_epi32(_mm_slli_si128(v##1, 8)); \
	t3 = _mm_cvtepi16_epi32(v##2); \
	t4 = _mm_cvtepi16_epi32(_mm_slli_si128(v##2, 8)); \
	vec_store(p, t); \
	t1 = _mm_cvtepi16_epi32(v##3); \
	t2 = _mm_cvtepi16_epi32(_mm_slli_si128(v##3, 8)); \
	t3 = _mm_cvtepi16_epi32(v##4); \
	t4 = _mm_cvtepi16_epi32(_mm_slli_si128(v##4, 8)); \
	vec_store(p, t); \
}

/**
 * load vector from int32_t array
 */
#define vec_load32(p, v) { \
	_vec_cell_reg(t); \
	vec_load(p, t); \
	v##1 = _mm_packs_epi32(t1, t2); \
	v##2 = _mm_packs_epi32(t3, t4); \
	vec_load(p, t); \
	v##3 = _mm_packs_epi32(t1, t2); \
	v##4 = _mm_packs_epi32(t3, t4); \
}
#endif

/**
 * print vector
 */
#ifdef DEBUG

#define vec_print(v) { \
	int16_t b[32]; \
	void *p = (void *)b; \
	vec_store(p, v); \
	fprintf(stderr, \
		"[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d," \
		"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]\n", \
		b[31], b[30], b[29], b[28], b[27], b[26], b[25], b[24], \
		b[23], b[22], b[21], b[20], b[19], b[18], b[17], b[16], \
		b[15], b[14], b[13], b[12], b[11], b[10], b[9], b[8], \
		b[7], b[6], b[5], b[4], b[3], b[2], b[1], b[0]); \
}

#else

#define vec_print(v)		;

#endif

/**
 * char vector operations
 */
#define vec_char_shift_r(a, b) { \
	(a##1) = _mm_or_si128( \
		_mm_srli_si128((b##1), sizeof(char)), \
		_mm_slli_si128((b##2), sizeof(__m128i)-sizeof(char))); \
	(a##2) = _mm_srli_si128((b##2), sizeof(char)); \
}

#define vec_char_shift_l(a, b) { \
	(a##2) = _mm_or_si128( \
		_mm_slli_si128((b##2), sizeof(char)), \
		_mm_srli_si128((b##1), sizeof(__m128i)-sizeof(char))); \
	(a##1) = _mm_slli_si128((b##1), sizeof(char)); \
}

#define vec_char_insert_msb(x, y) { \
	(x##2) = _mm_insert_epi8( \
		(x##2), (y), sizeof(__m128i)-sizeof(char)); \
}

#define vec_char_insert_lsb(x, y) { \
	(x##1) = _mm_insert_epi8((x##1), (y), 0); \
}

#define vec_char_store(p, v) { \
	_mm_store_si128((__m128i *)(p), v##1); \
	_mm_store_si128((__m128i *)(p), v##2); \
}

#define vec_char_print(s, v) { \
	int8_t b[32]; \
	void *p = (void *)b; \
	vec_char_store(p, v); \
	fprintf(s, \
		"[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d," \
		"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]\n", \
		b[31], b[30], b[29], b[28], b[27], b[26], b[25], b[24], \
		b[23], b[22], b[21], b[20], b[19], b[18], b[17], b[16], \
		b[15], b[14], b[13], b[12], b[11], b[10], b[9], b[8], \
		b[7], b[6], b[5], b[4], b[3], b[2], b[1], b[0]); \
}

#endif /* #ifndef _SIMD_INCLUDED */

#endif /* #ifndef _SSE_B16_R4_H_INCLUDED */
/**
 * end of sse_b16_r4.h
 */
