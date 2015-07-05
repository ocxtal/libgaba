
/**
 * @file cap.h
 * @brief macros for cap algorithm
 */
#ifndef _CAP_H_INCLUDED
#define _CAP_H_INCLUDED

#include "../sea.h"
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
#define cap_linear_topq(r, t, c)		naive_linear_topq(r, t, c)
#define cap_linear_leftq(r, t, c)		naive_linear_leftq(r, t, c)
#define cap_linear_topleftq(r, t, c)	naive_linear_topleftq(r, t, c)
#define cap_linear_top(r, t, c) 		naive_linear_top(r, t, c)
#define cap_linear_left(r, t, c)		naive_linear_left(r, t, c)
#define cap_linear_topleft(r, t, c)		naive_linear_topleft(r, t, c)

/**
 * @macro cap_linear_dir_exp
 * @brief determines the direction in the dynamic algorithm
 */
#define cap_linear_dir_exp_top(r, t, c)	( (t.p & 0x01) == 0 ? SEA_TOP : SEA_LEFT )
#define cap_linear_dir_exp_bottom(r, t, c)	( 0 )

/**
 * @macro cap_linear_fill_decl
 */
#define cap_linear_fill_decl(t, c, k, r) \
	dir_t r; \
	int64_t lb, ub;

/**
 * @macro cap_linear_fill_init
 */
#define cap_linear_fill_init(t, c, k, r) { \
	naive_linear_fill_init(t, c, k, r); \
	c.alim = c.aep + BW - 1; \
	c.blim = c.bep + BW; \
}

/**
 * @macro cap_linear_fill_former_body
 */
#define cap_linear_fill_former_body(t, c, k, r)	naive_linear_fill_former_body(t, c, k, r)

/**
 * @macro cap_linear_fill_go_down
 */
#define cap_linear_fill_go_down(t, c, k, r)		naive_linear_fill_go_down(t, c, k, r)

/**
 * @macro cap_linear_fill_go_right
 */
#define cap_linear_fill_go_right(t, c, k, r)	naive_linear_fill_go_right(t, c, k, r)

/**
 * @macro cap_linear_fill_latter_body
 */
#define cap_linear_fill_latter_body(t, c, k, r) { \
	cell_t d, u, l; \
	lb = t.i-c.aep, ub = c.bep-t.j+1; \
	debug("fill: lb(%lld), ub(%lld)", lb, ub); \
	if(lb < -BW/2) { lb = -BW/2; } \
	if(ub > BW/2) { ub = BW/2; } \
	for(t.q = -BW/2; t.q < lb; t.q++) { \
		*((cell_t *)c.pdp) = k.min; \
		c.pdp += sizeof(cell_t); \
		debug("write"); \
	} \
	for(t.q = lb; t.q < ub; t.q++) { \
		debug("i(%lld), j(%lld), p(%lld), q(%lld), a(%lld), b(%lld)", t.i, t.j, t.p, t.q, (t.i-t.q)-1, (t.j+t.q)-1); \
		rd_fetch(c.a, (t.i-t.q)-1); \
		rd_fetch(c.b, (t.j+t.q)-1); \
		debug("d(%d), t(%d), l(%d)", ((cell_t *)c.pdp)[cap_linear_topleft(r, t, c)], ((cell_t *)c.pdp)[cap_linear_top(r, t, c)], ((cell_t *)c.pdp)[cap_linear_left(r, t, c)]); \
		d = ((cell_t *)c.pdp)[cap_linear_topleft(r, t, c)] + (rd_cmp(c.a, c.b) ? k.m : k.x); \
		u = ((cell_t *)c.pdp)[cap_linear_top(r, t, c)] + k.gi; \
		l = ((cell_t *)c.pdp)[cap_linear_left(r, t, c)] + k.gi; \
		if(t.q == -BW/2) { \
			if(dir(r) == LEFT) { u = CELL_MIN; } \
			if(dir2(r) == LL) { d = CELL_MIN; } \
		} else if(t.q == BW/2-1) { \
			if(dir(r) == TOP) { l = CELL_MIN; } \
			if(dir2(r) == TT) { d = CELL_MIN; } \
		} \
		*((cell_t *)c.pdp) = d = MAX4(d, u, l, k.min); \
		c.pdp += sizeof(cell_t); \
		if(k.alg != NW && d >= t.max) { \
			t.max = d; t.mi = t.i-t.q; t.mj = t.j+t.q; t.mp = t.p; t.mq = t.q; \
		} \
	} \
	for(t.q = ub; t.q < BW/2; t.q++) { \
		*((cell_t *)c.pdp) = k.min; \
		c.pdp += sizeof(cell_t); \
		debug("write"); \
	} \
}

/**
 * @macro cap_linear_fill_check_term
 */
#define cap_linear_fill_check_term(t, c, k, r) ( \
	lb >= ub - 1 \
)

/**
 * @macro cap_linear_fill_check_chain
 */
#define cap_linear_fill_check_chain(t, c, k, r)	( 0 )

/**
 * @macro cap_linear_fill_check_alt
 */
#define cap_linear_fill_check_alt(t, c, k, r)		( 0 )

/**
 * @macro cap_linear_fill_check_mem
 */
#define cap_linear_fill_check_mem(t, c, k, r)		naive_linear_fill_check_mem(t, c, k, r)

/**
 * @macro cap_linear_fill_finish
 */
#define cap_linear_fill_finish(t, c, k, r) { \
	stat = TERM; /** never reaches here */ \
}

/**
 * @macro cap_linear_chain_save_len(t, c, k)
 */
#define cap_linear_chain_save_len(t, c, k)			naive_linear_chain_save_len(t, c, k)

/**
 * @macro cap_linear_chain_push_ivec
 */
#define cap_linear_chain_push_ivec(t, c, k)			naive_linear_chain_push_ivec(t, c, k)

/**
 * @macro cap_linear_search_terminal
 */
#define cap_linear_search_terminal(t, c, k)			naive_linear_search_terminal(t, c, k)

/**
 * @macro cap_linear_search_trigger
 */
#define cap_linear_search_trigger(t1, t2, c, k)		naive_linear_search_trigger(t1, t2, c, k)

/**
 * @macro cap_linear_search_max_score
 */
#define cap_linear_search_max_score(t, c, k)		naive_linear_search_max_score(t, c, k)

/**
 * @macro cap_linear_trace_decl
 */
#define cap_linear_trace_decl(t, c, k, r)			naive_linear_trace_decl(t, c, k, r)

/**
 * @macro cap_linear_trace_init
 */
#define cap_linear_trace_init(t, c, k, r)			naive_linear_trace_init(t, c, k, r)

/**
 * @macro cap_linear_trace_body
 */
#define cap_linear_trace_body(t, c, k, r)			naive_linear_trace_body(t, c, k, r)

/**
 * @macro cap_linear_trace_finish
 */
#define cap_linear_trace_finish(t, c, k, r)			naive_linear_trace_finish(t, c, k, r)


#endif /* #ifndef _CAP_H_INCLUDED */
/**
 * end of cap.h
 */
