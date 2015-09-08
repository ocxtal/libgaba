
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
 * @macro BLK
 * @brief block split length
 */
#define BLK 		( 32 )

#if DP == DYNAMIC
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
	(v) = _mm_insert_epi8(v, (d) & 0x03, BLK-1); \
}

#define dir_vec_store(ptr, v) { \
	_mm_store_si128((__m128i *)(ptr), v); \
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
#endif /* DP == DYNAMIC */

#define dir_vec_sum_i(ptr, dp) ( \
	(int64_t)(BLK - 1 - (dp) \
		- popcnt( \
			( _mm_movemask_epi8(_mm_slli_epi64( \
				_mm_lddqu_si128((__m128i *)(ptr)), 7)) \
			| _mm_movemask_epi8(_mm_slli_epi64( \
				_mm_lddqu_si128((__m128i *)(ptr) + 1), 7))<<16) \
			>>((dp)+1)) \
		) \
)


#endif /* #ifndef _DIR_H_INCLUDED */
/**
 * end of dir.h
 */