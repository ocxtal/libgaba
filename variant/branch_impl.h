
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
 * @macro bpl, bpb
 * @brief bytes per line, bytes per block
 */
#define branch_linear_bpl()					naive_linear_bpl()
#define branch_linear_dp_size()				( BLK * branch_linear_bpl() )
#define branch_linear_co_size()				( 2 * sizeof(int64_t) )
#define branch_linear_jam_size()			( branch_linear_co_size() + dr_size() )
#define branch_linear_phantom_size()		( 2 * branch_linear_bpl() + branch_linear_jam_size() )
#define branch_linear_bpb()					( branch_linear_dp_size() + branch_linear_jam_size() )

#define branch_affine_bpl()					naive_affine_bpl()
#define branch_affine_dp_size()				( BLK * branch_affine_bpl() )
#define branch_affine_co_size()				( 2 * sizeof(int64_t) )
#define branch_affine_jam_size()			( branch_affine_co_size() + dr_size() )
#define branch_affine_phantom_size()		( 2 * branch_affine_bpl() + branch_affine_jam_size() )
#define branch_affine_bpb()					( branch_affine_dp_size() + branch_affine_jam_size() )

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
	(vec_msb(cv) > vec_lsb(cv)) ? SEA_TOP : SEA_LEFT \
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
	_vec_single_const(mv, k->m);	/** (m, m, m, ..., m) */ \
	_vec_single_const(xv, k->x);	/** (x, x, x, ..., x) */ \
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
	/** initialize vectors */ \
	vec_set(maxv, k->max); \
	vec_set(zv, k->alg == SW ? 0 : CELL_MIN); \
	/** load coordinates onto local stack */ \
	p = _tail(k->pdp, p); \
	i = _tail(k->pdp, i) - DEF_VEC_LEN/2; \
	j = (p - 1) - (i - k->asp); \
	/** initialize direction array */ \
	dir_init(r, k, k->pdr, p); \
	/** make room for struct sea_joint_head */ \
	pdp += sizeof(struct sea_joint_head); \
	/** load vectors */ \
	int32_t *s = _tail(k->pdp, pv); \
	vec_load8(s, pv); \
	vec_load8(s, cv); \
	vec_store(pdp, pv); \
	vec_store(pdp, cv); \
	/** store the first (i, j) */ \
	*((int64_t *)pdp) = i; pdp += sizeof(int64_t); \
	*((int64_t *)pdp) = j; pdp += sizeof(int64_t); \
	/** store the first dr vector */ \
	dir_end_block(r, k, pdp, p); \
}

/**
 * @macro branch_linear_fill_init
 */
#define branch_linear_fill_init(k, r) { \
	/** load coordinates and vectors, initialize misc variables */ \
	branch_linear_fill_init_intl(k, r); \
	/** initialize char vectors */ \
	vec_char_setzero(wq);	/** vector on the top */ \
	for(q = -BW/2; q < BW/2; q++) { \
		rd_fetch(k->a, i+q); \
		pushq(rd_decode(k->a), wq); \
	} \
	vec_char_setzero(wt);	/** vector on the left */ \
	for(q = -BW/2; q < BW/2-1; q++) { \
		rd_fetch(k->b, j+q); \
		pusht(rd_decode(k->b), wt); \
	} \
}

/**
 * @macro branch_linear_fill_start
 */
#define branch_linear_fill_start(k, r) { \
	/** nothing to do */ \
	dir_start_block(r, k, pdp, p); \
}

/**
 * @macro branch_linear_fill_former_body
 */
#define branch_linear_fill_former_body(k, r) { \
	debug("%d, %d, %d", vec_msb(v), vec_center(v), vec_lsb(v)); \
}

/**
 * @macro branch_linear_fill_go_down
 */
#define branch_linear_fill_go_down(k, r) { \
	if(dir2(r) == TT) { \
		vec_shift_r(pv, pv); \
		vec_insert_msb(pv, CELL_MIN); \
	} \
	vec_shift_r(tmp2, cv); \
	vec_insert_msb(tmp2, CELL_MIN); \
	rd_fetch(k->b, j+BW/2-1); \
	j++; \
	pusht(rd_decode(k->b), wt); \
}

/**
 * @macro branch_linear_fill_go_right
 */
#define branch_linear_fill_go_right(k, r) { \
	if(dir2(r) == LL) { \
		vec_shift_l(pv, pv); \
		vec_insert_lsb(pv, CELL_MIN); \
	} \
	vec_shift_l(tmp2, cv); \
	vec_insert_lsb(tmp2, CELL_MIN); \
	rd_fetch(k->a, i+BW/2); \
	i++; \
	pushq(rd_decode(k->a), wq); \
}

/**
 * @macro branch_linear_fill_latter_body
 */
#define branch_linear_fill_latter_body(k, r) { \
 	vec_add(tmp1, cv, gv); \
	vec_add(tmp2, tmp2, gv); \
	vec_max(tmp1, tmp1, tmp2); \
	vec_comp_sel(tmp2, wq, wt, mv, xv); \
	vec_add(tmp2, pv, tmp2); \
	vec_max(tmp1, zv, tmp1); \
	vec_max(tmp1, tmp1, tmp2); \
	vec_assign(pv, cv); \
	vec_assign(cv, tmp1); \
	vec_max(maxv, maxv, cv); \
	vec_store(pdp, cv); \
	/** calculate the next advancing direction */ \
	dir_load_forward(r, k, pdp, p); \
}

/**
 * @macro branch_linear_fill_end
 */
#define branch_linear_fill_end(k, r) { \
	/** store (i, j) to the end of pdp */ \
	*((int64_t *)pdp) = i; pdp += sizeof(int64_t); \
	*((int64_t *)pdp) = j; pdp += sizeof(int64_t); \
	/** store direction vector */ \
	dir_end_block(r, k, pdp, p); \
}

/**
 * @macro branch_linear_fill_test_xdrop
 */
#define branch_linear_fill_test_xdrop(k, r) ( \
	(XSEA - k->alg - 1) & (vec_center(cv) + k->tx - vec_center(maxv)) \
)

/**
 * @macro branch_linear_fill_test_mem
 */
#define branch_linear_fill_test_mem(k, r) ( \
	  (k->aep-i-BLK) \
	| (k->bep-j-BLK) \
	| (k->tdp - pdp \
		- (branch_linear_bpb() \
		+ sizeof(struct sea_joint_tail) \
		+ sizeof(struct sea_joint_head) \
		+ branch_linear_bpl()) 			/* max vector */ \
)

/**
 * @macro branch_linear_fill_test_chain
 */
#define branch_linear_fill_test_chain(k, r) ( \
	(CELL_MAX / k->m) - p \
)

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
	/** retrieve chain vector pointers */ \
	uint8_t *v = pdp - 2*branch_linear_bpl() - branch_linear_jam_size(); \
	/** aggregate max */ \
	int32_t max; \
	vec_hmax(max, maxv); \
	vec_store(pdp, maxv); \
	/** create ivec at the end */ \
	pdp += sizeof(struct sea_joint_tail); \
	/** save terminal coordinates */ \
	_tail(pdp, p) = p; \
	_tail(pdp, i) = i + DEF_VEC_LEN/2; \
	_tail(pdp, v) = v; \
	_tail(pdp, bpc) = 8*sizeof(cell_t); \
	_tail(pdp, d2) = dir_raw(r); \
	/** save p-coordinate at the beginning of the block */ \
	_tail(k->pdp, max) = max; \
	/** search max if updated */ \
	if(k->alg != NW && max > k->max) { \
		k->max = max; \
		k->mi = -1; k->mp = p;	/** need_search */ \
	} \
}

/**
 * @macro branch_linear_set_terminal
 */
#define branch_linear_set_terminal(k, pdp)		 	naive_linear_set_terminal(k, pdp)

/**
 * @macro branch_linear_trace_decl
 */
#define branch_linear_trace_decl(k, r, pdp)			naive_linear_trace_decl(k, r, pdp)


/**
	dir_load_term(r, k, pdp, p); \
		dir_load_backward(r, k, pdp, bp); \
		if(dir(r) == TOP) { bj--; } else { bi--; } \
*/
/**
 * @macro (internal) branch_linear_fill_search_max
 */
#define branch_linear_fill_search_max(k, r, pdp, i, j, p, maxv) { \
	/** search max */ \
	int64_t bq, sp = _tail(k->pdp, p); \
	cell_t *pl = (cell_t *)(k->pdp + addr_linear( \
		p-sp, -BW/2, BLK, BW)); \
	cell_t *pt; \
	for(bq = -BW/2; bq < BW/2; bq++, pl++) { \
		if(vec_lsb(maxv) == max) { \
			for(pt = pl, bp = p; *pt != max && bp > sp; pt -= BW, bp--) { \
				if((p - sp) & (BLK-1) == 0) { \
					pt -= (2*sizeof(int64_t) + BLK)/sizeof(cell_t); \
				} \
			} \
			if(bp > k->mp) { \
				k->max = max; k->mp = sp = bp; k->mq = bq; \
			} \
		} \
		vec_shift_r(maxv, maxv); \
	} \
}

/**
 * @macro branch_linear_trace_init
 */
#define branch_linear_trace_init(k, r, pdp) { \
	naive_linear_trace_init(k, r, pdp); \
}

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
