
/**
 * @file cap.h
 * @brief macros for cap algorithm
 */
#ifndef _CAP_H_INCLUDED
#define _CAP_H_INCLUDED

#include "../include/sea.h"
#include "../util/util.h"
#include <stdint.h>
#include "naive_impl.h"

/**
 * use the same bandwidth and cell type as the naive implementation.
 */

#define cap_linear_bpl(c)			naive_linear_bpl(c)
#define cap_affine_bpl(c)			naive_affine_bpl(c)

/**
 * @macro (internal) cap_linear_topq, ...
 * @brief coordinate calculation helper macros
 */
#define cap_linear_topq(r, c)		naive_linear_topq(r, c)
#define cap_linear_leftq(r, c)		naive_linear_leftq(r, c)
#define cap_linear_topleftq(r, c)	naive_linear_topleftq(r, c)
#define cap_linear_top(r, c) 		naive_linear_top(r, c)
#define cap_linear_left(r, c)		naive_linear_left(r, c)
#define cap_linear_topleft(r, c)	naive_linear_topleft(r, c)

/**
 * @macro cap_linear_dir_exp
 * @brief determines the direction in the dynamic algorithm
 */
#define cap_linear_dir_exp_top(r, c)	( (c.p & 0x01) == 0 ? SEA_TOP : SEA_LEFT )
#define cap_linear_dir_exp_bottom(r, c)	( 0 )

/**
 * @macro cap_linear_fill_decl
 */
#define cap_linear_fill_decl(c, k, r) \
	dir_t r; \
	int64_t lb, ub;

/**
 * @macro cap_linear_fill_init
 */
#define cap_linear_fill_init(c, k, r) { \
	naive_linear_fill_init(c, k, r); \
	c.alim = c.aep + BW - 1; \
	c.blim = c.bep + BW; \
}

/**
 * @macro cap_linear_fill_former_body
 */
#define cap_linear_fill_former_body(c, k, r)	naive_linear_fill_former_body(c, k, r)

/**
 * @macro cap_linear_fill_go_down
 */
#define cap_linear_fill_go_down(c, k, r)		naive_linear_fill_go_down(c, k, r)

/**
 * @macro cap_linear_fill_go_right
 */
#define cap_linear_fill_go_right(c, k, r)		naive_linear_fill_go_right(c, k, r)

/**
 * @macro cap_linear_fill_latter_body
 */
#define cap_linear_fill_latter_body(c, k, r) { \
	cell_t t, d, l; \
	lb = c.i-c.aep, ub = c.bep-c.j+1; \
	debug("fill: lb(%lld), ub(%lld)", lb, ub); \
	if(lb < -BW/2) { lb = -BW/2; } \
	if(ub > BW/2) { ub = BW/2; } \
	for(c.q = -BW/2; c.q < lb; c.q++) { \
		*((cell_t *)c.pdp) = k.min; \
		c.pdp += sizeof(cell_t); \
		debug("write"); \
	} \
	for(c.q = lb; c.q < ub; c.q++) { \
		rd_fetch(c.a, (c.i-c.q)-1); \
		rd_fetch(c.b, (c.j+c.q)-1); \
		debug("d(%d), t(%d), l(%d)", ((cell_t *)c.pdp)[cap_linear_topleft(r, c)], ((cell_t *)c.pdp)[cap_linear_top(r, c)], ((cell_t *)c.pdp)[cap_linear_left(r, c)]); \
		d = ((cell_t *)c.pdp)[cap_linear_topleft(r, c)] + (rd_cmp(c.a, c.b) ? k.m : k.x); \
		t = ((cell_t *)c.pdp)[cap_linear_top(r, c)] + k.gi; \
		l = ((cell_t *)c.pdp)[cap_linear_left(r, c)] + k.gi; \
		if(c.q == -BW/2) { \
			if(dir(r) == LEFT) { t = CELL_MIN; } \
			if(dir2(r) == LL) { d = CELL_MIN; } \
		} else if(c.q == BW/2-1) { \
			if(dir(r) == TOP) { t = CELL_MIN; } \
			if(dir2(r) == TT) { d = CELL_MIN; } \
		} \
		*((cell_t *)c.pdp) = d = MAX4(d, t, l, k.min); \
		c.pdp += sizeof(cell_t); \
		if(k.alg != NW && d >= c.max) { \
			c.max = d; c.mi = c.i-c.q; c.mj = c.j+c.q; c.mp = c.p; c.mq = c.q; \
		} \
	} \
	for(c.q = ub; c.q < BW/2; c.q++) { \
		*((cell_t *)c.pdp) = k.min; \
		c.pdp += sizeof(cell_t); \
		debug("write"); \
	} \
}

/**
 * @macro cap_linear_fill_check_term
 */
#define cap_linear_fill_check_term(c, k, r) ( \
	lb >= ub - 1 \
)

/**
 * @macro cap_linear_fill_check_chain
 */
#define cap_linear_fill_check_chain(c, k, r)	( 0 )

/**
 * @macro cap_linear_fill_check_alt
 */
#define cap_linear_fill_check_alt(c, k, r)		( 0 )

/**
 * @macro cap_linear_fill_check_mem
 */
#define cap_linear_fill_check_mem(c, k, r)		naive_linear_fill_check_mem(c, k, r)

/**
 * @macro cap_linear_fill_finish
 */
#define cap_linear_fill_finish(c, k, r) { \
	stat = TERM; /** never reaches here */ \
}

/**
 * @macro cap_linear_chain_save_len(c, k)
 */
#define cap_linear_chain_save_len(c, k)			naive_linear_chain_save_len(c, k)

/**
 * @macro cap_linear_chain_push_ivec
 */
#define cap_linear_chain_push_ivec(c, k)		naive_linear_chain_push_ivec(c, k)

/**
 * @macro cap_linear_search_terminal
 */
#define cap_linear_search_terminal(c, k, t)		naive_linear_search_terminal(c, k, t)

/**
 * @macro cap_linear_search_max_score
 */
#define cap_linear_search_max_score(c, k, t)	naive_linear_search_max_score(c, k, t)

/**
 * @macro cap_linear_trace_decl
 */
#define cap_linear_trace_decl(c, k, r)			naive_linear_trace_decl(c, k, r)

/**
 * @macro cap_linear_trace_init
 */
#define cap_linear_trace_init(c, k, r)			naive_linear_trace_init(c, k, r)

/**
 * @macro cap_linear_trace_body
 */
#define cap_linear_trace_body(c, k, r)			naive_linear_trace_body(c, k, r)

/**
 * @macro cap_linear_trace_finish
 */
#define cap_linear_trace_finish(c, k, r)		naive_linear_trace_finish(c, k, r)


#endif /* #ifndef _CAP_H_INCLUDED */
/**
 * end of cap.h
 */
