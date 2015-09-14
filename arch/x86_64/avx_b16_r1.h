
/**
 * @file avx_b16_r1.h
 *
 * @brief a header for macros of packed 16-bit AVX2 instructions.
 *
 * @detail
 * This is a collection of wrapper macros of AVX2 SIMD intrinsics.
 * Each macro corresponds to a set of several intrinsics defined in
 * smmintrin.h. The details of the intrinsics are found in the intel's
 * website: https://software.intel.com/sites/landingpage/IntrinsicsGuide/
 * The required set of macros are documented in porting section of 
 * README.md in the top directory of the library.
 *
 * @sa avx.h
 */
#ifndef _AVX_B16_R1_H_INCLUDED
#define _AVX_B16_R1_H_INCLUDED

#include <immintrin.h>

/**
 * register declarations. 
 */
#define _vec_cell(v)			__m256i v
#define _vec_cell_reg(v)		__m256i register v
#define _vec_char_reg(v)		__m128i register v

/**
 * substitution to cell vectors
 */
#define vec_assign(a, b) { \
 	(a) = (b); \
}

#define vec_set(v, i) { \
	(v) = _mm256_set1_epi16(i); \
}

#define vec_setzero(v) { \
	(v) = _mm256_setzero_si256(); \
}

#define vec_setones(v) { \
	(v) = _mm256_set1_epi8(0xff); \
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
 	vec_set(v, i); (v) = _mm256_permute2x128_si256((v), (v), 0x80); \
}

#define vec_set_uhalf(v, i) { \
	vec_set(v, i); (v) = _mm256_permute2x128_si256((v), (v), 0x08); \
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
 	(v) = _mm256_inserti128_si256( \
 		(v), \
 		_mm_insert_epi16( \
 			_mm256_extracti128_si256((v), 1), (a), sizeof(__m128i)/sizeof(short)-1), \
 		1); \
}

#define vec_insert_lsb(v, a) { \
	(v) = _mm256_inserti128_si256( \
		(v), \
		_mm_insert_epi16(_mm256_extracti128_si256((v), 0), (a), 0), \
		0); \
}

#define vec_msb(v)		( (signed short)_mm_extract_epi16(_mm256_extracti128_si256((v), 1), sizeof(__m128i)/sizeof(short)-1) )
#define vec_lsb(v)		( (signed short)_mm_extract_epi16(_mm256_extracti128_si256((v), 0), 0) )
#define vec_center(v)	( (signed short)_mm_extract_epi16(_mm256_extracti128_si256((v), 1), 0) )

/**
 * arithmetic and logic operations
 */
#define vec_or(a, b, c) { \
 	(a) = _mm256_or_si256((b), (c)); \
}

#define vec_add(a, b, c) { \
	(a) = _mm256_adds_epi16((b), (c)); \
}

#define vec_adds(a, b, c) { \
	(a) = _mm256_adds_epu16((b), (c)); \
}

#define vec_sub(a, b, c) { \
	(a) = _mm256_subs_epi16((b), (c)); \
}

#define vec_subs(a, b, c) { \
	(a) = _mm256_subs_epu16((b), (c)); \
}

#define vec_max(a, b, c) { \
	(a) = _mm256_max_epi16((b), (c)); \
}

#define vec_min(a, b, c) { \
	(a) = _mm256_min_epi16((b), (c)); \
}

/**
 * shift operations
 *
 * For the details of the shift operation, see avx_b8_r1.h.
 */
#define vec_shift_r(a) { \
 	(a) = _mm256_alignr_epi8( \
 		_mm256_permute2x128_si256((a), (a), 0x81), \
		(a), \
		sizeof(short)); \
}

#define vec_shift_l(a) { \
	(a) = _mm256_alignr_epi8( \
		(a), \
		_mm256_permute2x128_si256((a), (a), 0x08), \
		sizeof(__m128i) - sizeof(short)); \
}

/**
 * compare and select
 */
#define vec_comp(a, b, c) { \
 	(a) = _mm256_cvtepi8_epi16(_mm_cmpeq_epi8((b), (c))); \
}

#define vec_select(a, b, c, d) { \
	(a) = _mm256_blendv_epi8((b), (c), (d)); \
}

/**
 * load and store operations
 */
#define vec_store(p, v) { \
 	_mm256_store_si256((__m256i *)(p), v); p += sizeof(__m256i); \
}

#define vec_store_packed(p, dv, dh) { \
	_mm256_store_si256( \
		(__m256i *)(p), \
		_mm256_or_si256(_mm256_slli_epi64(dh, 4), dv)); \
	p += sizeof(__m256i); \
}

/**
 * char vector operations
 */
#define vec_char_shift_r(v) { \
	(v) = _mm_srli_si128((v), 1); \
}

#define vec_char_shift_l(v) { \
	(v) = _mm_slli_si128((v), 1); \
}

#define vec_char_insert_msb(v, a) { \
	(v) = _mm_insert_epi8((v), (a), sizeof(__m128i)-1); \
}

#define vec_char_insert_lsb(v, a) { \
	(v) = _mm_insert_epi8((v), (a), 0); \
}

#endif /* #ifndef _AVX_B16_R1_H_INCLUDED */
/**
 * end of avx_b16_r1.h
 */
