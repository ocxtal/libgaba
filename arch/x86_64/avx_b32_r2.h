
/**
 * @file avx_b32_r2.h
 *
 * @brief a header for macros of packed 32-bit AVX2 instructions.
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
#ifndef _AVX_B32_R2_H_INCLUDED
#define _AVX_B32_R2_H_INCLUDED

#include <immintrin.h>

/**
 * register declarations. 
 */
#define DECLARE_VEC_CELL(v)			__m256i v##1, v##2
#define DECLARE_VEC_CELL_REG(v)		__m256i register v##1, v##2
#define DECLARE_VEC_CHAR_REG(v)		__m128i register v##1

/**
 * substitution to cell vectors
 */
#define VEC_ASSIGN(a, b) { \
 	(a##1) = (b##1); (a##2) = (b##2); \
}

#define VEC_SET(v, i) { \
	(v##1) = _mm256_set1_epi32(i); \
	(v##2) = _mm256_set1_epi32(i); \
}

#define VEC_SETZERO(v) { \
	(v##1) = _mm256_setzero_si256(); \
	(v##2) = _mm256_setzero_si256(); \
}

#define VEC_SETONES(v) { \
	(v##1) = _mm256_set1_epi8(0xff); \
	(v##2) = _mm256_set1_epi8(0xff); \
}

/**
 * substitution to char vectors
 */
#define VEC_CHAR_SETZERO(v) { \
 	(v##1) = _mm_setzero_si128(); \
}

#define VEC_CHAR_SETONES(v) { \
	(v##1) = _mm_set1_epi8(0xff); \
}

/**
 * special substitution macros
 */
#define VEC_SET_LHALF(v, i) { \
 	(v##1) = _mm256_set1_epi32(i); \
 	(v##2) = _mm256_setzero_si256(); \
 }

#define VEC_SET_UHALF(v, i) { \
	(v##1) = _mm256_setzero_si256(); \
	(v##2) = _mm256_set1_epi32(i); \
}

#define VEC_SETF_MSB(v)	 { \
	VEC_SETZERO(v); VEC_INSERT_MSB(v, 0xf0); \
}

#define VEC_SETF_LSB(v)	 { \
	VEC_SETZERO(v); VEC_INSERT_LSB(v, 0x0f); \
}

/**
 * insertion and extraction macros
 */
#define VEC_INSERT_MSB(v, a) { \
	(v##2) = _mm256_inserti128_si256( \
		(v##2), \
		_mm_insert_epi32( \
			_mm256_extracti128_si256((v##2), 1), (a), sizeof(__m128i)/sizeof(int)-1), \
		1); \
}

#define VEC_INSERT_LSB(v, a) { \
	(v##1) = _mm256_inserti128_si256( \
		(v##1), \
		_mm_insert_epi32( \
			_mm256_extracti128_si256((v##1), 0), (a), 0), \
		0); \
}

#define VEC_MSB(v)		( (signed int)_mm_extract_epi32(_mm256_extracti128_si256((v##2), 1), sizeof(__m128i)/sizeof(int)-1) )
#define VEC_LSB(v)		( (signed int)_mm_extract_epi32(_mm256_extracti128_si256((v##1), 0), 0) )
#define VEC_CENTER(v)	( (signed int)_mm_extract_epi32(_mm256_extracti128_si256((v##2), 0), 0) )

/**
 * arithmetic and logic operations
 */
#define VEC_OR(a, b, c) { \
	(a##1) = _mm256_or_si256((b##1), (c##1)); \
	(a##2) = _mm256_or_si256((b##2), (c##2)); \
}

#define VEC_ADD(a, b, c) { \
	(a##1) = _mm256_add_epi32((b##1), (c##1)); \
	(a##2) = _mm256_add_epi32((b##2), (c##2)); \
}

#define VEC_ADDS(a, b, c) { \
	(a##1) = _mm256_add_epu32((b##1), (c##1)); \
	(a##2) = _mm256_add_epu32((b##2), (c##2)); \
}

#define VEC_SUB(a, b, c) { \
	(a##1) = _mm256_sub_epi32((b##1), (c##1)); \
	(a##2) = _mm256_sub_epi32((b##2), (c##2)); \
}

#define VEC_SUBS(a, b, c) { \
	(a##1) = _mm256_sub_epu32((b##1), (c##1)); \
	(a##2) = _mm256_sub_epu32((b##2), (c##2)); \
}

#define VEC_MAX(a, b, c) { \
	(a##1) = _mm256_max_epi32((b##1), (c##1)); \
	(a##2) = _mm256_max_epi32((b##2), (c##2)); \
}

#define VEC_MIN(a, b, c) { \
	(a##1) = _mm256_min_epi32((b##1), (c##1)); \
	(a##2) = _mm256_min_epi32((b##2), (c##2)); \
}

/**
 * shift operations
 *
 * SHIFT_R:
 *     _mm256_permute2x128_si256(a1, a2, 0x21) -> temporary1
 *     _mm256_permute2x128_si256(a1, a2, 0x83) -> temporary2
 *             a2: the upper 32 cells         a1: the lower 32 cells
 *         +-------------+-------------+  +-------------+-------------+
 * inputs: | a2[255:128] |  a2[127:0]  |  | a1[255:128] |  a1[127:0]  |
 *         +-------------+-------------+  +-------------+-------------+
 *    v              temporary1                    temporary2
 *         +-------------+-------------+  +-------------+-------------+
 * outputs:|  a2[127:0]  | a1[255:128] |  |      0      | a2[255:128] |
 *         +-------------+-------------+  +-------------+-------------+
 *
 *     _mm256_alignr_epi8 operation with temporary1 generates the lower half of the result.
 *                   temporary1               a1: the lower 32 cells
 *         +-------------+-------------+  +-------------+-------------+
 * inputs: |  a2[127:0]  | a1[255:128] |  | a1[255:128] |  a1[127:0]  |
 *         +-------------+-------------+  +-------------+-------------+
 *    v
 *         +----------------------------------+----------------------------------+
 * output: |(a2[127:0]<<128 | a1[255:128])>>32|(a1[255:128]<<128 | a1[127:0])>>32|
 *         +----------------------------------+----------------------------------+
 *
 *     _mm256_alignr_epi8 operation with temporary2 generates the upper half of the result.
 *                   temporary2               a2: the upper 32 cells
 *         +-------------+-------------+  +-------------+-------------+
 * inputs: |      0      | a2[255:128] |  | a2[255:128] |  a2[127:0]  |
 *         +-------------+-------------+  +-------------+-------------+
 *    v
 *         +----------------------------------+----------------------------------+
 * output: |    (0<<128 | a2[255:128])>>32    |(a2[255:128]<<128 | a2[127:0])>>32|
 *         +----------------------------------+----------------------------------+
 *
 *
 * SHIFT_L:
 *     _mm256_permute2x128_si256(a2, a1, 0x03) -> temporary1
 *     _mm256_permute2x128_si256(a2, a1, 0x28) -> temporary2
 *             a2: the upper 32 cells         a1: the lower 32 cells
 *         +-------------+-------------+  +-------------+-------------+
 * inputs: | a2[255:128] |  a2[127:0]  |  | a1[255:128] |  a1[127:0]  |
 *         +-------------+-------------+  +-------------+-------------+
 *    v              temporary1                    temporary2
 *         +-------------+-------------+  +-------------+-------------+
 * outputs:|  a2[127:0]  | a1[255:128] |  |  a1[127:0]  |      0      |
 *         +-------------+-------------+  +-------------+-------------+
 *
 *     _mm256_alignr_epi8 operation with temporary1 generates the upper half of the result.
 *             a2: the upper 32 cells              temporary1               
 *         +-------------+-------------+  +-------------+-------------+
 * inputs: | a2[255:128] |  a2[127:0]  |  |  a2[127:0]  | a1[255:128] |
 *         +-------------+-------------+  +-------------+-------------+
 *    v
 *         +-----------------------------------+-----------------------------------+
 * output: |(a2[255:128]<<128 | a2[127:0])>>112|(a2[127:0]<<128 | a1[255:128])>>112|
 *         +-----------------------------------+-----------------------------------+
 *
 *     _mm256_alignr_epi8 operation with temporary2 generates the lower half of the result.
 *             a1: the lower 32 cells              temporary2
 *         +-------------+-------------+  +-------------+-------------+
 * inputs: | a1[255:128] |  a1[127:0]  |  |  a1[127:0]  |      0      |
 *         +-------------+-------------+  +-------------+-------------+
 *    v
 *         +-----------------------------------+-----------------------------------+
 * output: |(a1[255:128]<<128 | a1[127:0])>>112|     (a1[127:0]<<128 | 0)>>112     |
 *         +-----------------------------------+-----------------------------------+
 */
#define VEC_SHIFT_R(a) { \
	__m256i tmp1 = _mm256_permute2x128_si256((a##1), (a##2), 0x21); \
	__m256i tmp2 = _mm256_permute2x128_si256((a##1), (a##2), 0x83); \
	(a##1) = _mm256_alignr_epi8(tmp1, (a##1), sizeof(int)); \
	(a##2) = _mm256_alignr_epi8(tmp2, (a##2), sizeof(int)); \
}

#define VEC_SHIFT_L(a) { \
	__m256i tmp1 = _mm256_permute2x128_si256((a##2), (a##1), 0x28); \
	__m256i tmp2 = _mm256_permute2x128_si256((a##2), (a##1), 0x03); \
	(a##1) = _mm256_alignr_epi8((a##1), tmp1, sizeof(__m128i) - sizeof(int)); \
	(a##2) = _mm256_alignr_epi8((a##2), tmp2, sizeof(__m128i) - sizeof(int)); \
}

/**
 * compare and select
 */
#define VEC_COMPARE(a, b, c) { \
 	__m128i tmp = _mm_cmpeq_epi8((b##1), (c##1)); \
	(a##1) = _mm256_cvtepi8_epi32(tmp); \
	(a##2) = _mm256_cvtepi8_epi32(_mm_srli_si128(tmp, sizeof(__m128i)/2)); \
}

#define VEC_SELECT(a, b, c, d) { \
	(a##1) = _mm256_blendv_epi8((b##1), (c##1), (d##1)); \
	(a##2) = _mm256_blendv_epi8((b##2), (c##2), (d##2)); \
}

/**
 * load and store operations
 */
#define VEC_STORE(p, v)	{ \
	_mm256_store_si256((__m256i *)(p), v##1); p += sizeof(__m256i); \
	_mm256_store_si256((__m256i *)(p), v##2); p += sizeof(__m256i); \
}

/**
 * char vector operations
 */
#define VEC_CHAR_SHIFT_R(v) { \
 	(v##1) = _mm_srli_si128((v##1), 1); \
}

#define VEC_CHAR_SHIFT_L(v) { \
	(v##1) = _mm_slli_si128((v##1), 1); \
}

#define VEC_CHAR_INSERT_MSB(v, a) { \
	(v##1) = _mm_insert_epi8( \
		(v##1), (a), sizeof(__m128i)-sizeof(char)); \
}

#define VEC_CHAR_INSERT_LSB(v, a) { \
	(v##1) = _mm_insert_epi8( \
		(v##1), (a), 0); \
}

#endif /* #ifndef _AVX_B32_R2_H_INCLUDED */
/**
 * end of avx_b32_r1.h
 */
