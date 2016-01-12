
/**
 * @file branch2.h
 * @brief 8bit-diff-32bit-abs 32cell implementation
 */
#ifndef _BRANCH_H_INCLUDED
#define _BRANCH_H_INCLUDED

#include "branch_types.h"
#include "../sea.h"
#include "../util/util.h"
#include "../arch/arch_util.h"
#include <stdint.h>
#include "twig_types.h"			/** for struct twig_*_joint_vec */

/**
 * @macro branch_linear_dir_exp
 * @brief determines the next direction of the lane in the dynamic algorithm.
 */
#define branch_linear_dir_exp_top(r, k) ( \
	(vec_msb(cv) > vec_lsb(cv)) ? SEA_TOP : SEA_LEFT \
)
#define branch_linear_dir_exp_bottom(r, k) ( 0 )
#define branch_affine_dir_exp_top(r, k)		branch_linear_dir_exp_top(r, k)
#define branch_affine_dir_exp_bottom(r, k)	branch_linear_dir_exp_bottom(r, k)

/**
 * @macro branch_linear_fill_decl
 */
#define branch_linear_fill_decl(k) \
	dir_t dr; \
	int64_t p = 0; \
	uint8_t *cdp; \
	_vec_single_const(sbv, k->sc.score_sub); \
	_vec_single_const(gav, k->sc.score_gi_a); \
	_vec_single_const(gbv, k->sc.score_gi_b); \
	_vec_cell(dv); \
	_vec_cell(dh); \
	_vec_cell(accv); \
	_vec_cell(maxv);

#define branch_affine_fill_decl(k, pdp) \
	dir_t dr; \
	int64_t p = 0; \
	uint8_t *cdp; \
	_vec_single_const(sbv, k->sc.score_sub); \
	_vec_single_const(gav, k->sc.score_gi_a); \
	_vec_single_const(gbv, k->sc.score_gi_b); \
	_vec_cell(dv); \
	_vec_cell(dh); \
	_vec_cell(df); \
	_vec_cell(de); \
	_vec_cell(accv); \
	_vec_cell(maxv);

/**
 * @macro (internal) branch_linear_fill_init_intl
 */
#define branch_linear_fill_init_intl(k, pdp, sec) { \
}

/**
 * @macro branch_linear_fill_init
 */
#define branch_linear_fill_init(k, pdp, sec) { \
	/* load pointer */ \
	linear_joint_vec_t *s = (linear_joint_vec_t *)_tail(pdp, v); \
	\
	/* make room for struct sea_joint_head */ \
	_head(k->stack_top, p_tail) = pdp; \
	cdp = k->stack_top + sizeof(struct sea_joint_head); \
	\
	/* initialize sequence reader */ \
	rd_set_section(k->r, k->rr, sec); \
	rd_load(k->r, k->rr, s->wa, s->wb); \
	/** initialize direction array */ \
	dir_init(dr, k, pdp, _tail(pdp, psum)); \
	\
	/* load vectors */ \
	vec_load(s->dv, dv); \
	vec_load(s->dh, dh); \
	/** initialize vectors */ \
	vec_assign(maxv, zv); \
	\
	/* store initial vector */ \
	vec_store(cdp, vec_pack(dv, dh)); cdp += bpl(); \
	/** store max vector */ \
	vec_store(cdp, maxv); cdp += bpl(); \
	/** store the first dr vector */ \
	dir_end_block(dr, cdp); \
}

/**
 * @macro branch_linear_fill_start
 */
#define branch_linear_fill_start(k) { \
	dir_start_block(dr); \
	rd_prefetch(k->r, k->rr); \
}
#define branch_linear_fill_start_cap(k) { \
	dir_start_block(dr); \
	rd_prefetch_cap(k->r, k->rr); \
}

/**
 * @macro branch_linear_fill_body
 */
#define branch_linear_fill_body(k) { \
	if(dir(dr) == TOP) { \
		debug("go down"); \
		rd_go_down(k->r, k->rr);	/** go down */ \
		if(dir2(dr) == TT) { \
			vec_shift_r(pv, pv); vec_insert_msb(pv, DPCELL_MIN); \
		} \
		vec_shift_r(tmp2, cv); vec_insert_msb(tmp2, DPCELL_MIN); \
	} else { \
		debug("go right"); \
		rd_go_right(k->r, k->rr);	/** go right */ \
		if(dir2(dr) == LL) { \
			vec_shift_l(pv, pv); vec_insert_lsb(pv, DPCELL_MIN); \
		} \
		vec_shift_l(tmp2, cv); vec_insert_lsb(tmp2, DPCELL_MIN); \
	} \
	vec_max(tmp1, tmp1, tmp2); \
	vec_add(tmp1, tmp1, gv); \
	rd_cmp_vec(k->r, k->rr, match); \
	vec_sel(tmp2, match, mv, xv); \
	vec_add(tmp2, pv, tmp2); \
	vec_max(tmp1, tmp1, tmp2); \
	vec_assign(pv, cv); \
	vec_assign(cv, tmp1); \
	vec_max(maxv, maxv, cv); \
	/*vec_char_print(match);*/ \
	/*vec_print(cv);*/ \
	/*debug("p(%lld), i(%lld), j(%lld)", p, i, j);*/ \
	vec_store(cdp, cv); cdp += bpl(); \
	/** calculate the next advancing direction */ \
	dir_det_next(dr, p); \
}
#define branch_linear_fill_body_cap(k) { \
	branch_linear_fill_body(k); \
}

/**
 * @macro branch_linear_fill_empty_body
 */
#define branch_linear_fill_empty_body(k) { \
	vec_store(cdp, zv); cdp += bpl(); \
	dir_empty(dr); \
}

/**
 * @macro branch_linear_fill_end
 */
#define branch_linear_fill_end(k) { \
	/** store max vector */ \
	vec_store(cdp, maxv); cdp += bpl(); \
	/** store direction vector */ \
	dir_end_block(dr, cdp); \
}
#define branch_linear_fill_end_cap(k) { \
	branch_linear_fill_end(k); \
}

/**
 * @macro branch_linear_fill_test_xdrop
 */
#define branch_linear_fill_test_xdrop(k) ( \
	((int64_t)vec_center(cv) + k->tx - vec_center(maxv)) \
)
#define branch_linear_fill_test_xdrop_cap(k) ( \
	branch_linear_fill_test_xdrop(k) \
)

/**
 * @macro branch_linear_fill_test_bound
 */
#define branch_linear_fill_test_bound(k) ( \
	rd_test_fast_prefetch(k->r, k->rr, p) | dir_test_bound(dr, k, p) \
)
#define branch_linear_fill_test_bound_cap(k) ( \
	rd_test_break(k->r, k->rr, p) | dir_test_bound_cap(dr, k, p) \
)

/**
 * @macro branch_linear_fill_test_mem
 */
#define branch_linear_fill_test_mem(k) ( \
	(int64_t)(k->stack_end - cdp \
		- (3*bpb() \
		+ sizeof(tail_t) \
		+ bpl()))		/* max vector */ \
)
#define branch_linear_fill_test_mem_cap(k) ( \
	(int64_t)(k->stack_end - cdp \
		- (bpb() \
		+ sizeof(tail_t) \
		+ bpl()))		/* max vector */ \
)

/**
 * @macro branch_linear_fill_test_chain
 */
#define branch_linear_fill_test_chain(k) ( \
	(int64_t)DPCELL_MAX - (2*BLK/k->m) - vec_center(cv) \
)
#define branch_linear_fill_test_chain_cap(k) ( \
	branch_linear_fill_test_chain(k) \
)

/**
 * @macro branch_linear_fill_check_term
 */
#define branch_linear_fill_check_term(k) ( \
	fprintf(stderr, "%llx, %llx, %llx, %llx\n", \
		fill_test_xdrop(k), \
		fill_test_bound(k), \
		fill_test_mem(k), \
		fill_test_chain(k)), \
	(int64_t) \
	( fill_test_xdrop(k) \
	| fill_test_bound(k) \
	| fill_test_mem(k) \
	| fill_test_chain(k)) < 0 \
)
#define branch_linear_fill_check_term_cap(k) ( \
	fprintf(stderr, "%llx, %llx, %llx, %llx\n", \
		fill_test_xdrop_cap(k), \
		fill_test_bound_cap(k), \
		fill_test_mem_cap(k), \
		fill_test_chain_cap(k)), \
	(int64_t) \
	( fill_test_xdrop_cap(k) \
	| fill_test_bound_cap(k) \
	| fill_test_mem_cap(k) \
	| fill_test_chain_cap(k)) < 0 \
)

/**
 * @macro branch_linear_fill_finish
 */
#define branch_linear_fill_finish(k, pdp, sec) { \
	/** retrieve chain vector pointers */ \
	/*uint8_t *v = pdp - jam_size() - 2*bpl();*/ \
	joint_vec_t *s = (joint_vec_t *)cdp; \
	/*vec_cell_store(&s->wa, wa);*/ \
	/*vec_cell_store(&s->wb, wb);*/ \
	rd_store(k->r, k->rr, s->wa, s->wb); \
	vec_store(s->pv, pv); \
	vec_store(s->cv, cv); \
	cdp += sizeof(joint_vec_t); \
	/** create ivec at the end */ \
	cdp += sizeof(struct sea_joint_tail); \
	/** save terminal coordinates */ \
	_tail(cdp, psum) = _tail(pdp, psum) + p; \
	_tail(cdp, p) = p; \
	_tail(cdp, mp) = -1; \
	_tail(cdp, v) = s; \
	_tail(cdp, size) = cdp - k->stack_top; \
	_tail(cdp, var) = BASE; \
	_tail(cdp, d2) = dir_raw(dr); \
	/** aggregate max */ \
	int32_t max; \
	vec_hmax(max, maxv); \
	/** search max if updated */ \
	if(max >= k->max) { \
		k->max = max; k->m_tail = cdp; \
	} \
	debug("p(%lld), i(%lld), max(%d)", p, i, max); \
	/** write back cdp */ \
	k->stack_top = cdp; \
	/** write back sec */ \
	rd_get_section(k->r, k->rr, sec); \
}

#if 0
/**
 * @macro branch_linear_set_terminal
 */
#define branch_linear_set_terminal(k, pdp)		 	naive_linear_set_terminal(k, pdp)
#endif

/**
 * @macro branch_linear_trace_decl
 */
#define branch_linear_trace_decl(k) \
	dir_t dr; \
	dpcell_t *pvh; \
	dpcell_t cc;		/** current cell */ \
	int64_t p, q;

#define branch_linear_trace_search_max(k, hdp, tdp, pdp) { \
	/*uint8_t *hdp = tdp - _tail(tdp, size);*/ \
	/** vectors */ \
	_vec_cell_reg(tv); \
	/** p-coordinate */ \
	uint64_t ep = _tail(tdp, p); \
	/** masks */ \
	uint64_t cm = 0, pm = 0; \
	/** load address of the last max vector */ \
	int64_t bn = blk_num(ep - 1, 0);	/** num of the last block */ \
	block_t *pbk = (block_t *)(tdp - sizeof(tail_t)) - 1; \
	/*block_t *pbk = (block_t *)(hdp + sizeof(head_t)) + bn;*/ \
	/** load the first vector */ \
	vec_load(&pbk->t.maxv, tv);			/** load max of bn */ \
	/** aggregate max */ \
	int32_t max; \
	vec_hmax(max, tv); \
	_vec_cell_const(mv, max); \
	vec_comp_mask(cm, tv, mv);			/** first mask */ \
	/*vec_print(tv);*/ \
	/** search a breakpoint */ \
	debug("%lu, %lu", sizeof(block_t), bpb()); \
	debug("bn(%lld), p(%d)", bn, _tail(tdp, p)); \
	int64_t b;							/** head p-coord of the last block */ \
	for(b = bn; b > 0; b--, pbk--) { \
		vec_load(&(pbk - 1)->t.maxv, tv); \
		vec_comp_mask(pm, tv, mv); \
		/*vec_print(tv);*/ \
		debug("pm(%llx), cm(%llx)", pm, cm); \
		if(pm != cm) { break; }			/** breakpoint found */ \
	} \
	debug("pbk(%p), %lld", pbk, (int64_t)((uint8_t *)pbk - pdp)); \
	/** determine the vector (and p-coordinate) */ \
	for(p = BLK-1; p >= 0; p--) { \
		vec_load(&pbk->v.dp[p], tv); \
		vec_comp_mask(pm, tv, mv); \
		/*vec_print(tv);*/ \
		/*vec_print(mv);*/ \
		debug("pm(%llx)", pm); \
		if(pm != 0) { break; } \
	} \
	debug("b(%lld), p(%lld)", b, p); \
	/** calculate coordinates */ \
	p += b*BLK; \
	q = (int64_t)tzcnt(pm) - BW/2; \
	/** write back coordinates */ \
	_tail(tdp, mp) = p; \
	_tail(tdp, mq) = q; \
	_head(pdp, p) = p - ep; \
	_head(pdp, q) = q; \
}

/**
 * @macro branch_linear_trace_windback_ptr
 */
#define branch_linear_trace_windback_ptr(k) { \
	/** load the next direction pointer */ \
	dir_load_backward(dr, p); \
	/** update pdg, pvh, and ptb (dpcell_t *) */ \
	if((p & (BLK-1)) == 0) { \
		pvh -= sizeof(block_trailer_t) / sizeof(dpcell_t); \
	} \
	pvh -= bpl() / sizeof(dpcell_t); \
}

/**
 * @macro branch_linear_trace_init
 */
#define branch_linear_trace_init(k, hdp, tdp, pdp) { \
	if(_head(pdp, p) >= 0 && _tail(tdp, mp) == -1) { \
		/** max found in this block but pos is not determined yet */ \
		debug("search max"); \
		branch_linear_trace_search_max(k, hdp, tdp, pdp);	/** search and load coordinates */ \
	} else { \
		/** load coordinates */ \
		p = _tail(tdp, p) + _head(pdp, p); \
		q = _head(pdp, q); \
		/*debug("(%lld, %lld), (%lld, %lld), sp(%lld)", p, q, i, j, sp);*/ \
	} \
	dir_set_pdr(dr, k, hdp, p, _tail(tdp, psum)); \
	debug("size(%lu), size(%lu)", bpb(), sizeof(block_t)); \
	debug("(%lld, %lld)", p, q); \
	debug("num(%lld), addr(%lld)", blk_num(p, 0), blk_addr(p, 0)); \
	/** init writer */ \
	wr_start(k->l, k->ll); \
	/** load score and pointers */ \
	cc = *((dpcell_t *)(hdp + addr(p, q))); \
	pvh = (dpcell_t *)(hdp + addr(p - 1, q)); \
}

/**
 * @macro branch_linear_trace_body
 */
#define branch_linear_trace_body(k) { \
	/** load diagonal cell and score */ \
	debug("p(%lld), q(%lld)", p, q); \
	dpcell_t t; \
	if(cc == ((t = pvh[dir_topq(dr)]) + k->gi)) { \
		pvh += dir_topq(dr); wr_push(k->l, k->ll, I_CHAR); \
	} else if(cc == ((t = pvh[dir_leftq(dr)]) + k->gi)) { \
		pvh += dir_leftq(dr); wr_push(k->l, k->ll, D_CHAR); \
	} else { \
		pvh += dir_topq(dr); \
		branch_linear_trace_windback_ptr(k); \
		pvh += dir_leftq(dr); \
		wr_push(k->l, k->ll, (cc == (t = *pvh) + k->x) ? X_CHAR : M_CHAR); \
	} \
	cc = t; \
	/** windback to p-1 */ \
	branch_linear_trace_windback_ptr(k); \
}


/**
 * @macro branch_linear_trace_test_bound
 */
#define branch_linear_trace_test_bound(k) ( \
	p \
)

/**
 * @macro branch_linear_trace_check_term
 */
#define branch_linear_trace_check_term(k) ( \
	trace_test_bound(k) < 0 \
)

/**
 * @macro branch_linear_trace_finish
 */
#define branch_linear_trace_finish(k, hdp) { \
	/** finish writer */ \
	wr_end(k->l, k->ll); \
	/** update header */ \
	_head(hdp, p) = p; \
	_head(hdp, q) = (int64_t)(pvh - addr(p - 1, 0)) / sizeof(dpcell_t); \
	/*_head(hdp, q) = ((int64_t)pvh & (bpl()-1)) / sizeof(dpcell_t) - BW/2;*/ \
}

#endif /* #ifndef _BRANCH_H_INCLUDED */
/**
 * end of branch.h
 */


/**
 * end of branch2.h
 */
