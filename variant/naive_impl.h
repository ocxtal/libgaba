
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
 * @struct naive_linear_block
 */
struct naive_linear_block {
	cell_t dp[BLK][BW];
#if DP == DYNAMIC
	_dir_vec(dr);
#endif
};

/**
 * @macro linear_block_t
 */
#ifdef linear_block_t
	#undef linear_block_t
#endif

#define linear_block_t	struct naive_linear_block


/**
 * @macro naive_linear_bpl, naive_affine_bpl
 * @brief calculates bytes per lane
 */
#define naive_linear_bpl()				( sizeof(cell_t) * BW )
#define naive_linear_dp_size()			( BLK * naive_linear_bpl() )
#define naive_linear_co_size()			( 0 )
#define naive_linear_jam_size()			( naive_linear_co_size() + dr_size() )
#define naive_linear_head_size()		( 2 * naive_linear_bpl() + naive_linear_jam_size() + sizeof(struct sea_joint_head) )
#define naive_linear_tail_size()		( sizeof(struct sea_joint_tail) )
#define naive_linear_bpb()				( naive_linear_dp_size() + naive_linear_jam_size() )

#define naive_affine_bpl() 				( 3 * sizeof(cell_t) * BW )
#define naive_affine_dp_size()			( BLK * naive_affine_bpl() )
#define naive_affine_co_size()			( 0 )
#define naive_affine_jam_size()			( naive_affine_co_size() + dr_size() )
#define naive_affine_head_size()		( 2 * naive_affine_bpl() + naive_affine_jam_size() + sizeof(struct sea_joint_head) )
#define naive_affine_tail_size()		( sizeof(struct sea_joint_tail) )
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
	pvh[BW/2-1] > pvh[-BW/2] ? SEA_TOP : SEA_LEFT \
)
#define naive_linear_dir_exp_bottom(r, k, pdp)	( 0 )
#define naive_affine_dir_exp_top(r, k, pdp)		naive_linear_dir_exp_top(r, k, pdp)
#define naive_affine_dir_exp_bottom(r, k, pdp)	naive_linear_dir_exp_bottom(r, k, pdp)

/**
 * fill-in
 */
/**
 * @macro naive_linear_fill_decl
 */
#define naive_linear_fill_decl(k, r, pdp) \
	dir_t r; \
	cell_t xacc = 0; \
	cell_t *pdg, *pvh; \
	int64_t i, j, p, q;

/**
 * @macro naive_affine_fill_decl
 */
#define naive_affine_fill_decl(k, r, pdp)		naive_linear_fill_decl(k, r, pdp)

/**
 * @macro naive_linear_fill_init
 * @brief initialize fill-in step context
 */
#define naive_linear_fill_init_intl(k, r, pdp) { \
	/** load coordinates onto local stack */ \
	p = _tail(k->pdp, p); \
	i = _tail(k->pdp, i) - DEF_VEC_LEN/2; \
	j = (p - 1) - (i - k->asp) + k->bsp; \
	/** load the first direction pointer */ \
	dir_init(r, k, k->pdp, p); \
	/** make room for struct sea_joint_head */ \
	pdp += sizeof(struct sea_joint_head); \
}
#define naive_linear_fill_init(k, r, pdp) { \
	naive_linear_fill_init_intl(k, r, pdp); \
	/** load the first 32bytes of the vectors */ \
	cell_t *s = (cell_t *)pdp, *t = (cell_t *)_tail(k->pdp, v); \
	pdg = s + BW/2;		/** vector at p-2 */ \
	for(q = 0; q < BW; q++) { *s++ = *t++; } \
	pvh = s + BW/2;		/** vector at p-1 */ \
	for(q = 0; q < BW; q++) { *s++ = *t++; } \
	pdp = (uint8_t *)s; \
	/** write the first dr vector */ \
	dir_end_block(r, k, pdp, p); \
}
#define naive_affine_fill_init(k, r, pdp) { \
	naive_linear_fill_init_intl(k, r, pdp); \
	/** load the first 32bytes of the vectors */ \
	cell_t *s = (cell_t *)pdp, *t = (cell_t *)_tail(k->pdp, v); \
	pdg = s + BW/2;		/** vector at p-2 */ \
	for(q = 0; q < 3*BW; q++) { *s++ = *t++; } \
	pvh = s + BW/2;		/** vector at p-1 */ \
	for(q = 0; q < 3*BW; q++) { *s++ = *t++; } \
	pdp = (uint8_t *)s; \
	/** write the first dr vector */ \
	dir_end_block(r, k, pdp, p); \
}

/**
 * @macro naive_linear_fill_start
 */
#define naive_linear_fill_start(k, r, pdp) { \
	/** nothing to do */ \
	dir_start_block(r, k, pdp, p); \
}
#define naive_affine_fill_start(k, r, pdp) { \
	naive_linear_fill_start(k, r, pdp); \
}

/**
 * @macro naive_linear_fill_former_body
 */
#define naive_linear_fill_former_body(k, r, pdp) { \
	/** nothing to do */ \
}
#define naive_linear_fill_former_body_cap(k, r, pdp)	naive_linear_fill_former_body(k, r, pdp)
#define naive_affine_fill_former_body(k, r, pdp)		naive_linear_fill_former_body(k, r, pdp)
#define naive_affine_fill_former_body_cap(k, r, pdp)	naive_affine_fill_former_body(k, r, pdp)

/**
 * @macro naive_linear_fill_go_down
 */
#define naive_linear_fill_go_down(k, r, pdp) { \
	/** increment local coordinate */ \
	j++; \
}
#define naive_linear_fill_go_down_cap(k, r, pdp)		naive_linear_fill_go_down(k, r, pdp)
#define naive_affine_fill_go_down(k, r, pdp)			naive_linear_fill_go_down(k, r, pdp)
#define naive_affine_fill_go_down_cap(k, r, pdp)		naive_affine_fill_go_down(k, r, pdp)

/**
 * @macro naive_linear_fill_go_right
 */
#define naive_linear_fill_go_right(k, r, pdp) { \
	/** increment local coordinate */ \
	i++; \
}
#define naive_linear_fill_go_right_cap(k, r, pdp)		naive_linear_fill_go_right(k, r, pdp)
#define naive_affine_fill_go_right(k, r, pdp)			naive_linear_fill_go_right(k, r, pdp)
#define naive_affine_fill_go_right_cap(k, r, pdp)		naive_affine_fill_go_right(k, r, pdp)

/**
 * @macro naive_linear_fill_latter_body_intl
 */
#define naive_linear_fill_latter_body_intl(k, r, pdp, fetch) { \
	cell_t u, d, l; \
	xacc = -1; \
	for(q = -BW/2; q < BW/2; q++) { \
		/** fetch the next character */ \
		fetch(k->a, (i-q)-1, k->asp, k->aep, 128); \
		fetch(k->b, (j+q)-1, k->bsp, k->bep, 255); \
		/** load previous cells */ \
		d = pdg[q + naive_linear_topleftq(r, k)] + (rd_cmp(k->a, k->b) ? k->m : k->x); \
		u = pvh[q + naive_linear_topq(r, k)] + k->gi; \
		l = pvh[q + naive_linear_leftq(r, k)] + k->gi; \
		/** mask edge */ \
		if(q == -BW/2) { \
			if(dir(r) == LEFT) { u = CELL_MIN; } \
			if(dir2(r) == LL) { d = CELL_MIN; } \
		} else if(q == BW/2-1) { \
			if(dir(r) == TOP) { l = CELL_MIN; } \
			if(dir2(r) == TT) { d = CELL_MIN; } \
		} \
		/** calculate and write back the cell */ \
		debug("(%lld, %lld), (%lld, %lld), %d, %d, %d, %d", p, q, i, j, d, u, l, MAX4(d, u, l, k->min)); \
		*((cell_t *)pdp) = d = MAX4(d, u, l, k->min); \
		pdp += sizeof(cell_t); \
		/** update max and xdrop accumulator */ \
		if(d >= k->max) { \
			k->max = d; k->mp = p; k->mq = q; k->mi = i-q; \
		} \
		xacc &= (d + k->tx - k->max); \
	} \
	/** update pdg and pvh */ \
	pdg = pvh; pvh = (cell_t *)pdp - BW/2; \
	/** calculate the next advancing direction */ \
	dir_det_next(r, k, pdp, p);		/** increment p */ \
}

/**
 * @macro naive_linear_fill_latter_body
 */
#define naive_linear_fill_latter_body(k, r, pdp) { \
	naive_linear_fill_latter_body_intl(k, r, pdp, rd_fetch_fast); \
}

/**
 * @macro naive_linear_fill_latter_body_cap
 */
#define naive_linear_fill_latter_body_cap(k, r, pdp) { \
	naive_linear_fill_latter_body_intl(k, r, pdp, rd_fetch_safe); \
}

/**
 * @macro naive_affine_fill_latter_body_intl
 */
#define naive_affine_fill_latter_body_intl(k, r, pdp) { \
	cell_t u, d, l; \
	xacc = -1; \
	for(q = -BW/2; q < BW/2; q++) { \
		/** fetch the next character */ \
		fetch(k->a, (i-q)-1, k->asp, k->aep, 128); \
		fetch(k->b, (j+q)-1, k->bsp, k->bep, 255); \
		/** load previous cells */ \
		d = pdg[q + naive_linear_topleftq(r, k)] + (rd_cmp(k->a, k->b) ? k->m : k->x); \
		u = MAX2(pvh[q + naive_linear_topq(r, k)] + k->gi,	/** gap init */ \
			pvh[q + BW + naive_linear_topq(r, k)] + k->ge;	/** gap ext (f matrix) */ \
		l = MAX2(pvh[q + naive_linear_leftq(r, k)] + k->gi, \
			pvh[q + 2*BW + naive_linear_leftq(r, k)] + k->ge;	/** (e-matrix) */ \
		/** mask edge */ \
		if(q == -BW/2) { \
			if(dir(r) == LEFT) { u = CELL_MIN; } \
			if(dir2(r) == LL) { d = CELL_MIN; } \
		} else if(q == BW/2-1) { \
			if(dir(r) == TOP) { l = CELL_MIN; } \
			if(dir2(r) == TT) { d = CELL_MIN; } \
		} \
		/** calculate and write back the cell */ \
		debug("(%lld, %lld), (%lld, %lld), %d, %d, %d, %d", p, q, i, j, d, u, l, MAX4(d, u, l, k->min)); \
		*((cell_t *)pdp) = d = MAX4(d, u, l, k->min); \
		*((cell_t *)pdp + BW) = u; \
		*((cell_t *)pdp + 2*BW) = l; \
		pdp += sizeof(cell_t); \
		/** update max and xdrop accumulator */ \
		if(d >= k->max) { \
			k->max = d; k->mp = p; k->mq = q; k->mi = i-q; \
		} \
		xacc &= (d + k->tx - k->max); \
	} \
	/** update pdg and pvh */ \
	pdg = pvh; pvh = (cell_t *)pdp - BW/2; \
	/** calculate the next advancing direction */ \
	dir_det_next(r, k, pdp, p);		/** increment p */ \
}

/**
 * @macro naive_linear_fill_empty_body
 */
#define naive_linear_fill_empty_body(k, r, pdp) { \
	pdp += bpl(); \
	dir_empty(r, k, pdp, p); \
}
#define naive_affine_fill_empty_body(k, r, pdp)			naive_linear_fill_empty_body(k, r, pdp)

/**
 * @macro naive_linear_fill_end
 */
#define naive_linear_fill_end(k, r, pdp) { \
	/** store direction vector */ \
	dir_end_block(r, k, pdp, p); \
	debug("fill end pdp(%p), (%lld, %lld), (%lld, %lld), (%lld, %lld)", pdp, i, j, p, q, k->aep, k->bep); \
	debug("test xdrop(%d), mem(%d), chain(%d)", \
		(int)naive_linear_fill_test_xdrop(k, r, pdp), \
		(int)naive_linear_fill_test_mem(k, r, pdp), \
		(int)naive_linear_fill_test_chain(k, r, pdp)); \
}
#define naive_affine_fill_end(k, r, pdp)				naive_linear_fill_end(k, r, pdp)

/**
 * @macro naive_linear_fill_test_xdrop
 * @brief returns negative if xdrop test passed (= when termination is detected)
 */
#define naive_linear_fill_test_xdrop(k, r, pdp) ( \
	(int64_t)(XSEA - k->alg - 1) & (int64_t)xacc \
)
#define naive_linear_fill_test_xdrop_cap(k, r, pdp)		naive_linear_fill_test_xdrop(k, r, pdp)
#define naive_affine_fill_test_xdrop(k, r, pdp)			naive_linear_fill_test_xdrop(k, r, pdp)
#define naive_affine_fill_test_xdrop_cap(k, r, pdp)		naive_affine_fill_test_xdrop(k, r, pdp)

/**
 * @macro naive_linear_fill_test_bound
 * @brief check boundary
 */
#define naive_linear_fill_test_bound(k, r, pdp) ( \
	(k->aep-i-BLK) | (k->bep-j-BLK) | dir_test_bound(r, k, pdp, p) \
)
#define naive_linear_fill_test_bound_cap(k, r, pdp) ( \
	  (k->aep-i+BW/2) | (k->bep-j+BW/2) \
	| (cop(k->aep, k->bep, BW)-p) | dir_test_bound_cap(r, k, pdp, p) \
)
#define naive_affine_fill_test_bound(k, r, pdp)			naive_linear_fill_test_bound(k, r, pdp)
#define naive_affine_fill_test_bound_cap(k, r, pdp)		naive_linear_fill_test_bound_cap(k, r, pdp)

/**
 * @macro naive_linear_fill_test_mem
 * @brief returns negative if i+BLK > aep OR j+BLK > bep OR pdp + block_memsize > tdp
 */
#define naive_linear_fill_test_mem(k, r, pdp) ( \
	(int64_t)(k->tdp - pdp \
		- (3*bpb() \
		+ sizeof(struct sea_joint_tail) \
		+ sizeof(struct sea_joint_head))) \
)
#define naive_linear_fill_test_mem_cap(k, r, pdp) ( \
	(int64_t)(k->tdp - pdp \
		- (bpb() \
		+ sizeof(struct sea_joint_tail) \
		+ sizeof(struct sea_joint_head))) \
)
#define naive_affine_fill_test_mem(k, r, pdp)			naive_linear_fill_test_mem(k, r, pdp)
#define naive_affine_fill_test_mem_cap(k, r, pdp)		naive_linear_fill_test_mem_cap(k, r, pdp)

/**
 * @macro naive_linear_fill_test_chain
 * @brief returns negative if chain test passed
 */
#define naive_linear_fill_test_chain(k, r, pdp) ( \
	0 /** never chain */ \
)
#define naive_linear_fill_test_chain_cap(k, r, pdp)		naive_linear_fill_test_chain(k, r, pdp)
#define naive_affine_fill_test_chain(k, r, pdp)			naive_linear_fill_test_chain(k, r, pdp)
#define naive_affine_fill_test_chain_cap(k, r, pdp)		naive_linear_fill_test_chain_cap(k, r, pdp)

/**
 * @macro naive_linear_fill_check_term
 * @brief returns false if the next loop can be executed
 */
#define naive_linear_fill_check_term(k, r, pdp) ( \
	( fill_test_xdrop(k, r, pdp) \
	| fill_test_bound(k, r, pdp) \
	| fill_test_mem(k, r, pdp) \
	| fill_test_chain(k, r, pdp)) < 0 \
)
#define naive_linear_fill_check_term_cap(k, r, pdp) ( \
	( fill_test_xdrop_cap(k, r, pdp) \
	| fill_test_bound_cap(k, r, pdp) \
	| fill_test_mem_cap(k, r, pdp) \
	| fill_test_chain_cap(k, r, pdp)) < 0 \
)
#define naive_affine_fill_check_term(k, r, pdp)			naive_linear_fill_check_term(k, r, pdp)
#define naive_affine_fill_check_term_cap(k, r, pdp)		naive_linear_fill_check_term_cap(k, r, pdp)

/**
 * @macro naive_linear_fill_finish
 */
#define naive_linear_fill_finish(k, r, pdp) { \
	/** retrieve chain vector pointers */ \
	uint8_t *v = pdp - 2*bpl() - jam_size(); \
	/** create ivec at the end */ \
	pdp += sizeof(struct sea_joint_tail); \
	/** save terminal coordinates */ \
	_tail(pdp, p) = p; \
	_tail(pdp, i) = i + DEF_VEC_LEN/2; \
	_tail(pdp, v) = v; \
	_tail(pdp, bpc) = 8*sizeof(cell_t); \
	_tail(pdp, d2) = dir_raw(r); \
	/** save max */ \
	/*_head(k->pdp, max) = k->max;*/ \
}
#define naive_affine_fill_finish(k, r, pdp)				naive_linear_fill_finish(k, r, pdp)

/**
 * @macro naive_linear_set_terminal
 */
#define naive_linear_set_terminal(k, pdp) { \
	int64_t i, j, p, sp, as, bs, ae, be; \
	/** load band terminal coordinate */ \
	p = _tail(k->pdp, p) - 1;	/** end p-coordinate */ \
	i = _tail(k->pdp, i) - DEF_VEC_LEN/2; \
	j = p - (i - k->asp) + k->bsp; \
	sp = _tail(pdp, p);			/** start p-coordinate */ \
	as = k->aep; bs = k->bsp; \
	ae = k->aep; be = k->bep; \
	/** check if the band reached the corner of the matrix */ \
	if((uint64_t)(cop(ae-as, be-bs, BW) - sp) <= (p - sp)) { \
		/** windback the direction */ \
		/** check if the band contains the corner */ \
		if((uint64_t)(coq(i, j, BW) - coq(ae, be, BW) + BW/2) < BW) { \
			k->mp = cop(ae-as, be-bs, BW); \
			k->mq = coq(ae-as, be-bs, BW) - coq(i-as, j-bs, BW); \
			k->mi = ae; \
		} else { \
			/** the band did not reach the other end */ \
			k->mp = 0; k->mq = 0; k->mi = as; \
		} \
	} \
}
#define naive_affine_set_terminal(k, pdp)				naive_linear_set_terminal(k, pdp)

/**
 * traceback
 */
/**
 * @macro naive_linear_trace_decl
 */
#define naive_linear_trace_decl(k, r, pdp) \
	dir_t r; \
	cell_t *pdg, *pvh; \
	cell_t cc;		/** current cell */ \
	int64_t i, j, p, q, sp;

#define naive_affine_trace_decl(k, r, pdp)				naive_linear_trace_decl(k, r, pdp)

/**
 * @macro naive_linear_trace_windback_ptr
 */
#define naive_linear_trace_windback_ptr(k, r, pdp) { \
	/** load the next direction pointer */ \
	dir_load_backward_fast(r, k, pdp, p, sp); \
	/** update pdg, pvh, and ptb (cell_t *) */ \
	pvh = pdg; pdg -= bpl() / sizeof(cell_t); \
	if((((p - 1) - sp) & (BLK-1)) == 0) { \
		pdg -= jam_size() / sizeof(cell_t); \
	} \
}
#define naive_affine_trace_windback_ptr(k, r, pdp)		naive_linear_trace_windback_ptr(k, r, pdp)

/**
 * @macro naive_linear_trace_init
 */
#define naive_linear_trace_init(k, r, pdp) { \
	/** load coordinates */ \
	p = _head(k->pdp, p); \
	q = _head(k->pdp, q); \
	i = _head(k->pdp, i); \
	j = p - (i - k->asp) + k->bsp; \
	sp = _tail(pdp, p); \
	debug("(%lld, %lld), (%lld, %lld), sp(%lld)", p, q, i, j, sp); \
	/** initialize pointers */ \
	debug("num(%lld), addr(%lld)", blk_num(p-sp, 0), blk_addr(p-sp, 0)); \
	/**p++;*/ 								/** p = (p of start pos) + 1 */ \
	dir_set_pdr_fast(r, k, pdp, p, sp);		/** load direction of p and p-1 */ \
	pvh = (cell_t *)(pdp + addr((p - 1) - sp, 0)); \
	pdg = (cell_t *)(pdp + addr((p - 2) - sp, 0)); \
	/*pdg = (cell_t *)((uint8_t *)pdg - bpl() - (((p - sp) & (BLK-1)) == 0 ? jam_size() : 0));*/ \
	/*naive_linear_trace_windback_ptr(k, r, pdp);*/ \
	/** load score */ \
	cc = _head(k->pdp, score); \
	/** fetch characters */ \
	/*rd_fetch(k->a, i-1);*/	/** to avoid fetch before boundary check */ \
	/*rd_fetch(k->b, j-1);*/	/** to avoid fetch before boundary check */ \
}
#define naive_affine_trace_init(k, r, pdp)				naive_linear_trace_init(k, r, pdp)

/**
 * @macro naive_linear_trace_body
 */
#define naive_linear_trace_body(k, r, pdp) { \
	/** load diagonal cell and score */ \
	debug("p(%lld), q(%lld), i(%lld), j(%lld)", p, q, i, j); \
	rd_fetch(k->a, i-1); \
	rd_fetch(k->b, j-1); \
	cell_t h, v; \
	cell_t diag = pdg[q + naive_linear_topleftq(r, k)]; \
	cell_t sc = rd_cmp(k->a, k->b) ? k->m : k->x; \
	/*debug("%lld, %lld, %lld, %lld, %lld, %lld",*/ \
		/*naive_linear_leftq(r, k), naive_linear_topq(r, k), naive_linear_topleftq(r, k),*/ \
		/*dir_leftq(r), dir_topq(r), dir_topleftq(r));*/ \
	/*debug("(%lld, %lld), (%lld, %lld), d(%d), v(%d), h(%d), sc(%d), cc(%d)",*/ \
	/*	p, q, i, j, diag, pvh[q + naive_linear_leftq(r, k)], pvh[q + naive_linear_topq(r, k)], sc, cc);*/ \
	if(cc == (diag + sc)) { \
		/** update direction and pointers */ \
		q += naive_linear_topleftq(r, k); \
		naive_linear_trace_windback_ptr(k, r, pdp); \
		i--; /*rd_fetch(k->a, i-1);*/	/** to avoid fetch before boundary check */ \
		j--; /*rd_fetch(k->b, j-1);*/	/** to avoid fetch before boundary check */ \
		wr_push(k->l, rd_cmp(k->a, k->b) ? 'M' : 'X'); \
		/*if(sc == k->m) { wr_pushm(k->l); } else { wr_pushx(k->l); }*/ \
		cc = diag; \
	} else if(cc == ((v = pvh[q + naive_linear_topq(r, k)]) + k->gi)) { \
		q += naive_linear_topq(r, k); \
		j--; /*rd_fetch(k->b, j-1);*/	/** to avoid fetch before boundary check */ \
		wr_push(k->l, 'I'); \
		/*wr_pushi(k->l);*/ \
		cc = v; \
	} else if(cc == ((h = pvh[q + naive_linear_leftq(r, k)]) + k->gi)) { \
		q += naive_linear_leftq(r, k); \
		i--; /*rd_fetch(k->a, i-1);*/	/** to avoid fetch before boundary check */ \
		wr_push(k->l, 'D'); \
		/*wr_pushd(k->l);*/ \
		cc = h; \
	} else { \
		debug("out of band"); \
		return SEA_ERROR_OUT_OF_BAND; \
	} \
	debug("535 j(%lld)", j); \
	/** windback to p-1 */ \
	naive_linear_trace_windback_ptr(k, r, pdp); \
	debug("538 j(%lld)", j); \
}
#define naive_affine_trace_body(k, r, pdp) { \
	/** load diagonal cell and score */ \
	debug("p(%lld), q(%lld), i(%lld), j(%lld)", p, q, i, j); \
	rd_fetch(k->a, i-1); \
	rd_fetch(k->b, j-1); \
	cell_t h, v; \
	cell_t diag = pdg[q + naive_linear_topleftq(r, k)]; \
	cell_t sc = rd_cmp(k->a, k->b) ? k->m : k->x; \
	if(cc == (diag + sc)) { \
		/** update direction and pointers */ \
		q += naive_linear_topleftq(r, k); \
		naive_linear_trace_windback_ptr(k, r, pdp); \
		i--; /*rd_fetch(k->a, i-1);*/	/** to avoid fetch before boundary check */ \
		j--; /*rd_fetch(k->b, j-1);*/	/** to avoid fetch before boundary check */ \
		wr_push(k->l, rd_cmp(k->a, k->b) ? 'M' : 'X'); \
		/*if(sc == k->m) { wr_pushm(k->l); } else { wr_pushx(k->l); }*/ \
		cc = diag; \
/*	} else if(cc == pvh[q + 2*BW]) */ \
	} else if(cc == ((h = pvh[q + naive_linear_leftq(r, k)]) + k->gi)) { \
		q += naive_linear_leftq(r, k); \
		i--; /*rd_fetch(k->a, i-1);*/	/** to avoid fetch before boundary check */ \
		wr_push(k->l, 'D'); \
		/*wr_pushd(k->l);*/ \
		cc = h; \
	} else if(cc == ((v = pvh[q + naive_linear_topq(r, k)]) + k->gi)) { \
		q += naive_linear_topq(r, k); \
		j--; /*rd_fetch(k->b, j-1);*/	/** to avoid fetch before boundary check */ \
		wr_push(k->l, 'I'); \
		/*wr_pushi(k->l);*/ \
		cc = v; \
	} else { \
		debug("out of band"); \
		return SEA_ERROR_OUT_OF_BAND; \
	} \
	/** windback to p-1 */ \
	naive_linear_trace_windback_ptr(k, r, pdp); \
}

/**
 * @macro naive_linear_trace_test_bound
 * @brief returns negative if beginning bound is invaded
 */
#define naive_linear_trace_test_bound(k, r, pdp) ( \
	0 /*(i - k->asp - 1) | (j - k->bsp - 1)*/ \
)
#define naive_linear_trace_test_bound_cap(k, r, pdp) ( \
	(i - k->asp - 1) | (j - k->bsp - 1) \
)
#define naive_affine_trace_test_bound(k, r, pdp)		naive_linear_trace_test_bound(k, r, pdp)
#define naive_affine_trace_test_bound_cap(k, r, pdp)	naive_linear_trace_test_bound_cap(k, r, pdp)

/**
 * @macro naive_linear_trace_test_joint
 * @brief returns negative if beginning bound is invaded
 */
#define naive_linear_trace_test_joint(k, r, pdp) ( \
	(p - (sp + BW)) \
)
#define naive_linear_trace_test_joint_cap(k, r, pdp) ( \
	(p - sp) \
)
#define naive_affine_trace_test_joint(k, r, pdp)		naive_linear_trace_test_joint(k, r, pdp)
#define naive_affine_trace_test_joint_cap(k, r, pdp)	naive_linear_trace_test_joint_cap(k, r, pdp)

/**
 * @macro naive_linear_trace_test_sw
 * @brief returns negative if sw traceback reached zero
 */
#define naive_linear_trace_test_sw(k, r, pdp) ( \
	((k->alg == SW) ? -1 : 0) & (cc - 1) \
)
#define naive_linear_trace_test_sw_cap(k, r, pdp) ( \
	naive_linear_trace_test_sw(k, r, pdp) \
)
#define naive_affine_trace_test_sw(k, r, pdp)			naive_linear_trace_test_sw(k, r, pdp)
#define naive_affine_trace_test_sw_cap(k, r, pdp)		naive_linear_trace_test_sw_cap(k, r, pdp)

/**
 * @macro naive_linear_trace_check_term
 * @brief returns false if the next traceback loop can be executed
 */
#define naive_linear_trace_check_term(k, r, pdp) ( \
	( trace_test_bound(k, r, pdp) \
	| trace_test_joint(k, r, pdp) \
	| trace_test_sw(k, r, pdp)) < 0 \
)
#define naive_linear_trace_check_term_cap(k, r, pdp) ( \
	( trace_test_bound_cap(k, r, pdp) \
	| trace_test_joint_cap(k, r, pdp) \
	| trace_test_sw_cap(k, r, pdp)) < 0 \
)
#define naive_affine_trace_check_term(k, r, pdp)		naive_linear_trace_check_term(k, r, pdp)
#define naive_affine_trace_check_term_cap(k, r, pdp)	naive_linear_trace_check_term_cap(k, r, pdp)

/**
 * @macro naive_linear_trace_add_cap
 */
#define naive_linear_trace_add_cap(k, r, pdp) { \
	debug("add cap i(%lld), j(%lld), asp(%lld), bsp(%lld)", i, j, k->asp, k->bsp); \
	while(i-- > k->asp) { wr_push(k->l, 'D'); } \
	while(j-- > k->bsp) { wr_push(k->l, 'I'); } \
}
#define naive_affine_trace_add_cap(k, r, pdp)			naive_linear_trace_add_cap(k, r, pdp)

/**
 * @macro naive_linear_trace_finish
 */
#define naive_linear_trace_finish(k, r, pdp) { \
	_head(pdp, p) = p; \
	_head(pdp, q) = q; \
	_head(pdp, i) = i; \
	_head(pdp, score) = cc; \
}
#define naive_affine_trace_finish(k, r, pdp)			naive_linear_trace_finish(k, r, pdp)

#endif /* #ifndef _NAIVE_H_INCLUDED */
/**
 * end of naive.h
 */
