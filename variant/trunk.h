
/**
 * @file trunk.h
 * @brief macros for trunk (8bit 32cell diff) algorithms
 */
#ifndef _TRUNK_H_INCLUDED
#define _TRUNK_H_INCLUDED

#include "sea.h"
#include "util.h"
#include "naive.h"
#include "arch/b16c32.h"

/**
 * @typedef cell_t
 * @brief cell type in the trunk algorithms
 */
#ifdef cell_t
#undef cell_t
#define cell_t 		int8_t
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
#define trunk_linear_dir_exp 		( VEC_MSB(v) > VEC_LSB(v) )

/**
 * @macro trunk_linear_fill_decl
 */
#define trunk_linear_fill_decl(c, k, r) \
	cell_vec_t mv;		/** (m, m, m, ..., m) */ \
	cell_vec_t xv;		/** (x, x, x, ..., x) */ \
	cell_vec_t gv;		/** (g, g, g, ..., g) */ \
	char_vec_t wq;		/** a buffer for seq.a */ \
	char_vec_t wt;		/** a buffer for seq.b */ \
	cell_vec_t v;		/** score vector */ \
	cell_vec_t pv;		/** previous score vector */ \
	cell_vec_t tmp1;	/** temporary */ \
	cell_vec_t tmp2;	/** temporary */ \
	cell_vec_t maxv;	/** a vector which holds maximum scores */ \
 	cell_vec_t zv;		/** zero vector for the SW algorithm */

/**
 * @macro trunk_linear_fill_init
 */
#define trunk_linear_fill_init(c, k, r) { \
	VEC_SET(mv, k.m); \
	VEC_SET(xv, k.x); \
	VEC_SET(gv, k.gi); \
	VEC_SET(maxv, CELL_MIN); \
	VEC_SET(zv, 0); \
	VEC_SET(v, CELL_MIN); \
	for(c.q = 0; c.q < c.v.clen; c.q++) { \
		VEC_SHIFT_R(v, v); \
		VEC_INSERT_MSB(v, _read(c.v.pv, c.q, c.v.size)); \
	} \
	dir_next(r, c); \
	VEC_STORE(c.pdp, v);
	VEC_ASSIGN(pv, v); \
	VEC_SET(v, CELL_MIN); \
	for(c.q = 0; c.q < c.v.plen; c.q++) { \
		VEC_SHIFT_R(v, v); \
		VEC_INSERT_MSB(v, _read(c.v.cv, c.q, c.v.size)); \
	} \
	VEC_STORE(c.pdp, v); \
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
	if(dir2(r) == TT) { \
		VEC_SHIFT_R(pv, pv); \
		VEC_INSERT_MSB(pv, CELL_MIN); \
	} \
	VEC_SHIFT_R(tmp2, v); \
	VEC_INSERT_MSB(tmp2, CELL_MIN); \
	c.j++; \
	rd_fetch(c.b, c.j+BW/4); \
	PUSHT(rd_decode(c.b)); \
}

/**
 * @macro trunk_linear_fill_go_right
 */
#define trunk_linear_fill_go_right(c, k, r) { \
	if(dir2(r) == LL) { \
		VEC_SHIFT_L(pv, pv); \
		VEC_INSERT_LSB(pv, CELL_MIN); \
	} \
	VEC_SHIFT_L(tmp2, v); \
	VEC_INSERT_LSB(tmp2, CELL_MIN); \
	c.i++; \
	rd_fetch(c.a, c.i+BW/4); \
	PUSHQ(rd_decode(c.a)); \
}

/**
 * @macro trunk_linear_fill_latter_body
 */
#define trunk_linear_fill_latter_body(c, k, r) { \
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
 * @macro trunk_linear_fill_check_term
 */
#define trunk_linear_fill_check_term(c, k, r) ( \
	ALG == XSEA && VEC_CENTER(v) + k.tx - VEC_CENTER(maxv) < 0 \
)

/**
 * @macro trunk_linear_fill_check_chain
 */
#define trunk_linear_fill_check_chain(c, k, r) ( \
	   (VEC_LSB(v) > VEC_CENTER(v) - k.tc) \
	|| (VEC_MSB(v) > VEC_CENTER(v) - k.tc) \
)

/**
 * @macro trunk_linear_fill_check_mem
 */
#define trunk_linear_fill_check_mem(c, k, r) ( \
	((cell_t *)c.pdp + 4*BW) > (cell_t *)c.dp.ep \
)

/**
 * @macro trunk_linear_fill_finish
 */
#define trunk_linear_fill_finish(c, k, r) { \
	if(ALG != NW) { \
		VEC_STORE(mat, maxv); \
		VEC_ASSIGN(tmp1, maxv); \
		for(i = 1; i < BW; i++) { \
			VEC_SHIFT_R(tmp1); \
			VEC_MAX(maxv, tmp1, maxv); \
		} \
		VEC_STORE(mat, maxv); \
	} \
}

/**
 * @macro trunk_linear_chain_push_ivec
 */
#define trunk_linear_chain_push_ivec(c, k, r) 	naive_linear_chain_push_ivec(c, k, r)

/**
 * @macro trunk_linear_search_terminal
 */
#define trunk_linear_search_terminal(c, k) 		naive_linear_search_terminal(c, k)

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
#define trunk_linear_trace_decl(c, k, r)		naive_linear_trace_decl(c, k, r)

/**
 * @macro trunk_linear_trace_init
 */
#define trunk_linear_trace_init(c, k, r)		naive_linear_trace_init(c, k, r)

/**
 * @macro trunk_linear_trace_body
 */
#define trunk_linear_trace_body(c, k, r)		naive_linear_trace_body(c, k, r)

/**
 * @macro trunk_linear_trace_finish
 */
#define trunk_linear_trace_finish(c, k, r)		naive_linear_trace_finish(c, k, r)

#endif /* #ifndef _TRUNK_H_INCLUDED */
/**
 * end of trunk.h
 */
