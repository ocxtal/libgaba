
/**
 * @file branch.h
 * @brief macros for branch (16bit 32cell) algorithms
 */
#ifndef _BRANCH_H_INCLUDED
#define _BRANCH_H_INCLUDED

#include "../arch/b16c32.h"
#include "../sea.h"
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
#define branch_linear_topq(r, t, c)			naive_linear_topq(r, t, c)
#define branch_linear_leftq(r, t, c)		naive_linear_leftq(r, t, c)
#define branch_linear_topleftq(r, t, c)		naive_linear_topleftq(r, t, c)
#define branch_linear_top(r, t, c) 			naive_linear_top(r, t, c)
#define branch_linear_left(r, t, c) 		naive_linear_left(r, t, c)
#define branch_linear_topleft(r, t, c) 		naive_linear_topleft(r, t, c)

/**
 * @macro branch_linear_dir_exp
 * @brief determines the next direction of the lane in the dynamic algorithm.
 */
#define branch_linear_dir_exp_top(r, t, c) ( \
	(VEC_MSB(v) > VEC_LSB(v)) ? SEA_TOP : SEA_LEFT \
)
#define branch_linear_dir_exp_bottom(r, t, c) ( 0 )

/**
 * @macro branch_linear_fill_decl
 */
#define branch_linear_fill_decl(t, c, k, r) \
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
 * @macro (internal) branch_linear_fill_init_intl
 */
#define branch_linear_fill_init_intl(t, c, k, r) { \
	t.i -= BW/2; /** convert to the local coordinate*/ \
	t.j += BW/2; \
	c.alim = c.aep - BW/2; \
	c.blim = c.bep - BW/2 + 1; \
	dir_init(r, c.pdr[t.p]); \
	VEC_SET(mv, k.m); \
	VEC_SET(xv, k.x); \
	VEC_SET(gv, k.gi); \
	VEC_SET(maxv, CELL_MIN); \
	VEC_SET(zv, k.alg == SW ? 0 : CELL_MIN); \
	VEC_SET(v, CELL_MIN); \
	for(t.q = 0; t.q < c.v.clen; t.q++) { \
		VEC_SHIFT_R(v, v); \
		VEC_INSERT_MSB(v, _read(c.v.pv, t.q, c.v.size)); \
	} \
	VEC_STORE(c.pdp, v); \
	VEC_ASSIGN(pv, v); \
	VEC_SET(v, CELL_MIN); \
	for(t.q = 0; t.q < c.v.plen; t.q++) { \
		VEC_SHIFT_R(v, v); \
		VEC_INSERT_MSB(v, _read(c.v.cv, t.q, c.v.size)); \
	} \
	VEC_STORE(c.pdp, v); \
}

/**
 * @macro branch_linear_fill_init
 */
#define branch_linear_fill_init(t, c, k, r) { \
	branch_linear_fill_init_intl(t, c, k, r); \
	for(t.q = -BW/2; t.q < BW/2; t.q++) { \
		rd_fetch(c.a, t.i+t.q); \
		log("a: %d", rd_decode(c.a)); \
		PUSHQ(rd_decode(c.a), wq); \
	} \
	for(t.q = -BW/2; t.q < BW/2-1; t.q++) { \
		rd_fetch(c.b, t.j+t.q); \
		log("b: %d", rd_decode(c.b)); \
		PUSHT(rd_decode(c.b), wt); \
	} \
}

/**
 * @macro branch_linear_fill_former_body
 */
#define branch_linear_fill_former_body(t, c, k, r) { \
	debug("%d, %d, %d", VEC_MSB(v), VEC_CENTER(v), VEC_LSB(v)); \
	dir_next(r, t, c); \
}

/**
 * @macro branch_linear_fill_go_down
 */
#define branch_linear_fill_go_down(t, c, k, r) { \
	if(dir2(r) == TT) { \
		VEC_SHIFT_R(pv, pv); \
		VEC_INSERT_MSB(pv, CELL_MIN); \
	} \
	VEC_SHIFT_R(tmp2, v); \
	VEC_INSERT_MSB(tmp2, CELL_MIN); \
	rd_fetch(c.b, t.j+BW/2-1); \
	t.j++; \
	PUSHT(rd_decode(c.b), wt); \
}

/**
 * @macro branch_linear_fill_go_right
 */
#define branch_linear_fill_go_right(t, c, k, r) { \
	if(dir2(r) == LL) { \
		VEC_SHIFT_L(pv, pv); \
		VEC_INSERT_LSB(pv, CELL_MIN); \
	} \
	VEC_SHIFT_L(tmp2, v); \
	VEC_INSERT_LSB(tmp2, CELL_MIN); \
	rd_fetch(c.a, t.i+BW/2); \
	t.i++; \
	PUSHQ(rd_decode(c.a), wq); \
}

/**
 * @macro branch_linear_fill_latter_body
 */
#define branch_linear_fill_latter_body(t, c, k, r) { \
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
#define branch_linear_fill_check_term(t, c, k, r) ( \
	k.alg == XSEA && VEC_CENTER(v) + k.tx - VEC_CENTER(maxv) < 0 \
)

/**
 * @macro branch_linear_fill_check_chain
 */
#define branch_linear_fill_check_chain(t, c, k, r) ( \
	(t).p > CELL_MAX / (k).m \
)

/**
 * @macro branch_linear_fill_check_alt
 */
#define branch_linear_fill_check_alt(t, c, k, r) ( \
	   (VEC_LSB(v) > VEC_CENTER(v) - k.tc) \
	|| (VEC_MSB(v) > VEC_CENTER(v) - k.tc) \
)

/**
 * @macro branch_linear_fill_check_mem
 */
#define branch_linear_fill_check_mem(t, c, k, r) ( \
	((cell_t *)c.pdp + 4*BW) > (cell_t *)c.dp.ep \
)

/**
 * @macro branch_linear_fill_finish
 */
#define branch_linear_fill_finish(t, c, k, r) { \
	if(k.alg != NW) { \
		VEC_STORE(c.pdp, maxv); \
		VEC_ASSIGN(tmp1, maxv); \
		for(t.q = 1; t.q < BW; t.q++) { \
			VEC_SHIFT_R(tmp1, tmp1); \
			VEC_MAX(maxv, tmp1, maxv); \
		} \
		t.max = VEC_LSB(maxv); \
	} \
	VEC_STORE(c.pdp, pv); \
	VEC_STORE(c.pdp, v); \
}

/**
 * @macro branch_linear_chain_save_len
 */
#define branch_linear_chain_save_len(t, c, k)		naive_linear_chain_save_len(t, c, k)

/**
 * @macro branch_linear_chain_push_ivec
 */
#define branch_linear_chain_push_ivec(t, c, k) { \
	t.i += BW/2; \
	t.j -= BW/2; \
	c.v.size = sizeof(cell_t); \
	c.v.clen = c.v.plen = BW; \
	c.v.cv = (cell_t *)c.pdp - BW; \
	c.v.pv = (cell_t *)c.pdp - 2*BW; \
}

/**
 * @macro branch_linear_search_terminal
 */
#define branch_linear_search_terminal(t, c, k)	 	naive_linear_search_terminal(t, c, k)

/**
 * @macro branch_linear_search_trigger
 */
#define branch_linear_search_trigger(t1, t2, c, k)	naive_linear_search_trigger(t1, t2, c, k)

/**
 * @macro branch_linear_search_max_score
 */
#define branch_linear_search_max_score(t, c, k) { \
	debug("search start"); \
	int64_t i, j; \
	int64_t ep = t.p; \
	cell_t *pl = pb + ADDR(ep-sp+1, -BW/2, BW); \
	cell_t *pt; \
	dir_t r; \
	t.mp = 0; \
	for(t.q = -BW/2; t.q < BW/2; t.q++, pl++) { \
		debug("check pl(%p)", pl); \
		if(*pl != t.max) { continue; } \
		debug("found"); \
		t.p = ep; \
		dir_term(r, t, c); \
		i = t.i - t.q; \
		j = t.j + t.q; \
		for(pt = pl-BW; *pt != t.max && t.p > t.mp; pt -= BW) { \
			dir_prev(r, t, c); \
			if(dir(r) == TOP) { j--; } else { i--; } \
		} \
		if(t.p > t.mp) { \
			t.mi = i; t.mj = j; \
			t.mp = t.p; t.mq = t.q; \
		} \
	} \
}

/**
 * @macro branch_linear_trace_decl
 */
#define branch_linear_trace_decl(t, c, k, r)		naive_linear_trace_decl(t, c, k, r)

/**
 * @macro branch_linear_trace_init
 */
#define branch_linear_trace_init(t, c, k, r)		naive_linear_trace_init(t, c, k, r)

/**
 * @macro branch_linear_trace_body
 */
#define branch_linear_trace_body(t, c, k, r)		naive_linear_trace_body(t, c, k, r)

/**
 * @macro branch_linear_trace_finish
 */
#define branch_linear_trace_finish(t, c, k, r)		naive_linear_trace_finish(t, c, k, r)

#endif /* #ifndef _BRANCH_H_INCLUDED */
/**
 * end of branch.h
 */
