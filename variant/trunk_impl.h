
/**
 * @file trunk.h
 * @brief macros for trunk (8bit 32cell diff) algorithms
 */
#ifndef _TRUNK_H_INCLUDED
#define _TRUNK_H_INCLUDED

#include "../arch/b8c32.h"
#include "../sea.h"
#include "../util/util.h"
#include <stdint.h>
#include "naive_impl.h"
#include "branch_impl.h"

/**
 * @typedef cell_t
 * @brief cell type in the trunk algorithms
 */
#ifdef cell_t

	#undef cell_t
	#undef CELL_MIN
	#undef CELL_MAX

	#define cell_t 		int8_t
	#define pack_t		uint8_t
	#define CELL_MIN	( INT8_MIN )
	#define CELL_MAX	( INT8_MAX )

#endif

/**
 * @macro BW
 * @brief bandwidth in the trunk algorithm (= 32).
 */
#ifdef BW
#undef BW
#define BW 			( 32 )
#endif

/**
 * @macro BLK
 */
#ifdef BLK
#undef BLK
#define BLK 		( 16 )
#endif

/**
 * @macro trunk_linear_bpl
 * @brief calculates bytes per line
 */
#define trunk_linear_bpl(k) 		( sizeof(cell_t) * BW )
#define trunk_affine_bpl(k)			( 3 * sizeof(cell_t) * BW )

/**
 * @macro (internal) trunk_linear_topq, ...
 * @brief coordinate calculation helper macros
 */
#define trunk_linear_topq(r, k)			naive_linear_topq(r, k)
#define trunk_linear_leftq(r, k)		naive_linear_leftq(r, k)
#define trunk_linear_topleftq(r, k)		naive_linear_topleftq(r, k)
#define trunk_linear_top(r, k) 			naive_linear_top(r, k)
#define trunk_linear_left(r, k)			naive_linear_left(r, k)
#define trunk_linear_topleft(r, k)		naive_linear_topleft(r, k)

#define trunk_affine_topq(r, k)			naive_affine_topq(r, k)
#define trunk_affine_leftq(r, k)		naive_affine_leftq(r, k)
#define trunk_affine_topleftq(r, k)		naive_affine_topleftq(r, k)
#define trunk_affine_top(r, k) 			naive_affine_top(r, k)
#define trunk_affine_left(r, k)			naive_affine_left(r, k)
#define trunk_affine_topleft(r, k)		naive_affine_topleft(r, k)

/**
 * @macro trunk_linear_dir_exp
 * @brief determines the next direction of the lane in the dynamic algorithm.
 */
#if 0
#define trunk_linear_dir_exp_top(r, k, pdp) ( \
	scl += ((dir(r) == TOP \
		? vec_msb(dv) \
		: vec_msb(dh)) + k.gi), \
	scu += ((dir(r) == TOP \
		? vec_lsb(dv) \
		: vec_lsb(dh)) + k.gi), \
	(scl > scu) ? SEA_TOP : SEA_LEFT \
)
#endif
#define trunk_linear_dir_exp_top(r, k, pdp) ( \
	vec_acc_scl(acc) > vec_acc_scu(acc) ? SEA_TOP : SEA_LEFT \
)
#define trunk_linear_dir_exp_bottom(r, k, pdp) 	( 0 )
#define trunk_affine_dir_exp_top(r, k, pdp) 	trunk_linear_dir_exp_top(r, k, pdp)
#define trunk_affine_dir_exp_bottom(r, k, pdp)	trunk_linear_dir_exp_bottom(r, k, pdp)


/**
 * @macro trunk_linear_fill_decl
 */
#define trunk_linear_fill_decl(k, r) \
	dir_t r; \
	int64_t i, j, p, q; \
	_vec_acc(acc);		/** score accumulator */ \
	_vec_acc(max); 		/** max holder */ \
	_vec_single_const(mggv, k->m - 2*k->gi); \
	_vec_single_const(xggv, k->x - 2*k->gi); \
	_vec_char_reg(wq); \
	_vec_char_reg(wt); \
	_vec_cell_reg(dv); \
	_vec_cell_reg(dh); \
	_vec_cell_reg(t1); \
	_vec_cell_reg(t2);

#define trunk_affine_fill_decl(k, r) \
	trunk_linear_fill_decl(k, r); \
	_vec_cell_reg(de); \
	_vec_cell_reg(df); \

/**
 * @macro trunk_linear_fill_init
 */
#define trunk_linear_fill_init(k, r) { \
	/** load coordinates onto the local stack */ \
	i = _ivec(pdp, i) - DEF_VEC_LEN/2; \
	j = _ivec(pdp, j) + DEF_VEC_LEN/2; \
	p = _ivec(pdp, p); \
	q = _ivec(pdp, q); \
	/** initialize direction array */ \
	dir_init(r, k->pdr, p); \
	/** load scores of the current vector */ \
	vec_acc_set(acc, \
		p, \
		_ivec(pdp, cv)[BW-1], \
		_ivec(pdp, cv)[BW/2], \
		_ivec(pdp, cv)[0]); \
	/** load vectors */ \
	int32_t *s; \
	vec_load32_dvdh( \
		_ivec(pdp, pv), _ivec(pdp, cv), \
		dv, dh, \
		k->gi, dir(r)); \
	vec_store_dvdh(pdp, dv, dh); \
	/** initialize char vectors */ \
	vec_char_setzero(wq); \
	for(q = -BW/2; q < BW/2; q++) { \
		rd_fetch(k->a, i+q); \
		pushq(rd_decode(k->a), wq); \
	} \
	vec_char_setzero(wt); \
	for(q = -BW/2; q < BW/2-1; q++) { \
		rd_fetch(k->b, j+q); \
		pusht(rd_decode(k->b), wt); \
	} \
}

/**
 * @macro trunk_linear_fill_start
 */
#define trunk_linear_fill_start(k, r) { \
	/** update direction pointer */ \
	dir_load_forward(r, k, pdp, p); \
}

/**
 * @macro trunk_linear_fill_former_body
 */
#define trunk_linear_fill_former_body(k, r) { \
	/** update direction pointer */ \
	dir_next(r, k, pdp, p); \
	debug("scu(%d), score(%d), scl(%d)", scu, score, scl); \
}

/**
 * @macro trunk_linear_fill_go_down
 */
#define trunk_linear_fill_go_down(k, r) { \
	vec_shift_r(dv, dv); \
	rd_fetch(k->b, j+BW/2-1); \
	j++; \
	pusht(rd_decode(k->b), wt); \
}

/**
 * @macro trunk_linear_fill_go_right
 */
#define trunk_linear_fill_go_right(k, r) { \
	vec_shift_l(dh, dh); \
	rd_fetch(k->a, i+BW/2); \
	i++; \
	pushq(rd_decode(k->a), wq); \
}

/**
 * @macro trunk_linear_fill_latter_body
 */
#define trunk_linear_fill_latter_body(k, r) { \
	vec_comp_sel(t1, wq, wt, mv, xv); \
	vec_max(t1, t1, dv); \
	vec_max(t1, t1, dh); \
	vec_sub(t2, t1, dv); \
	vec_sub(dv, t1, dh); \
	vec_assign(dh, t2); \
	vec_store_dvdh(pdp, dv, dh); \
	if(dir(r) == TOP) { vec_assign(t1, dv); } else { vec_assign(t1, dh); } \
	vec_acc_accum_max(acc, max, t1, k->gi); \
}

/**
 * @macro trunk_linear_fill_end
 */
#define trunk_linear_fill_end(k, r) { \
	/** store (i, j) to the end of pdp */ \
	*((int64_t *)pdp) = i; pdp += sizeof(int64_t); \
	*((int64_t *)pdp) = j; pdp += sizeof(int64_t); \
}

/**
 * @macro trunk_linear_fill_test_xdrop
 */
#define trunk_linear_fill_test_xdrop(k, r) ( \
	(XSEA - k->alg - 1) & (vec_acc_scc(acc) + k->tx - vec_acc_scc(max)) \
)

/**
 * @macro trunk_linear_fill_test_mem
 */
#define trunk_linear_fill_test_mem(k, r) ( \
	  (k->aep-i-BLK) \
	| (k->bep-j-BLK) \
	| (k->tdp - pdp \
		-(BLK*(trunk_linear_bpl(k)+1) \
		+ sizeof(struct sea_ivec) \
		+ 2 * sizeof(int64_t)			/** (i, j) */ \
		+ 2 * sizeof(int32_t) * BW))	/** pv + cv */ \
)

/**
 * @macro trunk_linear_fill_test_chain
 */
#define trunk_linear_fill_test_chain(k, r)		( 0 )

#if 0
/**
 * @macro trunk_linear_fill_check_alt
 */
#define trunk_linear_fill_check_alt(k, r) ( \
	   (scl > score - k.tc) \
	|| (scu > score - k.tc) \
)
#endif

/**
 * @macro trunk_linear_fill_check_term
 */
#define trunk_linear_fill_check_term(k, r) ( \
	( trunk_linear_fill_test_xdrop(k, r) \
	| trunk_linear_fill_test_mem(k, r) \
	| trunk_linear_fill_test_chain(k, r)) >= 0 \
)

/**
 * @macro trunk_linear_fill_finish
 */
#define trunk_linear_fill_finish(k, r) { \
	/** save vectors */ \
	vec_store32_dvdh(pdp, dv, dh, vec_acc_scu(acc), k->gi, dir(r)); \
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
	/** search max */ \

	/** save p-coordinate at the beginning of the block */ \
	_ivec(k->pdp, max) = vec_acc_scc(max); \
	_ivec(k->pdp, ep) = p; \
}

/**
 * @macro trunk_linear_chain_save_len
 */
#define trunk_linear_chain_save_len(t, c, k)		( 3 * BW )

/**
 * @macro trunk_linear_chain_push_ivec
 *
 * absolute valueへの変換をSIMDを使って高速にしたい。
 * extract -> scatter -> addのループを32回回す。
 * prefix sumなので、log(32) = 5回のループでできないか。
 */
#define trunk_linear_chain_push_ivec(t, c, k) { \
	int16_t psc, csc; \
	cell_t *p = (cell_t *)c.pdp - 3*BW; \
	t.i += BW/2; \
	t.j -= BW/2; \
	psc = -k.gi - *(p + BW) - *p; \
	if(c.pdr[t.p] == TOP) { \
		for(t.q = 0; t.q < BW; t.q++) { \
			*((int16_t *)c.pdp) = psc; \
			psc += *p; \
			csc = psc + *(p + BW) + k.gi; \
			*((int16_t *)c.pdp + BW) = csc; \
			debug("psc(%d), csc(%d)", psc, csc); \
			p++; \
			c.pdp += sizeof(int16_t); \
		} \
	} else { \
		for(t.q = 0; t.q < BW; t.q++) { \
			psc += *p; \
			*((int16_t *)c.pdp) = psc; \
			csc = psc + *(p + BW) + k.gi; \
			*((int16_t *)c.pdp + BW) = csc; \
			debug("psc(%d), csc(%d)", psc, csc); \
			p++; \
			c.pdp += sizeof(int16_t); \
		} \
	} \
	c.pdp += sizeof(int16_t) * BW; \
	c.v.size = sizeof(int16_t); \
	c.v.plen = c.v.clen = BW; \
	c.v.pv = c.pdp - sizeof(int16_t) * 2*BW; \
	c.v.cv = c.pdp - sizeof(int16_t) * BW; \
	debug("ivec: dir(%d)", c.pdr[t.p]); \
}

/**
 * @macro trunk_linear_search_terminal
 */
#define trunk_linear_search_terminal(t, c, k) { \
	dir_t r; \
	cell_t *psc = pb + ADDR(t.p - sp, 0, BW); \
	int64_t sc = pb[ADDR(t.p - sp + 1, -BW/2 + 1, BW)]; /** score */ \
	t.mi = c.aep; \
	t.mj = c.bep; \
	t.mp = COP(t.mi, t.mj, BW) - COP(c.asp, c.bsp, BW); \
	t.mq = COQ(t.mi, t.mj, BW) - COQ(t.i, t.j, BW); /** COP(mi, mj) == COP(i, j)でなければならない */ \
	dir_term(r, t, c); \
	while(t.p > t.mp) { \
		dir_prev(r, t, c); \
		sc -= (dir(r) == TOP) ? DV(psc, k.gi) : DH(psc, k.gi); \
		psc -= trunk_linear_bpl(c); \
	} \
	while(t.q < t.mq) { \
		sc += (DV(psc+1, k.gi) - DH(psc, k.gi)); \
		psc++; t.q++; \
	} \
	while(t.q > t.mq) { \
		sc += (DH(psc-1, k.gi) - DV(psc, k.gi)); \
		psc--; t.q--; \
	} \
}

/**
 * @macro trunk_linear_search_trigger
 */
#define trunk_linear_search_trigger(t1, t2, c, k) ( \
	t1.max > (t2.max - (k.m - 2*k.gi)*BW/2 - (t1.max > t2.max)) \
)

/**
 * @macro trunk_linear_search_max_score
 */
/*
#define trunk_linear_search_max_score(t, c, k) { \
	t.mi = t.i; t.mj = t.j; t.mp = t.p; t.mq = 0; \
}
*/
#define trunk_linear_search_max_score(t, c, k) { \
	dir_t r; \
	int64_t msp = MAX2(t.mp + BW * (k.m - 2*k.gi) / (2 * k.x), sp); \
	int64_t mep = MIN2(t.mp + BW * (k.m - 2*k.gi) / (2 * k.m), t.p); \
	int32_t sc = 0, tsc; \
	struct sea_coords x, y; \
	_vec_cell_reg(dv); \
	_vec_cell_reg(dh); \
	\
	debug("msp(%lld), mep(%lld)", msp, mep); \
	debug("mi(%lld), mj(%lld), mp(%lld), mq(%lld), max(%d)", t.mi, t.mj, t.mp, t.mq, t.max); \
	memset(&x, 0, sizeof(struct sea_coords)); \
	memset(&y, 0, sizeof(struct sea_coords)); \
	c.pdp = (uint8_t *)(pb + ADDR(msp - sp, -BW/2, BW)); \
	dir_init(r, c.pdr[msp]); \
	vec_load_dvdh(c.pdp, dv, dh); \
	for(x.p = msp; x.p < mep;) { \
		dir_next_guided(r, x, c); \
		vec_load_dvdh(c.pdp, dv, dh); \
		debug("load: pdp(%p)", c.pdp); \
		sc += (((dir(r) == TOP) ? vec_lsb(dv) : vec_lsb(dh)) + k.gi); \
		if(dir(r) == TOP) { x.j++; } else { x.i++; } \
		debug("i(%lld), j(%lld), p(%lld), q(%lld), sc(%d)", x.i, x.j, x.p, x.q, sc); \
		for(x.q = -BW/2, tsc = sc; x.q < BW/2; x.q++) { \
			if(tsc >= x.max) { \
				x.max = tsc; \
				coord_save_m(x); \
				debug("max found: i(%lld), j(%lld), p(%lld), q(%lld), sc(%d)", x.i, x.j, x.p, x.q, x.max); \
			} \
			if(x.q == 0 && x.p == t.mp) { \
				y = x; y.max = tsc; \
				debug("anchor found: i(%lld), j(%lld), p(%lld), q(%lld), sc(%d), max(%d)", x.i, x.j, x.p, x.q, tsc, x.max); \
			} \
			tsc -= (vec_lsb(dh) + k.gi); \
			vec_shift_r(dh, dh); \
			vec_shift_r(dv, dv); \
			tsc += (vec_lsb(dv) + k.gi); \
		} \
	} \
	debug("t: mi(%lld), mj(%lld), mp(%lld), mq(%lld), max(%d)", t.mi, t.mj, t.mp, t.mq, t.max); \
	debug("x: mi(%lld), mj(%lld), mp(%lld), mq(%lld), max(%d)", x.mi, x.mj, x.mp, x.mq, x.max); \
	debug("y: mi(%lld), mj(%lld), mp(%lld), mq(%lld), max(%d)", y.mi, y.mj, y.mp, y.mq, y.max); \
	t.max += (x.max - y.max); \
	t.mi += (x.mi - y.i - x.mq); \
	t.mj += (x.mj - y.j + x.mq); \
	t.mp = x.mp; \
	t.mq = x.mq; \
	debug("mi(%lld), mj(%lld), mp(%lld), mq(%lld), max(%d)", t.mi, t.mj, t.mp, t.mq, t.max); \
}

#if 0
#define trunk_linear_search_max_score(t, c, k) { \
	/*if(t.max > t.max - (k.m - 2*k.gi)*BW/2) {*/ \
	dir_t r; \
	cell_t *psc = pb + ADDR(t.p - sp, 0, BW); \
	t.mp = MAX2(t.mp + BW * (k.m - 2*k.gi) / (2 * k.x), sp); \
	t.mq = 0; \
	dir_term(r, t, c); \
	while(t.p > t.mp) { \
		dir_prev(r, t, c); \
		t.max -= (dir(r) == TOP) ? DV(psc, k.gi) : DH(psc, k.gi); \
		if(dir(r) == TOP) { t.j--; } else { t.i--; } \
		psc -= trunk_linear_bpl(c); \
	} \
	/** ここで(i, j)はsearch開始座標 / (mi, mj)は0にセットする */ \
	{ \
		int64_t scu = 0, score = 0, scl = 0; \
		t.max = 0; \
		_vec_cell_reg(dv); \
		_vec_cell_reg(dh); \
		_vec_cell_reg(tmp); \
		vec_load_dvdh(c.pdp, dv, dh); \
		trunk_linear_fill_finish(k, r); \
		trunk_linear_chain_push_ivec(t, c, k); \
	} \
	{ \
		struct sea_coords b = t; \
		k.f->branch(&k, &c, &b); \
		t.max += b.max; \
		t.mp += b.mp; t.mq += b.mq; \
	} \
}
#endif

/**
 * @macro trunk_linear_trace_decl
 */
#define trunk_linear_trace_decl(k, r) \
	dir_t r; \
	cell_t *p = pb + ADDR(t.p - sp, t.q, BW);

/**
 * @macro trunk_linear_trace_init
 *
 * push_ivecの実装を使って、absolute scoreに変換する。
 * ここからnon-diffの計算をし、maxの場所を特定する。
 */
#define trunk_linear_trace_init(k, r) { \
	dir_term(r, t, c); \
	rd_fetch(c.a, t.i-1); \
	rd_fetch(c.b, t.j-1); \
}

/**
 * @macro trunk_linear_trace_body
 */
#define trunk_linear_trace_body(k, r) { \
	dir_prev(r, t, c); \
	debug("dir: d(%d), d2(%d)", dir(r), dir2(r)); \
	cell_t diag, dh, sc; \
	diag = (dh = DH(p, k.gi)) + DV(p + trunk_linear_left(r, t, c), k.gi); \
	sc = rd_cmp(c.a, c.b) ? k.m : k.x; \
	debug("traceback: diag(%d), sc(%d), dh(%d), dv(%d), dh-1(%d), dv-1(%d), left(%d), top(%d)", \
		diag, sc, \
		DH(p, k.gi), DV(p, k.gi), \
		DH(p + trunk_linear_top(r, t, c), k.gi), \
		DV(p + trunk_linear_left(r, t, c), k.gi), \
		trunk_linear_left(r, t, c), trunk_linear_top(r, t, c)); \
	if(sc == diag) { \
		p += trunk_linear_topleft(r, t, c); \
		t.q += trunk_linear_topleftq(r, t, c); \
		dir_prev(r, t, c); \
		t.i--; rd_fetch(c.a, t.i-1); \
		t.j--; rd_fetch(c.b, t.j-1); \
		if(sc == k.m) { wr_pushm(t.l); } else { wr_pushx(t.l); } \
	} else if(dh == k.gi) { \
		p += trunk_linear_left(r, t, c); \
		t.q += trunk_linear_leftq(r, t, c); \
		t.i--; rd_fetch(c.a, t.i-1); \
		wr_pushd(t.l); \
	} else if(DV(p, k.gi) == k.gi) { \
		p += trunk_linear_top(r, t, c); \
		t.q += trunk_linear_topq(r, t, c); \
		t.j--; rd_fetch(c.b, t.j-1); \
		wr_pushi(t.l); \
	} else { \
		debug("out of band"); \
		return SEA_ERROR_OUT_OF_BAND; \
	} \
	if(t.q < -BW/2 || t.q > BW/2-1) { \
		debug("out of band t.mq(%lld)", t.q); \
		return SEA_ERROR_OUT_OF_BAND; \
	} \
}

/**
 * @macro trunk_linear_trace_finish
 */
#define trunk_linear_trace_finish(k, r) { \
	t.mq = (p - pb + BW) % BW - BW/2; /** correct the q-coordinate */ \
}

#endif /* #ifndef _TRUNK_H_INCLUDED */
/**
 * end of trunk.h
 */
