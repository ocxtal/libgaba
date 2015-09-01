
/**
 * @file branch.h
 * @brief macros for branch (16bit 32cell) algorithms
 */
#ifndef _BRANCH_H_INCLUDED
#define _BRANCH_H_INCLUDED

#include "../arch/b16c32.h"
#include "../sea.h"
#include "../util/util.h"
#include <stdint.h>
#include "naive_impl.h"

/**
 * @typedef cell_t
 * @brief cell type in the branch algorithms
 */
#ifdef cell_t

	/** undefine types (previously defined in naive_impl.h) */
	#undef cell_t
	#undef CELL_MIN
	#undef CELL_MAX

	/** new defines */
	#define cell_t 		int16_t
	#define CELL_MIN	( INT16_MIN )
	#define CELL_MAX	( INT16_MAX )

#endif

/**
 * @macro BW
 * @brief bandwidth in the branch algorithm (= 32).
 */
#ifdef BW
#undef BW
#define BW 			( 32 )
#endif

/**
 * @macro BLK
 * @brief block split length
 */
#ifdef BLK
#undef BLK
#define BLK 		( 16 )
#endif

/**
 * @macro branch_linear_bpl
 * @brief calculates bytes per line
 */
#define branch_linear_bpl(k) 				( sizeof(cell_t) * BW )
#define branch_affine_bpl(k)				( 3 * sizeof(cell_t) * BW )

/**
 * @macro (internal) branch_linear_topq, ...
 * @brief coordinate calculation helper macros
 */
#define branch_linear_topq(r, k)			naive_linear_topq(r, k)
#define branch_linear_leftq(r, k)			naive_linear_leftq(r, k)
#define branch_linear_topleftq(r, k)		naive_linear_topleftq(r, k)
#define branch_linear_top(r, k) 			naive_linear_top(r, k)
#define branch_linear_left(r, k) 			naive_linear_left(r, k)
#define branch_linear_topleft(r, k) 		naive_linear_topleft(r, k)

#define branch_affine_topq(r, k)			naive_affine_topq(r, k)
#define branch_affine_leftq(r, k)			naive_affine_leftq(r, k)
#define branch_affine_topleftq(r, k)		naive_affine_topleftq(r, k)
#define branch_affine_top(r, k) 			naive_affine_top(r, k)
#define branch_affine_left(r, k) 			naive_affine_left(r, k)
#define branch_affine_topleft(r, k) 		naive_affine_topleft(r, k

/**
 * @macro branch_linear_dir_exp
 * @brief determines the next direction of the lane in the dynamic algorithm.
 */
#define branch_linear_dir_exp_top(r, k, pdp) ( \
	(VEC_MSB(cv) > VEC_LSB(cv)) ? SEA_TOP : SEA_LEFT \
)
#define branch_linear_dir_exp_bottom(r, k, pdp) ( 0 )
#define branch_affine_dir_exp_top(r, k, pdp)	branch_linear_dir_exp_top(r, k, pdp)
#define branch_affine_dir_exp_bottom(r, k, pdp) branch_linear_dir_exp_bottom(r, k, pdp)

/**
 * @macro branch_linear_fill_decl
 */
#define branch_linear_fill_decl(k, r) \
	dir_t r; \
	int64_t i, j, p, q; \
	_vec_cell_const(mv, k->m);		/** (m, m, m, ..., m) */ \
	_vec_cell_const(xv, k->x);		/** (x, x, x, ..., x) */ \
	_vec_cell_const(gv, k->gi);		/** (g, g, g, ..., g) */ \
	_vec_char_reg(wq);				/** a buffer for seq.a */ \
	_vec_char_reg(wt);				/** a buffer for seq.b */ \
	_vec_cell_reg(cv);				/** score vector */ \
	_vec_cell_reg(pv);				/** previous score vector */ \
	_vec_cell_reg(tmp1);			/** temporary */ \
	_vec_cell_reg(tmp2);			/** temporary */ \
	_vec_cell_reg(maxv);			/** a vector which holds maximum scores */ \
 	_vec_cell_reg(zv);				/** zero vector for the SW algorithm */

#define branch_affine_fill_decl(k, r) \
	branch_linear_fill_decl(k, r); \
	_vec_cell_reg(f); \
	_vec_cell_reg(h);

/**
 * @macro (internal) branch_linear_fill_init_intl
 */
#define branch_linear_fill_init_intl(k, r) { \
	/** load coordinates onto local stack */ \
	i = _ivec(pdp, i) - DEF_VEC_LEN/2; \
	j = _ivec(pdp, j) + DEF_VEC_LEN/2; \
	p = _ivec(pdp, p); \
	q = _ivec(pdp, q); \
	/** load vectors */ \
	int32_t *s; \
	s = _ivec(pdp, pv); VEC_LOAD32(s, pv); \
	s = _ivec(pdp, cv); VEC_LOAD32(s, cv); \
	VEC_STORE(pdp, pv); \
	VEC_STORE(pdp, cv); \
	/** initialize the other vectors */ \
	VEC_SET(maxv, k->max); \
	VEC_SET(zv, k->alg == SW ? 0 : CELL_MIN); \
	/** initialize direction array */ \
	dir_init(r, k->pdr, p); \
}

/**
 * @macro branch_linear_fill_init
 */
#define branch_linear_fill_init(k, r) { \
	/** load coordinates and vectors, initialize misc variables */ \
	branch_linear_fill_init_intl(k, r); \
	/** initialize char vectors */ \
	VEC_CHAR_SETZERO(wq);	/** vector on the top */ \
	for(q = -BW/2; q < BW/2; q++) { \
		rd_fetch(k->a, i+q); \
		PUSHQ(rd_decode(k->a), wq); \
	} \
	VEC_CHAR_SETZERO(wt);	/** vector on the left */ \
	for(q = -BW/2; q < BW/2-1; q++) { \
		rd_fetch(k->b, j+q); \
		PUSHT(rd_decode(k->b), wt); \
	} \
}

/**
 * @macro branch_linear_fill_start
 */
#define branch_linear_fill_start(k, r) { \
	/** update direction pointer */ \
	dir_load_forward(r, k, pdp, p); \
}

/**
 * @macro branch_linear_fill_former_body
 */
#define branch_linear_fill_former_body(k, r) { \
	debug("%d, %d, %d", VEC_MSB(v), VEC_CENTER(v), VEC_LSB(v)); \
	dir_next(r, k, pdp, p); \
}

/**
 * @macro branch_linear_fill_go_down
 */
#define branch_linear_fill_go_down(k, r) { \
	if(dir2(r) == TT) { \
		VEC_SHIFT_R(pv, pv); \
		VEC_INSERT_MSB(pv, CELL_MIN); \
	} \
	VEC_SHIFT_R(tmp2, cv); \
	VEC_INSERT_MSB(tmp2, CELL_MIN); \
	rd_fetch(k->b, j+BW/2-1); \
	j++; \
	PUSHT(rd_decode(k->b), wt); \
}

/**
 * @macro branch_linear_fill_go_right
 */
#define branch_linear_fill_go_right(k, r) { \
	if(dir2(r) == LL) { \
		VEC_SHIFT_L(pv, pv); \
		VEC_INSERT_LSB(pv, CELL_MIN); \
	} \
	VEC_SHIFT_L(tmp2, cv); \
	VEC_INSERT_LSB(tmp2, CELL_MIN); \
	rd_fetch(k->a, i+BW/2); \
	i++; \
	PUSHQ(rd_decode(k->a), wq); \
}

/**
 * @macro branch_linear_fill_latter_body
 */
#define branch_linear_fill_latter_body(k, r) { \
 	VEC_ADD(tmp1, cv, gv); \
	VEC_ADD(tmp2, tmp2, gv); \
	VEC_MAX(tmp1, tmp1, tmp2); \
	VEC_COMPARE(tmp2, wq, wt); \
	VEC_SELECT(tmp2, xv, mv, tmp2); \
	VEC_ADD(tmp2, pv, tmp2); \
	VEC_MAX(tmp1, zv, tmp1); \
	VEC_MAX(tmp1, tmp1, tmp2); \
	VEC_ASSIGN(pv, cv); \
	VEC_ASSIGN(cv, tmp1); \
	VEC_MAX(maxv, maxv, cv); \
	VEC_STORE(pdp, cv); \
}

/**
 * @macro branch_linear_fill_end
 */
#define branch_linear_fill_end(k, r) { \
	/** nothing to do */ \
}

/**
 * @macro branch_linear_fill_test_xdrop
 */
#define branch_linear_fill_test_xdrop(k, r) ( \
	(XSEA - k->alg - 1) & (VEC_CENTER(cv) + k->tx - VEC_CENTER(maxv)) \
)

/**
 * @macro branch_linear_fill_test_mem
 */
#define branch_linear_fill_test_mem(k, r) ( \
	  (k->aep-i-BLK) \
	| (k->bep-j-BLK) \
	| (k->tdp - pdp \
		-(BLK*(branch_linear_bpl(k)+1) \
		+ sizeof(struct sea_ivec) \
		+ branch_linear_bpl(k) /* max */ \
		+ 2 * sizeof(int32_t) * BW)) /* pv + cv */ \
)

/**
 * @macro branch_linear_fill_test_chain
 */
#define branch_linear_fill_test_chain(k, r) ( \
	(CELL_MAX / k->m) - p \
)

#if 0
/**
 * @macro branch_linear_fill_check_alt
 */
#define branch_linear_fill_check_alt(k, r) ( \
	   (VEC_LSB(v) > VEC_CENTER(v) - k.tc) \
	|| (VEC_MSB(v) > VEC_CENTER(v) - k.tc) \
)
#endif

/**
 * @macro branch_linear_fill_check_term
 */
#define branch_linear_fill_check_term(k, r) ( \
	( branch_linear_fill_test_xdrop(k, r) \
	| branch_linear_fill_test_mem(k, r) \
	| branch_linear_fill_test_chain(k, r)) >= 0 \
)

/**
 * @macro branch_linear_fill_finish
 */
#define branch_linear_fill_finish(k, r) { \
	/** aggregate max */ \
	VEC_STORE(pdp, maxv); \
	VEC_ASSIGN(tmp1, maxv); \
	for(q = 1; q < BW; q++) { \
		VEC_SHIFT_R(tmp1, tmp1); \
		VEC_MAX(maxv, tmp1, maxv); \
	} \
	k->max = VEC_LSB(maxv); \
	/** save p-coordinate at the beginning of the block */ \
	_ivec(k->pdp, max) = k->max; \
	_ivec(k->pdp, ep) = p; \
	/** save vectors */ \
	VEC_STORE32(pdp, pv); \
	VEC_STORE32(pdp, cv); \
	cell_t *pv = (cell_t *)pdp - 2*BW, *cv = (cell_t *)pdp - BW; \
	/** create ivec at the end */ \
	pdp += sizeof(struct sea_ivec); \
	/** save terminal coordinates */ \
	_ivec(pdp, i) = i+DEF_VEC_LEN/2; \
	_ivec(pdp, j) = j-DEF_VEC_LEN/2; \
	_ivec(pdp, p) = p; \
	_ivec(pdp, q) = q; \
	_ivec(pdp, pv) = (int32_t *)pv; \
	_ivec(pdp, cv) = (int32_t *)cv; \
}

/**
 * @macro branch_linear_search_terminal
 */
#define branch_linear_search_terminal(k, pdp)	 	naive_linear_search_terminal(k, pdp)

/**
 * @macro branch_linear_search_trigger
 */
#define branch_linear_search_trigger(k, pdp)		naive_linear_search_trigger(k, pdp)

/**
 * @macro branch_linear_search_max_score
 */
#define branch_linear_search_max_score(k, pdp) { \
	debug("search start"); \
	int64_t i, j, p, q; \
	int64_t ep = _ivec(pdp, ep), mp = _ivec(pdp, p); \
	cell_t *pl = (cell_t *)( \
		pdp + addr_linear( \
			(ep+1) - mp, \
			-BW/2, \
			BLK, BW)); \
	cell_t *pt; \
	dir_t r; \
	for(q = -BW/2; q < BW/2; q++, pl++) { \
		debug("check pl(%p)", pl); \
		if(*pl != k->max) { continue; } \
		debug("found"); \
		dir_load_term(r, k, pdp, ep); \
		i = 0; j = 0; p = ep; \
		for(pt = pl-BW; *pt != k->max && p > mp; pt -= BW) { \
			dir_load_backward(r, k, pdp, p); \
			if(dir(r) == TOP) { j--; } else { i--; } \
		} \
		if(p > mp) { \
			k->mi = i + _ivec(k->pdp, i) - q; \
			k->mj = j + _ivec(k->pdp, j) + q; \
			k->mp = p; k->mq = q; \
		} \
	} \
}

/**
 * @macro branch_linear_trace_decl
 */
#define branch_linear_trace_decl(k, r, pdp)			naive_linear_trace_decl(k, r, pdp)

/**
 * @macro branch_linear_trace_init
 */
#define branch_linear_trace_init(k, r, pdp)			naive_linear_trace_init(k, r, pdp)

/**
 * @macro branch_linear_trace_body
 */
#define branch_linear_trace_body(k, r, pdp)			naive_linear_trace_body(k, r, pdp)

/**
 * @macro branch_linear_trace_check_term
 */
#define branch_linear_trace_check_term(k, r, pdp)	naive_linear_trace_check_term(k, r, pdp)

/**
 * @macro branch_linear_trace_finish
 */
#define branch_linear_trace_finish(k, r, pdp)		naive_linear_trace_finish(k, r, pdp)

#endif /* #ifndef _BRANCH_H_INCLUDED */
/**
 * end of branch.h
 */
