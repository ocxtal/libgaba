
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
 * @macro trunk_linear_bpl
 * @brief calculates bytes per line
 */
#define trunk_linear_bpl(c) 		( sizeof(cell_t) * BW )

/**
 * @macro (internal) trunk_linear_topq, ...
 * @brief coordinate calculation helper macros
 */
#define trunk_linear_topq(r, t, c)		naive_linear_topq(r, t, c)
#define trunk_linear_leftq(r, t, c)		naive_linear_leftq(r, t, c)
#define trunk_linear_topleftq(r, t, c)	naive_linear_topleftq(r, t, c)
#define trunk_linear_top(r, t, c) 		naive_linear_top(r, t, c)
#define trunk_linear_left(r, t, c)		naive_linear_left(r, t, c)
#define trunk_linear_topleft(r, t, c)	naive_linear_topleft(r, t, c)

/**
 * @macro trunk_linear_dir_exp
 * @brief determines the next direction of the lane in the dynamic algorithm.
 */
#define trunk_linear_dir_exp_top(r, t, c) ( \
	scl += ((dir(r) == TOP \
		? VEC_MSB(dv) \
		: VEC_MSB(dh)) + k.gi), \
	scu += ((dir(r) == TOP \
		? VEC_LSB(dv) \
		: VEC_LSB(dh)) + k.gi), \
	(scl > scu) ? SEA_TOP : SEA_LEFT \
)
#define trunk_linear_dir_exp_bottom(r, t, c) ( 0 )

/**
 * @macro trunk_linear_fill_decl
 */
#define trunk_linear_fill_decl(t, c, k, r) \
	dir_t r; \
	int32_t score, scu, scl; \
	DECLARE_VEC_CELL_REG(mggv); \
	DECLARE_VEC_CELL_REG(xggv); \
	DECLARE_VEC_CHAR_REG(wq); \
	DECLARE_VEC_CHAR_REG(wt); \
	DECLARE_VEC_CELL_REG(dv); \
	DECLARE_VEC_CELL_REG(dh); \
	DECLARE_VEC_CELL_REG(dv_); \
	DECLARE_VEC_CELL_REG(tmp);

/**
 * @macro trunk_linear_fill_init
 */
#define trunk_linear_fill_init(t, c, k, r) { \
	t.i -= BW/2; \
	t.j += BW/2; \
	c.alim = c.aep - BW/2; \
	c.blim = c.bep - BW/2 + 1; \
	dir_init(r, c.pdr[t.p]); \
	VEC_SET(mggv, k.m - 2*k.gi); \
	VEC_SET(xggv, k.x - 2*k.gi); \
	VEC_SETZERO(dv); \
	VEC_SETZERO(dh); \
	for(t.q = 0; t.q < BW; t.q++) { \
		VEC_INSERT_MSB(dv, \
			  _read(c.v.cv, t.q, c.v.size) \
			- _read(c.v.pv, t.q + trunk_linear_topq(r, t, c), c.v.size) \
			- k.gi); \
		VEC_INSERT_MSB(dh, \
			  _read(c.v.cv, t.q, c.v.size) \
			- _read(c.v.pv, t.q + trunk_linear_leftq(r, t, c), c.v.size) \
			- k.gi); \
		VEC_SHIFT_R(dv, dv); \
		VEC_SHIFT_R(dh, dh); \
	} \
	if(dir(r) == TOP) { \
		VEC_INSERT_MSB(dh, 0); \
	} else { \
		VEC_INSERT_LSB(dv, 0); \
	} \
	scu = _read(c.v.cv, 0, c.v.size); \
	t.max = score = _read(c.v.cv, BW/2, c.v.size); \
	scl = _read(c.v.cv, BW-1, c.v.size); \
	c.pdp += trunk_linear_bpl(c); \
	VEC_STORE_DVDH(c.pdp, dv, dh); \
	VEC_CHAR_SETZERO(wq); \
	for(t.q = -BW/2; t.q < BW/2; t.q++) { \
		rd_fetch(c.a, t.i+t.q); \
		PUSHQ(rd_decode(c.a), wq); \
	} \
	VEC_CHAR_SETZERO(wt); \
	for(t.q = -BW/2; t.q < BW/2-1; t.q++) { \
		rd_fetch(c.b, t.j+t.q); \
		PUSHT(rd_decode(c.b), wt); \
	} \
}

/**
 * @macro trunk_linear_fill_former_body
 */
#define trunk_linear_fill_former_body(t, c, k, r) { \
	dir_next(r, t, c); \
	debug("scu(%d), score(%d), scl(%d)", scu, score, scl); \
}

/**
 * @macro trunk_linear_fill_go_down
 */
#define trunk_linear_fill_go_down(t, c, k, r) { \
	VEC_SHIFT_R(dv, dv); \
	rd_fetch(c.b, t.j+BW/2-1); \
	t.j++; \
	PUSHT(rd_decode(c.b), wt); \
}

/**
 * @macro trunk_linear_fill_go_right
 */
#define trunk_linear_fill_go_right(t, c, k, r) { \
	VEC_SHIFT_L(dh, dh); \
	rd_fetch(c.a, t.i+BW/2); \
	t.i++; \
	PUSHQ(rd_decode(c.a), wq); \
}

/**
 * @macro trunk_linear_fill_latter_body
 */
#define trunk_linear_fill_latter_body(t, c, k, r) { \
	VEC_COMPARE(tmp, wq, wt); \
	VEC_SELECT(tmp, xggv, mggv, tmp); \
	VEC_MAX(tmp, tmp, dv); \
	VEC_MAX(tmp, tmp, dh); \
	VEC_SUB(dv_, tmp, dh); \
	VEC_SUB(dh, tmp, dv); \
	VEC_ASSIGN(dv, dv_); \
	VEC_STORE_DVDH(c.pdp, dv, dh); \
	if(k.alg != NW && score >= t.max) { \
		t.max = score; \
		t.mi = t.i; t.mj = t.j; \
		t.mp = COP(t.mi, t.mj, BW) - COP(c.asp, c.bsp, BW); t.mq = 0; \
	} \
}

/**
 * @macro trunk_linear_fill_check_term
 */
#define trunk_linear_fill_check_term(t, c, k, r) ( \
	score += ((dir(r) == TOP \
		? VEC_CENTER(dv) \
		: VEC_CENTER(dh)) + k.gi), \
	k.alg == XSEA && score + k.tx - t.max < 0 \
)

/**
 * @macro trunk_linear_fill_check_chain
 */
#define trunk_linear_fill_check_chain(t, c, k, r)		( 0 )

/**
 * @macro trunk_linear_fill_check_alt
 */
#define trunk_linear_fill_check_alt(t, c, k, r) ( \
	   (scl > score - k.tc) \
	|| (scu > score - k.tc) \
)

/**
 * @macro trunk_linear_fill_check_mem
 */
#define trunk_linear_fill_check_mem(t, c, k, r) ( \
	((cell_t *)c.pdp + 2*BW) > (cell_t *)c.dp.ep \
)

/**
 * @macro trunk_linear_fill_finish
 */
#define trunk_linear_fill_finish(t, c, k, r) { \
	VEC_SUB(tmp, dv, dh); \
	VEC_STORE(c.pdp, tmp); \
	print_lane((cell_t *)c.pdp - BW, c.pdp); \
	VEC_STORE(c.pdp, dh); \
	print_lane((cell_t *)c.pdp - BW, c.pdp); \
	*((int32_t *)c.pdp) = scu; \
	c.pdp += sizeof(int32_t); \
	*((int32_t *)c.pdp) = score; \
	c.pdp += sizeof(int32_t); \
	*((int32_t *)c.pdp) = scl; \
	c.pdp += sizeof(int32_t); \
	*((int32_t *)c.pdp) = t.max; \
	c.pdp += sizeof(int32_t); \
	c.pdp += (trunk_linear_bpl(c) - sizeof(int32_t) * 4); \
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
	DECLARE_VEC_CELL_REG(dv); \
	DECLARE_VEC_CELL_REG(dh); \
	\
	debug("msp(%lld), mep(%lld)", msp, mep); \
	debug("mi(%lld), mj(%lld), mp(%lld), mq(%lld), max(%d)", t.mi, t.mj, t.mp, t.mq, t.max); \
	memset(&x, 0, sizeof(struct sea_coords)); \
	memset(&y, 0, sizeof(struct sea_coords)); \
	c.pdp = (uint8_t *)(pb + ADDR(msp - sp, -BW/2, BW)); \
	dir_init(r, c.pdr[msp]); \
	VEC_LOAD_DVDH(c.pdp, dv, dh); \
	for(x.p = msp; x.p < mep;) { \
		dir_next_guided(r, x, c); \
		VEC_LOAD_DVDH(c.pdp, dv, dh); \
		debug("load: pdp(%p)", c.pdp); \
		sc += (((dir(r) == TOP) ? VEC_LSB(dv) : VEC_LSB(dh)) + k.gi); \
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
			tsc -= (VEC_LSB(dh) + k.gi); \
			VEC_SHIFT_R(dh, dh); \
			VEC_SHIFT_R(dv, dv); \
			tsc += (VEC_LSB(dv) + k.gi); \
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
		DECLARE_VEC_CELL_REG(dv); \
		DECLARE_VEC_CELL_REG(dh); \
		DECLARE_VEC_CELL_REG(tmp); \
		VEC_LOAD_DVDH(c.pdp, dv, dh); \
		trunk_linear_fill_finish(t, c, k, r); \
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
#define trunk_linear_trace_decl(t, c, k, r) \
	dir_t r; \
	cell_t *p = pb + ADDR(t.p - sp, t.q, BW);

/**
 * @macro trunk_linear_trace_init
 *
 * push_ivecの実装を使って、absolute scoreに変換する。
 * ここからnon-diffの計算をし、maxの場所を特定する。
 */
#define trunk_linear_trace_init(t, c, k, r) { \
	dir_term(r, t, c); \
	rd_fetch(c.a, t.i-1); \
	rd_fetch(c.b, t.j-1); \
}

/**
 * @macro trunk_linear_trace_body
 */
#define trunk_linear_trace_body(t, c, k, r) { \
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
#define trunk_linear_trace_finish(t, c, k, r) { \
	t.mq = (p - pb + BW) % BW - BW/2; /** correct the q-coordinate */ \
}

#endif /* #ifndef _TRUNK_H_INCLUDED */
/**
 * end of trunk.h
 */
