
/**
 * @file dp.c
 * @brief a skeleton of the chainable dynamic programming algorithm
 *
 * @author Hajime Suzuki
 * @date 2015/5/7
 * @licence Apache v2.
 *
 */
#include <stdio.h>
#include <stdlib.h>				/** for `NULL' */
#include <string.h>				/** for memcpy */
#include <stdint.h>				/** int8_t, int16_t, ... */
#include "include/sea.h"		/** global declarations */
#include "util/util.h"			/** internal declarations */
#include "util/log.h"
#include "variant/variant.h"	/** dynamic programming variants */

int32_t
DECLARE_FUNC_GLOBAL(BASE, SUFFIX)(
	sea_t const *ctx,
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
	sea_t k = *ctx;
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
	if((k.flags & SEA_FLAGS_MASK_DP) == SEA_DYNAMIC) {
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
		while(c.i <= c.alim && c.j <= c.blim) {
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
//			print_lane(curr, c.pdp);

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
		void *spdp = c.pdp;
		int32_t ret = SEA_SUCCESS;

		if(stat == CONT) {
			/** never reaches here */
			debug("error: stat == CONT");
			return SEA_ERROR;
		} else if(stat == MEM) {
			sea_mem_t mdp = {NULL, NULL};

			/** push */
			mdp = c.dp;			/** push memory context to mdp */
			c.size *= 2;		/** twice the memory */
			// c.size = 100000;
			c.dp.ep = (c.dp.sp = malloc(c.size)) + c.size;
			memcpy(c.dp.sp,
				(cell_t *)c.pdp - chain_save_len(c, k),
				sizeof(cell_t) * chain_save_len(c, k));
			c.pdp = (cell_t *)c.dp.sp + chain_save_len(c, k);
			debug("mem: c.size(%lld)", c.size);

			/** chain */
			chain_push_ivec(c, k);
			print_lane(c.v.pv, c.v.pv + c.v.plen*sizeof(cell_t));
			print_lane(c.v.cv, c.v.cv + c.v.clen*sizeof(cell_t));
			ret = CALL_FUNC(BASE, SUFFIX)(ctx, &c);

			/** pop */
			free(c.dp.sp);
			c.dp = mdp;
		} else if(stat == CHAIN) {
			/** chain to the next algorithm */
			debug("chain: %p", func_next(k, CALL_FUNC(BASE, SUFFIX)));
			chain_push_ivec(c, k);
			print_lane(c.v.pv, c.v.pv + c.v.plen*sizeof(cell_t));
			print_lane(c.v.cv, c.v.cv + c.v.clen*sizeof(cell_t));
			ret = func_next(k, CALL_FUNC(BASE, SUFFIX))(ctx, &c);
		} else if(stat == ALT) {
			/** chain to the alternative algorithm */
			debug("alt: %p", func_alt(k, CALL_FUNC(BASE, SUFFIX)));
			chain_push_ivec(c, k);
			print_lane(c.v.pv, c.v.pv + c.v.plen*sizeof(cell_t));
			print_lane(c.v.cv, c.v.cv + c.v.clen*sizeof(cell_t));
			ret = func_alt(k, CALL_FUNC(BASE, SUFFIX))(ctx, &c);			
		} else if(stat == CAP) {
			/** chain to the cap algorithm */
			debug("cap: %p", k.f->cap);
			chain_push_ivec(c, k);
			print_lane(c.v.pv, c.v.pv + c.v.plen*sizeof(cell_t));
			print_lane(c.v.cv, c.v.cv + c.v.clen*sizeof(cell_t));
			ret = k.f->cap(ctx, &c);
		} else if(stat == TERM) {
			/** search */
			debug("term: %lld, %lld, %lld, %lld", c.i, c.j, c.p, c.q);
			if(k.alg == NW) {
				search_terminal(c, k);
			} else {
				search_max_score(c, k);
				if(c.max > (CELL_MAX - k.m)) {
					return SEA_ERROR_OVERFLOW;
				}
				if(c.max == 0) {
					c.mi = c.mj = 0;
					wr_alloc(c.l, 0, k); proc->l = c.l;
					debug("wr_alloc without traceback");
					return SEA_SUCCESS;
				}
			}
			debug("trace from: %lld, %lld, %lld, %lld", c.mi, c.mj, c.mp, c.mq);

			/** トレースしない場合はここでリターン */
			if(k.f->pushm == NULL) {			/** 汚い実装 */
				debug("return without traceback");
				return SEA_SUCCESS;
			}

			/** initialize traceback */
			wr_alloc(c.l, c.p, k); proc->l = c.l;
			debug("wr_alloc: c.l.p(%p)", c.l.p);
		}

		/** restore pdp */
		c.pdp = spdp;

		debug("return: (%d)", ret);
		/** check return status */
		if(ret != SEA_SUCCESS) {
			return(ret);
		}
	}

	/** traceback */ {
		debug("trace");
		trace_decl(c, k, r);
		trace_init(c, k, r);
		if(sp == 0) {
			while(c.mi > 0 && c.mj > 0) {
				debug("%lld, %lld, %lld, %lld", c.mi, c.mj, c.mp, c.mq);
				trace_body(c, k, r);
			}
			while(c.mi > 0) { c.mi--; wr_pushd(c.l); }
			while(c.mj > 0) { c.mj--; wr_pushi(c.l); }
		} else {
			while(c.mp > sp) {
				debug("%lld, %lld, %lld, %lld", c.mi, c.mj, c.mp, c.mq);
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
