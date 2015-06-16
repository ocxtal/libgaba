
/**
 * @file naive.h
 * @brief macros for the naive algorithm
 */
#ifndef _NAIVE_H_INCLUDED
#define _NAIVE_H_INCLUDED

#include "../include/sea.h"
#include "../util/util.h"
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
#define naive_linear_topq(r, t, c)		( - (dir(r) == LEFT) )
#define naive_linear_leftq(r, t, c)		( dir(r) == TOP )
#define naive_linear_topleftq(r, t, c)	( - (dir2(r) == LL) + (dir2(r) == TT) )
#define naive_linear_top(r, t, c)		( -BW + naive_linear_topq(r, t, c) )
#define naive_linear_left(r, t, c)		( -BW + naive_linear_leftq(r, t, c) )
#define naive_linear_topleft(r, t, c)	( -2 * BW + naive_linear_topleftq(r, t, c) )

#define naive_affine_topq(r, t, c)		naive_linear_topq(r, t, c)
#define naive_affine_leftq(r, t, c)		naive_linear_leftq(r, t, c)
#define naive_affine_topleftq(r, t, c)	naive_linear_topleftq(r, t, c)
#define naive_affine_top(r, t, c)		( -3 * BW + naive_affine_topq(r, t, c) )
#define naive_affine_left(r, t, c)		( -3 * BW + naive_affine_leftq(r, t, c) )
#define naive_affine_topleft(r, t, c)	( -6 * BW + naive_affine_topleftq(r, t, c) )

/**
 * @macro naive_linear_dir_exp, naive_affine_dir_exp
 * @brief determines the next direction in the dynamic banded algorithm.
 */
#define naive_linear_dir_exp_top(r, t, c) ( \
	*((cell_t *)c.pdp-1) > *((cell_t *)c.pdp-BW) ? SEA_TOP : SEA_LEFT \
)
#define naive_linear_dir_exp_bottom(r, t, c) ( 0 )
#define naive_affine_dir_exp(r, t, c) ( \
	*((cell_t *)c.pdp-2*BW-1) > *((cell_t *)c.pdp-3*BW) ? SEA_TOP : SEA_LEFT \
)
#define naive_affine_dir_exp_bottom(r, t, c) ( 0 )

/**
 * @macro naive_linear_fill_decl
 */
#define naive_linear_fill_decl(t, c, k, r) \
	dir_t r; \
	cell_t xacc = 0;

/**
 * @macro naive_affine_fill_decl
 */
#define naive_affine_fill_decl(t, c, k, r)		naive_linear_fill_decl(t, c, k, r)

/**
 * @macro naive_linear_fill_init
 * @brief initialize fill-in step context
 */
#define naive_linear_fill_init(t, c, k, r) { \
	t.i -= BW/2; \
	t.j += BW/2; \
	c.alim = c.aep - BW/2; \
	c.blim = c.bep - BW/2 + 1; \
	dir_init(r, c.pdr[t.p]); \
	for(t.q = 0; t.q < c.v.clen; t.q++) { \
		*((cell_t *)c.pdp) = _read(c.v.pv, t.q, c.v.size); \
		c.pdp += sizeof(cell_t); \
	} \
	for(t.q = 0; t.q < c.v.plen; t.q++) { \
		*((cell_t *)c.pdp) = _read(c.v.cv, t.q, c.v.size); \
		c.pdp += sizeof(cell_t); \
	} \
}

/**
 * @macro naive_affine_fill_init
 */
#define naive_affine_fill_init(t, c, k, r) {}

/**
 * @macro naive_linear_fill_former_body
 */
#define naive_linear_fill_former_body(t, c, k, r) { \
	dir_next(r, t, c); \
}

/**
 * @macro naive_linear_fill_go_down
 */
#define naive_linear_fill_go_down(t, c, k, r) { \
	t.j++; \
}

/**
 * @macro naive_linear_fill_go_right
 */
#define naive_linear_fill_go_right(t, c, k, r) { \
	t.i++; \
}

/**
 * @macro naive_linear_fill_latter_body
 */
#define naive_linear_fill_latter_body(t, c, k, r) { \
	cell_t u, d, l; \
	xacc = 0; \
	for(t.q = -BW/2; t.q < BW/2; t.q++) { \
		rd_fetch(c.a, (t.i-t.q)-1); \
		rd_fetch(c.b, (t.j+t.q)-1); \
		d = ((cell_t *)c.pdp)[naive_linear_topleft(r, t, c)] + (rd_cmp(c.a, c.b) ? k.m : k.x); \
		u = ((cell_t *)c.pdp)[naive_linear_top(r, t, c)] + k.gi; \
		l = ((cell_t *)c.pdp)[naive_linear_left(r, t, c)] + k.gi; \
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
		xacc &= (d + k.tx - t.max); \
	} \
}

/**
 * @macro naive_linear_fill_check_term
 */
#define naive_linear_fill_check_term(t, c, k, r) ( \
	k.alg == XSEA && xacc < 0 \
)

/**
 * @macro naive_linear_fill_check_chain
 */
#define naive_linear_fill_check_chain(t, c, k, r) ( \
	0 /** never chain */ \
)

/**
 * @macro naive_linear_fill_check_alt
 */
#define naive_linear_fill_check_alt(t, c, k, r) ( \
	   (*((cell_t *)c.pdp-BW) > *((cell_t *)c.pdp-BW/2) - k.tc) \
	|| (*((cell_t *)c.pdp-1)  > *((cell_t *)c.pdp-BW/2) - k.tc) \
)

/**
 * @macro naive_linear_fill_check_mem
 */
#define naive_linear_fill_check_mem(t, c, k, r) ( \
	((cell_t *)c.pdp + BW) > (cell_t *)c.dp.ep \
)

/**
 * @macro naive_linear_fill_finish
 */
#define naive_linear_fill_finish(t, c, k, r) { \
	/** blank */ \
}

/**
 * @macro naive_linear_chain_save_len
 */
#define naive_linear_chain_save_len(t, c, k)	( 2 * BW )

/**
 * @macro naive_linear_chain_push_ivec
 */
#define naive_linear_chain_push_ivec(t, c, k) { \
	t.i += BW/2; \
	t.j -= BW/2; \
	c.v.size = sizeof(cell_t); \
	c.v.clen = c.v.plen = BW; \
	c.v.cv = (cell_t *)c.pdp - BW; \
	c.v.pv = (cell_t *)c.pdp - 2*BW; \
}

/**
 * @macro naive_linear_search_terminal
 * 無条件に(mi, mj)を上書きして良い。
 */
#define naive_linear_search_terminal(t, c, k) { \
	t.mi = c.aep; \
	t.mj = c.bep; \
	t.mp = COP(t.mi-c.asp, t.mj-c.bsp, BW) - COP(c.asp, c.bsp, BW); \
	t.mq = COQ(t.mi-c.asp, t.mj-c.bsp, BW) - COQ(t.i-c.asp, t.j-c.bsp, BW); \
}

/**
 * @macro naive_linear_search_trigger
 */
#define naive_linear_search_trigger(t1, t2, c, k) ( \
	t1.max > t2.max \
)

/**
 * @macro naive_linear_search_max_score
 * tmaxを見て上書きするかどうか決める。
 */
#define naive_linear_search_max_score(t, c, k) { \
	/** no need to search */ \
}

/**
 * @macro naive_linear_trace_decl
 */
#define naive_linear_trace_decl(t, c, k, r) \
	dir_t r; \
	cell_t *p = pb + ADDR(t.p - sp, t.q, BW); \
	cell_t score = *p;

/**
 * @macro naive_linear_trace_init
 */
#define naive_linear_trace_init(t, c, k, r) { \
	dir_term(r, t, c); \
	debug("dir: d(%d), d2(%d)", dir(r), dir2(r)); \
	rd_fetch(c.a, t.i-1); \
	rd_fetch(c.b, t.j-1); \
}

/**
 * @macro naive_linear_trace_body
 */
#define naive_linear_trace_body(t, c, k, r) { \
	dir_prev(r, t, c); \
	debug("dir: d(%d), d2(%d)", dir(r), dir2(r)); \
	cell_t diag = p[naive_linear_topleft(r, t, c)]; \
	cell_t sc = rd_cmp(c.a, c.b) ? k.m : k.x; \
	cell_t h, v; \
	debug("traceback: score(%d), diag(%d), sc(%d), h(%d), v(%d)", \
		score, diag, sc, \
		p[naive_linear_left(r, t, c)], p[naive_linear_top(r, t, c)]); \
	if(score == (diag + sc)) { \
		p += naive_linear_topleft(r, t, c); \
		dir_prev(r, t, c); \
		t.i--; rd_fetch(c.a, t.i-1); \
		t.j--; rd_fetch(c.b, t.j-1); \
		if(sc == k.m) { wr_pushm(t.l); } else { wr_pushx(t.l); } \
		if(k.alg == SW && score <= sc) { \
			return SEA_TERMINATED; \
		} \
		score = diag; \
	} else if(score == ((h = p[naive_linear_left(r, t, c)]) + k.gi)) { \
		p += naive_linear_left(r, t, c); \
		t.i--; rd_fetch(c.a, t.i-1); \
		wr_pushd(t.l); \
		score = h; \
	} else if(score == ((v = p[naive_linear_top(r, t, c)]) + k.gi)) { \
		p += naive_linear_top(r, t, c); \
		t.j--; rd_fetch(c.b, t.j-1); \
		wr_pushi(t.l); \
		score = v; \
	} else { \
		debug("out of band"); \
		return SEA_ERROR_OUT_OF_BAND; \
	} \
}

/**
 * @macro naive_linear_trace_finish
 */
#define naive_linear_trace_finish(t, c, k, r) { \
	t.q = (p - pb + BW) % BW - BW/2; /** correct q-coordinate */ \
}

#endif /* #ifndef _NAIVE_H_INCLUDED */
/**
 * end of naive.h
 */
