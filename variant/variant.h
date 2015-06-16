
/**
 * @file variant.h
 *
 * @brief macro implementations
 */
#ifndef _VARIANT_H_INCLUDED
#define _VARIANT_H_INCLUDED

#include "../include/sea.h"
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
};

typedef struct _dir dir_t;

/**
 * @macro dir_init
 * @brief initialize _dir struct
 */
#define dir_init(r, dir) { \
	(r).d2 = (dir)<<1; \
}

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
 * @macro dir_next_dynamic
 * @brief set 2-bit direction flag from external expression.
 *
 * @detail
 * dir_exp_top and dir_exp_bottom must be aliased to a direction determiner.
 */
#define dir_next_dynamic(r, t, c) { \
	uint8_t d = dir_exp_top(r, t, c) | dir_exp_bottom(r, t, c); \
	debug("dynamic band: d(%d)", d); \
	(c).pdr[++(t).p] = d; \
	(r).d2 = (d<<2) | ((r).d2>>2); \
}

/**
 * @macro dir_next_guided
 * @brief set 2-bit direction flag from direction array.
 */
#define dir_next_guided(r, t, c) { \
	uint8_t d = (c).pdr[++(t).p]; \
	debug("guided band: d(%d)", d); \
	(r).d2 = (d<<2) | ((r).d2>>2); \
}

/**
 * @macro dir_check_term_dynamic
 */
#define dir_check_term_dynamic(r, t, c)		( 1 )

/**
 * @macro dir_check_term_guided
 */
#define dir_check_term_guided(r, t, c)		( t.p < (c.dr.ep - c.dr.sp) )

/**
 * @macro dir_term
 */
#define dir_term(r, t, c) { \
	int8_t d = (c).pdr[(t).p]; \
	(r).d2 = d; \
}

/**
 * @macro dir_prev
 * @brief set 2-bit direction flag in reverse access.
 */
#define dir_prev(r, t, c) { \
	int8_t d = (c).pdr[--(t).p]; \
/*	(r).d = 0x01 & (r).d2; */ \
	(r).d2 = 0x0f & (((r).d2<<2) | d); \
}

/**
 * macro aliasing
 */
#ifndef DP
	#warning "DP undefined"
#endif

static int32_t const _dp = DP;

#if DP == DYNAMIC
	#define dir_next 			dir_next_dynamic
	#define dir_check_term		dir_check_term_dynamic
#elif DP == GUIDED
	#define dir_next 			dir_next_guided
	#define dir_check_term 		dir_check_term_guided
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

static int32_t const _cost = COST;

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
#define fill_former_body	LABEL_WITH_SUFFIX(BASE, COST_SUFFIX, _fill_former_body)
#define fill_go_down		LABEL_WITH_SUFFIX(BASE, COST_SUFFIX, _fill_go_down)
#define fill_go_right		LABEL_WITH_SUFFIX(BASE, COST_SUFFIX, _fill_go_right)
#define fill_latter_body	LABEL_WITH_SUFFIX(BASE, COST_SUFFIX, _fill_latter_body)
#define fill_check_term		LABEL_WITH_SUFFIX(BASE, COST_SUFFIX, _fill_check_term)
#define fill_check_chain	LABEL_WITH_SUFFIX(BASE, COST_SUFFIX, _fill_check_chain)
#define fill_check_alt		LABEL_WITH_SUFFIX(BASE, COST_SUFFIX, _fill_check_alt)
#define fill_check_mem		LABEL_WITH_SUFFIX(BASE, COST_SUFFIX, _fill_check_mem)
#define fill_finish			LABEL_WITH_SUFFIX(BASE, COST_SUFFIX, _fill_finish)
#define chain_save_len		LABEL_WITH_SUFFIX(BASE, COST_SUFFIX, _chain_save_len)
#define chain_push_ivec		LABEL_WITH_SUFFIX(BASE, COST_SUFFIX, _chain_push_ivec)
#define search_terminal		LABEL_WITH_SUFFIX(BASE, COST_SUFFIX, _search_terminal)
#define search_trigger		LABEL_WITH_SUFFIX(BASE, COST_SUFFIX, _search_trigger)
#define search_max_score	LABEL_WITH_SUFFIX(BASE, COST_SUFFIX, _search_max_score)
#define trace_decl			LABEL_WITH_SUFFIX(BASE, COST_SUFFIX, _trace_decl)
#define trace_init			LABEL_WITH_SUFFIX(BASE, COST_SUFFIX, _trace_init)
#define trace_body			LABEL_WITH_SUFFIX(BASE, COST_SUFFIX, _trace_body)
#define trace_finish		LABEL_WITH_SUFFIX(BASE, COST_SUFFIX, _trace_finish)

#endif /* #ifndef _VARIANT_H_INCLUDED */

/**
 * end of variant.h
 */
