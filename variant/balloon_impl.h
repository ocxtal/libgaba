
/**
 * @file balloon.h
 * @brief macros for the balloon algorithm
 */
#ifndef _BALLOON_H_INCLUDED
#define _BALLOON_H_INCLUDED

#include "../sea.h"
#include "../util/util.h"
#include "naive_impl.h"

/**
 * the cell_t and the BW macros are inherited from `naive.h'.
 */
// #define cell_t 		int32_t
// #define BW 			( 32 )

/**
 * @macro balloon_linear_bpl
 * @brief calculates bytes per lane
 */
#define balloon_linear_bpl(c) 			( sizeof(cell_t) * BW )
#define balloon_affine_bpl(c) 			( 3 * sizeof(cell_t) * BW )

/**
 * @macro balloon_linear_topq, ...
 * @brief coordinate conversion macros
 */
#define balloon_linear_topq(r, t, c) 		naive_linear_topq(r, t, c)
#define balloon_linear_leftq(r, t, c) 		naive_linear_leftq(r, t, c)
#define balloon_linear_topleftq(r, t, c)	naive_linear_topleftq(r, t, c)
// #define balloon_linear_top(r, t, c)		( - eq - 1 + balloon_linear_topq(r, t, c) )
// #define balloon_linear_left(r, t, c)		( - eq - 1 + balloon_linear_leftq(r, t, c) )
// #define balloon_linear_topleft(r, t, c)	( - eq - peq - 2 + balloon_linear_topleftq(r, t, c) )

/**
 * @macro (internal) balloon_linear_dir_next
 */
#define balloon_linear_dir_next(rt, rb, c) { \
	(rt).d = balloon_linear_dir_exp_top(rt, c); \
	(rb).d = balloon_linear_dir_exp_bottom(rb, c); \
	(c).pdr[(c).p++] = (rt).d; \
	(rt).d2 = ((rt).d<<1) | ((rt).d2>>1); \
	(rb).d2 = ((rb).d<<1) | ((rb).d2>>1); \
}

/**
 * @macro balloon_linear_dir_exp_top
 * @brief determine the next direction in the dynamic banded algorithm.
 */
#define balloon_linear_dir_exp_top(r, t, c) ( \
	(pc[0] > max - k.tb) ? SEA_UE_LEFT : SEA_UE_TOP \
)
#define balloon_linear_dir_exp(r, t, c) 	balloon_linear_dir_exp_top(r, t, c)

/**
 * @macro balloon_linear_dir_exp_bottom
 */
#define balloon_linear_dir_exp_bottom(r, t, c) ( \
	(pc[eq-1] > max - k.tb) ? SEA_LE_TOP : SEA_LE_LEFT \
)

/**
 * @macro balloon_linear_fill_decl
 */
#define balloon_linear_fill_decl(t, c, k, r) \
	dir_t r, b; \
	cell_t *pp, *pc; \
	cell_t max = 0; \
	int32_t eq, mq = 0;

/**
 * @macro balloon_linear_fill_init
 */
#define balloon_linear_fill_init(t, c, k, r) { \
	c.alim = c.aep; \
	c.blim = c.bep - BW + 1; \
	dir_init(r, c.pdr[t.p]); \
	dir_init(b, c.pdr[t.p]); \
	pc = c.pdp; \
	for(t.q = 0, eq = c.v.plen; t.q < eq; t.q++) { \
		*((cell_t *)c.pdp) = _read(c.v.pv, t.q, c.v.size); \
		c.pdp += sizeof(cell_t); \
	} \
	*((cell_t *)c.pdp) = c.v.clen; \
	c.pdp += sizeof(cell_t); \
	/*balloon_linear_dir_next(r, b, c); */ \
	dir_next(r, t, c); \
	pp = pc; \
	pc = c.pdp; \
	for(t.q = 0, eq = c.v.clen; t.q < eq; t.q++) { \
		*((cell_t *)c.pdp) = _read(c.v.cv, t.q, c.v.size); \
		c.pdp += sizeof(cell_t); \
	} \
	*((cell_t *)c.pdp) = c.v.plen; \
	c.pdp += sizeof(cell_t); \
}

/**
 * @macro balloon_linear_fill_former_body
 */
#define balloon_linear_fill_former_body(t, c, k, r) { \
	/*balloon_linear_dir_next(r, b, c); */ \
	dir_next(r, t, c); \
}

/**
 * @macro balloon_linear_fill_go_down
 */
#define balloon_linear_fill_go_down(t, c, k, r) { \
	eq -= (dir(b) == LEFT); \
}

/**
 * @macro balloon_linear_fill_go_right
 */
#define balloon_linear_fill_go_right(t, c, k, r) { \
	eq += (dir(b) == TOP); \
}

/**
 * @macro balloon_linear_fill_latter_body
 */
#define balloon_linear_fill_latter_body(t, c, k, r) { \
	cell_t d, u, l; \
	max = CELL_MIN; \
	for(t.q = 0; t.q < eq; t.q++) { \
		rd_fetch(c.a, t.i-t.q); \
		rd_fetch(c.b, t.j+t.q); \
		d = pp[t.q + balloon_linear_topleftq(r, t, c)] + (rd_cmp(c.a, c.b) ? k.m : k.x); \
		u = pc[t.q + balloon_linear_topq(r, t, c)] + k.gi; \
		l = pc[t.q + balloon_linear_leftq(r, t, c)] + k.gi; \
		if(t.q == 0) { \
			if(dir_ue(r) == LEFT) { u = CELL_MIN; } \
			if(dir2_ue(r) == LL) { d = CELL_MIN; } \
		} else if(t.q == eq-1) { \
			if(dir_le(b) == TOP) { l = CELL_MIN; } \
			if(dir2_le(b) == TT) { d = CELL_MIN; } \
		} \
		*((cell_t *)c.pdp) = d = MAX4(d, u, l, k.min); \
		c.pdp += sizeof(cell_t); \
		if(k.alg != NW && d >= max) { \
			max = d; mq = t.q; \
		} \
	} \
	*((cell_t *)c.pdp) = eq; \
	c.pdp += sizeof(cell_t); \
	pp = pc; pc = (cell_t *)c.pdp; \
	if(k.alg != NW && max >= t.max) { \
		t.max = max; \
		t.mi = t.i-mq; t.mj = t.j+mq; \
		t.mp = t.p; t.mq = mq; \
	} \
}

/**
 * @macro balloon_linear_fill_check_term
 */
#define balloon_linear_fill_check_term(t, c, k, r) ( \
	k.alg == XSEA && t.max > max + k.tx \
)

/**
 * @macro balloon_linear_fill_check_chain
 */
#define balloon_linear_fill_check_chain(t, c, k, r)		( 0 )

/**
 * @macro balloon_linear_fill_check_alt
 */
#define balloon_linear_fill_check_alt(t, c, k, r) ( \
	eq < BW \
)

/**
 * @macro balloon_linear_fill_check_mem
 */
#define balloon_linear_fill_check_mem(t, c, k, r) ( \
	((cell_t *)c.pdp + eq + 2) > (cell_t *)c.dp.ep \
)

/**
 * @macro balloon_linear_fill_finish
 */
#define balloon_linear_fill_finish(t, c, k, r) { \
	/** blank */ \
}

/**
 * @macro balloon_linear_chain_save_len
 */
#define balloon_linear_chain_save_len(t, c, k) ( \
	(((cell_t *)c.pdp)[-1]) + (((cell_t *)c.pdp)[-1 - (((cell_t *)c.pdp)[-1])]) + 2 \
)

/**
 * @macro balloon_linear_chain_push_ivec
 */
#define balloon_linear_chain_push_ivec(t, c, k) { \
	t.p--; t.j--; /** always comes from top */ \
	*((cell_t *)c.pdp) = BW; /** correct the lane width */ \
	c.pdp += sizeof(cell_t); \
	*((cell_t *)c.pdp-2) = CELL_MIN; /** fill the BW-th cell with min */ \
	c.v.size = sizeof(cell_t); \
	cell_t *p = c.pdp; \
	c.v.clen = p[-1]; \
	c.v.plen = p[-1 - p[-1]]; \
	c.v.cv = (cell_t *)c.pdp - c.v.clen - 1; \
	c.v.pv = (cell_t *)c.v.cv - c.v.plen - 1; \
}

/**
 * @macro balloon_linear_search_terminal
 */
#define balloon_linear_search_terminal(t, c, k)			naive_linear_search_terminal(t, c, k)

/**
 * @macro balloon_linear_search_trigger
 */
#define balloon_linear_search_trigger(t1, t2, c, k)		naive_linear_search_trigger(t1, t2, c, k)

/**
 * @macro balloon_linear_search_max_score
 */
#define balloon_linear_search_max_score(t, c, k) 		naive_linear_search_max_score(t, c, k)

/**
 * @macro balloon_linear_trace_decl
 */
#define balloon_linear_trace_decl(t, c, k, r) \
	dir_t r; \
	cell_t *p, *pp, *pc; \
	cell_t score;

/**
 * @macro balloon_linear_trace_init
 */
#define balloon_linear_trace_init(t, c, k, r) { \
	dir_term(r, t, c); \
	p = (cell_t *)c.pdp - ((cell_t *)c.pdp)[-1] - 1; \
	pc = p - p[-1] - 1; \
	pp = pc - pc[-1] - 1; \
	score = p[t.q]; \
	rd_fetch(c.a, t.i-1); \
	rd_fetch(c.b, t.j-1); \
}

/**
 * @macro balloon_linear_trace_body
 */
#define balloon_linear_trace_body(t, c, k, r) { \
	dir_prev(r, t, c); \
	cell_t diag = pp[t.q + balloon_linear_topleftq(r, t, c)]; \
	cell_t sc = rd_cmp(c.a, c.b) ? k.m : k.x; \
	cell_t h, v; \
	if(score == (diag + sc)) { \
		p = pp; pc = p - p[-1] - 1; pp = pc - pc[-1] - 1; \
		t.q += balloon_linear_topleftq(r, t, c); \
		dir_prev(r, t, c); \
		t.i--; rd_fetch(c.a, t.i-1); \
		t.j--; rd_fetch(c.b, t.j-1); \
		if(sc == k.m) { wr_pushm(t.l); } else { wr_pushx(t.l); } \
		if(k.alg == SW && score <= sc) { \
			return SEA_TERMINATED; \
		} \
		score = diag; \
	} else if(score == ((h = pc[t.q + balloon_linear_leftq(r, t, c)]) + k.gi)) { \
		p = pc; pc = pp; pp = pc - pc[-1] - 1; \
		t.q += balloon_linear_leftq(r, t, c); \
		t.i--; rd_fetch(c.a, t.i-1); \
		wr_pushd(t.l); \
		score = h; \
	} else if(score == ((v = pc[t.q + balloon_linear_topq(r, t, c)]) + k.gi)) { \
		p = pc; pc = pp; pp = pc - pc[-1] - 1; \
		t.q += balloon_linear_topq(r, t, c); \
		t.j--; rd_fetch(c.b, t.j-1); \
		wr_pushd(t.l); \
		score = v; \
	} else { \
		return SEA_ERROR_OUT_OF_BAND; \
	} \
}

/**
 * @macro balloon_linear_trace_finish
 */
#define balloon_linear_trace_finish(t, c, k, r) { \
	/** blank */ \
}

#endif /* #ifndef _BALLOON_H_INCLUDED */
/**
 * end of balloon.h
 */
