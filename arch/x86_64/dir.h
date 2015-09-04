
/**
 * @file dir.h
 *
 * @brief direction determiner appender
 */
#ifndef _DIR_H_INCLUDED
#define _DIR_H_INCLUDED

#include <smmintrin.h>
#include <stdint.h>

/**
 * direction holder register
 */
#define dir_vec(v)		__m128i v;

#define dir_vec_setzero(v) { \
	(v) = _mm_setzero_si128(v); \
}

#define dir_vec_append(v, d) { \
	(v) = _mm_srli_si128(v); \
	(v) = _mm_insert_epi8(v, d, 15); \
}

#define dir_vec_store(p, v) { \
	_mm_store_si128((__m128i *)p, v); p += sizeof(__m128i); \
}

#endif /* #ifndef _DIR_H_INCLUDED */
/**
 * end of dir.h
 */