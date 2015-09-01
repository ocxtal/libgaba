
/**
 * @file variant.h
 *
 * @brief macro implementations
 */
#ifndef _VARIANT_H_INCLUDED
#define _VARIANT_H_INCLUDED

#include "../sea.h"
#include "../util/util.h"
#include <stdint.h>

/**
 * define constants
 */
#define DYNAMIC		( 2 )		// avoid 1
#define GUIDED 		( 3 )
#define LINEAR 		( 4 )
#define AFFINE 		( 5 )

/**
 * defaults
 */
// #define DP 				GUIDED
// #define COST 			LINEAR

/**
 * direction determiner variants
 */

/**
 * @struct _dir
 * @brief direction flag container.
 */
struct _dir {
	uint8_t d2;
	uint8_t *pdr;
};

typedef struct _dir dir_t;

/**
 * @macro dir_ue
 * @brief direction of the upper edge
 */
#define dir_ue(r)	( 0x04 & (r).d2 )

/**
 * @macro dir2_ue
 * @brief 2-bit direction flag of the upper edge
 */
#define dir2_ue(r) 	( 0x05 & (r).d2 )

/**
 * @macro dir_le
 * @brief direction of the upper edge
 */
#define dir_le(r)	( (0x08 & (r).d2)>>1 )

/**
 * @macro dir2_le
 * @brief 2-bit direction flag of the upper edge
 */
#define dir2_le(r) 	( (0x0a & (r).d2)>>1 )

/**
 * @macro dir
 * @brief extract the most recent direction flag
 */
#define dir(r)		dir_ue(r)

/**
 * @macro dir2
 * @brief extract the most recent 2-bit direction flag
 */
#define dir2(r) 	dir2_ue(r)

/**
 * @macro dir_init
 * @brief initialize _dir struct
 */
#define dir_init(r, dr, p) { \
	(r).pdr = (dr); \
	(r).d2 = (r).pdr[p]<<2; \
}

/**
 * @macro dir_load_dynamic
 * @brief load the next dir pointer
 */
#define dir_load_forward_dynamic(r, k, dp, p) { \
	(r).pdr = (uint8_t *)((cell_t *)(dp) + BLK * BW) - (p); \
}

/**
 * @macro dir_load_guided
 */
#define dir_load_forward_guided(r, k, dp, p) { \
	/** nothing to do, pdr must be initialized with k->pdr in fill_decl */ \
}

/**
 * @macro dir_next_dynamic
 * @brief set 2-bit direction flag from external expression.
 *
 * @detail
 * dir_exp_top and dir_exp_bottom must be aliased to a direction determiner.
 */
#define dir_next_dynamic(r, k, dp, p) { \
	uint8_t d = dir_exp_top(r, k, dp) | dir_exp_bottom(r, k, dp); \
	debug("dynamic band: d(%d)", d); \
	(r).pdr[++(p)] = d; \
	(r).d2 = (d<<2) | ((r).d2>>2); \
}

/**
 * @macro dir_next_guided
 * @brief set 2-bit direction flag from direction array.
 */
#define dir_next_guided(r, k, dp, p) { \
	uint8_t d = (r).pdr[++(p)]; \
	debug("guided band: d(%d)", d); \
	(r).d2 = (d<<2) | ((r).d2>>2); \
}

/**
 * @macro dir_test_term_dynamic
 */
#define dir_test_term_dynamic(r, k)			( 0 )

/**
 * @macro dir_test_term_guided
 */
#define dir_test_term_guided(r, k, dp, p)	( (p) - (c.dr.ep - c.dr.sp) )

/**
 * @macro dir_load_term_dynamic
 */
#define dir_load_term_dynamic(r, k, dp, p) { \
	(r).pdr = dp \
		+ bpl(k) * ((((p) - _ivec(dp, ep)) & ~(BLK-1)) + BLK) \
		- _ivec(dp, p); \
	(r).d2 = (r).pdr[p]; \
}

/**
 * @macro dir_load_term_guided
 */
#define dir_load_term_guided(r, k, dp, p) { \
	/** pdr = k->pdr; */ \
	(r).d2 = (r).pdr[p]; \
}

/**
 * @macro dir_load_backward_dynamic
 */
#define dir_load_backward_dynamic(r, k, dp, p) { \
	uint8_t *s = (r).pdr + --(p); \
	(r).d2 = 0x0f & (((r).d2<<2) | *s); \
	if(((uint64_t)s & (BLK-1)) == 0) { (r).pdr -= BLK * bpl(k); } \
}

/**
 * @macro dir_load_backward_guided
 * @brief set 2-bit direction flag in reverse access.
 */
#define dir_load_backward_guided(r, k, dp, p) { \
	(r).d2 = 0x0f & (((r).d2<<2) | (r).pdr[--p]); \
}

/**
 * @macro addr_linear_dynamic
 */
#define addr_linear_dynamic(p, q, blk, band) ( \
	(band) * ((p) + ((p) & ~((blk)-1))) + (q) + (band)/2 \
)

/**
 * @macro addr_linear_guided
 * @brief address calculation macro
 */
#define addr_linear_guided(p, q, blk, band) ( \
	(band) * (p) + (q) + (band)/2 \
)

/**
 * @macro addr_affine_dynamic
 */
#define addr_affine_dynamic(p, q, blk, band) ( \
)

/**
 * @macro addr_affine_guided
 */
#define addr_affine_guided(p, q, blk, band) ( \
)

/**
 * macro aliasing
 */
#ifndef DP
	#warning "DP undefined"
#endif

#if DP == DYNAMIC
	#define dir_load_forward 	dir_load_forward_dynamic
	#define dir_next 			dir_next_dynamic
	#define dir_test_term		dir_test_term_dynamic
	#define dir_load_term 		dir_load_term_dynamic
	#define dir_load_backward 	dir_load_backward_dynamic

	#define addr_linear 		addr_linear_dynamic
	#define addr_affine 		addr_affine_dynamic
#elif DP == GUIDED
	#define dir_load_forward 	dir_load_forward_guided
	#define dir_next 			dir_next_guided
	#define dir_test_term 		dir_test_term_guided
	#define dir_load_term 		dir_load_term_guided
	#define dir_load_backward 	dir_load_backward_guided

	#define addr_linear 		addr_linear_guided
	#define addr_affine 		addr_affine_guided
#else /* #if DP == DYNAMIC */
	#error "DP must be DYNAMIC or GUIDED."
#endif /* #if DP == DYNAMIC */


/**
 * variant selector
 */
#define HEADER(base)			QUOTE(HEADER_WITH_SUFFIX(base, _impl.h))
#include HEADER(BASE)

#ifndef COST
	#warning "COST undefined"
#endif

#if COST == LINEAR

	#define COST_SUFFIX 		_linear

#elif COST == AFFINE

	#define COST_SUFFIX 		_affine

#else /* #if COST == LINEAR */

	#error "COST must be LINEAR or AFFINE."

#endif /* #if COST == LINEAR */

#define bpl					LABEL_WITH_SUFFIX(BASE, COST_SUFFIX, _bpl)
#define topq				LABEL_WITH_SUFFIX(BASE, COST_SUFFIX, _topq)
#define leftq				LABEL_WITH_SUFFIX(BASE, COST_SUFFIX, _leftq)
#define topleftq			LABEL_WITH_SUFFIX(BASE, COST_SUFFIX, _topleftq)
#define dir_exp_top 		LABEL_WITH_SUFFIX(BASE, COST_SUFFIX, _dir_exp_top)
#define dir_exp_bottom 		LABEL_WITH_SUFFIX(BASE, COST_SUFFIX, _dir_exp_bottom)
#define fill_decl			LABEL_WITH_SUFFIX(BASE, COST_SUFFIX, _fill_decl)
#define fill_init			LABEL_WITH_SUFFIX(BASE, COST_SUFFIX, _fill_init)
#define fill_start 			LABEL_WITH_SUFFIX(BASE, COST_SUFFIX, _fill_start)
#define fill_former_body	LABEL_WITH_SUFFIX(BASE, COST_SUFFIX, _fill_former_body)
#define fill_go_down		LABEL_WITH_SUFFIX(BASE, COST_SUFFIX, _fill_go_down)
#define fill_go_right		LABEL_WITH_SUFFIX(BASE, COST_SUFFIX, _fill_go_right)
#define fill_latter_body	LABEL_WITH_SUFFIX(BASE, COST_SUFFIX, _fill_latter_body)
#define fill_check_term		LABEL_WITH_SUFFIX(BASE, COST_SUFFIX, _fill_check_term)
#define fill_check_chain	LABEL_WITH_SUFFIX(BASE, COST_SUFFIX, _fill_check_chain)
#define fill_check_alt		LABEL_WITH_SUFFIX(BASE, COST_SUFFIX, _fill_check_alt)
#define fill_check_mem		LABEL_WITH_SUFFIX(BASE, COST_SUFFIX, _fill_check_mem)
#define fill_end 			LABEL_WITH_SUFFIX(BASE, COST_SUFFIX, _fill_end)
#define fill_finish			LABEL_WITH_SUFFIX(BASE, COST_SUFFIX, _fill_finish)
//#define chain_save_len		LABEL_WITH_SUFFIX(BASE, COST_SUFFIX, _chain_save_len)
//#define chain_push_ivec		LABEL_WITH_SUFFIX(BASE, COST_SUFFIX, _chain_push_ivec)
#define search_terminal		LABEL_WITH_SUFFIX(BASE, COST_SUFFIX, _search_terminal)
#define search_trigger		LABEL_WITH_SUFFIX(BASE, COST_SUFFIX, _search_trigger)
#define search_max_score	LABEL_WITH_SUFFIX(BASE, COST_SUFFIX, _search_max_score)
#define trace_decl			LABEL_WITH_SUFFIX(BASE, COST_SUFFIX, _trace_decl)
#define trace_init			LABEL_WITH_SUFFIX(BASE, COST_SUFFIX, _trace_init)
#define trace_body			LABEL_WITH_SUFFIX(BASE, COST_SUFFIX, _trace_body)
#define trace_check_term 	LABEL_WITH_SUFFIX(BASE, COST_SUFFIX, _trace_check_term)
#define trace_finish		LABEL_WITH_SUFFIX(BASE, COST_SUFFIX, _trace_finish)

#endif /* #ifndef _VARIANT_H_INCLUDED */

/**
 * end of variant.h
 */
