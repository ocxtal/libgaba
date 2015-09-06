
/**
 * @file dynamic.h
 *
 * @brief direction determiner and address calculation macros for the dynamic band algorithm
 */
#ifndef _DYNAMIC_H_INCLUDED
#define _DYNAMIC_H_INCLUDED

#include "../sea.h"
#include "../util/util.h"
// #include "../arch/dir.h"		/** simd appender */
#include <stdint.h>

/**
 * @macro BLK
 * @brief block split length
 */
#define BLK 		( 32 )

#ifndef _DIR_H_INCLUDED
/**
 * 2-bit packed direction holder
 */
#define dir_vec(v)		uint64_t v;
#define dir_vec_size()	( 2*sizeof(uint64_t) )

#define dir_vec_setzero(v) { \
	(v) = 0; \
}
#define dir_vec_append(v, d) { \
	debug("v(%llx), d(%d)", v, d); \
	(v) = ((v)>>2) | (((uint64_t)(d))<<62); \
}
#define dir_vec_store(ptr, v) { \
	debug("v(%llx)", v); \
	*((uint64_t *)ptr) = (v); ptr += dir_vec_size(); \
}
#define dir_vec_stride_size()		( bpb() )
#define dir_vec_base_addr(p, sp) ( \
	  phantom_size() \
	+ (dynamic_blk_num(p-sp, 0) + 1) * dir_vec_stride_size() \
	- dr_size() \
)
#define dir_vec_acc(ptr, p, sp) ( \
	0x03 & ((*((uint64_t *)ptr))>>((((p) - (sp)) & (BLK-1))<<1)) \
)
#endif /* #ifndef _DIR_H_INCLUDED */

/**
 * address calculation macros (in bytes)
 */
#define dynamic_dr_size() ( \
	dir_vec_size()	/** dr array (bytes) per block */ \
)
#define dynamic_blk_num(p, q) ( \
	((p) & ~(BLK-1)) / BLK \
)
#define dynamic_blk_addr(p, q) ( \
	((p) & (BLK-1)) * bpl() + ((q)+BW/2) * sizeof(cell_t) \
)
#define dynamic_addr(p, q) ( \
	  dynamic_blk_num(p, q) * bpb() \
	+ dynamic_blk_addr(p, q) \
	+ phantom_size() \
)

/**
 * direction determiner variants
 */
/**
 * @struct _dir
 * @brief direction flag container.
 */
struct _dir {
	dir_vec(v);
	uint8_t *pdr;
	uint8_t d2;
};

typedef struct _dir dir_t;

/**
 * @macro dir, dir2, dir_ue, dir_le, dir2_ue, dir2_le
 * @brief direction (or 2-bit direction) of the upper (lower) edge
 */
#define dynamic_dir_ue(r)	( 0x04 & (r).d2 )
#define dynamic_dir2_ue(r) 	( 0x05 & (r).d2 )
#define dynamic_dir_le(r)	( (0x08 & (r).d2)>>1 )
#define dynamic_dir2_le(r) 	( (0x0a & (r).d2)>>1 )
#define dynamic_dir(r)		dir_ue(r)
#define dynamic_dir2(r) 	dir2_ue(r)
#define dynamic_dir_raw(r)	( (r).d2 )

/**
 * @macro dynamic_dir_init
 * @brief initialize _dir struct
 */
#define dynamic_dir_init(r, k, dp, p) { \
	/** clear varaibles */ \
	dir_vec_setzero((r).v); \
	(r).pdr = NULL; \
	(r).d2 = _tail((dp), d2); \
}

/**
 * @macro dynamic_dir_start_block
 */
#define dynamic_dir_start_block(r, k, dp, p) { \
	/** nothing to do */ \
}

/**
 * @macro dynamic_dir_det_next
 * @brief set 2-bit direction flag from external expression.
 *
 * @detail
 * dir_exp_top and dir_exp_bottom must be aliased to a direction determiner.
 */
#define dynamic_dir_det_next(r, k, dp, p) { \
	(p)++; \
	uint8_t d = dir_exp_top(r, k, dp) | dir_exp_bottom(r, k, dp); \
	debug("dynamic band: d(%d)", d); \
	(r).d2 = (d<<2) | ((r).d2>>2); \
	dir_vec_append((r).v, 0x03 & (r).d2); \
}

/**
 * @macro dynamic_dir_end_block
 */
#define dynamic_dir_end_block(r, k, dp, p) { \
	/** store direction vector at the end of the pdp */ \
	dir_vec_store(dp, (r).v); \
}

/**
 * @macro dynamic_dir_set_pdr
 */
#define dynamic_dir_set_pdr(r, k, dp, p, sp) { \
	/** calculate (virtual) pdr */ \
	(r).pdr = (uint8_t *)(dp) + dir_vec_base_addr(p, sp); \
}

/**
 * @macro dynamic_dir_load_backward
 */
#define dynamic_dir_load_backward(r, k, dp, p, sp) { \
	p--; \
	(r).d2 = 0x0f & (((r).d2<<2) | dir_vec_acc((r).pdr, p, sp)); \
}

/**
 * @macro dynamic_dir_stride_jam
 */
#define dynamic_dir_stride_jam(r, k, dp, p, sp) { \
	/** windback by a block (dp matrix and (i, j)) */ \
	(r).pdr -= dir_vec_stride_size(); \
}

#endif /* #ifndef _DYNAMIC_H_INCLUDED */
/**
 * end of dynamic.h
 */
