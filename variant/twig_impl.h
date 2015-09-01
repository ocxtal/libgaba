
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
 * @macro BLK
 * @brief block split length
 */
#ifdef BLK
#undef BLK
#define BLK 		( 16 )
#endif

/**
 * @macro twig_linear_bpl
 * @brief calculates bytes per line
 */
#define twig_linear_bpl(k) 					( sizeof(cell_t) * BW )
#define twig_affine_bpl(k)					( 3 * sizeof(cell_t) *BW )

/**
 * @macro (internal) twig_linear_topq, ...
 * @brief coordinate calculation helper macros
 */
#define twig_linear_topq(r, k)				naive_linear_topq(r, k)
#define twig_linear_leftq(r, k)				naive_linear_leftq(r, k)
#define twig_linear_topleftq(r, k)			naive_linear_topleftq(r, k)
#define twig_linear_top(r, k) 				naive_linear_top(r, k)
#define twig_linear_left(r, k) 				naive_linear_left(r, k)
#define twig_linear_topleft(r, k) 			naive_linear_topleft(r, k)

#define twig_affine_topq(r, k)				naive_affine_topq(r, k)
#define twig_affine_leftq(r, k)				naive_affine_leftq(r, k)
#define twig_affine_topleftq(r, k)			naive_affine_topleftq(r, k)
#define twig_affine_top(r, k) 				naive_affine_top(r, k)
#define twig_affine_left(r, k) 				naive_affine_left(r, k)
#define twig_affine_topleft(r, k) 			naive_affine_topleft(r, k)

/**
 * @macro twig_linear_dir_exp
 * @brief determines the next direction of the lane in the dynamic algorithm.
 *
 * sseレジスタにアクセスできないといけない。
 */
#define twig_linear_dir_exp_top(r, k, pdp)		branch_linear_dir_exp_top(r, k, pdp)
#define twig_linear_dir_exp_bottom(r, k, pdp)	branch_linear_dir_exp_bottom(r, k, pdp)
#define twig_affine_dir_exp_top(r, k, pdp)		branch_affine_dir_exp_top(r, k, pdp)
#define twig_affine_dir_exp_bottom(r, k, pdp)	branch_affine_dir_exp_bottom(r, k, pdp)

/**
 * @macro twig_linear_fill_decl
 */
#define twig_linear_fill_decl(k, r)				branch_linear_fill_decl(k, r)
#define twig_affine_fill_decl(k, r)				branch_affine_fill_decl(k, r)

/**
 * @macro twig_linear_fill_init
 */
#define twig_linear_fill_init(k, r) { \
	/** delegate to branch implementation */ \
	branch_linear_fill_init_intl(k, r); \
	/** initialize char vectors */ \
	VEC_CHAR_SETZERO(wq);	/** vector on the top */ \
	for(q = 0; q < BW/2; q++) { \
		rd_fetch(k->a, i+q); \
		debug("a: %d", rd_decode(k->a)); \
		PUSHQ(rd_decode(k->a), wq); \
	} \
	VEC_CHAR_SETONES(wt);	/** vector on the left */ \
	for(q = 0; q < BW/2-1; q++) { \
		rd_fetch(k->b, j+q); \
		debug("b: %d", rd_decode(k->b)); \
		PUSHT(rd_decode(k->b), wt); \
	} \
}

/**
 * @macro twig_linear_fill_start
 */
#define twig_linear_fill_start(k, r) 			branch_linear_fill_start(k, r)

/**
 * @macro twig_linear_fill_former_body
 */
#define twig_linear_fill_former_body(k, r)		branch_linear_fill_former_body(k, r)

/**
 * @macro twig_linear_fill_go_down
 */
#define twig_linear_fill_go_down(k, r)			branch_linear_fill_go_down(k, r)

/**
 * @macro twig_linear_fill_go_right
 */
#define twig_linear_fill_go_right(k, r)			branch_linear_fill_go_right(k, r)

/**
 * @macro twig_linear_fill_latter_body
 */
#define twig_linear_fill_latter_body(k, r)		branch_linear_fill_latter_body(k, r)

/**
 * @macro twig_linear_fill_end
 */
#define twig_linear_fill_end(k, r)				branch_linear_fill_end(k, r)

/**
 * @macro twig_linear_fill_test_xdrop
 */
#define twig_linear_fill_test_xdrop(k, r)		branch_linear_fill_test_xdrop(k, r)

/**
 * @macro twig_linear_fill_test_mem
 */
#define twig_linear_fill_test_mem(k, r) 		branch_linear_fill_test_mem(k, r)

/**
 * @macro twig_linear_fill_test_chain
 */
#define twig_linear_fill_test_chain(k, r) 		branch_linear_fill_test_chain(k, r)

#if 0
/**
 * @macro twig_linear_fill_check_alt
 */
#define twig_linear_fill_check_alt(k, r)		( 0 )
#endif

/**
 * @macro twig_linear_fill_check_term
 */
#define twig_linear_fill_check_term(k, r)		branch_linear_fill_check_term(k, r)

/**
 * @macro twig_linear_fill_finish
 */
#define twig_linear_fill_finish(k, r)			branch_linear_fill_finish(k, r)

/**
 * @macro twig_linear_search_terminal
 */
#define twig_linear_search_terminal(k, pdp) 	naive_linear_search_terminal(k, pdp)

/**
 * @macro twig_linear_search_trigger
 */
#define twig_linear_search_trigger(k, pdp)		naive_linear_search_trigger(k, pdp)

/**
 * @macro twig_linear_search_max_score
 */
#define twig_linear_search_max_score(k, pdp)	branch_linear_search_max_score(k, pdp)

/**
 * @macro twig_linear_trace_decl
 */
#define twig_linear_trace_decl(k, r, pdp)		naive_linear_trace_decl(k, r, pdp)

/**
 * @macro twig_linear_trace_init
 */
#define twig_linear_trace_init(k, r, pdp)		naive_linear_trace_init(k, r, pdp)

/**
 * @macro twig_linear_trace_body
 */
#define twig_linear_trace_body(k, r, pdp)		naive_linear_trace_body(k, r, pdp)

/**
 * @macro twig_lienar_trace_check_term
 */
#define twig_linear_trace_check_term(k, r, pdp)	naive_linear_trace_check_term(k, r, pdp)

/**
 * @macro twig_linear_trace_finish
 */
#define twig_linear_trace_finish(k, r, pdp)		naive_linear_trace_finish(k, r, pdp)

#endif /* #ifndef _TWIG_H_INCLUDED */
/**
 * end of twig.h
 */
