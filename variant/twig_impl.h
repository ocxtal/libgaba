
/**
 * @file twig.h
 * @brief macros for twig (8bit 16cell) algorithms
 */
#ifndef _TWIG_H_INCLUDED
#define _TWIG_H_INCLUDED

#include "../arch/b8c16.h"
#include "../sea.h"
#include "../util/util.h"
#include <stdint.h>
#include "naive_impl.h"
#include "branch_impl.h"

/**
 * @typedef cell_t
 * @brief cell type in the twig algorithms
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
 * @brief bandwidth in the twig algorithm (= 16).
 */
#ifdef BW
#undef BW
#define BW 			( 16 )
#endif

/**
 * @macro twig_linear_bpl
 * @brief calculates bytes per line
 */
#define twig_linear_bpl(c) 			( sizeof(cell_t) * BW )

/**
 * @macro (internal) twig_linear_topq, ...
 * @brief coordinate calculation helper macros
 */
#define twig_linear_topq(r, t, c)			naive_linear_topq(r, t, c)
#define twig_linear_leftq(r, t, c)			naive_linear_leftq(r, t, c)
#define twig_linear_topleftq(r, t, c)		naive_linear_topleftq(r, t, c)
#define twig_linear_top(r, t, c) 			naive_linear_top(r, t, c)
#define twig_linear_left(r, t, c) 			naive_linear_left(r, t, c)
#define twig_linear_topleft(r, t, c) 		naive_linear_topleft(r, t, c)

/**
 * @macro twig_linear_dir_exp
 * @brief determines the next direction of the lane in the dynamic algorithm.
 *
 * sseレジスタにアクセスできないといけない。
 */
#define twig_linear_dir_exp_top(r, t, c)		branch_linear_dir_exp_top(r, t, c)
#define twig_linear_dir_exp_bottom(r, t, c)		branch_linear_dir_exp_bottom(r, t, c)

/**
 * @macro twig_linear_fill_decl
 */
#define twig_linear_fill_decl(t, c, k, r)			branch_linear_fill_decl(t, c, k, r)

/**
 * @macro twig_linear_fill_init
 */
#define twig_linear_fill_init(t, c, k, r) { \
	branch_linear_fill_init_intl(t, c, k, r); \
	VEC_CHAR_SETZERO(wq); \
	VEC_CHAR_SETONES(wt); \
	for(t.q = 0; t.q < BW/2; t.q++) { \
		rd_fetch(c.a, t.i+t.q); \
		debug("a: %d", rd_decode(c.a)); \
		PUSHQ(rd_decode(c.a), wq); \
	} \
	for(t.q = 0; t.q < BW/2-1; t.q++) { \
		rd_fetch(c.b, t.j+t.q); \
		debug("b: %d", rd_decode(c.b)); \
		PUSHT(rd_decode(c.b), wt); \
	} \
}

/**
 * @macro twig_linear_fill_former_body
 */
#define twig_linear_fill_former_body(t, c, k, r)	branch_linear_fill_former_body(t, c, k, r)

/**
 * @macro twig_linear_fill_go_down
 */
#define twig_linear_fill_go_down(t, c, k, r)		branch_linear_fill_go_down(t, c, k, r)

/**
 * @macro twig_linear_fill_go_right
 */
#define twig_linear_fill_go_right(t, c, k, r)		branch_linear_fill_go_right(t, c, k, r)

/**
 * @macro twig_linear_fill_latter_body
 */
#define twig_linear_fill_latter_body(t, c, k, r)	branch_linear_fill_latter_body(t, c, k, r)

/**
 * @macro twig_linear_fill_check_term
 */
#define twig_linear_fill_check_term(t, c, k, r)		branch_linear_fill_check_term(t, c, k, r)

/**
 * @macro twig_linear_fill_check_chain
 */
#define twig_linear_fill_check_chain(t, c, k, r) 	branch_linear_fill_check_chain(t, c, k, r)

/**
 * @macro twig_linear_fill_check_alt
 */
#define twig_linear_fill_check_alt(t, c, k, r)		( 0 )

/**
 * @macro twig_linear_fill_check_mem
 */
#define twig_linear_fill_check_mem(t, c, k, r) ( \
	((cell_t *)c.pdp + 8*BW) > (cell_t *)c.dp.ep \
)

/**
 * @macro twig_linear_fill_finish
 */
#define twig_linear_fill_finish(t, c, k, r) { \
	if(k.alg != NW) { \
		VEC_STORE(c.pdp, maxv); \
		VEC_ASSIGN(tmp1, maxv); \
		for(t.q = 1; t.q < BW; t.q++) { \
			VEC_SHIFT_R(tmp1, tmp1); \
			VEC_MAX(maxv, tmp1, maxv); \
		} \
		t.max = VEC_LSB(maxv); \
	} \
	VEC_SET(zv, CELL_MIN); \
	VEC_STORE(c.pdp, zv); \
	VEC_STORE(c.pdp, pv); \
	VEC_STORE(c.pdp, zv); \
	VEC_STORE(c.pdp, v); \
	VEC_STORE(c.pdp, zv); \
}

/**
 * @macro twig_linear_chain_save_len
 */
#define twig_linear_chain_save_len(t, c, k)		( 5 * BW )

/**
 * @macro twig_linear_chain_push_ivec
 */
#define twig_linear_chain_push_ivec(t, c, k) { \
	t.i += BW; /** BW == 16 */ \
	t.j -= BW; \
	c.v.size = sizeof(cell_t); \
	c.v.clen = c.v.plen = 2*BW; \
	c.v.cv = (cell_t *)c.pdp - 5*BW/2; \
	c.v.pv = (cell_t *)c.pdp - 9*BW/2; \
}

/**
 * @macro twig_linear_search_terminal
 */
#define twig_linear_search_terminal(t, c, k) 		naive_linear_search_terminal(t, c, k)

/**
 * @macro twig_linear_search_trigger
 */
#define twig_linear_search_trigger(t1, t2, c, k)	naive_linear_search_trigger(t1, t2, c, k)

/**
 * @macro twig_linear_search_max_score
 */
#define twig_linear_search_max_score(t, c, k)		branch_linear_search_max_score(t, c, k)

/**
 * @macro twig_linear_trace_decl
 */
#define twig_linear_trace_decl(t, c, k, r)			naive_linear_trace_decl(t, c, k, r)

/**
 * @macro twig_linear_trace_init
 */
#define twig_linear_trace_init(t, c, k, r)			naive_linear_trace_init(t, c, k, r)

/**
 * @macro twig_linear_trace_body
 */
#define twig_linear_trace_body(t, c, k, r)			naive_linear_trace_body(t, c, k, r)

/**
 * @macro twig_linear_trace_finish
 */
#define twig_linear_trace_finish(t, c, k, r)		naive_linear_trace_finish(t, c, k, r)

#endif /* #ifndef _TWIG_H_INCLUDED */
/**
 * end of twig.h
 */
