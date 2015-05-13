
/**
 * @file branch.h
 * @brief macros for branch (16bit 32cell) algorithms
 */
#ifndef _BRANCH_H_INCLUDED
#define _BRANCH_H_INCLUDED

#include "../arch/b16c32.h"
#include "../include/sea.h"
#include "../util/util.h"
#include <stdint.h>
#include "naive_impl.h"

/**
 * @typedef cell_t
 * @brief cell type in the branch algorithms
 */
#ifdef cell_t

	#undef cell_t
	#undef CELL_MIN
	#undef CELL_MAX

	#define cell_t 		int16_t
	#define CELL_MIN	( INT16_MIN )
	#define CELL_MAX	( INT16_MAX )

#endif

/**
 * @macro BW
 * @brief bandwidth in the branch algorithm (= 32).
 */
#ifdef BW
#undef BW
#define BW 			( 32 )
#endif

/**
 * @macro branch_linear_bpl
 * @brief calculates bytes per line
 */
#define branch_linear_bpl(c) 		( sizeof(cell_t) * BW )

/**
 * @macro (internal) branch_linear_topq, ...
 * @brief coordinate calculation helper macros
 */
#define branch_linear_topq(r, c)			naive_linear_topq(r, c)
#define branch_linear_leftq(r, c)			naive_linear_leftq(r, c)
#define branch_linear_topleftq(r, c)		naive_linear_topleftq(r, c)
#define branch_linear_top(r, c) 			naive_linear_top(r, c)
#define branch_linear_left(r, c) 			naive_linear_left(r, c)
#define branch_linear_topleft(r, c) 		naive_linear_topleft(r, c)

/**
 * @macro branch_linear_dir_exp
 * @brief determines the next direction of the lane in the dynamic algorithm.
 */
#define branch_linear_dir_exp(r, c) ( \
	(VEC_MSB(v) > VEC_LSB(v)) ? TOP : LEFT \
)

/**
 * @macro branch_linear_fill_decl
 */
#define branch_linear_fill_decl(c, k, r) \
	dir_t r; \
	DECLARE_VEC_CELL(mv);			/** (m, m, m, ..., m) */ \
	DECLARE_VEC_CELL(xv);			/** (x, x, x, ..., x) */ \
	DECLARE_VEC_CELL(gv);			/** (g, g, g, ..., g) */ \
	DECLARE_VEC_CHAR_REG(wq);		/** a buffer for seq.a */ \
	DECLARE_VEC_CHAR_REG(wt);		/** a buffer for seq.b */ \
	DECLARE_VEC_CELL_REG(v);		/** score vector */ \
	DECLARE_VEC_CELL_REG(pv);		/** previous score vector */ \
	DECLARE_VEC_CELL_REG(tmp1);		/** temporary */ \
	DECLARE_VEC_CELL_REG(tmp2);		/** temporary */ \
	DECLARE_VEC_CELL_REG(maxv);		/** a vector which holds maximum scores */ \
 	DECLARE_VEC_CELL_REG(zv);		/** zero vector for the SW algorithm */

/**
 * @macro branch_linear_fill_init
 */
#define branch_linear_fill_init(c, k, r) { \
	c.i -= BW/2; /** convert to the local coordinate*/ \
	c.j += BW/2; \
	c.alim = c.alen - BW/2; \
	c.blim = c.blen - BW/2; \
	dir_init(r, c.pdr[c.p]); \
	VEC_SET(mv, k.m); \
	VEC_SET(xv, k.x); \
	VEC_SET(gv, k.gi); \
	VEC_SET(maxv, CELL_MIN); \
	VEC_SET(zv, k.alg == SW ? 0 : CELL_MIN); \
	VEC_SET(v, CELL_MIN); \
	for(c.q = 0; c.q < c.v.clen; c.q++) { \
		VEC_SHIFT_R(v, v); \
		VEC_INSERT_MSB(v, _read(c.v.pv, c.q, c.v.size)); \
	} \
	VEC_STORE(c.pdp, v); \
	VEC_ASSIGN(pv, v); \
	VEC_SET(v, CELL_MIN); \
	for(c.q = 0; c.q < c.v.plen; c.q++) { \
		VEC_SHIFT_R(v, v); \
		VEC_INSERT_MSB(v, _read(c.v.cv, c.q, c.v.size)); \
	} \
	VEC_STORE(c.pdp, v); \
	VEC_CHAR_SETZERO(wq); \
	VEC_CHAR_SETONES(wt); \
	for(c.q = 0; c.q < BW/2; c.q++) { \
		rd_fetch(c.a, c.q); \
		PUSHQ(rd_decode(c.a), wq); \
	} \
	for(c.q = 0; c.q < BW/2-1; c.q++) { \
		rd_fetch(c.b, c.q); \
		PUSHT(rd_decode(c.b), wt); \
	} \
}

/**
 * @macro branch_linear_fill_former_body
 */
#define branch_linear_fill_former_body(c, k, r) { \
	debug("%d, %d, %d", VEC_MSB(v), VEC_CENTER(v), VEC_LSB(v)); \
	dir_next(r, c); \
}

/**
 * @macro branch_linear_fill_go_down
 */
#define branch_linear_fill_go_down(c, k, r) { \
	if(dir2(r) == TT) { \
		VEC_SHIFT_R(pv, pv); \
		VEC_INSERT_MSB(pv, CELL_MIN); \
	} \
	VEC_SHIFT_R(tmp2, v); \
	VEC_INSERT_MSB(tmp2, CELL_MIN); \
	rd_fetch(c.b, c.j+BW/2-1); \
	c.j++; \
	PUSHT(rd_decode(c.b), wt); \
}

/**
 * @macro branch_linear_fill_go_right
 */
#define branch_linear_fill_go_right(c, k, r) { \
	if(dir2(r) == LL) { \
		VEC_SHIFT_L(pv, pv); \
		VEC_INSERT_LSB(pv, CELL_MIN); \
	} \
	VEC_SHIFT_L(tmp2, v); \
	VEC_INSERT_LSB(tmp2, CELL_MIN); \
	rd_fetch(c.a, c.i+BW/2); \
	c.i++; \
	PUSHQ(rd_decode(c.a), wq); \
}

/**
 * @macro branch_linear_fill_latter_body
 */
#define branch_linear_fill_latter_body(c, k, r) { \
	VEC_CHAR_PRINT(stderr, wq); \
	VEC_CHAR_PRINT(stderr, wt); \
 	VEC_ADD(tmp1, v, gv); \
	VEC_ADD(tmp2, tmp2, gv); \
	VEC_MAX(tmp1, tmp1, tmp2); \
	VEC_COMPARE(tmp2, wq, wt); \
	VEC_SELECT(tmp2, xv, mv, tmp2); \
	VEC_ADD(tmp2, pv, tmp2); \
	VEC_MAX(tmp1, zv, tmp1); \
	VEC_MAX(tmp1, tmp1, tmp2); \
	VEC_ASSIGN(pv, v); \
	VEC_ASSIGN(v, tmp1); \
	VEC_MAX(maxv, maxv, v); \
	VEC_STORE(c.pdp, v); \
}

/**
 * @macro branch_linear_fill_check_term
 */
#define branch_linear_fill_check_term(c, k, r) ( \
	k.alg == XSEA && VEC_CENTER(v) + k.tx - VEC_CENTER(maxv) < 0 \
)

/**
 * @macro branch_linear_fill_check_chain
 */
#define branch_linear_fill_check_chain(c, k, r) ( \
	   (VEC_LSB(v) > VEC_CENTER(v) - k.tc) \
	|| (VEC_MSB(v) > VEC_CENTER(v) - k.tc) \
)

/**
 * @macro branch_linear_fill_check_mem
 */
#define branch_linear_fill_check_mem(c, k, r) ( \
	((cell_t *)c.pdp + 4*BW) > (cell_t *)c.dp.ep \
)

/**
 * @macro branch_linear_fill_finish
 */
#define branch_linear_fill_finish(c, k, r) { \
	if(k.alg != NW) { \
		VEC_STORE(c.pdp, maxv); \
		VEC_ASSIGN(tmp1, maxv); \
		for(c.q = 1; c.q < BW; c.q++) { \
			VEC_SHIFT_R(tmp1, tmp1); \
			VEC_MAX(maxv, tmp1, maxv); \
		} \
		VEC_STORE(c.pdp, maxv); \
	} \
}

/**
 * @macro branch_linear_chain_push_ivec
 */
#define branch_linear_chain_push_ivec(c) { \
	dir_t r; \
	c.i += BW/2; \
	c.j -= BW/2; \
	c.v.size = sizeof(cell_t); \
	c.v.clen = c.v.plen = BW; \
	c.v.cv = (cell_t *)c.pdp - 3*BW; \
	c.v.pv = (cell_t *)c.pdp - 4*BW; \
}

/**
 * @macro branch_linear_search_terminal
 */
#define branch_linear_search_terminal(c, k) 	naive_linear_search_terminal(c, k)

/**
 * @macro branch_linear_search_max_score
 */
#define branch_linear_search_max_score(c, k) { \
	int64_t i, j, p = c.p, q; \
	cell_t *pl = pb + ADDR(c.p+1, -BW/2, BW); \
	cell_t *pt; \
	cell_t max = *(pl + BW); \
	dir_t r; \
	c.mi = c.mj = c.mp = c.mq = 0; \
	if(max != CELL_MAX && max != 0) { \
		for(q = -BW/2; q < BW/2; q++, pl += BW) { \
			c.p = p; \
			dir_term(r, c); \
			if(*pl == max) { \
				i = c.i - q; \
				j = c.j + q; \
			} \
			for(pt = pl-BW; *pt != max; pt -= BW) { \
				dir_prev(r, c); \
				if(dir(r) == TOP) { j--; } else { i--; } \
			} \
			if(c.p > c.mp) { \
				c.mi = i; c.mj = j; \
				c.mp = c.p; c.mq = q; \
			} \
		} \
	} \
	c.alen = c.mi; \
	c.blen = c.mj; \
}

/**
 * @macro branch_linear_trace_decl
 */
#define branch_linear_trace_decl(c, k, r)		naive_linear_trace_decl(c, k, r)

/**
 * @macro branch_linear_trace_init
 */
#define branch_linear_trace_init(c, k, r)		naive_linear_trace_init(c, k, r)

/**
 * @macro branch_linear_trace_body
 */
#define branch_linear_trace_body(c, k, r)		naive_linear_trace_body(c, k, r)

/**
 * @macro branch_linear_trace_finish
 */
#define branch_linear_trace_finish(c, k, r)		naive_linear_trace_finish(c, k, r)

#endif /* #ifndef _BRANCH_H_INCLUDED */
/**
 * end of branch.h
 */
