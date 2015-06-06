
/**
 * @file dp.c
 * @brief a skeleton of the chainable dynamic programming algorithm
 *
 * @author Hajime Suzuki
 * @date 2015/5/7
 * @licence Apache v2.
 *
 * @memo
 * chainする際のmaxの値の取り扱いが不明瞭になっているので整理する。
 * searchが終わった後のmaxは確定max
 * searchが終わる前のmaxは未確定max
 * どんなアルゴリズムにすべきか...?
 *
 * maxが確定する条件
 * chainしない場合(stat == TERMで終了した場合)
 *   これは簡単で、search_max_scoreで得られた結果が確定maxになる
 *   自分のところでmaxが得られない場合があることに注意する。
 * chainする場合(それ以外)
 *   chainから帰ってきた後、未確定maxとchainの結果を比較して、
 *   自分のところでsearchを行う必要があるか調べる。必要があれば
 *   searchを行ってtracebackの開始点を上書きする。
 *
 * 未確定maxでchainする場合の問題点
 *   tracebackした内容が途中で破棄される場合がある。
 *
 * 確定maxでchainする場合の問題点
 *   searchのコストが余計にかかる。
 *
 * 結局、searchかtraceかどちらのコストがより小さいか、という話になる。
 * tmaxとrmaxの両方を保持する。
 * 入力としてtmaxを受け取る。fill-inのときにtmaxを超える可能性があることが
 * わかったら、tmax変数を書き換える。chainの際にはそのtmaxを渡す。
 * termしたとき、miとmjが自分のfill-inした範囲内にあるときはtracebackを開始する。
 * chainが終了したとき、tracebackが開始されているかチェックする。
 * 開始されていた場合、rmaxと
 *
 *
 * max arrayのようなものがあると良い?
 */
#include <stdio.h>				/** for debug */
#include "util/log.h"			/** for debug */
#include <stdlib.h>				/** for `NULL' */
#include <string.h>				/** for memcpy */
#include <stdint.h>				/** int8_t, int16_t, ... */
#include "include/sea.h"		/** global declarations */
#include "util/util.h"			/** internal declarations */
#include "variant/variant.h"	/** dynamic programming variants */

int32_t
DECLARE_FUNC_GLOBAL(BASE, SUFFIX)(
	sea_consts_t const *ctx,
	sea_proc_t *proc)
{
	debug("entry point: (%p)", CALL_FUNC(BASE, SUFFIX));

	/** check if arguments are sanity */ {
		if(ctx == NULL || proc == NULL) {
			debug("invalid pointer detected: ctx(%p), proc(%p)", ctx, proc);
			return SEA_ERROR_INVALID_ARGS;
		}
	}

	/** load contexts onto the stack */
	sea_consts_t k = *ctx;
	sea_proc_t c = *proc;

	/** make the dp pointer aligned */ {
		int64_t a = k.memaln;
		c.pdp = (void *)((((uint64_t)c.pdp + a - 1) / a) * a);
	}

	/** variable declarations */
	int64_t sp = c.p;
	cell_t *pb = (cell_t *)(c.pdp + bpl(c));
	int8_t stat = CONT;

	/** check direction array size */
	if(c.dr.sp != NULL) {
		if((c.dp.ep - c.pdp) > bpl(c) * ((uint8_t *)c.dr.ep - (uint8_t *)c.dr.sp - c.p)) {
			size_t s = 2 * (c.dr.ep - c.dr.sp);
			c.dr.ep = (c.dr.sp = c.pdr = (void *)realloc(c.dr.sp, s)) + s;
			debug("Realloc direction array at %p, with size %zu", c.dr.sp, s);
		}
	}

	/** fill-in */ {
		/** init direction and vector */
		#ifdef DEBUG
			cell_t *curr = (cell_t *)c.pdp;		/** debug */
		#endif
		fill_decl(c, k, r);
		fill_init(c, k, r);
		debug("init: r(%d), c.max(%d)", dir2(r), c.max);
//		print_lane(curr, c.pdp);

		stat = CAP;
		while(c.i <= c.alim && c.j <= c.blim && dir_check_term(r, c)) {
			#ifdef DEBUG
				curr = (cell_t *)c.pdp;		/** debug */
			#endif

			fill_former_body(c, k, r);
			if(dir(r) == TOP) {
				debug("go down: r(%d)", dir2(r));
				fill_go_down(c, k, r);
			} else {
				debug("go right: r(%d)", dir2(r));
				fill_go_right(c, k, r);
			}
			fill_latter_body(c, k, r);

			/** debug */
			debug("pos: %lld, %lld, %lld, %lld, usage(%zu)", c.i, c.j, c.p, c.q, (size_t)(c.pdp - (void *)pb));
			print_lane(curr, c.pdp);

			if(fill_check_term(c, k, r)) {
				debug("term detected");
				stat = TERM; break;
			}
			if(fill_check_chain(c, k, r)) {
				debug("chain detected");
				stat = CHAIN; break;
			}
			if(fill_check_alt(c, k, r)) {
				debug("alt detected");
				stat = ALT; break;
			}
			if(fill_check_mem(c, k, r)) {
				debug("mem detected");
				stat = MEM; break;
			}
		}
		if(stat == CAP) {
			debug("edge violation: c.i(%lld), c.alim(%lld), c.j(%lld), c.blim(%lld)", c.i, c.alim, c.j, c.blim);
		}

		debug("finish: stat(%d)", stat);
		fill_finish(c, k, r);
	}

	/** chain */ {
		int32_t (*cfn)(sea_consts_t const *, sea_proc_t *) = NULL;

		/** check status sanity */
		if(stat == CONT) {
			/** never reaches here */
			debug("error: stat == CONT");
			return SEA_ERROR;
		}

		/** set the next function */
		switch(stat) {
			case MEM:   cfn = CALL_FUNC(BASE, SUFFIX); break;
			case CHAIN: cfn = func_next(k, CALL_FUNC(BASE, SUFFIX)); break;
			case ALT:   cfn = func_alt(k, CALL_FUNC(BASE, SUFFIX)); break;
			case CAP:   cfn = k.f->cap; break;
			default:    cfn = NULL; break;
		}

		/** chain chain chain */
		if(cfn != NULL) {
			debug("chain: cfn(%p), stat(%d)", cfn, stat);
			void *spdp = c.pdp;
			sea_mem_t mdp = c.dp;
			int32_t tmax = c.max;
			int32_t ret = SEA_SUCCESS;

			/** malloc memory if stat == MEM */
			if(stat == MEM) {
				c.size *= 2;
				c.dp.ep = (c.dp.sp = malloc(c.size)) + c.size;
				memcpy(c.dp.sp,
					(cell_t *)c.pdp - chain_save_len(c, k),
					sizeof(cell_t) * chain_save_len(c, k));
				c.pdp = (cell_t *)c.dp.sp + chain_save_len(c, k);
				debug("malloc memory: size(%llu), c.pdp(%p)", c.size, c.pdp);
			}

			/** call func */
			chain_push_ivec(c, k);
			print_lane(c.v.pv, c.v.pv + c.v.plen*sizeof(cell_t));
			print_lane(c.v.cv, c.v.cv + c.v.clen*sizeof(cell_t));
			ret = cfn(ctx, &c);
			c.pdp = spdp;

			/** clean up memory if stat == MEM */
			if(stat == MEM) {
				free(c.dp.sp); c.dp = mdp;
			}

			/** check return status */
			debug("return: (%d)", ret);
			if(ret != SEA_SUCCESS) {
				return(ret);
			}

			/** check if search is needed */
			if(k.alg != NW && tmax > c.max) {
				debug("re-search triggered");
				stat = SEARCH;
			}
		}

		/** check if search is needed */
		if(stat == TERM || stat == SEARCH) {
			if(k.alg == NW) {
				search_terminal(c, k);
			} else if(c.mp > sp) {			/** fixme! */
				search_max_score(c, k);
				if(c.max > (CELL_MAX - k.m)) {
					return SEA_ERROR_OVERFLOW;
				}
				if(c.max <= 0) {			/** fixme! */
					c.i = c.mi = c.asp; c.j = c.mj = c.bsp;
					wr_alloc(c.l, 0); proc->l = c.l;
					debug("wr_alloc without traceback");
					return SEA_SUCCESS;
				}
			}
		}
	}

	/** traceback */ {
		debug("c.mi(%lld), c.mj(%lld), c.mp(%lld), c.mq(%lld)", c.mi, c.mj, c.mp, c.mq);
		debug("c.i(%lld), c.j(%lld), c.p(%lld), c.q(%lld)", c.i, c.j, c.p, c.q);
		/** check if wr_alloc is needed */
		if(c.mp <= c.p) {
			c.i = c.mi; c.j = c.mj;
			c.p = c.mp; c.q = c.mq;
			wr_alloc(c.l, c.mp); proc->l = c.l;
			debug("wr_alloc: c.l.p(%p)", c.l.p);
		}

		debug("trace");
		trace_decl(c, k, r);
		trace_init(c, k, r);
		if(sp == 0) {
			while(c.i > c.asp && c.j > c.bsp) {
				debug("%lld, %lld, %lld, %lld", c.i, c.j, c.p, c.q);
				trace_body(c, k, r);
			}
			while(c.i > c.asp) { c.i--; wr_pushd(c.l); }
			while(c.j > c.bsp) { c.j--; wr_pushi(c.l); }
		} else {
			while(c.p > sp) {
				debug("%lld, %lld, %lld, %lld", c.i, c.j, c.p, c.q);
				trace_body(c, k, r);
			}
		}
		debug("trace finish: c.mp(%lld), c.l.pos(%lld), c.l.size(%lld)", c.mp, c.l.pos, c.l.size);
		trace_finish(c, k, r);
	}

	/** write back the local context to the upper stack */
	*proc = c;

	/** finish!! */
	return SEA_SUCCESS;
}

/**
 * end of dp.c
 */
