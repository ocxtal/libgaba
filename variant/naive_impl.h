
/**
 * @file naive.h
 * @brief macros for the naive algorithm
 */
#ifndef _NAIVE_H_INCLUDED
#define _NAIVE_H_INCLUDED

#include "../include/sea.h"
#include "../include/util.h"
#include <stdint.h>

/**
 * @macro cell_t
 * @brief cell type in the naive algorithms, alias to `(signed) int32_t'.
 */
#define cell_t 		int32_t
#define CELL_MIN	( INT32_MIN + 10 )
#define CELL_MAX	( INT32_MAX - 10 )

/**
 * @macro BW
 * @brief bandwidth in the naive algorithm
 */
#define BW 			( 32 )

/**
 * @macro naive_linear_bpl, naive_affine_bpl
 * @brief calculates bytes per lane
 */
#define naive_linear_bpl(c)			( sizeof(cell_t) * BW )
#define naive_affine_bpl(c) 		( 3 * sizeof(cell_t) * BW )

/**
 * @macro (internal) naive_linear_topq, naive_linear_leftq, ...
 * @brief coordinate calculation helper macros
 */
#define naive_linear_topq(r, c)		( - !dir(r) )
#define naive_linear_leftq(r, c)	( dir(r) )
#define naive_linear_topleftq(r, c)	( !dir(r) + (dir2(r)>>1) )
#define naive_linear_top(r, c)		( -BW + naive_linear_topq(r, c) )
#define naive_linear_left(r, c)		( -BW + naive_linear_leftq(r, c) )
#define naive_linear_topleft(r, c)	( -2 * BW + naive_linear_topleftq(r, c) )

#define naive_affine_topq(r, c)		naive_linear_topq(r, c)
#define naive_affine_leftq(r, c)	naive_linear_leftq(r, c)
#define naive_affine_topleftq(r, c)	naive_linear_topleftq(r, c)
#define naive_affine_top(r, c)		( -3 * BW + naive_affine_topq(r, c) )
#define naive_affine_left(r, c)		( -3 * BW + naive_affine_leftq(r, c) )
#define naive_affine_topleft(r, c)	( -6 * BW + naive_affine_topleftq(r, c) )

/**
 * @macro naive_linear_dir_exp, naive_affine_dir_exp
 * @brief determines the next direction in the dynamic banded algorithm.
 */
#define naive_linear_dir_exp(r, c) ( \
	*((cell_t *)c.pdp-1) > *((cell_t *)c.pdp-BW) ? TOP : LEFT \
)
#define naive_affine_dir_exp(r, c) ( \
	*((cell_t *)c.pdp-2*BW-1) > *((cell_t *)c.pdp-3*BW) ? TOP : LEFT \
)

/**
 * @macro naive_linear_fill_decl
 */
#define naive_linear_fill_decl(c, k, r) \
	dir_t r; \
	cell_t xacc = 0;

/**
 * @macro naive_linear_fill_init
 * @brief initialize fill-in step context
 */
#define naive_linear_fill_init(c, k, r) { \
	dir_init(r); \
	for(c.q = 0; c.q < c.v.clen; c.q++) { \
		*((cell_t *)c.pdp) = _read(c.v.pv, c.q, c.v.size); \
		c.pdp += sizeof(cell_t); \
	} \
	dir_next(r, c); \
	for(c.q = 0; c.q < c.v.plen; c.q++) { \
		*((cell_t *)c.pdp) = _read(c.v.cv, c.q, c.v.size); \
		c.pdp += sizeof(cell_t); \
	} \
}

/**
 * @macro naive_linear_fill_former_body
 */
#define naive_linear_fill_former_body(c, k, r) { \
	dir_next(r, c); \
}

/**
 * @macro naive_linear_fill_go_down
 */
#define naive_linear_fill_go_down(c, k, r) { \
	c.j++; \
}

/**
 * @macro naive_linear_fill_go_right
 */
#define naive_linear_fill_go_right(c, k, r) { \
	c.i++; \
}

/**
 * @macro naive_linear_fill_latter_body
 */
#define naive_linear_fill_latter_body(c, k, r) { \
	cell_t t, d, l; \
	xacc = 0; \
	for(c.q = -BW/2; c.q < BW/2; c.q++) { \
		rd_fetch(c.a, (c.i-c.q)-1); \
		rd_fetch(c.b, (c.j+c.q)-1); \
		d = ((cell_t *)c.pdp)[naive_linear_topleft(r, c)] + (rd_cmp(c.a, c.b) ? k.m : k.x); \
		t = ((cell_t *)c.pdp)[naive_linear_top(r, c)] + k.gi; \
		l = ((cell_t *)c.pdp)[naive_linear_left(r, c)] + k.gi; \
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
		xacc &= (d + k.tx - c.max); \
	} \
}

/**
 * @macro naive_linear_fill_check_term
 */
#define naive_linear_fill_check_term(c, k, r) ( \
	k.alg == XSEA && xacc < 0 \
)

/**
 * @macro naive_linear_fill_check_chain
 */
#define naive_linear_fill_check_chain(c, k, r) ( \
	   (*((cell_t *)c.pdp-BW) > *((cell_t *)c.pdp-BW/2) - k.tc) \
	|| (*((cell_t *)c.pdp-1)    > *((cell_t *)c.pdp-BW/2) - k.tc) \
)

/**
 * @macro naive_linear_fill_check_mem
 */
#define naive_linear_fill_check_mem(c, k, r) ( \
	((cell_t *)c.pdp + BW) > (cell_t *)c.dp.ep \
)

/**
 * @macro naive_linear_fill_finish
 */
#define naive_linear_fill_finish(c, k, r) { \
	/** blank */ \
}

/**
 * @macro naive_linear_chain_push_ivec
 */
#define naive_linear_chain_push_ivec(c, v) { \
	dir_t r; \
	dir_term(r, c); \
	if(dir2(r) == TOP) { \
		c.j--; \
	} else { \
		c.i--; \
	} \
	v.size = sizeof(cell_t); \
	v.clen = v.plen = BW; \
	v.cv = (cell_t *)c.pdp - BW; \
	v.pv = (cell_t *)c.pdp - 2*BW; \
}

/**
 * @macro naive_linear_search_terminal
 */
#define naive_linear_search_terminal(c, k) { \
	c.mi = c.alen; \
	c.mj = c.blen; \
	c.mp = COP(c.mi, c.mj, BW); \
	c.mq = COQ(c.mi, c.mj, BW) - COQ(c.i, c.j, BW); \
}

/**
 * @macro naive_linear_search_max_score
 */
#define naive_linear_search_max_score(c, k) { \
	c.alen = c.mi; \
	c.blen = c.mj; \
}

/**
 * @macro naive_linear_trace_decl
 */
#define naive_linear_trace_decl(c, k, r) \
	dir_t r; \
	cell_t *p = pb + ADDR(c.mp - sp, c.mq, BW); \
	cell_t score = *p;

/**
 * @macro naive_linear_trace_init
 */
#define naive_linear_trace_init(c, k, r) { \
	dir_term(r, c); \
	rd_fetch(c.a, c.mi-1); \
	rd_fetch(c.b, c.mj-1); \
}

/**
 * @macro naive_linear_trace_body
 */
#define naive_linear_trace_body(c, k, r) { \
	dir_prev(r, c); \
	cell_t diag = p[naive_linear_topleft(r, c)]; \
	cell_t sc = rd_cmp(c.a, c.b) ? k.m : k.x; \
	cell_t h, v; \
	if(score == (diag + sc)) { \
		p += naive_linear_topleft(r, c); \
		dir_prev(r, c); \
		c.mi--; rd_fetch(c.a, c.mi-1); \
		c.mj--; rd_fetch(c.b, c.mj-1); \
		if(sc == k.m) { wr_pushm(c.l); } else { wr_pushx(c.l); } \
		if(k.alg == SW && score <= sc) { \
			return SEA_TERMINATED; \
		} \
		score = diag; \
	} else if(score == ((h = p[naive_linear_left(r, c)]) + k.gi)) { \
		p += naive_linear_left(r, c); \
		c.mq += naive_linear_leftq(r, c); \
		c.mi--; rd_fetch(c.a, c.mi-1); \
		wr_pushd(c.l); \
		score = h; \
	} else if(score == ((v = p[naive_linear_top(r, c)]) + k.gi)) { \
		p += naive_linear_top(r, c); \
		c.mq += naive_linear_topq(r, c); \
		c.mj--; rd_fetch(c.b, c.mj-1); \
		wr_pushi(c.l); \
		score = v; \
	} else { \
		return SEA_ERROR_OUT_OF_BAND; \
	} \
}

/**
 * @macro naive_linear_trace_finish
 */
#define naive_linear_trace_finish(c, k, r) { \
	/** blank */ \
}

#endif /* #ifndef _NAIVE_H_INCLUDED */
/**
 * end of naive.h
 */
