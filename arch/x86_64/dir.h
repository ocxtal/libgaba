
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
#define dir_vec_size()	( sizeof(__m128i) )

#define dir_vec_setzero(v) { \
	(v) = _mm_setzero_si128(); \
}

#define dir_vec_append(v, d) { \
	(v) = _mm_srli_si128(v, 1); \
	(v) = _mm_insert_epi8(v, d, 15); \
}

#define dir_vec_store(ptr, v) { \
	_mm_store_si128((__m128i *)ptr, v); ptr += sizeof(__m128i); \
}

#define dir_vec_stride_size()		( bpb() - dir_vec_size() )
#define dir_vec_base_addr(p, sp) ( \
	  phantom_size() \
	+ (dynamic_blk_num(p-sp, 0) + 1) * dir_vec_stride_size() \
	- sp \
)

#define dir_vec_acc(ptr, p, sp) ( \
	(ptr)[p] \
)

#endif /* #ifndef _DIR_H_INCLUDED */
/**
 * end of dir.h
 */