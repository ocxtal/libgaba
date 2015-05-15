
/**
 * @file variant.h
 *
 * @brief macro implementations
 */
#ifndef _VARIANT_H_INCLUDED
#define _VARIANT_H_INCLUDED

#include "../include/sea.h"
#include "../util/util.h"

/**
 * defaults
 */
// #define DP 				SEA_DYNAMIC
// #define COST 			SEA_LINEAR_GAP_COST

/**
 * direction determiner variants
 */
/** uncomment this line to preset direction variant */
//#define _DP 		SEA_DYNAMIC

/**
 * @struct _dir
 * @brief direction flag container.
 */
struct _dir {
	uint8_t d, d2;
};

typedef struct _dir dir_t;

/**
 * @macro dir_init
 * @brief initialize _dir struct
 */
#define dir_init(r, dir) { \
	(r).d = (dir); \
	(r).d2 = (dir)<<1; \
}

/**
 * @macro dir
 * @brief extract the most recent direction flag
 */
#define dir(r)		( (r).d )

/**
 * @macro dir2
 * @brief extract the most recent 2-bit direction flag
 */
#define dir2(r) 	( (r).d2 )

/**
 * @macro dir_next_dynamic
 * @brief set 2-bit direction flag from external expression.
 *
 * @detail
 * dir_exp must be aliased to a direction determiner.
 */
#define dir_next_dynamic(r, c) { \
	(r).d = dir_exp(r, c); \
	(c).pdr[++(c).p] = (r).d; \
	(r).d2 = ((r).d<<1) | ((r).d2>>1); \
}

/**
 * @macro dir_next_guided
 * @brief set 2-bit direction flag from direction array.
 */
#define dir_next_guided(r, c) { \
	(r).d = (c).pdr[++(c).p]; \
	(r).d2 = ((r).d)<<1) | ((r).d2>>1); \
}

/**
 * @macro dir_term
 */
#define dir_term(r, c) { \
	int8_t d = (c).pdr[(c).mp]; \
	(r).d = LEFT; \
	(r).d2 = d; \
}

/**
 * @macro dir_prev
 * @brief set 2-bit direction flag in reverse access.
 */
#define dir_prev(r, c) { \
	int8_t d = (c).pdr[--(c).mp]; \
	(r).d = 0x01 & (r).d2; \
	(r).d2 = 0x03 & (((r).d2<<1) | d); \
}

/**
 * macro aliasing
 */
#if DP == SEA_DYNAMIC
	#define dir_next 			dir_next_dynamic
#elif DP == SEA_GUIDED
	#define dir_next 			dir_next_guided
#else /* #if DP == SEA_DYNAMIC */
	#error "DP must be SEA_DYNAMIC or SEA_GUIDED."
#endif /* #if DP == SEA_DYNAMIC */


/**
 * variant selector
 */
#define HEADER(base)			QUOTE(HEADER_WITH_SUFFIX(base, _impl.h))
#include HEADER(BASE)

#if COST == SEA_LINEAR_GAP_COST

	#define COST_SUFFIX 		_linear

#elif COST == SEA_AFFINE_GAP_COST

	#define COST_SUFFIX 		_affine

#else /* #if COST == SEA_LINEAR_GAP_COST */

	#error "COST must be SEA_LINEAR_GAP_COST or SEA_AFFINE_GAP_COST."

#endif /* #if COST == SEA_LINEAR_GAP_COST */

#define bpl					LABEL_WITH_SUFFIX(BASE, COST_SUFFIX, _bpl)
#define dir_exp				LABEL_WITH_SUFFIX(BASE, COST_SUFFIX, _dir_exp)
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
#define search_max_score	LABEL_WITH_SUFFIX(BASE, COST_SUFFIX, _search_max_score)
#define trace_decl			LABEL_WITH_SUFFIX(BASE, COST_SUFFIX, _trace_decl)
#define trace_init			LABEL_WITH_SUFFIX(BASE, COST_SUFFIX, _trace_init)
#define trace_body			LABEL_WITH_SUFFIX(BASE, COST_SUFFIX, _trace_body)
#define trace_finish		LABEL_WITH_SUFFIX(BASE, COST_SUFFIX, _trace_finish)


// #include "naive.h"
// #include "twig.h"
// #include "branch.h"
// #include "trunk.h"
// #include "balloon.h"
// #include "bulge.h"
// #include "cap.h"


#endif /* #ifndef _VARIANT_H_INCLUDED */

/**
 * end of variant.h
 */
