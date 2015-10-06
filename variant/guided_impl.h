
/**
 * @file guided.h
 *
 * @brief direction determiner and address calculation macros for the guided band algorithm
 */
#ifndef _GUIDED_H_INCLUDED
#define _GUIDED_H_INCLUDED

#include "../sea.h"
#include "../util/util.h"
#include "../arch/dir.h"
#include <stdint.h>

/**
 * @macro BLK
 * @brief block split length
 */
#define BLK_SFT		( 5 )
#define BLK 		( 1<<BLK_SFT )

/**
 * address calculation macros
 */
#define guided_dir_size()		( 0 )
#define guided_blk_num(p, q) ( \
	((p) & ~(BLK-1))>>BLK_SFT \
)
#define guided_blk_addr(p, q) ( \
	((p) & (BLK-1)) * bpl() + ((q)+BW/2) * sizeof(dpcell_t) \
)
#define guided_addr(p, q) ( \
	  guided_blk_num(p, q) * bpb() \
	+ guided_blk_addr(p, q) \
	+ sizeof(head_t) \
)

/**
 * direction determiner variants
 */
struct _dir {
	uint8_t *pdr;
	uint8_t d2;
};
typedef struct _dir dir_t;

/**
 * @macro dir, dir2, dir_ue, dir_le, dir2_ue, dir2_le
 * @brief direction (or 2-bit direction) of the upper (lower) edge
 */
#define guided_dir_ue(r)	( 0x01 & (r).d2 )
#define guided_dir2_ue(r) 	( 0x05 & (r).d2 )
#define guided_dir_le(r)	( (0x02 & (r).d2)>>1 )
#define guided_dir2_le(r) 	( (0x0a & (r).d2)>>1 )
#define guided_dir(r)		dir_ue(r)
#define guided_dir2(r) 		dir2_ue(r)
#define guided_dir_raw(r) 	( (r).d2 )

#define guided_dir_topq(r)		( (int64_t)-1 + (0x01 & (r).d2) )
#define guided_dir_leftq(r)		( (int64_t)0x01 & (r).d2 )
#define guided_dir_topleftq(r)	( (int64_t)(0x01 & ((r).d2>>2)) - (0x01 & ~(r).d2)) )

/**
 * direction determiners for fill-in macros
 */
/**
 * @macro guided_dir_init
 */
#define guided_dir_init(r, k, dp, sp) { \
	(r).pdr = (k)->pdr + (sp); \
	(r).d2 = (r).pdr[0] | ((r).pdr[-1]<<2); \
}

/**
 * @macro guided_dir_start_block
 */
#define guided_dir_start_block(r) { \
	/** nothing to do */ \
}

/**
 * @macro guided_dir_det_next
 * @brief set 2-bit direction flag from direction array.
 */
#define guided_dir_det_next(r, p) { \
	(r).d2 = ((r).d2<<2) | (r).pdr[++(p)]; \
}

/**
 * @macro guided_dir_empty
 */
#define guided_dir_empty(r) { \
	/** nothing to do */ \
}

/**
 * @macro guided_dir_end_block
 */
#define guided_dir_end_block(r, dp) { \
	/** nothing to do */ \
}

/**
 * @macro guided_dir_test_bound
 */
#define guided_dir_test_bound(r, k, p) ( \
	((k)->tdr - (r).pdr + 2) - p - BLK \
)
#define guided_dir_test_bound_cap(r, k, p) ( \
	((k)->tdr - (r).pdr + 2) - p \
)

/**
 * direction loaders for search and traceback functions
 */
/**
 * @macro guided_dir_set_pdr
 */
#define guided_dir_set_pdr(r, k, dp, p, sp) { \
	(r).pdr = (k)->pdr + (sp); \
	(r).d2 = (r).pdr[p]; \
}

/**
 * @macro guided_dir_load_forward
 */
#define guided_dir_load_forward(r, p) { \
	(r).d2 = (r).pdr[++p]; \
}
#define guided_dir_go_forward(r, p) { \
	p++; \
}

/**
 * @macro guided_dir_load_backward
 * @brief set 2-bit direction flag in reverse access.
 */
#define guided_dir_load_backward(r, p) { \
	(r).d2 = (r).pdr[--p]; \
}
#define guided_dir_go_backward(r, p) { \
	p--; \
}

#endif /* #ifndef _GUIDED_H_INCLUDED */
/**
 * end of guided.h
 */
