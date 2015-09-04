
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
 * @macro naive_linear_bpl, naive_affine_bpl
 * @brief calculates bytes per lane
 */
#define naive_linear_bpl()				( sizeof(cell_t) * BW )
#define naive_linear_dp_size()			( BLK * naive_linear_bpl() )
#define naive_linear_co_size()			( 0 )
#define naive_linear_jam_size()			( naive_linear_co_size() + dr_size() )
#define naive_linear_phantom_size()		( 2 * naive_linear_bpl() + naive_linear_jam_size() )
#define naive_linear_bpb()				( naive_linear_dp_size() + naive_linear_jam_size() )

#define naive_affine_bpl() 				( 3 * sizeof(cell_t) * BW )
#define naive_affine_dp_size()			( BLK * naive_affine_bpl() )
#define naive_affine_co_size()			( 0 )
#define naive_affine_jam_size()			( naive_affine_co_size() + dr_size() )
#define naive_affine_phantom_size()		( 2 * naive_affine_bpl() + naive_affine_jam_size() )
#define naive_affine_bpb()				( naive_affine_dp_size() + naive_affine_jam_size() )

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
 * direction determiners
 */
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
 * fill-in
 */
/**
 * @macro naive_linear_fill_decl
 */
#define naive_linear_fill_decl(k, r, pdp) \
	dir_t r; \
	cell_t xacc = 0; \
	int64_t i, j, p, q;

/**
 * @macro naive_affine_fill_decl
 */
#define naive_affine_fill_decl(k, r, pdp)		naive_linear_fill_decl(k, r, pdp)

/**
 * @macro naive_linear_fill_init
 * @brief initialize fill-in step context
 */
#define naive_linear_fill_init(k, r, pdp) { \
	/** load coordinates onto local stack */ \
	p = _tail(k->pdp, p); \
	i = _tail(k->pdp, i) - DEF_VEC_LEN/2; \
	j = (p - 1) - (i - k->asp); \
	/** load the first direction pointer */ \
	dir_init(r, k, k->pdp, p); \
	/** make room for struct sea_joint_head */ \
	pdp += sizeof(struct sea_joint_head); \
	/** load the first 32bytes of the vectors */ \
	cell_t *s = (cell_t *)pdp, *t = _tail(k->pdp, v); \
	for(q = 0; q < BW; q++) { *s++ = *t++; } /** vector at sp-2 */ \
	for(q = 0; q < BW; q++) { *s++ = *t++; } /** vector at sp-1 */ \
	pdp = (uint8_t *)s; \
	/** write the first dr vector */ \
	dir_end_block(r, k, pdp, p); \
}

/**
 * @macro naive_affine_fill_init
 */
#define naive_affine_fill_init(k, r, pdp) {}

/**
 * @macro naive_linear_fill_start
 */
#define naive_linear_fill_start(k, r, pdp) { \
	/** nothing to do */ \
	dir_start_block(r, k, pdp, p); \
}

/**
 * @macro naive_linear_fill_former_body
 */
#define naive_linear_fill_former_body(k, r, pdp) { \
	/** nothing to do */ \
}

/**
 * @macro naive_linear_fill_go_down
 */
#define naive_linear_fill_go_down(k, r, pdp) { \
	/** increment local coordinate */ \
	j++; \
}

/**
 * @macro naive_linear_fill_go_right
 */
#define naive_linear_fill_go_right(k, r, pdp) { \
	/** increment local coordinate */ \
	i++; \
}

/**
 * @macro naive_linear_fill_latter_body
 */
#define naive_linear_fill_latter_body(k, r, pdp) { \
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
	/** calculate the next advancing direction */ \
	dir_load_forward(r, k, pdp, p);		/** increment p */ \
}

/**
 * @macro naive_linear_fill_end
 */
#define naive_linear_fill_end(k, r, pdp) { \
	/** store direction vector */ \
	dir_end_block(r, k, pdp, p); \
}

/**
 * @macro naive_linear_fill_test_xdrop
 * @brief returns negative if xdrop test passed (= when termination is detected)
 */
#define naive_linear_fill_test_xdrop(k, r, pdp) ( \
	(XSEA - k->alg - 1) & xacc \
)

/**
 * @macro naive_linear_fill_test_mem
 * @brief returns negative if i+BLK > aep OR j+BLK > bep OR pdp + block_memsize > tdp
 */
#define naive_linear_fill_test_mem(k, r, pdp) ( \
	  (k->aep-i-BLK) \
	| (k->bep-j-BLK) \
	| (k->tdp - pdp \
		- (naive_linear_bpb() \
		+ sizeof(struct sea_joint_tail) \
		+ sizeof(struct sea_joint_head))) \
)

/**
 * @macro naive_linear_fill_test_chain
 * @brief returns negative if chain test passed
 */
#define naive_linear_fill_test_chain(k, r, pdp) ( \
	0 /** never chain */ \
)

/**
 * @macro naive_linear_fill_check_term
 * @brief returns false if the next loop can't be executed
 */
#define naive_linear_fill_check_term(k, r, pdp) ( \
	( naive_linear_fill_test_xdrop(k, r, pdp) \
	| naive_linear_fill_test_mem(k, r, pdp) \
	| naive_linear_fill_test_chain(k, r, pdp)) >= 0 \
)

/**
 * @macro naive_linear_fill_finish
 */
#define naive_linear_fill_finish(k, r, pdp) { \
	/** retrieve chain vector pointers */ \
	uint8_t *v = pdp - 2*naive_linear_bpl() - naive_linear_jam_size(); \
	/** create ivec at the end */ \
	pdp += sizeof(struct sea_joint_tail); \
	/** save terminal coordinates */ \
	_tail(pdp, p) = p; \
	_tail(pdp, i) = i + DEF_VEC_LEN/2; \
	_tail(pdp, v) = v; \
	_tail(pdp, bpc) = 8*sizeof(cell_t); \
	_tail(pdp, d2) = dir_raw(r); \
	/** save p-coordinate at the beginning of the block */ \
	_head(k->pdp, max) = k->max; \
}

/**
 * @macro naive_linear_set_terminal
 */
#define naive_linear_set_terminal(k, pdp) { \
	int64_t i, j, p, sp, as, bs, ae, be; \
	/** load band terminal coordinate */ \
	i = _tail(k->pdp, i) - DEF_VEC_LEN/2; \
	j = _tail(k->pdp, j) + DEF_VEC_LEN/2; \
	p = _tail(k->pdp, p) - 1;	/** end p-coordinate */ \
	sp = _tail(pdp, p);			/** start p-coordinate */ \
	as = k->aep; bs = k->bsp; \
	ae = k->aep; be = k->bep; \
	/** check if the band reached the corner of the matrix */ \
	if((uint64_t)(cop(ae-as, be-bs, BW) - sp) <= (p - sp)) { \
		/** windback the direction */ \
		/** check if the band contains the corner */ \
		if((uint64_t)(coq(i, j, BW) - coq(ae, be, BW) + BW/2) < BW) { \
			k->mi = ae; k->mj = be; \
			k->mp = cop(ae-as, be-bs, BW); \
			k->mq = coq(ae-as, be-bs, BW) - coq(i-as, j-bs, BW); \
		} else { \
			/** the band did not reach the other end */ \
			k->mi = as; k->mj = bs; \
			k->mp = 0; k->mq = 0; \
		} \
	} \
}

/**
 * traceback
 */
/**
 * @macro naive_linear_trace_decl
 */
#define naive_linear_trace_decl(k, r, pdp) \
	dir_t r; \
	cell_t *ptb = (cell_t *)( \
		pdp + addr( \
			_tail(pdp, ep) - _tail(pdp, p), \
			_tail(pdp, q))); \
	cell_t score = *ptb; \
	int64_t i, j, p;

/**
 * @macro naive_linear_trace_init
 */
#define naive_linear_trace_init(k, r, pdp) { \
	i = _tail(k->pdp, i) - DEF_VEC_LEN/2; \
	j = _tail(k->pdp, j) + DEF_VEC_LEN/2; \
	p = _tail(k->pdp, p); \
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
	_tail_ld_p(pdp) - p \
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
	_tail(pdp, i) = i; \
	_tail(pdp, j) = j; \
	_tail(pdp, p) = p; \
	_tail(pdp, q) = ((uint64_t)ptb - (uint64_t)pdp + BW) % BW - BW/2; /** correct q-coordinate */ \
}

#endif /* #ifndef _NAIVE_H_INCLUDED */
/**
 * end of naive.h
 */
