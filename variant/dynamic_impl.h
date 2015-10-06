
/**
 * @file dynamic.h
 *
 * @brief direction determiner and address calculation macros for the dynamic band algorithm
 */
#ifndef _DYNAMIC_H_INCLUDED
#define _DYNAMIC_H_INCLUDED

#include "../sea.h"
#include "../util/util.h"
#include "../arch/arch_util.h"
// #include "../arch/dir.h"		/** simd appender */
#include <stdint.h>

/**
 * @macro BLK
 * @brief block split length
 */
#define BLK_SFT		( 5 )
#define BLK 		( 1<<BLK_SFT )

/**
 * address calculation macros (in bytes)
 */
#define dynamic_dir_size() ( \
	2 * sizeof(uint64_t) \
)
#define dynamic_stride_size() ( \
	bpb() - dynamic_dir_size() \
)
#define dynamic_blk_num(p, q) ( \
	((p) & ~(BLK-1))>>BLK_SFT \
)
#define dynamic_blk_addr(p, q) ( \
	((p) & (BLK-1)) * bpl() + ((q)+BW/2) * sizeof(dpcell_t) \
)
#define dynamic_addr(p, q) ( \
	  dynamic_blk_num(p, q) * bpb() \
	+ dynamic_blk_addr(p, q) \
	+ sizeof(head_t) \
)

/**
 * direction determiner variants
 */
/**
 * @struct _dir
 * @brief direction flag container.
 */
struct _dir {
	uint8_t *pdr;
	uint64_t dir_vec, dir_vec_tail;
};

typedef struct _dir dir_t;

/**
 * @macro dir, dir2, dir_ue, dir_le, dir2_ue, dir2_le
 * @brief direction (or 2-bit direction) of the upper (lower) edge
 */
#define dynamic_dir_ue(r)	( 0x01 & (r).dir_vec )
#define dynamic_dir2_ue(r) 	( 0x05 & (r).dir_vec )
#define dynamic_dir_le(r)	( (0x02 & (r).dir_vec)>>1 )
#define dynamic_dir2_le(r) 	( (0x0a & (r).dir_vec)>>1 )
#define dynamic_dir(r)		dir_ue(r)
#define dynamic_dir2(r) 	dir2_ue(r)
#define dynamic_dir_raw(r)	( 0x0f & (r).dir_vec )

#define dynamic_dir_topq(r)		( (int64_t)-1 + (0x01 & (r).dir_vec) )
#define dynamic_dir_leftq(r)	( (int64_t)0x01 & (r).dir_vec )
#define dynamic_dir_topleftq(r)	( (int64_t)(0x01 & ((r).dir_vec>>2)) - (0x01 & ~(r).dir_vec) )

/**
 * direction determiners for fill-in functions
 */
/**
 * @macro dynamic_dir_init
 * @brief initialize _dir struct
 */
#define dynamic_dir_init(r, k, dp, sp) { \
	/** clear varaibles */ \
	(r).pdr = NULL; \
	(r).dir_vec = (uint64_t)_tail((dp), d2); \
}

/**
 * @macro dynamic_dir_start_block
 */
#define dynamic_dir_start_block(r) { \
	/** nothing to do */ \
}

/**
 * @macro dynamic_dir_det_next
 * @brief set 2-bit direction flag from external expression.
 *
 * @detail
 * dir_exp_top and dir_exp_bottom must be aliased to a direction determiner.
 */
#define dynamic_dir_det_next(r, p) { \
	(p)++; \
	uint8_t d = dir_exp_top(r, k) | dir_exp_bottom(r, k); \
	(r).dir_vec = ((r).dir_vec<<2) | d;		/** use shld if needed */ \
}

/**
 * @macro dynamic_dir_empty
 * @brief empty body
 */
#define dynamic_dir_empty(r) { \
	(r).dir_vec <<= 2;						/** use shld if needed */ \
}

/**
 * @macro dynamic_dir_end_block
 */
#define dynamic_dir_end_block(r, dp) { \
	/** store direction vector at the end of the pdp */ \
	*((uint64_t *)(dp)) = (r).dir_vec; \
	(dp) += dir_size(); \
}

/**
 * @macro dynamic_dir_test_bound
 */
#define dynamic_dir_test_bound(r, k, p)			( 0 )
#define dynamic_dir_test_bound_cap(r, k, p)		( 0 )

/**
 * @macro dynamic_dir_set_pdr
 */
#define dynamic_dir_set_pdr(r, k, dp, p, sp) { \
	/** calculate (virtual) pdr */ \
	(r).pdr = (uint8_t *)(dp) + dynamic_addr(p, sp); \
	(r).dir_vec = *((uint64_t *)(r).pdr)>>((p & (BLK-1))<<1); \
	(r).dir_vec_tail = *((uint64_t *)(r).pdr)<<(64 - ((p & (BLK-1))<<1)); \
}

/**
 * @macro dynamic_dir_load_forward
 */
#define dynamic_dir_load_forward(r, p) { \
	if((++p & (BLK-1)) == 0) { \
		(r).pdr += dynamic_stride_size(); \
		(r).dir_vec_tail = *((uint64_t *)(r).pdr); \
	} \
	/** shld */ \
	(r).dir_vec = ((r).dir_vec<<2) | ((r).dir_vec_tail>>62); \
	(r).dir_vec_tail <<= 2; \
}

/**
 * @macro dynamic_dir_go_forward
 */
#define dynamic_dir_go_forward(r, p) { \
	if((++p & (BLK-1)) == 0) { \
		(r).pdr += dynamic_stride_size(); \
	} \
}

/**
 * @macro dynamic_dir_load_backward
 */
#define dynamic_dir_load_backward(r, p) { \
	(r).dir_vec >>= 2; \
	if((p-- & (BLK-1)) == 0) { \
		(r).pdr -= dynamic_stride_size(); \
		(r).dir_vec = *((uint64_t *)(r).pdr); \
	} \
}

/**
 * @macro dynamic_dir_go_backward
 * this macro do not update direction pointer
 */
#define dynamic_dir_go_backward(r, p) { \
	if((p-- & (BLK-1)) == 0) { \
		(r).pdr -= dynamic_stride_size(); \
	} \
}

#endif /* #ifndef _DYNAMIC_H_INCLUDED */
/**
 * end of dynamic.h
 */
