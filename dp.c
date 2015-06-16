
/**
 * @file dp.c
 * @brief a skeleton of the chainable dynamic programming algorithm
 *
 * @author Hajime Suzuki
 * @date 2015/5/7
 * @licence Apache v2.
 */
#include <stdio.h>				/** for debug */
#include "util/log.h"			/** for debug */
#include <stdlib.h>				/** for `NULL' */
#include <string.h>				/** for memcpy */
#include <stdint.h>				/** int8_t, int16_t, ... */
#include <stddef.h>				/** offsetof */
#include "include/sea.h"		/** global declarations */
#include "util/util.h"			/** internal declarations */
#include "variant/variant.h"	/** dynamic programming variants */

int32_t
DECLARE_FUNC_GLOBAL(BASE, SUFFIX)(
	struct sea_consts const *ctx,
	struct sea_process *proc,
	struct sea_coords *co)
{
	debug("entry point: (%p)", CALL_FUNC(BASE, SUFFIX));

	/** check if arguments are sanity */ {
		if(ctx == NULL || proc == NULL) {
			debug("invalid pointer detected: ctx(%p), proc(%p)", ctx, proc);
			return SEA_ERROR_INVALID_ARGS;
		}
	}

	/** load contexts onto the stack */
	struct sea_consts k = *ctx;
	struct sea_process c = *proc;
	struct sea_coords t = *co;

	/** make the dp pointer aligned */ {
		int64_t a = k.memaln;
		c.pdp = (void *)((((uint64_t)c.pdp + a - 1) / a) * a);
	}

	/** variable declarations */
	int64_t sp = t.p;
	cell_t *pb = (cell_t *)(c.pdp + bpl(c));
	int8_t stat = CONT;

	/** check direction array size */
	if(c.dr.sp != NULL) {
		if((c.dp.ep - c.pdp) > bpl(c) * ((uint8_t *)c.dr.ep - (uint8_t *)c.dr.sp - t.p)) {
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
		fill_decl(t, c, k, r);
		fill_init(t, c, k, r);
		debug("init: r(%d), t.max(%d)", dir2(r), t.max);
//		print_lane(curr, c.pdp);

		stat = CAP;
		while(t.i <= c.alim && t.j <= c.blim && dir_check_term(r, t, c)) {
			#ifdef DEBUG
				curr = (cell_t *)c.pdp;		/** debug */
			#endif

			fill_former_body(t, c, k, r);
			if(dir(r) == TOP) {
				debug("go down: r(%d)", dir2(r));
				fill_go_down(t, c, k, r);
			} else {
				debug("go right: r(%d)", dir2(r));
				fill_go_right(t, c, k, r);
			}
			fill_latter_body(t, c, k, r);

			/** debug */
			debug("pos: %lld, %lld, %lld, %lld, usage(%zu)", t.i, t.j, t.p, t.q, (size_t)(c.pdp - (void *)pb));
			print_lane(curr, c.pdp);

			if(fill_check_term(t, c, k, r)) {
				debug("term detected");
				stat = TERM; break;
			}
			if(fill_check_chain(t, c, k, r)) {
				debug("chain detected");
				stat = CHAIN; break;
			}
/*			if(fill_check_alt(t, c, k, r)) {
				debug("alt detected");
				stat = ALT; break;
			}*/
			if(fill_check_mem(t, c, k, r)) {
				debug("mem detected");
				stat = MEM; break;
			}
		}
		if(stat == CAP) {
			debug("edge violation: t.i(%lld), c.alim(%lld), t.j(%lld), c.blim(%lld)", t.i, c.alim, t.j, c.blim);
		}

		debug("finish: stat(%d)", stat);
		fill_finish(t, c, k, r);
	}

	/** chain */ {
		int32_t (*cfn)(struct sea_consts const *, struct sea_process *, struct sea_coords *) = NULL;
		struct sea_coords tn;

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

		if(cfn != NULL) {
			/** chain chain chain */
			debug("chain: cfn(%p), stat(%d)", cfn, stat);
			struct sea_mem mdp = c.dp;
			int32_t ret = SEA_SUCCESS;

			/** save coordinates */
			tn = t;

			/** malloc memory if stat == MEM */
			if(stat == MEM) {
				c.size *= 2;
				c.dp.ep = (c.dp.sp = malloc(c.size)) + c.size;
				memcpy(c.dp.sp,
					(cell_t *)c.pdp - chain_save_len(t, c, k),
					sizeof(cell_t) * chain_save_len(t, c, k));
				c.pdp = (cell_t *)c.dp.sp + chain_save_len(t, c, k);
				debug("malloc memory: size(%llu), c.pdp(%p)", c.size, c.pdp);
			}

			/** call func */
			chain_push_ivec(t, c, k);
			print_lane(c.v.pv, c.v.pv + c.v.plen*sizeof(cell_t));
			print_lane(c.v.cv, c.v.cv + c.v.clen*sizeof(cell_t));
			ret = cfn(ctx, &c, &tn);

			/** clean up memory if stat == MEM */
			if(stat == MEM) {
				free(c.dp.sp); c.dp = mdp;
			}

			/** check return status */
			debug("return: (%d)", ret);
			if(ret != SEA_SUCCESS) {
				return(ret);
			}
			if(k.alg != NW && (stat == TERM || search_trigger(t, tn, c, k))) {
				search_max_score(t, c, k);
				debug("check if replace is needed");
				if(tn.max > t.max) { t = tn; }
			}
		} else {
			/** termination */
			if(k.alg == NW) {
				debug("set terminal");
				search_terminal(t, c, k);
			} else {
				debug("search max score");
				search_max_score(t, c, k);
				if(t.max > (CELL_MAX - k.m)) {
					return SEA_ERROR_OVERFLOW;
				}
				if(t.max <= 0) {
					t.i = t.mi = c.asp; t.j = t.mj = c.bsp;
					debug("wr_alloc without traceback");
					return SEA_SUCCESS;
				}
			}
		}
	}

	/** traceback */ {
		debug("t.mi(%lld), t.mj(%lld), t.mp(%lld), t.mq(%lld)", t.mi, t.mj, t.mp, t.mq);
		debug("t.i(%lld), t.j(%lld), t.p(%lld), t.q(%lld)", t.i, t.j, t.p, t.q);
		/** check if wr_alloc is needed */
		if(t.mp <= t.p) {	/** これバグになる */
			t.i = t.mi; t.j = t.mj;
			t.p = t.mp; t.q = t.mq;
			wr_alloc(t.l, t.mp); co->l = t.l;
			debug("wr_alloc: t.l.p(%p)", t.l.p);
		}

		debug("trace");
		trace_decl(t, c, k, r);
		trace_init(t, c, k, r);
		if(sp == 0) {
			while(t.i > c.asp && t.j > c.bsp) {
				debug("%lld, %lld, %lld, %lld", t.i, t.j, t.p, t.q);
				debug("topleftq(%d), dir2(%d)", topleftq(r, t, c), dir2(r));
				trace_body(t, c, k, r);
			}
			while(t.i > c.asp) { t.i--; wr_pushd(t.l); }
			while(t.j > c.bsp) { t.j--; wr_pushi(t.l); }
		} else {
			while(t.p > sp) {
				debug("%lld, %lld, %lld, %lld", t.i, t.j, t.p, t.q);
				debug("topleftq(%d), dir2(%d)", topleftq(r, t, c), dir2(r));
				trace_body(t, c, k, r);
			}
		}
		debug("trace finish: t.mp(%lld), t.l.pos(%lld), t.l.size(%lld)", t.mp, t.l.pos, t.l.size);
		trace_finish(t, c, k, r);
	}

	/** write back the local context to the upper stack */
	*co = t;
	//*proc = c;

	/** finish!! */
	return SEA_SUCCESS;
}

/**
 * end of dp.c
 */
