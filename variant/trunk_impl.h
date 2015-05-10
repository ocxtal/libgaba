
/**
 * @file trunk.h
 * @brief macros for trunk (8bit 32cell diff) algorithms
 */
#ifndef _TRUNK_H_INCLUDED
#define _TRUNK_H_INCLUDED

#include "../arch/b8c32.h"
#include "../include/sea.h"
#include "../util/util.h"
#include <stdint.h>
#include "naive_impl.h"
#include "branch_impl.h"

/**
 * @typedef cell_t
 * @brief cell type in the trunk algorithms
 */
#ifdef cell_t

	#undef cell_t
	#undef CELL_MIN
	#undef CELL_MAX

	#define cell_t 		int8_t
	#define CELL_MIN	( INT8_MIN )
	#define CELL_MAX	( INT8_MAX )

#endif

/**
 * @macro BW
 * @brief bandwidth in the trunk algorithm (= 32).
 */
#ifdef BW
#undef BW
#define BW 			( 32 )
#endif

/**
 * @macro trunk_linear_bpl
 * @brief calculates bytes per line
 */
#define trunk_linear_bpl(c) 		( sizeof(cell_t) * BW )

/**
 * @macro (internal) trunk_linear_topq, ...
 * @brief coordinate calculation helper macros
 */
#define trunk_linear_topq			naive_linear_topq
#define trunk_linear_leftq			naive_linear_leftq
#define trunk_linear_top 			naive_linear_top
#define trunk_linear_left 			naive_linear_left
#define trunk_linear_topleft 		naive_linear_topleft

/**
 * @macro trunk_linear_dir_exp
 * @brief determines the next direction of the lane in the dynamic algorithm.
 *
 * sseレジスタにアクセスできないといけない。
 */
#define trunk_linear_dir_exp(r, c)	( scu > scl )

/**
 * @macro trunk_linear_fill_decl
 */
#define trunk_linear_fill_decl(c, k, r) \
	dir_t r; \
	int32_t score, max, scu, scl; \
	DECLARE_VEC_CELL_REG(mggv); \
	DECLARE_VEC_CELL_REG(xggv); \
	DECLARE_VEC_CHAR_REG(wq); \
	DECLARE_VEC_CHAR_REG(wt); \
	DECLARE_VEC_CELL_REG(dv); \
	DECLARE_VEC_CELL_REG(dh); \
	DECLARE_VEC_CELL_REG(dv_); \
	DECLARE_VEC_CELL_REG(tmp);

/**
 * @macro trunk_linear_fill_init
 */
#define trunk_linear_fill_init(c, k, r) { \
	VEC_SET(mggv, k.m - 2*k.gi); \
	VEC_SET(xggv, k.x - 2*k.gi); \
	VEC_SET_LHALF(dv, k.m - 2*k.gi); \
	VEC_SET_UHALF(dh, k.m - 2*k.gi); \
	VEC_SHIFT_L(dv, dv); \
	VEC_STORE_DVDH(c.pdp, dv, dh); \
	max = 0; \
	score = 0; \
	scl = scu = (2*k.gi - k.m) * BW/2; \
	dir_next(r, c); \
}

/**
 * @macro trunk_linear_fill_former_body
 */
#define trunk_linear_fill_former_body(c, k, r) { \
	dir_next(r, c); \
}

/**
 * @macro trunk_linear_fill_go_down
 */
#define trunk_linear_fill_go_down(c, k, r) { \
	VEC_SHIFT_R(dv, dv); \
	c.j++; \
	rd_fetch(c.b, c.j+BW/2); \
	PUSHT(rd_decode(c.b), wt); \
}

/**
 * @macro trunk_linear_fill_go_right
 */
#define trunk_linear_fill_go_right(c, k, r) { \
	VEC_SHIFT_L(dh, dh); \
	c.i++; \
	rd_fetch(c.a, c.i+BW/2); \
	PUSHQ(rd_decode(c.a), wq); \
}

/**
 * @macro trunk_linear_fill_latter_body
 */
#define trunk_linear_fill_latter_body(c, k, r) { \
	VEC_COMPARE(tmp, wq, wt); \
	VEC_SELECT(tmp, xggv, mggv, tmp); \
	VEC_MAX(tmp, tmp, dv); \
	VEC_MAX(tmp, tmp, dh); \
	VEC_SUB(dv_, tmp, dh); \
	VEC_SUB(dh, tmp, dv); \
	VEC_ASSIGN(dv, dv_); \
	VEC_STORE_DVDH(c.pdp, dv, dh); \
	if(dir(r) == TOP) { \
		VEC_ASSIGN(tmp, dv); \
	} else { \
		VEC_ASSIGN(tmp, dh); \
	} \
	scl += (VEC_MSB(tmp) + k.gi); \
	scu += (VEC_LSB(tmp) + k.gi); \
	score += (VEC_CENTER(tmp) + k.gi); \
	if(k.alg != NW && score >= max) { \
		max = score; \
		c.mi = c.i; c.mj = c.j; \
		c.mp = COP(c.mi, c.mj, BW); c.mq = 0; \
	} \
}

/**
 * @macro trunk_linear_fill_check_term
 */
#define trunk_linear_fill_check_term(c, k, r) ( \
	k.alg == XSEA && score + k.tx - max < 0 \
)

/**
 * @macro trunk_linear_fill_check_chain
 */
#define trunk_linear_fill_check_chain(c, k, r) ( \
	   (scl > score - k.tc) \
	|| (scu > score - k.tc) \
)

/**
 * @macro trunk_linear_fill_check_mem
 */
#define trunk_linear_fill_check_mem(c, k, r) ( \
	((cell_t *)c.pdp + 2*BW) > (cell_t *)c.dp.ep \
)

/**
 * @macro trunk_linear_fill_finish
 */
#define trunk_linear_fill_finish(c, k, r) { \
}

/**
 * @macro trunk_linear_chain_push_ivec
 */
#define trunk_linear_chain_push_ivec(c, v) { \
}

/**
 * @macro trunk_linear_search_terminal
 */
#define trunk_linear_search_terminal(c, k) { \
}

/**
 * @macro trunk_linear_search_max_score
 */
#define trunk_linear_search_max_score(c, k) { \
	c.alen = c.mi; \
	c.blen = c.mj; \
}

/**
 * @macro trunk_linear_trace_decl
 */
#define trunk_linear_trace_decl(c, k, r) { \
}

/**
 * @macro trunk_linear_trace_init
 */
#define trunk_linear_trace_init(c, k, r) { \
}

/**
 * @macro trunk_linear_trace_body
 */
#define trunk_linear_trace_body(c, k, r) { \
}

/**
 * @macro trunk_linear_trace_finish
 */
#define trunk_linear_trace_finish(c, k, r) { \
}

#endif /* #ifndef _TRUNK_H_INCLUDED */
/**
 * end of trunk.h
 */
