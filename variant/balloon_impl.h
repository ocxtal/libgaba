
/**
 * @file balloon.h
 * @brief macros for the balloon algorithm
 */
#ifndef _BALLOON_H_INCLUDED
#define _BALLOON_H_INCLUDED

#include "../include/sea.h"
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
#define balloon_linear_topq(r, c) 		naive_linear_topq(r, c)
#define balloon_linear_leftq(r, c) 		naive_linear_leftq(r, c)
#define balloon_linear_topleftq(r, c)	naive_linear_topleftq(r, c)
// #define balloon_linear_top(r, c)		( - eq - 1 + balloon_linear_topq(r, c) )
// #define balloon_linear_left(r, c)		( - eq - 1 + balloon_linear_leftq(r, c) )
// #define balloon_linear_topleft(r, c)	( - eq - peq - 2 + balloon_linear_topleftq(r, c) )

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
#define balloon_linear_dir_exp_top(r, c) ( \
	(pc[0] > max - k.tb) ? LEFT : TOP \
)
#define balloon_linear_dir_exp(r, c) 	balloon_linear_dir_exp_top(r, c)

/**
 * @macro balloon_linear_dir_exp_bottom
 */
#define balloon_linear_dir_exp_bottom(r, c) ( \
	(pc[eq-1] > max - k.tb) ? TOP : LEFT \
)

/**
 * @macro balloon_linear_fill_decl
 */
#define balloon_linear_fill_decl(c, k, r) \
	dir_t r, b; \
	cell_t *pp, *pc; \
	cell_t max = 0; \
	int32_t eq, mq = 0;

/**
 * @macro balloon_linear_fill_init
 */
#define balloon_linear_fill_init(c, k, r) { \
	c.alim = c.aep; \
	c.blim = c.bep - BW + 1; \
	dir_init(r, c.pdr[c.p]); \
	dir_init(b, c.pdr[c.p]); \
	pc = c.pdp; \
	for(c.q = 0, eq = c.v.plen; c.q < eq; c.q++) { \
		*((cell_t *)c.pdp) = _read(c.v.pv, c.q, c.v.size); \
		c.pdp += sizeof(cell_t); \
	} \
	*((cell_t *)c.pdp) = c.v.clen; \
	c.pdp += sizeof(cell_t); \
	balloon_linear_dir_next(r, b, c); \
	pp = pc; \
	pc = c.pdp; \
	for(c.q = 0, eq = c.v.clen; c.q < eq; c.q++) { \
		*((cell_t *)c.pdp) = _read(c.v.cv, c.q, c.v.size); \
		c.pdp += sizeof(cell_t); \
	} \
	*((cell_t *)c.pdp) = c.v.plen; \
	c.pdp += sizeof(cell_t); \
}

/**
 * @macro balloon_linear_fill_former_body
 */
#define balloon_linear_fill_former_body(c, k, r) { \
	balloon_linear_dir_next(r, b, c); \
}

/**
 * @macro balloon_linear_fill_go_down
 */
#define balloon_linear_fill_go_down(c, k, r) { \
	eq -= (dir(b) == LEFT); \
}

/**
 * @macro balloon_linear_fill_go_right
 */
#define balloon_linear_fill_go_right(c, k, r) { \
	eq += (dir(b) == TOP); \
}

/**
 * @macro balloon_linear_fill_latter_body
 */
#define balloon_linear_fill_latter_body(c, k, r) { \
	cell_t d, t, l; \
	max = CELL_MIN; \
	for(c.q = 0; c.q < eq; c.q++) { \
		rd_fetch(c.a, c.i-c.q); \
		rd_fetch(c.b, c.j+c.q); \
		d = pp[c.q + balloon_linear_topleftq(r, c)] + (rd_cmp(c.a, c.b) ? k.m : k.x); \
		t = pc[c.q + balloon_linear_topq(r, c)] + k.gi; \
		l = pc[c.q + balloon_linear_leftq(r, c)] + k.gi; \
		if(c.q == 0) { \
			if(dir(r) == LEFT) { t = CELL_MIN; } \
			if(dir2(r) == LL) { d = CELL_MIN; } \
		} else if(c.q == eq-1) { \
			if(dir(b) == TOP) { l = CELL_MIN; } \
			if(dir2(b) == TT) { d = CELL_MIN; } \
		} \
		*((cell_t *)c.pdp) = d = MAX4(d, t, l, k.min); \
		c.pdp += sizeof(cell_t); \
		if(k.alg != NW && d >= max) { \
			max = d; mq = c.q; \
		} \
	} \
	*((cell_t *)c.pdp) = eq; \
	c.pdp += sizeof(cell_t); \
	pp = pc; pc = (cell_t *)c.pdp; \
	if(k.alg != NW && max >= c.max) { \
		c.max = max; \
		c.mi = c.i-mq; c.mj = c.j+mq; \
		c.mp = c.p; c.mq = mq; \
	} \
}

/**
 * @macro balloon_linear_fill_check_term
 */
#define balloon_linear_fill_check_term(c, k, r) ( \
	k.alg == XSEA && c.max > max + k.tx \
)

/**
 * @macro balloon_linear_fill_check_chain
 */
#define balloon_linear_fill_check_chain(c, k, r)		( 0 )

/**
 * @macro balloon_linear_fill_check_alt
 */
#define balloon_linear_fill_check_alt(c, k, r) ( \
	eq < BW \
)

/**
 * @macro balloon_linear_fill_check_mem
 */
#define balloon_linear_fill_check_mem(c, k, r) ( \
	((cell_t *)c.pdp + eq + 2) > (cell_t *)c.dp.ep \
)

/**
 * @macro balloon_linear_fill_finish
 */
#define balloon_linear_fill_finish(c, k, r) { \
	/** blank */ \
}

/**
 * @macro balloon_linear_chain_save_len
 */
#define balloon_linear_chain_save_len(c, k) ( \
	(((cell_t *)c.pdp)[-1]) + (((cell_t *)c.pdp)[-1 - (((cell_t *)c.pdp)[-1])]) + 2 \
)

/**
 * @macro balloon_linear_chain_push_ivec
 */
#define balloon_linear_chain_push_ivec(c, k) { \
	c.p--; c.j--; /** always comes from top */ \
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
#define balloon_linear_search_terminal(c, k) { \
	c.mi = c.aep; \
	c.mj = c.bep; \
	c.mp = COP(c.mi, c.mj, BW) - COP(c.asp, c.bsp, BW); \
	c.mq = COQ(c.mi, c.mj, BW) - COQ(c.i, c.j, BW); \
}

/**
 * @macro balloon_linear_search_max_score
 */
#define balloon_linear_search_max_score(c, k) { \
	c.aep = c.mi; \
	c.bep = c.mj; \
}

/**
 * @macro balloon_linear_trace_decl
 */
#define balloon_linear_trace_decl(c, k, r) \
	dir_t r; \
	cell_t *p, *pp, *pc; \
	cell_t score;

/**
 * @macro balloon_linear_trace_init
 */
#define balloon_linear_trace_init(c, k, r) { \
	dir_term(r, c); \
	p = (cell_t *)c.pdp - ((cell_t *)c.pdp)[-1] - 1; \
	pc = p - p[-1] - 1; \
	pp = pc - pc[-1] - 1; \
	score = p[c.q]; \
	rd_fetch(c.a, c.i-1); \
	rd_fetch(c.b, c.j-1); \
}

/**
 * @macro balloon_linear_trace_body
 */
#define balloon_linear_trace_body(c, k, r) { \
	dir_prev(r, c); \
	cell_t diag = pp[c.q + balloon_linear_topleftq(r, c)]; \
	cell_t sc = rd_cmp(c.a, c.b) ? k.m : k.x; \
	cell_t h, v; \
	if(score == (diag + sc)) { \
		p = pp; pc = p - p[-1] - 1; pp = pc - pc[-1] - 1; \
		c.q += balloon_linear_topleftq(r, c); \
		dir_prev(r, c); \
		c.i--; rd_fetch(c.a, c.i-1); \
		c.j--; rd_fetch(c.b, c.j-1); \
		if(sc == k.m) { wr_pushm(c.l); } else { wr_pushx(c.l); } \
		if(k.alg == SW && score <= sc) { \
			return SEA_TERMINATED; \
		} \
		score = diag; \
	} else if(score == ((h = pc[c.q + balloon_linear_leftq(r, c)]) + k.gi)) { \
		p = pc; pc = pp; pp = pc - pc[-1] - 1; \
		c.q += balloon_linear_leftq(r, c); \
		c.i--; rd_fetch(c.a, c.i-1); \
		wr_pushd(c.l); \
		score = h; \
	} else if(score == ((v = pc[c.q + balloon_linear_topq(r, c)]) + k.gi)) { \
		p = pc; pc = pp; pp = pc - pc[-1] - 1; \
		c.q += balloon_linear_topq(r, c); \
		c.j--; rd_fetch(c.b, c.j-1); \
		wr_pushd(c.l); \
		score = v; \
	} else { \
		return SEA_ERROR_OUT_OF_BAND; \
	} \
}

/**
 * @macro balloon_linear_trace_finish
 */
#define balloon_linear_trace_finish(c, k, r) { \
	/** blank */ \
}

#endif /* #ifndef _BALLOON_H_INCLUDED */
/**
 * end of balloon.h
 */
