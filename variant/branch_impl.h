
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
 * @macro bpl, bpb
 * @brief bytes per line, bytes per block
 */
#define branch_linear_bpl()					( vec_size() )
#define branch_linear_dp_size()				( BLK * branch_linear_bpl() )
#define branch_linear_co_size()				( 2 * sizeof(int64_t) + vec_size() )
#define branch_linear_jam_size()			( branch_linear_co_size() + dr_size() )
#define branch_linear_phantom_size()		( 2 * branch_linear_bpl() + branch_linear_jam_size() + sizeof(struct sea_joint_head) )
#define branch_linear_bpb()					( branch_linear_dp_size() + branch_linear_jam_size() )

#define branch_affine_bpl()					( 3 * vec_size() )
#define branch_affine_dp_size()				( BLK * branch_affine_bpl() )
#define branch_affine_co_size()				( 2 * sizeof(int64_t) + vec_size() )
#define branch_affine_jam_size()			( branch_affine_co_size() + dr_size() )
#define branch_affine_phantom_size()		( 2 * branch_affine_bpl() + branch_affine_jam_size() + sizeof(struct sea_joint_head) )
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
#define branch_linear_fill_decl(k, r, pdp) \
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

#define branch_affine_fill_decl(k, r, pdp) \
	branch_linear_fill_decl(k, r, pdp); \
	_vec_cell_reg(f); \
	_vec_cell_reg(h);

/**
 * @macro (internal) branch_linear_fill_init_intl
 */
#define branch_linear_fill_init_intl(k, r, pdp) { \
	/** initialize vectors */ \
	vec_set(zv, k->alg == SW ? 0 : CELL_MIN); \
	vec_assign(maxv, zv);	/** init maxv with CELL_MIN */ \
	/** load coordinates onto local stack */ \
	p = _tail(k->pdp, p); \
	i = _tail(k->pdp, i) - DEF_VEC_LEN/2; \
	j = (p - 1) - (i - k->asp) + k->bsp; \
	/** initialize direction array */ \
	debug("dir_init"); \
	dir_init(r, k, k->pdp, p); \
	/** make room for struct sea_joint_head */ \
	pdp += sizeof(struct sea_joint_head); \
	/** load vectors */ \
	debug("load vectors BW(%d)", BW); \
	uint8_t *s = _tail(k->pdp, v); \
	dump(s, 8*BW); \
	vec_load8(s, pv); \
	vec_load8(s + 16, cv); \
	vec_store(pdp, pv); pdp += vec_size(); \
	vec_store(pdp, cv); pdp += vec_size(); \
	vec_print(stderr, pv); \
	vec_print(stderr, cv); \
	/** store the first (i, j) */ \
	*((int64_t *)pdp) = i; pdp += sizeof(int64_t); \
	*((int64_t *)pdp) = j; pdp += sizeof(int64_t); \
	vec_store(pdp, maxv); pdp += vec_size(); \
	/** store the first dr vector */ \
	dir_end_block(r, k, pdp, p); \
}

/**
 * @macro branch_linear_fill_init
 */
#define branch_linear_fill_init(k, r, pdp) { \
	/** load coordinates and vectors, initialize misc variables */ \
	branch_linear_fill_init_intl(k, r, pdp); \
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
#define branch_linear_fill_start(k, r, pdp) { \
	/** nothing to do */ \
	dir_start_block(r, k, pdp, p); \
}

/**
 * @macro branch_linear_fill_former_body
 */
#define branch_linear_fill_former_body(k, r, pdp) { \
	debug("%d, %d, %d", vec_msb(cv), vec_center(cv), vec_lsb(cv)); \
}
#define branch_linear_fill_former_body_cap(k, r, pdp) { \
}

/**
 * @macro branch_linear_fill_go_down
 */
#define branch_linear_fill_go_down_intl(k, r, pdp, fetch) { \
	if(dir2(r) == TT) { \
		vec_shift_r(pv, pv); \
		vec_insert_msb(pv, CELL_MIN); \
	} \
	vec_shift_r(tmp2, cv); \
	vec_insert_msb(tmp2, CELL_MIN); \
	fetch(k->b, j+BW/2-1, k->asp, k->aep, 128); \
	j++; \
	pusht(rd_decode(k->b), wt); \
}
#define branch_linear_fill_go_down(k, r, pdp) { \
	branch_linear_fill_go_down_intl(k, r, pdp, rd_fetch_fast); \
}
#define branch_linear_fill_go_down_cap(k, r, pdp) { \
	branch_linear_fill_go_down_intl(k, r, pdp, rd_fetch_safe); \
}

/**
 * @macro branch_linear_fill_go_right
 */
#define branch_linear_fill_go_right_intl(k, r, pdp, fetch) { \
	if(dir2(r) == LL) { \
		vec_shift_l(pv, pv); \
		vec_insert_lsb(pv, CELL_MIN); \
	} \
	vec_shift_l(tmp2, cv); \
	vec_insert_lsb(tmp2, CELL_MIN); \
	fetch(k->a, i+BW/2, k->bsp, k->bep, 255); \
	i++; \
	pushq(rd_decode(k->a), wq); \
}
#define branch_linear_fill_go_right(k, r, pdp) { \
	branch_linear_fill_go_right_intl(k, r, pdp, rd_fetch_fast); \
}
#define branch_linear_fill_go_right_cap(k, r, pdp) { \
	branch_linear_fill_go_right_intl(k, r, pdp, rd_fetch_safe); \
}

/**
 * @macro branch_linear_fill_latter_body
 */
#define branch_linear_fill_latter_body(k, r, pdp) { \
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
	vec_print(stdout, cv); \
	debug("p(%lld), i(%lld), j(%lld)", p, i, j); \
	vec_store(pdp, cv); pdp += vec_size(); \
	/** calculate the next advancing direction */ \
	dir_det_next(r, k, pdp, p); \
}
#define branch_linear_fill_latter_body_cap(k, r, pdp) { \
	branch_linear_fill_latter_body(k, r, pdp); \
}

/**
 * @macro branch_linear_fill_empty_body
 */
#define branch_linear_fill_empty_body(k, r, pdp) { \
	naive_linear_fill_empty_body(k, r, pdp); \
}

/**
 * @macro branch_linear_fill_end
 */
#define branch_linear_fill_end(k, r, pdp) { \
	/** store (i, j) to the end of pdp */ \
	*((int64_t *)pdp) = i; pdp += sizeof(int64_t); \
	*((int64_t *)pdp) = j; pdp += sizeof(int64_t); \
	vec_store(pdp, maxv); pdp += vec_size(); \
	/** store direction vector */ \
	dir_end_block(r, k, pdp, p); \
}

/**
 * @macro branch_linear_fill_test_xdrop
 */
#define branch_linear_fill_test_xdrop(k, r, pdp) ( \
	  (int64_t)(XSEA - k->alg - 1) \
	& (int64_t)(vec_center(cv) + k->tx - vec_center(maxv)) \
)
#define branch_linear_fill_test_xdrop_cap(k, r, pdp) ( \
	branch_linear_fill_test_xdrop(k, r, pdp) \
)

/**
 * @macro branch_linear_fill_test_bound
 */
#define branch_linear_fill_test_bound(k, r, pdp) ( \
	(k->aep-i-BLK) | (k->bep-j-BLK) \
)
#define branch_linear_fill_test_bound_cap(k, r, pdp) ( \
	(k->aep-i+BW/2) | (k->bep-j+BW/2) | (cop(k->aep, k->bep, BW)-p) \
)

/**
 * @macro branch_linear_fill_test_mem
 */
#define branch_linear_fill_test_mem(k, r, pdp) ( \
	(int64_t)(k->tdp - pdp \
		- (3*branch_linear_bpb() \
		+ sizeof(struct sea_joint_tail) \
		+ sizeof(struct sea_joint_head) \
		+ branch_linear_bpl()))		/* max vector */ \
)
#define branch_linear_fill_test_mem_cap(k, r, pdp) ( \
	(int64_t)(k->tdp - pdp \
		- (branch_linear_bpb() \
		+ sizeof(struct sea_joint_tail) \
		+ sizeof(struct sea_joint_head) \
		+ branch_linear_bpl()))		/* max vector */ \
)

/**
 * @macro branch_linear_fill_test_chain
 */
#define branch_linear_fill_test_chain(k, r, pdp) ( \
	(CELL_MAX / k->m) - p \
)
#define branch_linear_fill_test_chain_cap(k, r, pdp) ( \
	branch_linear_fill_test_chain(k, r, pdp) \
)

/**
 * @macro branch_linear_fill_check_term
 */
#define branch_linear_fill_check_term(k, r, pdp) ( \
	( fill_test_xdrop(k, r, pdp) \
	| fill_test_bound(k, r, pdp) \
	| fill_test_mem(k, r, pdp) \
	| fill_test_chain(k, r, pdp)) < 0 \
)
#define branch_linear_fill_check_term_cap(k, r, pdp) ( \
	( fill_test_xdrop_cap(k, r, pdp) \
	| fill_test_bound_cap(k, r, pdp) \
	| fill_test_mem_cap(k, r, pdp) \
	| fill_test_chain_cap(k, r, pdp)) < 0 \
)

/**
 * @macro branch_linear_fill_finish
 */
#define branch_linear_fill_finish(k, r, pdp) { \
	/** retrieve chain vector pointers */ \
	uint8_t *v = pdp - 2*branch_linear_bpl() - branch_linear_jam_size(); \
	/** aggregate max */ \
	int32_t max; \
	vec_hmax(max, maxv); \
	vec_print(stdout, maxv); \
	/*vec_store(pdp, maxv); pdp += vec_size();*/ \
	/** create ivec at the end */ \
	pdp += sizeof(struct sea_joint_tail); \
	/** save terminal coordinates */ \
	debug("p(%lld), i(%lld), max(%d)", p, i, max); \
	_tail(pdp, p) = p; \
	_tail(pdp, i) = i + DEF_VEC_LEN/2; \
	_tail(pdp, v) = v; \
	_tail(pdp, bpc) = 8*sizeof(cell_t); \
	_tail(pdp, d2) = dir_raw(r); \
	/** save max */ \
	_head(k->pdp, max) = max; \
	/** search max if updated */ \
	if(k->alg != NW && max >= k->max) { \
		k->max = max; \
		k->mi = -1; k->mp = p-1;	/** need_search */ \
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

#if 0
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
#endif

#define branch_linear_trace_search_detect_break(ptr, diff_ptr, p, diff_p, sp, pm, cm, mv) { \
}

#define branch_linear_trace_search_max(k, r, pdp) { \
	/** vectors */ \
	_vec_cell_const(mv, _head(pdp, max)); \
	_vec_cell_reg(tv); \
	/** p-coordinate */ \
	sp = _tail(pdp, p); \
	/** masks */ \
	uint64_t cm = 0, pm = 0; \
	/** load address of the last max vector */ \
	int64_t const ofs = dp_size() + co_size() - vec_size();		/** offset */ \
	int64_t bn = blk_num((_tail(k->pdp, p)-1) - sp, 0);			/** num of the last block */ \
	uint8_t *pbk = pdp + addr(bn*BLK, -BW/2) + ofs; \
	debug("sp(%lld), ep(%lld), bn(%lld), bp(%lld)", sp, (_tail(k->pdp, p)-1), bn, bn*BLK+sp); \
	/** determine the block */ \
	/** load the first vector */ \
	vec_load(pbk, tv);					/** load max of n */ \
	vec_comp_mask(cm, tv, mv); \
	debug("pbk(%p)", pbk); \
	vec_print(stdout, tv); \
	debug("cm(%llu)", cm); \
	/** search a breakpoint */ \
	int64_t bp;							/** head p-coord of the last block */ \
	for(bp = bn*BLK + sp; bp > sp; bp -= BLK, pbk -= bpb(), cm = pm) { \
		vec_load(pbk - bpb(), tv);		/** load max of n-1 */ \
		vec_comp_mask(pm, tv, mv); \
		debug("pbk(%p)", pbk - bpb()); \
		vec_print(stdout, tv); \
		debug("cm(%llu), pm(%llu)", cm, pm); \
		if(pm != cm) { break; }			/** breakpoint found */ \
	} \
	debug("bp(%lld)", bp); \
	/** move to the last vector of the block */ \
	pbk -= ofs;							/** head of the block */ \
	uint8_t *pln = pbk + (dp_size() - bpl()); \
	/** determine the vector (and p-coordinate) */ \
	for(p = bp + (BLK-1); p > bp; p--, pln -= bpl()) { \
		vec_load(pln, tv);				/** load p */ \
		vec_comp_mask(pm, tv, mv); \
		vec_print(stdout, tv); \
		if(pm != 0) { break; } \
	} \
	debug("p(%lld), pm(%llx)", p, pm); \
	/** initialize pointers */ \
	pdg = (cell_t *)pln + BW/2; \
	dir_set_pdr(r, k, pdp, p, sp); \
	/** calculate coordinates */ \
	q = (int64_t)tzcnt(pm) - BW/2; \
	i = *((int64_t *)(pbk + dp_size())) \
		- dir_sum_i_blk(r, k, pdp, p, sp) - q; \
	j = p - (i - k->asp) + k->bsp; \
	/** write back coordinates */ \
	_head(k->pdp, p) = k->mp = p; \
	_head(k->pdp, q) = k->mq = q; \
	_head(k->pdp, i) = k->mi = i; \
	debug("i(%lld), j(%lld), di(%lld)", ((int64_t *)(pbk + dp_size()))[0], ((int64_t *)(pbk + dp_size()))[1], dir_sum_i_blk(r, k, pdp, p, sp)); \
}

/**
 * @macro branch_linear_trace_init
 */
#define branch_linear_trace_init(k, r, pdp) { \
	if(_head(k->pdp, i) == -1) { \
		/** max found in this block but pos is not determined yet */ \
		debug("search max"); \
		branch_linear_trace_search_max(k, r, pdp); \
	} else { \
		/** load coordinates */ \
		p = _head(k->pdp, p); \
		q = _head(k->pdp, q); \
		i = _head(k->pdp, i); \
		j = p - (i - k->asp) + k->bsp; \
		sp = _tail(pdp, p); \
		debug("(%lld, %lld), (%lld, %lld), sp(%lld)", p, q, i, j, sp); \
		/** initialize pointers */ \
		debug("num(%lld), addr(%lld)", blk_num(p-sp, 0), blk_addr(p-sp, 0)); \
		pdg = (cell_t *)(pdp + addr(p - sp, 0)); \
		dir_set_pdr(r, k, pdp, p, sp); \
	} \
	/** load score */ \
	cc = pdg[q]; \
	/** windback pointers from p+1 to p */ \
	p++; \
	naive_linear_trace_windback_ptr(k, r, pdp); \
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
