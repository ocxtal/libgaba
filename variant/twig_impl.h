
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
#endif

#define cell_t 		int8_t
#define CELL_MIN	( INT8_MIN )
#define CELL_MAX	( INT8_MAX )

/**
 * @macro BW
 * @brief bandwidth in the twig algorithm (= 16).
 */
#ifdef BW
#undef BW
#endif

#define BW 			( 16 )

/**
 * @struct twig_linear_block
 */
struct twig_linear_block {
	cell_t dp[BLK][BW];
	int64_t i, j;
	_vec_cell(maxv);
#if DP == DYNAMIC
	_dir_vec(dr);
#endif
};

/**
 * @macro linear_block_t
 */
#ifdef linear_block_t
	#undef linear_block_t
#endif

#define linear_block_t	struct twig_linear_block

/**
 * @macro bpl, bpb
 * @brief bytes per line, bytes per block
 */
typedef struct twig_linear_block _linear_block;
#define twig_linear_bpl()					branch_linear_bpl()
#define twig_linear_dp_size()				branch_linear_dp_size()
#define twig_linear_co_size()				branch_linear_co_size()
#define twig_linear_jam_size()				branch_linear_jam_size()
#define twig_linear_head_size()				branch_linear_head_size()
#define twig_linear_tail_size()				branch_linear_tail_size()
#define twig_linear_bpb()					branch_linear_bpb()

/* typedef struct twig_affine_block _affine_block; */
#define twig_affine_bpl()					branch_affine_bpl()
#define twig_affine_dp_size()				branch_affine_dp_size()
#define twig_affine_co_size()				branch_affine_co_size()
#define twig_affine_jam_size()				branch_affine_jam_size()
#define twig_affine_head_size()				branch_affine_head_size()
#define twig_affine_tail_size()				branch_affine_tail_size()
#define twig_affine_bpb()					branch_affine_bpb()

/**
 * @macro (internal) twig_linear_topq, ...
 * @brief coordinate calculation helper macros
 */
#define twig_linear_topq(r, k)				branch_linear_topq(r, k)
#define twig_linear_leftq(r, k)				branch_linear_leftq(r, k)
#define twig_linear_topleftq(r, k)			branch_linear_topleftq(r, k)
#define twig_linear_top(r, k) 				branch_linear_top(r, k)
#define twig_linear_left(r, k) 				branch_linear_left(r, k)
#define twig_linear_topleft(r, k) 			branch_linear_topleft(r, k)

#define twig_affine_topq(r, k)				branch_affine_topq(r, k)
#define twig_affine_leftq(r, k)				branch_affine_leftq(r, k)
#define twig_affine_topleftq(r, k)			branch_affine_topleftq(r, k)
#define twig_affine_top(r, k) 				branch_affine_top(r, k)
#define twig_affine_left(r, k) 				branch_affine_left(r, k)
#define twig_affine_topleft(r, k) 			branch_affine_topleft(r, k)

/**
 * @macro twig_linear_dir_exp
 * @brief determines the next direction of the lane in the dynamic algorithm.
 *
 * sseレジスタにアクセスできないといけない。
 */
#define twig_linear_dir_exp_top(r, k, pdp)			branch_linear_dir_exp_top(r, k, pdp)
#define twig_linear_dir_exp_bottom(r, k, pdp)		branch_linear_dir_exp_bottom(r, k, pdp)
#define twig_affine_dir_exp_top(r, k, pdp)			branch_affine_dir_exp_top(r, k, pdp)
#define twig_affine_dir_exp_bottom(r, k, pdp)		branch_affine_dir_exp_bottom(r, k, pdp)

/**
 * @macro twig_linear_fill_decl
 */
#define twig_linear_fill_decl(k, r, pdp)			branch_linear_fill_decl(k, r, pdp)
#define twig_affine_fill_decl(k, r, pdp)			branch_affine_fill_decl(k, r, pdp)

/**
 * @macro twig_linear_fill_init
 */
#define twig_linear_fill_init(k, r, pdp) { \
	/** delegate to branch implementation */ \
	branch_linear_fill_init_intl(k, r, pdp); \
	/** initialize char vectors */ \
	vec_char_setzero(wq);	/** vector on the top */ \
	for(q = 0; q < BW/2; q++) { \
		rd_fetch(k->a, i+q); \
		debug("a: %d", rd_decode(k->a)); \
		pushq(rd_decode(k->a), wq); \
	} \
	vec_char_setones(wt);	/** vector on the left */ \
	for(q = 0; q < BW/2-1; q++) { \
		rd_fetch(k->b, j+q); \
		debug("b: %d", rd_decode(k->b)); \
		pusht(rd_decode(k->b), wt); \
	} \
}

/**
 * @macro twig_linear_fill_start
 */
#define twig_linear_fill_start(k, r, pdp) 			branch_linear_fill_start(k, r, pdp)

/**
 * @macro twig_linear_fill_former_body
 */
#define twig_linear_fill_former_body(k, r, pdp)		branch_linear_fill_former_body(k, r, pdp)
#define twig_linear_fill_former_body_cap(k, r, pdp)	branch_linear_fill_former_body_cap(k, r, pdp)

/**
 * @macro twig_linear_fill_go_down
 */
#define twig_linear_fill_go_down(k, r, pdp)			branch_linear_fill_go_down(k, r, pdp)
#define twig_linear_fill_go_down_cap(k, r, pdp)		branch_linear_fill_go_down_cap(k, r, pdp)

/**
 * @macro twig_linear_fill_go_right
 */
#define twig_linear_fill_go_right(k, r, pdp)		branch_linear_fill_go_right(k, r, pdp)
#define twig_linear_fill_go_right_cap(k, r, pdp)	branch_linear_fill_go_right_cap(k, r, pdp)

/**
 * @macro twig_linear_fill_latter_body
 */
#define twig_linear_fill_latter_body(k, r, pdp)		branch_linear_fill_latter_body(k, r, pdp)
#define twig_linear_fill_latter_body_cap(k, r, pdp)	branch_linear_fill_latter_body_cap(k, r, pdp)

/**
 * @macro twig_linear_fill_empty_body
 */
#define twig_linear_fill_empty_body(k, r, pdp)		branch_linear_fill_empty_body(k, r, pdp)

/**
 * @macro twig_linear_fill_end
 */
#define twig_linear_fill_end(k, r, pdp)				branch_linear_fill_end(k, r, pdp)

/**
 * @macro twig_linear_fill_test_xdrop
 */
#define twig_linear_fill_test_xdrop(k, r, pdp)		branch_linear_fill_test_xdrop(k, r, pdp)
#define twig_linear_fill_test_xdrop_cap(k, r, pdp)	branch_linear_fill_test_xdrop_cap(k, r, pdp)

/**
 * @macro twig_linear_fill_test_bound
 */
#define twig_linear_fill_test_bound(k, r, pdp) 		branch_linear_fill_test_bound(k, r, pdp)
#define twig_linear_fill_test_bound_cap(k, r, pdp)	branch_linear_fill_test_bound_cap(k, r, pdp)

/**
 * @macro twig_linear_fill_test_mem
 */
#define twig_linear_fill_test_mem(k, r, pdp) 		branch_linear_fill_test_mem(k, r, pdp)
#define twig_linear_fill_test_mem_cap(k, r, pdp)	branch_linear_fill_test_mem_cap(k, r, pdp)

#if 0
/**
 * @macro twig_linear_fill_test_chain
 */
#define twig_linear_fill_test_chain(k, r, pdp) 		branch_linear_fill_test_chain(k, r, pdp)
#define twig_linear_fill_test_chain_cap(k, r, pdp)	branch_linear_fill_test_chain_cap(k, r, pdp)
#endif

/**
 * @macro twig_linear_fill_test_chain
 */
#define twig_linear_fill_test_chain(k, r, pdp) ( \
	/*(CELL_MAX / k->m)*/ 32 - p \
)
#define twig_linear_fill_test_chain_cap(k, r, pdp) ( \
	twig_linear_fill_test_chain(k, r, pdp) \
)

/**
 * @macro twig_linear_fill_check_term
 */
#define twig_linear_fill_check_term(k, r, pdp)		branch_linear_fill_check_term(k, r, pdp)
#define twig_linear_fill_check_term_cap(k, r, pdp)	branch_linear_fill_check_term_cap(k, r, pdp)

/**
 * @macro twig_linear_fill_finish
 */
#define twig_linear_fill_finish(k, r, pdp)			branch_linear_fill_finish(k, r, pdp)

/**
 * @macro twig_linear_set_terminal
 */
#define twig_linear_set_terminal(k, pdp) 			branch_linear_set_terminal(k, pdp)

/**
 * @macro twig_linear_trace_decl
 */
#define twig_linear_trace_decl(k, r, pdp)			branch_linear_trace_decl(k, r, pdp)

/**
 * @macro twig_linear_trace_init
 */
#define twig_linear_trace_init(k, r, pdp)			branch_linear_trace_init(k, r, pdp)

/**
 * @macro twig_linear_trace_body
 */
#define twig_linear_trace_body(k, r, pdp)			branch_linear_trace_body(k, r, pdp)

/**
 * @macro twig_lienar_trace_check_term
 */
#define twig_linear_trace_check_term(k, r, pdp)		branch_linear_trace_check_term(k, r, pdp)

/**
 * @macro twig_linear_trace_finish
 */
#define twig_linear_trace_finish(k, r, pdp)			branch_linear_trace_finish(k, r, pdp)

#endif /* #ifndef _TWIG_H_INCLUDED */
/**
 * end of twig.h
 */
