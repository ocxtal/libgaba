
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
#define _DP 			SEA_DYNAMIC
#define _SCORE 			SEA_LINEAR_GAP_COST

/**
 * sequence reader implementations
 */

/**
 * @macro rd_init
 * @brief initialize a sequence reader instance.
 */
#define rd_init(r, fp, base, spos) { \
	(r).p = (void *)base; \
	(r).spos = spos; \
	(r).b = 0; \
	(r).pop = fp; \
}

/**
 * @macro rd_fetch
 * @brief fetch a decoded base into r.b.
 */
#define rd_fetch(r, pos) { \
	(r).b = (r).pop((r).p, (r).spos + pos); \
}

/**
 * @macro rd_cmp
 * @brief compare two fetched bases.
 * @return true if two bases are the same, false otherwise.
 */
#define rd_cmp(r1, r2)	( (r1).b == (r2).b )

/**
 * @macro rd_decode
 * @brief get a cached char
 */
#define rd_decode(r)	( (r).b )
/**
 * @macro rd_close
 * @brief fill the instance with zero.
 */
#define rd_close(r) { \
	(r).p = NULL; \
	(r).spos = 0; \
	(r).b = 0; \
	(r).pop = NULL; \
}

/**
 * alignment writer implementations
 */

/**
 * @enum _ALN_CHAR
 * @brief alignment character, used in ascii output format.
 */
enum _ALN_CHAR {
	MCHAR = 'M',
	XCHAR = 'X',
	ICHAR = 'I',
	DCHAR = 'D'
};

/**
 * @macro wr_init
 * @brief initialize an alignment writer instance.
 */
#define wr_init(w, k) { \
	(w).p = NULL; \
	(w).pos = 0; \
	(w).pushm = (k)->f.pushm; \
	(w).pushx = (k)->f.pushx; \
	(w).pushi = (k)->f.pushi; \
	(w).pushd = (k)->f.pushd; \
}

/**
 * @macro wr_alloc
 * @brief allocate a memory for the alignment string.
 */
#define wr_alloc(w, size) { \
	if((w).p != NULL) { \
		free((w).p); (w).p = NULL; \
	} \
	(w).p = malloc(sizeof(uint8_t) * size); \
	(w).pos = size - 1; \
	(w).size = size; \
}

/**
 * @macro wr_pushm, wr_pushx, wr_pushi, wr_pushd
 * @brief push an alignment character
 */
#define wr_pushm(w) { \
	(w).pos = (w).pushm((w).p, (w).pos); \
}
#define wr_pushx(w) { \
	(w).pos = (w).pushx((w).p, (w).pos); \
}
#define wr_pushi(w) { \
	(w).pos = (w).pushi((w).p, (w).pos); \
}
#define wr_pushd(w) { \
	(w).pos = (w).pushd((w).p, (w).pos); \
}

/**
 * @macro wr_finish
 * @brief finish the instance (move the content of the array to the front)
 */
#define wr_finish(w) { \
	int64_t i, j; \
	for(i = (w).pos, j = 0; i < (w).size; i++, j++) { \
		(w).p[j] = (w).p[i]; \
	} \
	(w).p[j] = 0; \
}

/**
 * @macro wr_close
 * @brief free malloc'd memory and fill the instance with zero.
 */
#define wr_close(w) { \
	if((w).p != NULL) { \
		free((w).p); (w).p = NULL; \
	} \
	(w).pos = 0; \
	(w).pushm = (w).pushx = (w).pushi = (w).pushd = NULL; \
}


/**
 * direction determiner variants
 */
/** uncomment this line to preset direction variant */
//#define _DP 		SEA_DYNAMIC

/**
 * @enum _DIR
 * @brief constants for direction flags.
 */
enum _DIR {
	LEFT 	= 0,
	TOP 	= 0x01
};

/**
 * @enum _DIR2
 */
enum _DIR2 {
	LL = (LEFT<<0) | (LEFT<<1),
	LT = (LEFT<<0) | (TOP<<1),
	TL = (TOP<<0) | (LEFT<<1),
	TT = (TOP<<0) | (TOP<<1)
};

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
#define dir_init(r) { \
	(r).d = LEFT; \
	(r).d2 = LL; \
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
	(c).pdr[(c).p++] = (r).d; \
	(r).d2 = ((r).d<<1) | ((r).d2>>1); \
}

/**
 * @macro dir_next_guided
 * @brief set 2-bit direction flag from direction array.
 */
#define dir_next_guided(r, c) { \
	(r).d = (c).pdr[(c).p++]; \
	(r).d2 = ((r).d)<<1) | ((r).d2>>1); \
}

/**
 * @macro dir_term
 */
#define dir_term(r, c) { \
	int8_t d = (c).pdr[--(c).p]; \
	(r).d = LEFT; \
	(r).d2 = d; \
}

/**
 * @macro dir_prev
 * @brief set 2-bit direction flag in reverse access.
 */
#define dir_prev(r, c) { \
	int8_t d = (c).pdr[--(c).p]; \
	(r).d = 0x01 & (r).d2; \
	(r).d2 = 0x03 & (((r).d2<<1) | d); \
}

/**
 * macro aliasing
 */
#if _DP == SEA_DYNAMIC
	#define dir_next 			dir_next_dynamic
#elif _DP == SEA_GUIDED
	#define dir_next 			dir_next_guided
#else /* #if _DP == SEA_DYNAMIC */
	#error "_DP must be SEA_DYNAMIC or SEA_GUIDED."
#endif /* #if _DP == SEA_DYNAMIC */


/**
 * variant selector
 */
#define HEADER(base)			QUOTE(HEADER_WITH_SUFFIX(base, _impl.h))
#include HEADER(BASE)

#if _SCORE == SEA_LINEAR_GAP_COST

	#define VARIANT				_linear

#elif _SCORE == SEA_AFFINE_GAP_COST

	#define VARIANT				_affine

#else /* #if _SCORE == SEA_LINEAR_GAP_COST */

	#error "_SCORE must be SEA_LINEAR_GAP_COST or SEA_AFFINE_GAP_COST."

#endif /* #if _SCORE == SEA_LINEAR_GAP_COST */

#define bpl					FUNC_WITH_SUFFIX(BASE, VARIANT, _bpl)
#define dir_exp				FUNC_WITH_SUFFIX(BASE, VARIANT, _dir_exp)
#define fill_decl			FUNC_WITH_SUFFIX(BASE, VARIANT, _fill_decl)
#define fill_init			FUNC_WITH_SUFFIX(BASE, VARIANT, _fill_init)
#define fill_former_body	FUNC_WITH_SUFFIX(BASE, VARIANT, _fill_former_body)
#define fill_go_down		FUNC_WITH_SUFFIX(BASE, VARIANT, _fill_go_down)
#define fill_go_right		FUNC_WITH_SUFFIX(BASE, VARIANT, _fill_go_right)
#define fill_latter_body	FUNC_WITH_SUFFIX(BASE, VARIANT, _fill_latter_body)
#define fill_check_term		FUNC_WITH_SUFFIX(BASE, VARIANT, _fill_check_term)
#define fill_check_chain	FUNC_WITH_SUFFIX(BASE, VARIANT, _fill_check_chain)
#define fill_check_mem		FUNC_WITH_SUFFIX(BASE, VARIANT, _fill_check_mem)
#define fill_finish			FUNC_WITH_SUFFIX(BASE, VARIANT, _fill_finish)
#define chain_push_ivec		FUNC_WITH_SUFFIX(BASE, VARIANT, _chain_push_ivec)
#define search_terminal		FUNC_WITH_SUFFIX(BASE, VARIANT, _search_terminal)
#define search_max_score	FUNC_WITH_SUFFIX(BASE, VARIANT, _search_max_score)
#define trace_decl			FUNC_WITH_SUFFIX(BASE, VARIANT, _trace_decl)
#define trace_init			FUNC_WITH_SUFFIX(BASE, VARIANT, _trace_init)
#define trace_body			FUNC_WITH_SUFFIX(BASE, VARIANT, _trace_body)
#define trace_finish		FUNC_WITH_SUFFIX(BASE, VARIANT, _trace_finish)


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
