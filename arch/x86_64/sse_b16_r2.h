
/**
 * @file sse_b16_r2.h
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
#ifndef _SSE_B16_R2_H_INCLUDED
#define _SSE_B16_R2_H_INCLUDED

#include <smmintrin.h>

/**
 * register declarations. 
 */
#define _vec_cell(v)			__m128i v##1, v##2
#define _vec_cell_reg(v)		__m128i register v##1, v##2
#define _vec_char_reg(v)		__m128i register v##1

/**
 * substitution to cell vectors
 */
#define vec_assign(a, b) { \
	(a##1) = (b##1); (a##2) = (b##2); \
}

#define vec_set(v, i) { \
	(v##1) = _mm_set1_epi16(i); \
	(v##2) = _mm_set1_epi16(i); \
}

#define vec_setzero(v) { \
	(v##1) = _mm_setzero_si128(); \
	(v##2) = _mm_setzero_si128(); \
}

#define vec_setones(v) { \
	(v##1) = _mm_set1_epi8(0xff); \
	(v##2) = _mm_set1_epi8(0xff); \
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
	(v##1) = _mm_set1_epi16(i); \
	(v##2) = _mm_setzero_si128(); \
}

#define vec_set_uhalf(v, i) { \
	(v##1) = _mm_setzero_si128(); \
	(v##2) = _mm_set1_epi16(i); \
}

#define vec_setf_msb(v) { \
	vec_setzero(v); vec_insert_msb(v, 0xf0); \
}

#define vec_setf_lsb(v) { \
	vec_setzero(v); vec_insert_lsb(v, 0x0f); \
}

/**
 * insertion and extraction macros
 */
#define vec_insert_msb(v, a) { \
	(v##2) = _mm_insert_epi16((v##2), (a), sizeof(__m128i)/sizeof(short)-1); \
}

#define vec_insert_lsb(v, a) { \
	(v##1) = _mm_insert_epi16((v##1), (a), 0); \
}

#define vec_msb(v)		( (signed short)_mm_extract_epi16((v##2), sizeof(__m128i)/sizeof(short)-1) )
#define vec_lsb(v)		( (signed short)_mm_extract_epi16((v##1), 0) )
#define vec_center(v)	( (signed short)_mm_extract_epi16((v##2), 0) )

/**
 * arithmetic and logic operations
 */
#define vec_or(a, b, c) { \
	(a##1) = _mm_or_si128((b##1), (c##1)); \
	(a##2) = _mm_or_si128((b##2), (c##2)); \
}

#define vec_add(a, b, c) { \
	(a##1) = _mm_adds_epi16((b##1), (c##1)); \
	(a##2) = _mm_adds_epi16((b##2), (c##2)); \
}

#define vec_adds(a, b, c) { \
	(a##1) = _mm_adds_epu16((b##1), (c##1)); \
	(a##2) = _mm_adds_epu16((b##2), (c##2)); \
}

#define vec_sub(a, b, c) { \
	(a##1) = _mm_subs_epi16((b##1), (c##1)); \
	(a##2) = _mm_subs_epi16((b##2), (c##2)); \
}

#define vec_subs(a, b, c) { \
	(a##1) = _mm_subs_epu16((b##1), (c##1)); \
	(a##2) = _mm_subs_epu16((b##2), (c##2)); \
}

#define vec_max(a, b, c) { \
	(a##1) = _mm_max_epi16((b##1), (c##1)); \
	(a##2) = _mm_max_epi16((b##2), (c##2)); \
}

#define vec_min(a, b, c) { \
	(a##1) = _mm_min_epi16((b##1), (c##1)); \
	(a##2) = _mm_min_epi16((b##2), (c##2)); \
}

/**
 * shift operations
 */
#define vec_shift_r(a) { \
	(a##1) = _mm_or_si128( \
		_mm_srli_si128((a##1), sizeof(short)), \
		_mm_slli_si128((a##2), sizeof(__m128i)-sizeof(short))); \
	(a##2) = _mm_srli_si128((a##2), sizeof(short)); \
}

#define vec_shift_l(a) { \
	(a##2) = _mm_or_si128( \
		_mm_slli_si128((a##2), sizeof(short)), \
		_mm_srli_si128((a##1), sizeof(__m128i)-sizeof(short))); \
	(a##1) = _mm_slli_si128((a##1), sizeof(short)); \
}

/**
 * compare and select
 */
#define vec_comp(a, b, c) { \
	(a##2) = _mm_cmpeq_epi8((b##1), (c##1)); \
	(a##1) = _mm_cvtepi8_epi16((a##2)); \
	(a##2) = _mm_srli_si128((a##2), 8); \
	(a##2) = _mm_cvtepi8_epi16((a##2)); \
}

#define vec_select(a, b, c, d) { \
	(a##1) = _mm_blendv_epi8((b##1), (c##1), (d##1)); \
	(a##2) = _mm_blendv_epi8((b##2), (c##2), (d##2)); \
}


/**
 * load and store operations
 */
#define vec_store(p, v) { \
	_mm_store_si128((__m128i *)(p), v##1); p += sizeof(__m128i); \
	_mm_store_si128((__m128i *)(p), v##2); p += sizeof(__m128i); \
}

/**
 * char vector operations
 */
#define vec_char_shift_r(a) { \
	(a##1) = _mm_srli_si128((a##1), 1); \
}

#define vec_char_shift_l(a) { \
	(a##1) = _mm_slli_si128((a##1), 1); \
}

#define vec_char_insert_msb(x, y) { \
	(x##1) = _mm_insert_epi8((x##1), (y), sizeof(__m128i)-sizeof(char)); \
}

#define vec_char_insert_lsb(x, y) { \
	(x##1) = _mm_insert_epi8((x##1), (y), 0); \
}

#endif /* #infdef _SSE_B16_R2_H_INCLUDED */
/**
 * end of sse_b16_r2.h
 */