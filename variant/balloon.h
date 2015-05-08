
/**
 * @file balloon.h
 * @brief macros for the balloon algorithm
 */
#ifndef _BALLOON_H_INCLUDED
#define _BALLOON_H_INCLUDED

#include "sea.h"
#include "util.h"
#include "naive.h"

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
#define balloon_linear_top(r, c)		
#define balloon_linear_left(r, c)
#define balloon_linear_topleft(r, c)

/**
 * @macro (internal) balloon_linear_dir_next
 */
#define balloon_linear_dir_next(rt, rb, c) { \
	(rt).d = balloon_linear_dir_exp_top(rt, c); \
	(rb).d = balloon_linear_dir_exp_bottom(rb, c); \
	(c).pdf[(c).p++] = (rt).d; \
	(rt).d2 = ((rt).d<<1) | ((rt).d2>>1); \
	(rb).d2 = ((rb).d<<1) | ((rb).d2>>1); \	
}

/**
 * @macro balloon_linear_dir_exp_top
 * @brief determine the next direction in the dynamic banded algorithm.
 */
#define balloon_linear_dir_exp_top(r, c) ( \
	(pc[0] > max - c.tb) ? LEFT : TOP \
)
#define balloon_linear_dir_exp(r, c) 	balloon_linear_dir_exp_top(r, c)

/**
 * @macro balloon_linear_dir_exp_bottom
 */
#define balloon_linear_dir_exp_bottom(r, c) { \
	(pc[eq-1] > max - c.tb) ? TOP : LEFT \
}

/**
 * @macro balloon_linear_fill_decl
 */
#define balloon_linear_fill_decl(c, k, r) \
	dir_t r, b; \
	cell_t *pp, *pc; \
	cell_t max; \
	int32_t eq, mq = 0;

/**
 * @macro balloon_linear_fill_init
 */
#define balloon_linear_fill_init(c, k, r) { \
	dir_init(r); \
	dir_init(b); \
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
	max = CELL_MIN; \
	for(c.q = 0; c.q < eq; c.q++) { \
		rd_fetch(c.a, c.i-c.q); \
		rd_fetch(c.b, c.j+c.q); \
		d = pp[balloon_linear_topleft(c, r)] + (rd_cmp(c.a, c.b) ? k.m : k.x); \
		t = pc[balloon_linear_top(c, r)] + k.gi; \
		l = pc[balloon_linear_left(c, r)] + k.gi; \
		if(c.q == 0) { \
			if(dir(r) == LEFT) { t = CELL_MIN; } \
			if(dir2(r) == LEFT) { d = CELL_MIN; } \
		} \
		if(c.q == eq-1) { \
			if(dir(b) = TOP) { l = CELL_MIN; } \
			if(dir2(b) = TT) { d = CELL_MIN; } \
		} \
		*((cell_t *)c.pdp) = d = MAX4(d, t, l, k.min); \
		c.pdp += sizeof(cell_t); \
		if(k.alg != NW && d >= max) { \
			max = d; mq = c.q; \
		} \
	} \
	*((cell_t *)c.pdp) = eq; \
	c.pdp += sizeof(cell_t); \
	if(k.alg != NW) && max >= c.max) { \
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
 * @macor balloon_linear_fill_check_chain
 */
#define balloon_linear_fill_check_chain(c, k, r) ( \
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
 * @macro balloon_linear_chain_push_ivec
 */
#define balloon_linear_chain_push_ivec(c, v) { \
	c.p--; c.j--; /** always comes from top */ \
	*((cell_t *)c.pdp) = BW; /** correct the lane width */ \
	c.pdp += sizeof(cell_t); \
	*((cell_t *)c.pdp-2) = CELL_MIN; /** fill BW-th cell with min */ \
	v.size = sizeof(cell_t); \
	v.clen = BW; \
	v.plen = BW; \
	v.cv = (cell_t *)c.pdp - BW - 1; \
	v.pv = (cell_t *)v.cv - BW - 1; \
}

/**
 * @macro balloon_linear_search_terminal
 */
#define balloon_linear_search_terminal(c, k) { \
}

/**
 * @macro balloon_linear_search_max_score
 */
#define balloon_linear_search_max_score(c, k) { \
}

/**
 * @macro balloon_linear_trace_decl
 */
#define balloon_linear_trace_decl(c, k, r)

/**
 * @macro balloon_linear_trace_init
 */
#define balloon_linear_trace_init(c, k, r) { \
}

/**
 * @macro balloon_linear_trace_body
 */
#define balloon_linear_trace_body(c, k, r) { \
}

/**
 * @macro balloon_linear_trace_finish
 */
#define balloon_linear_trace_finish(c, k, r) { \
}

#endif /* #ifndef _BALLOON_H_INCLUDED */
/**
 * end of balloon.h
 */
