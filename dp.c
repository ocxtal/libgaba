
/**
 * @file dp.c
 * @brief a skeleton of the chainable dynamic programming algorithm
 *
 * @author Hajime Suzuki
 * @date 2015/5/7
 * @licence Apache v2.
 *
 */
#include <stdlib.h>				/** for `NULL' */
#include "include/sea.h"		/** global declarations */
#include "util/util.h"			/** internal declarations */
#include "variant/variant.h"	/** dynamic programming variants */

/**
 * ctxとprocはthread localにしたい。
 * gccのtlsで十分か?
 * fiberと組み合わせると破綻する。これをうまく扱う方法はあるか?
 * 
 */

int32_t
DECLARE_FUNC_GLOBAL(BASE, SUFFIX)(
	sea_ctx_t const *ctx,
	sea_proc_t *proc)
{
	/** check if arguments are sanity */ {
		if(ctx == NULL || proc == NULL) {
			return SEA_ERROR_INVALID_ARGS;
		}
	}

	/** load contexts onto the stack */
	sea_ctx_t k = *ctx;
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
		if((c.dp.ep - c.pdp) / bpl(c) > ((uint8_t *)c.dr.ep - c.pdr)) {
			size_t s = 2 * (c.dr.ep - c.dr.sp);
			c.dr.ep = (c.dr.sp = (void *)realloc(c.dr.sp, s)) + s;
		}
	}

	/** fill-in */ {
		/** init direction and vector */
		fill_decl(c, k, r);
		fill_init(c, k, r);

		stat = CAP;
		while(c.i <= c.alim && c.j <= c.blim) {
			fill_former_body(c, k, r);
			if(dir(r) == TOP) {
				fill_go_down(c, k, r);
			} else {
				fill_go_right(c, k, r);
			}
			fill_latter_body(c, k, r);

			if(fill_check_term(c, k, r)) {
				stat = TERM; break;
			}
			if(fill_check_chain(c, k, r)) {
				stat = CHAIN; break;
			}
			if(fill_check_mem(c, k, r)) {
				stat = MEM; break;
			}
		}

		fill_finish(c, k, r);
	}

	/** chain */ {
		sea_ivec_t v;
		void *spdp = c.pdp;
		int32_t ret = SEA_SUCCESS;

		if(stat == CONT) {
			/** never reaches here */
			return SEA_ERROR;
		} else if(stat == MEM) {
			sea_mem_t mdp = {NULL, NULL};

			/** push */
			mdp = c.dp;			/** push memory context to mdp */
			c.size *= 2;		/** twice the memory */
			c.dp.ep = (c.dp.sp = c.pdp = malloc(c.size)) + c.size;

			/** chain */
			chain_push_ivec(c, v);
			ret = CALL_FUNC(BASE, SUFFIX)(ctx, &c);

			/** pop */
			free(c.dp.sp);
			c.dp = mdp;
		} else if(stat == CHAIN) {
			/** chain to alternatve algorithm */
			chain_push_ivec(c, v);
			ret = func_next(k, CALL_FUNC(BASE, SUFFIX))(ctx, &c);
		} else if(stat == CAP) {
			/** chain to the cap algorithm */
			chain_push_ivec(c, v);
			ret = k.f->cap(ctx, &c);
		} else if(stat == TERM) {
			/** search */
			if(k.alg == NW) {
				search_terminal(c, k);
			} else {
				search_max_score(c, k);
				if(c.max > (CELL_MAX - k.m)) {
					return SEA_ERROR_OVERFLOW;
				}
				if(c.max == 0) {
					c.mi = c.mj = 0;
					return SEA_SUCCESS;
				}
			}

			/** トレースしない場合はここでリターン */
			if(k.f->pushm == NULL) {			/** 汚い実装 */
				return SEA_SUCCESS;
			}

			/** initialize traceback */
			wr_alloc(c.l, c.p); proc->l = c.l;
		}

		/** restore pdp */
		c.pdp = spdp;

		/** check return status */
		if(ret != SEA_SUCCESS) {
			return(ret);
		}
	}

	/** traceback */ {
		trace_decl(c, k, r);
		trace_init(c, k, r);
		if(sp == 0) {
			while(c.mi > 0 && c.mj > 0) {
				trace_body(c, k, r);
			}
			while(c.mi > 0) { c.mi--; wr_pushd(c.l); }
			while(c.mj > 0) { c.mj--; wr_pushi(c.l); }
		} else {
			while(c.mp > sp) {
				trace_body(c, k, r);
			}
		}
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
