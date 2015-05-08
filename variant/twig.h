
/**
 * @file twig.h
 * @brief macros for twig (8bit 16cell) algorithms
 */
#ifndef _TWIG_H_INCLUDED
#define _TWIG_H_INCLUDED

#include "sea.h"
#include "util.h"
#include "naive.h"
#include "branch.h"
#include "arch/b8c16.h"

/**
 * @typedef cell_t
 * @brief cell type in the twig algorithms
 */
#ifdef cell_t
#undef cell_t
#define cell_t 		int8_t
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
#define twig_linear_topq			naive_linear_topq
#define twig_linear_leftq			naive_linear_leftq
#define twig_linear_top 			naive_linear_top
#define twig_linear_left 			naive_linear_left
#define twig_linear_topleft 		naive_linear_topleft

/**
 * @macro twig_linear_dir_exp
 * @brief determines the next direction of the lane in the dynamic algorithm.
 *
 * sseレジスタにアクセスできないといけない。
 */
#define twig_linear_dir_exp(r, c)				branch_linear_dir_exp(r, c)

/**
 * @macro twig_linear_fill_decl
 */
#define twig_linear_fill_decl(c, k, r)			branch_linear_fill_decl(c, k, r)

/**
 * @macro twig_linear_fill_init
 */
#define twig_linear_fill_init(c, k, r)			branch_linear_fill_init(c, k, r)

/**
 * @macro twig_linear_fill_former_body
 */
#define twig_linear_fill_former_body(c, k, r)	branch_linear_fill_former_body(c, k, r)

/**
 * @macro twig_linear_fill_go_down
 */
#define twig_linear_fill_go_down(c, k, r)		branch_linear_fill_go_down(c, k, r)

/**
 * @macro twig_linear_fill_go_right
 */
#define twig_linear_fill_go_right(c, k, r)		branch_linear_fill_go_right(c, k, r)

/**
 * @macro twig_linear_fill_latter_body
 */
#define twig_linear_fill_latter_body(c, k, r)	branch_linear_fill_latter_body(c, k, r)

/**
 * @macro twig_linear_fill_check_term
 */
#define twig_linear_fill_check_term(c, k, r)	branch_linear_fill_check_term(c, k, r)

/**
 * @macro twig_linear_fill_check_chain
 */
#define twig_linear_fill_check_chain(c, k, r) 	branch_linear_fill_check_chain(c, k, r)

/**
 * @macro twig_linear_fill_check_mem
 */
#define twig_linear_fill_check_mem(c, k, r) ( \
	((cell_t *)c.pdp + 8*BW) > (cell_t *)c.dp.ep \
)

/**
 * @macro twig_linear_fill_finish
 */
#define twig_linear_fill_finish(c, k, r) { \
	if(ALG != NW) { \
		VEC_STORE(mat, maxv); \
		VEC_ASSIGN(tmp1, maxv); \
		for(i = 1; i < BW; i++) { \
			VEC_SHIFT_R(tmp1); \
			VEC_MAX(maxv, tmp1, maxv); \
		} \
		VEC_STORE(mat, maxv); \
	} \
	VEC_SET(zv, k.min); \
	VEC_STORE(mat, zv); \
	VEC_STORE(mat, pv); \
	VEC_STORE(mat, zv); \
	VEC_STORE(mat, p); \
	VEC_STORE(mat, zv); \
}

/**
 * @macro twig_linear_chain_push_ivec
 */
#define twig_linear_chain_push_ivec(c, k, r) { \
	dir_t r; \
	dir_term(r, c); \
	if(dir2(r) == TOP) { \
		c.j--; \
	} else { \
		c.i--; \
	} \
	v.size = sizeof(cell_t); \
	v.clen = v.plen = BW; \
	v.cv = (cell_t *)c.pdp - 5*BW/2; \
	v.pv = (cell_t *)c.pdp - 9*BW/2; \
}

/**
 * @macro twig_linear_search_terminal
 */
#define twig_linear_search_terminal(c, k) 		naive_linear_search_terminal(c, k)

/**
 * @macro twig_linear_search_max_score
 */
#define twig_linear_search_max_score(c, k) { \
	c.alen = c.mi; \
	c.blen = c.mj; \
}

/**
 * @macro twig_linear_trace_decl
 */
#define twig_linear_trace_decl(c, k, r)			naive_linear_trace_decl(c, k, r)

/**
 * @macro twig_linear_trace_init
 */
#define twig_linear_trace_init(c, k, r)			naive_linear_trace_init(c, k, r)

/**
 * @macro twig_linear_trace_body
 */
#define twig_linear_trace_body(c, k, r)			naive_linear_trace_body(c, k, r)

/**
 * @macro twig_linear_trace_finish
 */
#define twig_linear_trace_finish(c, k, r)		naive_linear_trace_finish(c, k, r)

#endif /* #ifndef _TWIG_H_INCLUDED */
/**
 * end of twig.h
 */
