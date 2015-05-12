
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
#include "include/sea.h"		/** global declarations */
#include "util/util.h"			/** internal declarations */
#include "variant/variant.h"	/** dynamic programming variants */

int32_t
DECLARE_FUNC_GLOBAL(BASE, SUFFIX)(
	sea_ctx_t const *ctx,
	sea_proc_t *proc)
{
	debug("enter: (%p)", CALL_FUNC(BASE, SUFFIX));

	/** check if arguments are sanity */ {
		debug("check pointers: ctx(%p), proc(%p)", ctx, proc);
		if(ctx == NULL || proc == NULL) {
			debug("invalid pointers");
			return SEA_ERROR_INVALID_ARGS;
		}
	}

	/** load contexts onto the stack */
	sea_ctx_t k = *ctx;
	sea_proc_t c = *proc;

	/** make the dp pointer aligned */ {
		int64_t a = k.memaln;
		c.pdp = (void *)((((uint64_t)c.pdp + a - 1) / a) * a);
		debug("align c.pdp: c.pdp(%p), a(%lld)", c.pdp, a);
	}

	/** variable declarations */
	int64_t sp = c.p;
	cell_t *pb = (cell_t *)(c.pdp + bpl(c));
	int8_t stat = CONT;
	debug("save sp and pb: sp(%lld), pb(%p)", sp, pb);

	/** check direction array size */
	if((k.flags & SEA_FLAGS_MASK_DP) == SEA_DYNAMIC) {
		if((c.dp.ep - c.pdp) / bpl(c) > ((uint8_t *)c.dr.ep - c.pdr)) {
			debug("resize c.pdr: c.pdr(%p)", c.pdr);
			size_t s = 2 * (c.dr.ep - c.dr.sp);
			c.dr.ep = (c.dr.sp = c.pdr = (void *)realloc(c.dr.sp, s)) + s;
			debug("resized c.pdr: c.pdr(%p), size(%zu)", c.pdr, s);
		}
	}

	/** fill-in */ {
		/** init direction and vector */
		cell_t *curr = (cell_t *)c.pdp;		/** debug */
		fill_decl(c, k, r);
		fill_init(c, k, r);
		debug("init: r(%d)", dir2(r));
		print_lane(curr, c.pdp);

		stat = CAP;
		while(c.i <= c.alim && c.j <= c.blim) {
			cell_t *curr = (cell_t *)c.pdp;		/** debug */
			debug("%lld, %lld, %lld, %lld", c.i, c.j, c.p, c.q);

			fill_former_body(c, k, r);
			if(dir(r) == TOP) {
				debug("go down: r(%d)", dir2(r));
				fill_go_down(c, k, r);
			} else {
				debug("go right: r(%d)", dir2(r));
				fill_go_right(c, k, r);
			}
			fill_latter_body(c, k, r);

			if(fill_check_term(c, k, r)) {
				debug("term");
				stat = TERM; break;
			}
			if(fill_check_chain(c, k, r)) {
				debug("chain");
				stat = CHAIN; break;
			}
			if(fill_check_mem(c, k, r)) {
				debug("mem");
				stat = MEM; break;
			}

			/** debug */
			print_lane(curr, c.pdp);
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
			c.dp.ep = (c.dp.sp = c.pdp = malloc(c.size)) + c.size;
			debug("mem: c.size(%llu)", c.size);

			/** chain */
			chain_push_ivec(c);
			print_lane(c.v.pv, c.v.pv + c.v.plen*sizeof(cell_t));
			print_lane(c.v.cv, c.v.cv + c.v.clen*sizeof(cell_t));
			ret = CALL_FUNC(BASE, SUFFIX)(ctx, &c);

			/** pop */
			free(c.dp.sp);
			c.dp = mdp;
		} else if(stat == CHAIN) {
			/** chain to alternatve algorithm */
			debug("chain: %p", func_next(k, CALL_FUNC(BASE, SUFFIX)));
			chain_push_ivec(c);
			print_lane(c.v.pv, c.v.pv + c.v.plen*sizeof(cell_t));
			print_lane(c.v.cv, c.v.cv + c.v.clen*sizeof(cell_t));
			ret = func_next(k, CALL_FUNC(BASE, SUFFIX))(ctx, &c);
		} else if(stat == CAP) {
			/** chain to the cap algorithm */
			debug("cap: %p", k.f->cap);
			chain_push_ivec(c);
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
					wr_alloc(c.l, 0); proc->l = c.l;
					debug("wr_alloc without traceback");
					return SEA_SUCCESS;
				}
			}

			/** トレースしない場合はここでリターン */
			if(k.f->pushm == NULL) {			/** 汚い実装 */
				debug("return without traceback");
				return SEA_SUCCESS;
			}

			/** initialize traceback */
			wr_alloc(c.l, c.p); proc->l = c.l;
			debug("wr_alloc: c.l.p(%p)", c.l.p);
		}

		/** restore pdp */
		c.pdp = spdp;

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
		debug("trace finish: c.p(%lld)", c.p);
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
