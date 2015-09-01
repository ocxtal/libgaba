
/**
 * @file naive.h
 * @brief macros for the naive algorithm
 */
#ifndef _NAIVE_H_INCLUDED
#define _NAIVE_H_INCLUDED

#include "../sea.h"
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
 * @macro BLK
 * @brief block split length
 */
#define BLK 		( 16 )

/**
 * @macro naive_linear_bpl, naive_affine_bpl
 * @brief calculates bytes per lane
 */
#define naive_linear_bpl(k)				( sizeof(cell_t) * BW )
#define naive_affine_bpl(k) 			( 3 * sizeof(cell_t) * BW )

/**
 * @macro (internal) naive_linear_topq, naive_linear_leftq, ...
 * @brief coordinate calculation helper macros
 */
#define naive_linear_topq(r, k)			( - (dir(r) == LEFT) )
#define naive_linear_leftq(r, k)		( dir(r) == TOP )
#define naive_linear_topleftq(r, k)		( - (dir2(r) == LL) + (dir2(r) == TT) )
#define naive_linear_top(r, k)			( -BW + naive_linear_topq(r, k) )
#define naive_linear_left(r, k)			( -BW + naive_linear_leftq(r, k) )
#define naive_linear_topleft(r, k)		( -2 * BW + naive_linear_topleftq(r, k) )

#define naive_affine_topq(r, k)			naive_linear_topq(r, k)
#define naive_affine_leftq(r, k)		naive_linear_leftq(r, k)
#define naive_affine_topleftq(r, k)		naive_linear_topleftq(r, k)
#define naive_affine_top(r, k)			( -3 * BW + naive_affine_topq(r, k) )
#define naive_affine_left(r, k)			( -3 * BW + naive_affine_leftq(r, k) )
#define naive_affine_topleft(r, k)		( -6 * BW + naive_affine_topleftq(r, k) )

/**
 * @macro naive_linear_dir_exp, naive_affine_dir_exp
 * @brief determines the next direction in the dynamic banded algorithm.
 */
#define naive_linear_dir_exp_top(r, k, pdp) ( \
	*((cell_t *)pdp-1) > *((cell_t *)pdp-BW) ? SEA_TOP : SEA_LEFT \
)
#define naive_linear_dir_exp_bottom(r, k, pdp) ( 0 )
#define naive_affine_dir_exp(r, k, pdp) ( \
	*((cell_t *)pdp-2*BW-1) > *((cell_t *)pdp-3*BW) ? SEA_TOP : SEA_LEFT \
)
#define naive_affine_dir_exp_bottom(r, k, pdp) ( 0 )

/**
 * @macro naive_linear_fill_decl
 */
#define naive_linear_fill_decl(k, r) \
	dir_t r; \
	cell_t xacc = 0; \
	int64_t i, j, p, q;

/**
 * @macro naive_affine_fill_decl
 */
#define naive_affine_fill_decl(k, r)		naive_linear_fill_decl(k, r)

/**
 * @macro naive_linear_fill_init
 * @brief initialize fill-in step context
 */
#define naive_linear_fill_init(k, r) { \
	/** load coordinates onto local stack */ \
	i = _ivec(pdp, i) - DEF_VEC_LEN/2;	/** len == 32 */ \
	j = _ivec(pdp, j) + DEF_VEC_LEN/2; \
	p = _ivec(pdp, p); \
	q = _ivec(pdp, q); \
	/** load the first 32bytes of the vectors */ \
	cell_t *s = (cell_t *)pdp, *t; \
	for(q = 0, t = _ivec(pdp, pv); q < BW; q++) { *s++ = *t++; } \
	for(q = 0, t = _ivec(pdp, cv); q < BW; q++) { *s++ = *t++; } \
	pdp = (uint8_t *)s; \
	/** initialize direction array */ \
	dir_init(r, k->pdr, p); \
}

/**
 * @macro naive_affine_fill_init
 */
#define naive_affine_fill_init(k, r) {}

/**
 * @macro naive_linear_fill_start
 */
#define naive_linear_fill_start(k, r) { \
	/** update direction pointer */ \
	dir_load_forward(r, k, pdp, p); \
	/*pdr = (uint8_t *)((cell_t *)pdp + BLK*BW) - p;*/ \
}

/**
 * @macro naive_linear_fill_former_body
 */
#define naive_linear_fill_former_body(k, r) { \
	/** calculate the next advancing direction */ \
	dir_next(r, k, pdp, p); \
}

/**
 * @macro naive_linear_fill_go_down
 */
#define naive_linear_fill_go_down(k, r) { \
	/** increment local coordinate */ \
	j++; \
}

/**
 * @macro naive_linear_fill_go_right
 */
#define naive_linear_fill_go_right(k, r) { \
	/** increment local coordinate */ \
	i++; \
}

/**
 * @macro naive_linear_fill_latter_body
 */
#define naive_linear_fill_latter_body(k, r) { \
	cell_t u, d, l; \
	xacc = 0; \
	for(q = -BW/2; q < BW/2; q++) { \
		rd_fetch(k->a, (i-q)-1); \
		rd_fetch(k->b, (j+q)-1); \
		d = ((cell_t *)pdp)[naive_linear_topleft(r, k)] + (rd_cmp(k->a, k->b) ? k->m : k->x); \
		u = ((cell_t *)pdp)[naive_linear_top(r, k)] + k->gi; \
		l = ((cell_t *)pdp)[naive_linear_left(r, k)] + k->gi; \
		if(q == -BW/2) { \
			if(dir(r) == LEFT) { u = CELL_MIN; } \
			if(dir2(r) == LL) { d = CELL_MIN; } \
		} else if(q == BW/2-1) { \
			if(dir(r) == TOP) { l = CELL_MIN; } \
			if(dir2(r) == TT) { d = CELL_MIN; } \
		} \
		*((cell_t *)pdp) = d = MAX4(d, u, l, k->min); \
		pdp += sizeof(cell_t); \
		if(k->alg != NW && d >= k->max) { \
			k->max = d; k->mi = i-q; k->mj = j+q; k->mp = p; k->mq = q; \
		} \
		xacc &= (d + k->tx - k->max); \
	} \
}

/**
 * @macro naive_linear_fill_end
 */
#define naive_linear_fill_end(k, r) { \
	/** nothing to do */ \
}

/**
 * @macro naive_linear_fill_test_xdrop
 * @brief returns negative if xdrop test passed (= when termination is detected)
 */
#define naive_linear_fill_test_xdrop(k, r) ( \
	(XSEA - k->alg - 1) & xacc \
)

/**
 * @macro naive_linear_fill_test_mem
 * @brief returns negative if i+BLK > aep OR j+BLK > bep OR pdp + block_memsize > tdp
 */
#define naive_linear_fill_test_mem(k, r) ( \
	  (k->aep-i-BLK) \
	| (k->bep-j-BLK) \
	| (k->tdp - pdp - BLK*(naive_linear_bpl(k)+1) - sizeof(struct sea_ivec)) \
)

/**
 * @macro naive_linear_fill_test_chain
 * @brief returns negative if chain test passed
 */
#define naive_linear_fill_test_chain(k, r) ( \
	0 /** never chain */ \
)

#if 0
/**
 * @macro naive_linear_fill_check_alt
 */
#define naive_linear_fill_check_alt(k, r) ( \
	   (*((cell_t *)c.pdp-BW) > *((cell_t *)c.pdp-BW/2) - k.tc) \
	|| (*((cell_t *)c.pdp-1)  > *((cell_t *)c.pdp-BW/2) - k.tc) \
)
#endif

/**
 * @macro naive_linear_fill_check_term
 * @brief returns false if the next loop can't be executed
 */
#define naive_linear_fill_check_term(k, r) ( \
	( naive_linear_fill_test_xdrop(k, r) \
	| naive_linear_fill_test_mem(k, r) \
	| naive_linear_fill_test_chain(k, r)) >= 0 \
)

/**
 * @macro naive_linear_fill_finish
 */
#define naive_linear_fill_finish(k, r) { \
	/** save p-coordinate at the beginning of the block */ \
	_ivec(k->pdp, max) = k->max; \
	_ivec(k->pdp, ep) = p; \
	/** retrieve chain vector pointers */ \
	int32_t *pv = (int32_t *)pdp - 2*BW, *cv = (int32_t *)pdp - BW; \
	/** create ivec at the end */ \
	pdp += sizeof(struct sea_ivec); \
	/** save terminal coordinates */ \
	_ivec(pdp, i) = i+DEF_VEC_LEN/2; \
	_ivec(pdp, j) = j-DEF_VEC_LEN/2; \
	_ivec(pdp, p) = p; \
	_ivec(pdp, q) = q; \
	_ivec(pdp, pv) = pv; \
	_ivec(pdp, cv) = cv; \
	_ivec(pdp, len) = BW;	/** fixed to 32 */ \
}

/**
 * @macro naive_linear_search_terminal
 * 無条件に(mi, mj)を上書きして良い。
 */
#define naive_linear_search_terminal(k, pdp) { \
	_ivec(k->pdp, i) = k->aep; \
	_ivec(k->pdp, j) = k->bep; \
	_ivec(k->pdp, p) = cop(k->mi-k->asp, k->mj-k->bsp, BW) \
		- cop(k->asp, k->bsp, BW); \
	_ivec(k->pdp, q) = coq(k->mi-k->asp, k->mj-k->bsp, BW) \
		- coq(_ivec(k->pdp, i)-k->asp, _ivec(k->pdp, j)-k->bsp, BW); \
}

/**
 * @macro naive_linear_search_trigger
 * @brief compare current max with global max
 */
#define naive_linear_search_trigger(k, pdp) ( \
	_ivec(pdp, max) > (k->max - (_ivec(pdp, max) > k->max)) \
)

/**
 * @macro naive_linear_search_max_score
 * tmaxを見て上書きするかどうか決める。
 */
#define naive_linear_search_max_score(k, pdp) { \
	/** no need to search */ \
}

/**
 * @macro naive_linear_trace_decl
 */
#define naive_linear_trace_decl(k, r, pdp) \
	dir_t r; \
	cell_t *ptb = (cell_t *)( \
		pdp + addr_linear( \
			_ivec(pdp, ep) - _ivec(pdp, p), \
			_ivec(pdp, q), \
			BLK, BW)); \
	cell_t score = *ptb; \
	int64_t i, j, p;

/**
 * @macro naive_linear_trace_init
 */
#define naive_linear_trace_init(k, r, pdp) { \
	i = _ivec(k->pdp, i) - DEF_VEC_LEN/2; \
	j = _ivec(k->pdp, j) + DEF_VEC_LEN/2; \
	p = _ivec(k->pdp, p); \
	dir_load_term(r, k, pdp, p); \
	debug("dir: d(%d), d2(%d)", dir(r), dir2(r)); \
	/*rd_fetch(c.a, t.i-1);*/ \
	/*rd_fetch(c.b, t.j-1);*/ \
}

/**
 * @macro naive_linear_trace_body
 */
#define naive_linear_trace_body(k, r, pdp) { \
	dir_load_backward(r, k, pdp, p); \
	rd_fetch(k->a, i-1); \
	rd_fetch(k->b, j-1); \
	debug("dir: d(%d), d2(%d)", dir(r), dir2(r)); \
	cell_t diag = ptb[naive_linear_topleft(r, k)]; \
	cell_t sc = rd_cmp(k->a, k->b) ? k->m : k->x; \
	cell_t h, v; \
	debug("traceback: score(%d), diag(%d), sc(%d), h(%d), v(%d)", \
		score, diag, sc, \
		ptb[naive_linear_left(r, k)], ptb[naive_linear_top(r, k)]); \
	if(score == (diag + sc)) { \
		ptb += naive_linear_topleft(r, k); \
		dir_load_backward(r, k, pdp, p); \
		i--; /*rd_fetch(k->a, i-1);*/ \
		j--; /*rd_fetch(k->b, j-1);*/ \
		if(sc == k->m) { wr_pushm(k->l); } else { wr_pushx(k->l); } \
		/*if(k->alg == SW && score <= sc) { return SEA_TERMINATED; }*/ \
		score = diag; \
	} else if(score == ((h = ptb[naive_linear_left(r, k)]) + k->gi)) { \
		ptb += naive_linear_left(r, k); \
		i--; /*rd_fetch(k->a, i-1);*/ \
		wr_pushd(k->l); \
		score = h; \
	} else if(score == ((v = ptb[naive_linear_top(r, k)]) + k->gi)) { \
		ptb += naive_linear_top(r, k); \
		j--; /*rd_fetch(k->b, j-1);*/ \
		wr_pushi(k->l); \
		score = v; \
	} else { \
		debug("out of band"); \
		return SEA_ERROR_OUT_OF_BAND; \
	} \
}

/**
 * @macro naive_linear_trace_test_bound
 * @brief returns negative if beginning bound is invaded
 */
#define naive_linear_trace_test_bound(k, r, pdp) ( \
	_ivec(pdp, p) - p \
)

/**
 * @macro naive_linear_trace_test_sw
 * @brief returns negative if sw traceback reached zero
 */
#define naive_linear_trace_test_sw(k, r, pdp) ( \
	((k->alg == SW) ? -1 : 0) & (score - 1) \
)

/**
 * @macro naive_linear_trace_check_term
 * @brief returns false if the next traceback loop can't be executed
 */
#define naive_linear_trace_check_term(k, r, pdp) ( \
	( naive_linear_trace_test_bound(k, r, pdp) \
	| naive_linear_trace_test_sw(k, r, pdp)) >= 0 \
)

/**
 * @macro naive_linear_trace_finish
 */
#define naive_linear_trace_finish(k, r, pdp) { \
	_ivec(pdp, i) = i; \
	_ivec(pdp, j) = j; \
	_ivec(pdp, p) = p; \
	_ivec(pdp, q) = ((uint64_t)ptb - (uint64_t)pdp + BW) % BW - BW/2; /** correct q-coordinate */ \
}

#endif /* #ifndef _NAIVE_H_INCLUDED */
/**
 * end of naive.h
 */
