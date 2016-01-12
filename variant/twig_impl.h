
/**
 * @file twig.h
 * @brief macros for twig (8bit 16cell) algorithms
 */
#ifndef _TWIG_H_INCLUDED
#define _TWIG_H_INCLUDED

#include "twig_types.h"
#include "../sea.h"
#include "../util/util.h"
#include "../arch/arch_util.h"
#include <stdint.h>
#include "branch_impl.h"

/**
 * @macro twig_linear_dir_exp
 * @brief determines the next direction of the lane in the dynamic algorithm.
 */
#define twig_linear_dir_exp_top(r, k)		branch_linear_dir_exp_top(r, k)
#define twig_linear_dir_exp_bottom(r, k)	branch_linear_dir_exp_bottom(r, k)
#define twig_affine_dir_exp_top(r, k)		branch_affine_dir_exp_top(r, k)
#define twig_affine_dir_exp_bottom(r, k)	branch_affine_dir_exp_bottom(r, k)

/**
 * @macro twig_linear_fill_decl
 */
#define twig_linear_fill_decl(k)			branch_linear_fill_decl(k)
#define twig_affine_fill_decl(k)			branch_affine_fill_decl(k)

/**
 * @macro twig_linear_fill_init
 */
#define twig_linear_fill_init(k, pdp, sec) { \
	/** delegate to branch implementation */ \
	branch_linear_fill_init_intl(k, pdp, sec); \
	/** load vectors */ \
	debug("load vectors BW(%d)", BW); \
	joint_vec_t *s = (joint_vec_t *)_tail(pdp, v); \
	if(_tail(pdp, var) == 0) { \
		rd_load_init_16(k->r, k->rr); \
	} else if(_tail(pdp, var) == TWIG) { \
		rd_load_16_16(k->r, k->rr, &s->wa, &s->wb); \
	} \
	vec_load(&s->pv, pv); \
	vec_load(&s->cv, cv); \
	vec_store(cdp, pv); cdp += bpl(); \
	vec_store(cdp, cv); cdp += bpl(); \
	vec_store(cdp, maxv); cdp += bpl(); \
	/** store the first dr vector */ \
	dir_end_block(dr, cdp); \
	vec_print(cv); \
}

#if 0
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

#endif

/**
 * @macro twig_linear_fill_start
 */
#define twig_linear_fill_start(k) 			branch_linear_fill_start(k)
#define twig_linear_fill_start_cap(k)		branch_linear_fill_start_cap(k)

/**
 * @macro twig_linear_fill_body
 */
#define twig_linear_fill_body(k)			branch_linear_fill_body(k)
#define twig_linear_fill_body_cap(k)		branch_linear_fill_body_cap(k)

/**
 * @macro twig_linear_fill_empty_body
 */
#define twig_linear_fill_empty_body(k)		branch_linear_fill_empty_body(k)

/**
 * @macro twig_linear_fill_end
 */
#define twig_linear_fill_end(k)				branch_linear_fill_end(k)
#define twig_linear_fill_end_cap(k)			branch_linear_fill_end_cap(k)

/**
 * @macro twig_linear_fill_test_xdrop
 */
#define twig_linear_fill_test_xdrop(k)		branch_linear_fill_test_xdrop(k)
#define twig_linear_fill_test_xdrop_cap(k)	branch_linear_fill_test_xdrop_cap(k)

/**
 * @macro twig_linear_fill_test_bound
 */
#define twig_linear_fill_test_bound(k) 		branch_linear_fill_test_bound(k)
#define twig_linear_fill_test_bound_cap(k)	branch_linear_fill_test_bound_cap(k)

/**
 * @macro twig_linear_fill_test_mem
 */
#define twig_linear_fill_test_mem(k) 		branch_linear_fill_test_mem(k)
#define twig_linear_fill_test_mem_cap(k)	branch_linear_fill_test_mem_cap(k)

/**
 * @macro twig_linear_fill_test_chain
 */
#define twig_linear_fill_test_chain(k)		branch_linear_fill_test_chain(k)
#define twig_linear_fill_test_chain_cap(k)	branch_linear_fill_test_chain_cap(k)
#if 0
#define twig_linear_fill_test_chain(k) ( \
	(CELL_MAX / k->m)/*32*/ - p \
)
#define twig_linear_fill_test_chain_cap(k) ( \
	twig_linear_fill_test_chain(k) \
)
#endif

/**
 * @macro twig_linear_fill_check_term
 */
#define twig_linear_fill_check_term(k)		branch_linear_fill_check_term(k)
#define twig_linear_fill_check_term_cap(k)	branch_linear_fill_check_term_cap(k)

/**
 * @macro twig_linear_fill_finish
 */
#define twig_linear_fill_finish(k, pdp, sec)	branch_linear_fill_finish(k, pdp, sec)

#if 0
/**
 * @macro twig_linear_set_terminal
 */
#define twig_linear_set_terminal(k, pdp) 		branch_linear_set_terminal(k, pdp)
#endif

/**
 * @macro twig_linear_trace_decl
 */
#define twig_linear_trace_decl(k)			branch_linear_trace_decl(k)

/**
 * @macro twig_linear_trace_init
 */
#define twig_linear_trace_init(k, hdp, tdp, pdp)	branch_linear_trace_init(k, hdp, tdp, pdp)

/**
 * @macro twig_linear_trace_body
 */
#define twig_linear_trace_body(k)			branch_linear_trace_body(k)

/**
 * @macro twig_linear_trace_test_bound
 */
#define twig_linear_trace_test_bound(k)		branch_linear_trace_test_bound(k)

/**
 * @macro twig_linear_trace_check_term
 */
#define twig_linear_trace_check_term(k)		branch_linear_trace_check_term(k)

/**
 * @macro twig_linear_trace_finish
 */
#define twig_linear_trace_finish(k, hdp)	branch_linear_trace_finish(k, hdp)

#endif /* #ifndef _TWIG_H_INCLUDED */
/**
 * end of twig.h
 */
