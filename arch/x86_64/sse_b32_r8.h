
/**
 * @file sse_b32_r4.h
 *
 * @brief a header for macros of packed 32-bit SSE4 instructions.
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
#ifndef _SSE_B32_R4_H_INCLUDED
#define _SSE_B32_R4_H_INCLUDED

#include <smmintrin.h>

/**
 * register declarations. 
 */
#define DECLARE_VEC_CELL(v)			__m128i v##1, v##2, v##3, v##4, v##5, v##6, v##7, v##8
#define DECLARE_VEC_CELL_REG(v)		__m128i v##1, v##2, v##3, v##4, v##5, v##6, v##7, v##8
#define DECLARE_VEC_CHAR_REG(v)		__m128i register v##1, v##2

/**
 * substitution to cell vectors
 */
#define VEC_ASSIGN(a, b) { \
	(a##1) = (b##1); (a##2) = (b##2); \
	(a##3) = (b##3); (a##4) = (b##4); \
	(a##5) = (b##5); (a##6) = (b##6); \
	(a##7) = (b##7); (a##8) = (b##8); \
}

#define VEC_SET(v, i) { \
	(v##1) = _mm_set1_epi32(i); \
	(v##2) = _mm_set1_epi32(i); \
	(v##3) = _mm_set1_epi32(i); \
	(v##4) = _mm_set1_epi32(i); \
	(v##5) = _mm_set1_epi32(i); \
	(v##6) = _mm_set1_epi32(i); \
	(v##7) = _mm_set1_epi32(i); \
	(v##8) = _mm_set1_epi32(i); \
}

#define VEC_SETZERO(v) { \
	(v##1) = _mm_setzero_si128(); \
	(v##2) = _mm_setzero_si128(); \
	(v##3) = _mm_setzero_si128(); \
	(v##4) = _mm_setzero_si128(); \
	(v##5) = _mm_setzero_si128(); \
	(v##6) = _mm_setzero_si128(); \
	(v##7) = _mm_setzero_si128(); \
	(v##8) = _mm_setzero_si128(); \
}

#define VEC_SETONES(v) { \
	(v##1) = _mm_set1_epi8(0xff); \
	(v##2) = _mm_set1_epi8(0xff); \
	(v##3) = _mm_set1_epi8(0xff); \
	(v##4) = _mm_set1_epi8(0xff); \
	(v##5) = _mm_set1_epi8(0xff); \
	(v##6) = _mm_set1_epi8(0xff); \
	(v##7) = _mm_set1_epi8(0xff); \
	(v##8) = _mm_set1_epi8(0xff); \
}

/**
 * substitution to char vectors
 */
#define VEC_CHAR_SETZERO(v) { \
	(v##1) = _mm_setzero_si128(); \
	(v##2) = _mm_setzero_si128(); \
}

#define VEC_CHAR_SETONES(v) { \
	(v##1) = _mm_set1_epi8(0xff); \
	(v##2) = _mm_set1_epi8(0xff); \
}

/**
 * special substitution macros
 */
#define VEC_SET_LHALF(v, i) { \
	(v##1) = _mm_set1_epi32(i); \
	(v##2) = _mm_set1_epi32(i); \
	(v##3) = _mm_set1_epi32(i); \
	(v##4) = _mm_set1_epi32(i); \
	(v##5) = _mm_setzero_si128(); \
	(v##6) = _mm_setzero_si128(); \
	(v##7) = _mm_setzero_si128(); \
	(v##8) = _mm_setzero_si128(); \
}

#define VEC_SET_UHALF(v, i) { \
	(v##1) = _mm_setzero_si128(); \
	(v##2) = _mm_setzero_si128(); \
	(v##3) = _mm_setzero_si128(); \
	(v##4) = _mm_setzero_si128(); \
	(v##5) = _mm_set1_epi32(i); \
	(v##6) = _mm_set1_epi32(i); \
	(v##7) = _mm_set1_epi32(i); \
	(v##8) = _mm_set1_epi32(i); \
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
	(v##8) = _mm_insert_epi32( \
		(v##8), (a), sizeof(__m128i)/sizeof(int)-1); \
}

#define VEC_INSERT_LSB(v, a) { \
	(v##1) = _mm_insert_epi32((v##1), (a), 0); \
}

#define VEC_MSB(v)		( (signed int)_mm_extract_epi32((v##8), sizeof(__m128i)/sizeof(int)-1) )
#define VEC_LSB(v)		( (signed int)_mm_extract_epi32((v##1), 0) )
#define VEC_CENTER(v) 	( (signed int)_mm_extract_epi32((v##5), 0) )

/**
 * arithmetic and logic operations
 */
#define VEC_OR(a, b, c) { \
	(a##1) = _mm_or_si128((b##1), (c##1)); \
	(a##2) = _mm_or_si128((b##2), (c##2)); \
	(a##3) = _mm_or_si128((b##3), (c##3)); \
	(a##4) = _mm_or_si128((b##4), (c##4)); \
	(a##5) = _mm_or_si128((b##5), (c##5)); \
	(a##6) = _mm_or_si128((b##6), (c##6)); \
	(a##7) = _mm_or_si128((b##7), (c##7)); \
	(a##8) = _mm_or_si128((b##8), (c##8)); \
}

#define VEC_ADD(a, b, c) { \
	(a##1) = _mm_add_epi32((b##1), (c##1)); \
	(a##2) = _mm_add_epi32((b##2), (c##2)); \
	(a##3) = _mm_add_epi32((b##3), (c##3)); \
	(a##4) = _mm_add_epi32((b##4), (c##4)); \
	(a##5) = _mm_add_epi32((b##5), (c##5)); \
	(a##6) = _mm_add_epi32((b##6), (c##6)); \
	(a##7) = _mm_add_epi32((b##7), (c##7)); \
	(a##8) = _mm_add_epi32((b##8), (c##8)); \
}

#define VEC_ADDS(a, b, c) { \
	(a##1) = _mm_add_epu32((b##1), (c##1)); \
	(a##2) = _mm_add_epu32((b##2), (c##2)); \
	(a##3) = _mm_add_epu32((b##3), (c##3)); \
	(a##4) = _mm_add_epu32((b##4), (c##4)); \
	(a##5) = _mm_add_epu32((b##5), (c##5)); \
	(a##6) = _mm_add_epu32((b##6), (c##6)); \
	(a##7) = _mm_add_epu32((b##7), (c##7)); \
	(a##8) = _mm_add_epu32((b##8), (c##8)); \
}

#define VEC_SUB(a, b, c) { \
	(a##1) = _mm_sub_epi32((b##1), (c##1)); \
	(a##2) = _mm_sub_epi32((b##2), (c##2)); \
	(a##3) = _mm_sub_epi32((b##3), (c##3)); \
	(a##4) = _mm_sub_epi32((b##4), (c##4)); \
	(a##5) = _mm_sub_epi32((b##5), (c##5)); \
	(a##6) = _mm_sub_epi32((b##6), (c##6)); \
	(a##7) = _mm_sub_epi32((b##7), (c##7)); \
	(a##8) = _mm_sub_epi32((b##8), (c##8)); \
}

#define VEC_SUBS(a, b, c) { \
	(a##1) = _mm_sub_epu32((b##1), (c##1)); \
	(a##2) = _mm_sub_epu32((b##2), (c##2)); \
	(a##3) = _mm_sub_epu32((b##3), (c##3)); \
	(a##4) = _mm_sub_epu32((b##4), (c##4)); \
	(a##5) = _mm_sub_epu32((b##5), (c##5)); \
	(a##6) = _mm_sub_epu32((b##6), (c##6)); \
	(a##7) = _mm_sub_epu32((b##7), (c##7)); \
	(a##8) = _mm_sub_epu32((b##8), (c##8)); \
}

#define VEC_MAX(a, b, c) { \
	(a##1) = _mm_max_epi32((b##1), (c##1)); \
	(a##2) = _mm_max_epi32((b##2), (c##2)); \
	(a##3) = _mm_max_epi32((b##3), (c##3)); \
	(a##4) = _mm_max_epi32((b##4), (c##4)); \
	(a##5) = _mm_max_epi32((b##5), (c##5)); \
	(a##6) = _mm_max_epi32((b##6), (c##6)); \
	(a##7) = _mm_max_epi32((b##7), (c##7)); \
	(a##8) = _mm_max_epi32((b##8), (c##8)); \
}

#define VEC_MIN(a, b, c) { \
	(a##1) = _mm_min_epi32((b##1), (c##1)); \
	(a##2) = _mm_min_epi32((b##2), (c##2)); \
	(a##3) = _mm_min_epi32((b##3), (c##3)); \
	(a##4) = _mm_min_epi32((b##4), (c##4)); \
	(a##5) = _mm_min_epi32((b##5), (c##5)); \
	(a##6) = _mm_min_epi32((b##6), (c##6)); \
	(a##7) = _mm_min_epi32((b##7), (c##7)); \
	(a##8) = _mm_min_epi32((b##8), (c##8)); \
}

/**
 * shift operations
 */
#define VEC_SHIFT_R(a) { \
	(a##1) = _mm_or_si128( \
		_mm_srli_si128((a##1), sizeof(int)), \
		_mm_slli_si128((a##2), sizeof(__m128i)-sizeof(int))); \
	(a##2) = _mm_or_si128( \
		_mm_srli_si128((a##2), sizeof(int)), \
		_mm_slli_si128((a##3), sizeof(__m128i)-sizeof(int))); \
	(a##3) = _mm_or_si128( \
		_mm_srli_si128((a##3), sizeof(int)), \
		_mm_slli_si128((a##4), sizeof(__m128i)-sizeof(int))); \
	(a##4) = _mm_or_si128( \
		_mm_srli_si128((a##4), sizeof(int)), \
		_mm_slli_si128((a##5), sizeof(__m128i)-sizeof(int))); \
	(a##5) = _mm_or_si128( \
		_mm_srli_si128((a##5), sizeof(int)), \
		_mm_slli_si128((a##6), sizeof(__m128i)-sizeof(int))); \
	(a##6) = _mm_or_si128( \
		_mm_srli_si128((a##6), sizeof(int)), \
		_mm_slli_si128((a##7), sizeof(__m128i)-sizeof(int))); \
	(a##7) = _mm_or_si128( \
		_mm_srli_si128((a##7), sizeof(int)), \
		_mm_slli_si128((a##8), sizeof(__m128i)-sizeof(int))); \
	(a##8) = _mm_srli_si128((a##8), sizeof(int)); \
}

#define VEC_SHIFT_L(a) { \
	(a##8) = _mm_or_si128( \
		_mm_slli_si128((a##8), sizeof(int)), \
		_mm_srli_si128((a##7), sizeof(__m128i)-sizeof(int))); \
	(a##7) = _mm_or_si128( \
		_mm_slli_si128((a##7), sizeof(int)), \
		_mm_srli_si128((a##6), sizeof(__m128i)-sizeof(int))); \
	(a##6) = _mm_or_si128( \
		_mm_slli_si128((a##6), sizeof(int)), \
		_mm_srli_si128((a##5), sizeof(__m128i)-sizeof(int))); \
	(a##5) = _mm_or_si128( \
		_mm_slli_si128((a##5), sizeof(int)), \
		_mm_srli_si128((a##4), sizeof(__m128i)-sizeof(int))); \
	(a##4) = _mm_or_si128( \
		_mm_slli_si128((a##4), sizeof(int)), \
		_mm_srli_si128((a##3), sizeof(__m128i)-sizeof(int))); \
	(a##3) = _mm_or_si128( \
		_mm_slli_si128((a##3), sizeof(int)), \
		_mm_srli_si128((a##2), sizeof(__m128i)-sizeof(int))); \
	(a##2) = _mm_or_si128( \
		_mm_slli_si128((a##2), sizeof(int)), \
		_mm_srli_si128((a##1), sizeof(__m128i)-sizeof(int))); \
	(a##1) = _mm_slli_si128((a##1), sizeof(int)); \
}

/**
 * compare and select
 */
#define VEC_COMPARE(a, b, c) { \
	(a##4) = _mm_cmpeq_epi8((b##1), (c##1)); \
	(a##1) = _mm_cvtepi8_epi32((a##4)); \
	(a##4) = _mm_srli_si128((a##4), 4); \
	(a##2) = _mm_cvtepi8_epi32((a##4)); \
	(a##4) = _mm_srli_si128((a##4), 4); \
	(a##3) = _mm_cvtepi8_epi32((a##4)); \
	(a##4) = _mm_srli_si128((a##4), 4); \
	(a##4) = _mm_cvtepi8_epi32((a##4)); \
	(a##8) = _mm_cmpeq_epi8((b##2), (c##2)); \
	(a##5) = _mm_cvtepi8_epi32((a##8)); \
	(a##8) = _mm_srli_si128((a##8), 4); \
	(a##6) = _mm_cvtepi8_epi32((a##8)); \
	(a##8) = _mm_srli_si128((a##8), 4); \
	(a##7) = _mm_cvtepi8_epi32((a##8)); \
	(a##8) = _mm_srli_si128((a##8), 4); \
	(a##8) = _mm_cvtepi8_epi32((a##8)); \
}

#define VEC_SELECT(a, b, c, d) { \
	(a##1) = _mm_blendv_epi8((b##1), (c##1), (d##1)); \
	(a##2) = _mm_blendv_epi8((b##2), (c##2), (d##2)); \
	(a##3) = _mm_blendv_epi8((b##3), (c##3), (d##3)); \
	(a##4) = _mm_blendv_epi8((b##4), (c##4), (d##4)); \
	(a##5) = _mm_blendv_epi8((b##5), (c##5), (d##5)); \
	(a##6) = _mm_blendv_epi8((b##6), (c##6), (d##6)); \
	(a##7) = _mm_blendv_epi8((b##7), (c##7), (d##7)); \
	(a##8) = _mm_blendv_epi8((b##8), (c##8), (d##8)); \
}

/**
 * load and store operations
 */
#define VEC_STORE(p, v) { \
	_mm_store_si128((__m128i *)(p), v##1); p += sizeof(__m128i); \
	_mm_store_si128((__m128i *)(p), v##2); p += sizeof(__m128i); \
	_mm_store_si128((__m128i *)(p), v##3); p += sizeof(__m128i); \
	_mm_store_si128((__m128i *)(p), v##4); p += sizeof(__m128i); \
	_mm_store_si128((__m128i *)(p), v##5); p += sizeof(__m128i); \
	_mm_store_si128((__m128i *)(p), v##6); p += sizeof(__m128i); \
	_mm_store_si128((__m128i *)(p), v##7); p += sizeof(__m128i); \
	_mm_store_si128((__m128i *)(p), v##8); p += sizeof(__m128i); \
}

/**
 * char vector operations
 */
#define VEC_CHAR_SHIFT_R(a) { \
	(a##1) = _mm_or_si128( \
		_mm_srli_si128((a##1), sizeof(char)), \
		_mm_slli_si128((a##2), sizeof(__m128i)-sizeof(char))); \
	(a##2) = _mm_srli_si128((a##2), sizeof(char)); \
}

#define VEC_CHAR_SHIFT_L(a) { \
	(a##2) = _mm_or_si128( \
		_mm_slli_si128((a##2), sizeof(char)), \
		_mm_srli_si128((a##1), sizeof(__m128i)-sizeof(char))); \
	(a##1) = _mm_slli_si128((a##1), sizeof(char)); \
}

#define VEC_CHAR_INSERT_MSB(x, y) { \
	(x##2) = _mm_insert_epi8( \
		(x##2), (y), sizeof(__m128i)-sizeof(char)); \
}

#define VEC_CHAR_INSERT_LSB(x, y) { \
	(x##1) = _mm_insert_epi8((x##1), (y), 0); \
}

#endif /* #ifndef _SSE_B32_R4_H_INCLUDED */
/**
 * end of sse_b32_r4.h
 */
