
/**
 * @file trunk.h
 * @brief macros for trunk (8bit 32cell diff) algorithms
 */
#ifndef _TRUNK_H_INCLUDED
#define _TRUNK_H_INCLUDED

#include "../arch/b8c32.h"
#include "../include/sea.h"
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
#define trunk_linear_topq(r, c)		naive_linear_topq(r, c)
#define trunk_linear_leftq(r, c)	naive_linear_leftq(r, c)
#define trunk_linear_topleftq(r, c)	naive_linear_topleftq(r, c)
#define trunk_linear_top(r, c) 		naive_linear_top(r, c)
#define trunk_linear_left(r, c)		naive_linear_left(r, c)
#define trunk_linear_topleft(r, c)	naive_linear_topleft(r, c)

/**
 * @macro trunk_linear_dir_exp
 * @brief determines the next direction of the lane in the dynamic algorithm.
 */
#define trunk_linear_dir_exp(r, c) ( \
	scl += ((dir(r) == TOP \
		? VEC_MSB(dv) \
		: VEC_MSB(dh)) + k.gi), \
	scu += ((dir(r) == TOP \
		? VEC_LSB(dv) \
		: VEC_LSB(dh)) + k.gi), \
	(scu > scl) ? LEFT : TOP \
)

/**
 * @macro trunk_linear_fill_decl
 */
#define trunk_linear_fill_decl(c, k, r) \
	dir_t r; \
	int32_t score, max, scu, scl; \
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
#define trunk_linear_fill_init(c, k, r) { \
	c.i -= BW/2; \
	c.j += BW/2; \
	c.alim = c.aep - BW/2; \
	c.blim = c.bep - BW/2 + 1; \
	dir_init(r, c.pdr[c.p]); \
	VEC_SET(mggv, k.m - 2*k.gi); \
	VEC_SET(xggv, k.x - 2*k.gi); \
	for(c.q = 0; c.q < BW; c.q++) { \
		VEC_INSERT_MSB(dv, \
			  _read(c.v.cv, c.q, c.v.size) \
			- _read(c.v.pv, c.q - !dir(r), c.v.size) \
			- k.gi); \
		VEC_INSERT_MSB(dh, \
			  _read(c.v.cv, c.q, c.v.size) \
			- _read(c.v.pv, c.q + dir(r), c.v.size) \
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
	max = score = _read(c.v.cv, BW/2, c.v.size); \
	scl = _read(c.v.cv, BW-1, c.v.size); \
	c.pdp += trunk_linear_bpl(c); \
	VEC_STORE_DVDH(c.pdp, dv, dh); \
	for(c.q = -BW/2; c.q < BW/2; c.q++) { \
		rd_fetch(c.a, c.i+c.q); \
		PUSHQ(rd_decode(c.a), wq); \
	} \
	for(c.q = -BW/2; c.q < BW/2-1; c.q++) { \
		rd_fetch(c.b, c.j+c.q); \
		PUSHT(rd_decode(c.b), wt); \
	} \
}

/**
 * @macro trunk_linear_fill_former_body
 */
#define trunk_linear_fill_former_body(c, k, r) { \
	dir_next(r, c); \
	debug("scu(%d), score(%d), scl(%d)", scu, score, scl); \
}

/**
 * @macro trunk_linear_fill_go_down
 */
#define trunk_linear_fill_go_down(c, k, r) { \
	VEC_SHIFT_R(dv, dv); \
	rd_fetch(c.b, c.j+BW/2-1); \
	c.j++; \
	PUSHT(rd_decode(c.b), wt); \
}

/**
 * @macro trunk_linear_fill_go_right
 */
#define trunk_linear_fill_go_right(c, k, r) { \
	VEC_SHIFT_L(dh, dh); \
	rd_fetch(c.a, c.i+BW/2); \
	c.i++; \
	PUSHQ(rd_decode(c.a), wq); \
}

/**
 * @macro trunk_linear_fill_latter_body
 */
#define trunk_linear_fill_latter_body(c, k, r) { \
	VEC_COMPARE(tmp, wq, wt); \
	VEC_SELECT(tmp, xggv, mggv, tmp); \
	VEC_MAX(tmp, tmp, dv); \
	VEC_MAX(tmp, tmp, dh); \
	VEC_SUB(dv_, tmp, dh); \
	VEC_SUB(dh, tmp, dv); \
	VEC_ASSIGN(dv, dv_); \
	VEC_STORE_DVDH(c.pdp, dv, dh); \
	if(k.alg != NW && score >= max) { \
		max = score; \
		c.mi = c.i; c.mj = c.j; \
		c.mp = COP(c.mi, c.mj, BW) - COP(c.asp, c.bsp, BW); c.mq = 0; \
	} \
}

/**
 * @macro trunk_linear_fill_check_term
 */
#define trunk_linear_fill_check_term(c, k, r) ( \
	score += ((dir(r) == TOP \
		? VEC_CENTER(dv) \
		: VEC_CENTER(dh)) + k.gi), \
	k.alg == XSEA && score + k.tx - max < 0 \
)

/**
 * @macro trunk_linear_fill_check_chain
 */
#define trunk_linear_fill_check_chain(c, k, r)		( 0 )

/**
 * @macro trunk_linear_fill_check_alt
 */
#define trunk_linear_fill_check_alt(c, k, r) ( \
	   (scl > score - k.tc) \
	|| (scu > score - k.tc) \
)

/**
 * @macro trunk_linear_fill_check_mem
 */
#define trunk_linear_fill_check_mem(c, k, r) ( \
	((cell_t *)c.pdp + 2*BW) > (cell_t *)c.dp.ep \
)

/**
 * @macro trunk_linear_fill_finish
 */
#define trunk_linear_fill_finish(c, k, r) { \
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
	*((int32_t *)c.pdp) = max; \
	c.pdp += sizeof(int32_t); \
	c.pdp += (trunk_linear_bpl(c) - sizeof(int32_t) * 4); \
}

/**
 * @macro trunk_linear_chain_save_len
 */
#define trunk_linear_chain_save_len(c, k)		( 3 * BW )

/**
 * @macro trunk_linear_chain_push_ivec
 *
 * absolute valueへの変換をSIMDを使って高速にしたい。
 * extract -> scatter -> addのループを32回回す。
 * prefix sumなので、log(32) = 5回のループでできないか。
 */
#define trunk_linear_chain_push_ivec(c, k) { \
	int16_t psc, csc; \
	cell_t *p = (cell_t *)c.pdp - 3*BW; \
	debug("compensate max: c.max(%d), base(%d)", c.max, *((int32_t *)((cell_t *)c.pdp - BW))); \
	c.max -= *((int32_t *)((cell_t *)c.pdp - BW)); \
	c.i += BW/2; \
	c.j -= BW/2; \
	psc = -k.gi - *(p + BW) - *p; \
	if(c.pdr[c.p] == TOP) { \
		for(c.q = 0; c.q < BW; c.q++) { \
			*((int16_t *)c.pdp) = psc; \
			psc += *p; \
			csc = psc + *(p + BW) + k.gi; \
			*((int16_t *)c.pdp + BW) = csc; \
			debug("psc(%d), csc(%d)", psc, csc); \
			p++; \
			c.pdp += sizeof(int16_t); \
		} \
	} else { \
		for(c.q = 0; c.q < BW; c.q++) { \
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
	c.v.pv = (int16_t *)c.pdp - 2*BW; \
	c.v.cv = (int16_t *)c.pdp - BW; \
	debug("ivec: dir(%d)", c.pdr[c.p]); \
}

/**
 * @macro trunk_linear_search_terminal
 */
#define trunk_linear_search_terminal(c, k) { \
	c.mi = c.aep; \
	c.mj = c.bep; \
	c.mp = COP(c.mi, c.mj, BW) - COP(c.asp, c.bsp, BW); \
	c.mq = COQ(c.mi, c.mj, BW) - COQ(c.i, c.j, BW); \
}

/**
 * @macro trunk_linear_search_max_score
 */
#define trunk_linear_search_max_score(c, k) { \
	c.aep = c.mi; \
	c.bep = c.mj; \
}

/**
 * @macro trunk_linear_trace_decl
 */
#define trunk_linear_trace_decl(c, k, r) \
	dir_t r; \
	cell_t *p = pb + ADDR(c.p - sp, c.q, BW);

/**
 * @macro trunk_linear_trace_init
 *
 * push_ivecの実装を使って、absolute scoreに変換する。
 * ここからnon-diffの計算をし、maxの場所を特定する。
 */
#define trunk_linear_trace_init(c, k, r) { \
	dir_term(r, c); \
	rd_fetch(c.a, c.i-1); \
	rd_fetch(c.b, c.j-1); \
}

/**
 * @macro trunk_linear_trace_body
 */
#define trunk_linear_trace_body(c, k, r) { \
	dir_prev(r, c); \
	debug("dir: d(%d), d2(%d)", dir(r), dir2(r)); \
	cell_t diag, dh, sc; \
	diag = (dh = DH(p, k.gi)) + DV(p + trunk_linear_left(r, c), k.gi); \
	sc = rd_cmp(c.a, c.b) ? k.m : k.x; \
	debug("traceback: diag(%d), sc(%d), dh(%d), dv(%d), dh-1(%d), dv-1(%d), left(%d), top(%d)", \
		diag, sc, \
		DH(p, k.gi), DV(p, k.gi), \
		DH(p + trunk_linear_top(r, c), k.gi), \
		DV(p + trunk_linear_left(r, c), k.gi), \
		trunk_linear_left(r, c), trunk_linear_top(r, c)); \
	if(sc == diag) { \
		p += trunk_linear_topleft(r, c); \
		c.q += trunk_linear_topleftq(r, c); \
		dir_prev(r, c); \
		c.i--; rd_fetch(c.a, c.i-1); \
		c.j--; rd_fetch(c.b, c.j-1); \
		if(sc == k.m) { wr_pushm(c.l); } else { wr_pushx(c.l); } \
	} else if(dh == k.gi) { \
		p += trunk_linear_left(r, c); \
		c.q += trunk_linear_leftq(r, c); \
		c.i--; rd_fetch(c.a, c.i-1); \
		wr_pushd(c.l); \
	} else if(DV(p, k.gi) == k.gi) { \
		p += trunk_linear_top(r, c); \
		c.q += trunk_linear_topq(r, c); \
		c.j--; rd_fetch(c.b, c.j-1); \
		wr_pushi(c.l); \
	} else { \
		debug("out of band"); \
		return SEA_ERROR_OUT_OF_BAND; \
	} \
	if(c.q < -BW/2 || c.q > BW/2-1) { \
		debug("out of band c.mq(%lld)", c.q); \
		return SEA_ERROR_OUT_OF_BAND; \
	} \
}

/**
 * @macro trunk_linear_trace_finish
 */
#define trunk_linear_trace_finish(c, k, r) { \
	c.mq = (p - pb + BW) % BW - BW/2; /** correct the q-coordinate */ \
}

#endif /* #ifndef _TRUNK_H_INCLUDED */
/**
 * end of trunk.h
 */
