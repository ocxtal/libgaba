
/**
 * @file dp.c
 * @brief a skeleton of the chainable dynamic programming algorithm
 *
 * @author Hajime Suzuki
 * @date 2015/5/7
 * @licence Apache v2.
 *
 * @memo
 * prototype implementation of thisptr-based chain
 */
#include <stdio.h>				/** for debug */
#include "util/log.h"			/** for debug */
#include <stdlib.h>				/** for `NULL' */
#include <string.h>				/** for memcpy */
#include <stdint.h>				/** int8_t, int16_t, ... */
#include <stddef.h>				/** offsetof */
#include "sea.h"				/** global declarations */
#include "util/util.h"			/** internal declarations */
#include "variant/variant.h"	/** dynamic programming variants */

extern bench_t fill, search, trace;

/**
 * @fn fill
 * @brief fill-in the matrix until the detection of termination
 */
inline int32_t
DECLARE_FUNC(BASE, SUFFIX, _fill)(
	struct sea_local_context *this,
	uint8_t *pdp)
{
	struct sea_local_context register *k = this;

	bench_start(fill);
	fill_decl(k, r);					/** declare variables */
	fill_init(k, r);					/** load vectors and coordinates to registers */

	do {
		fill_start(k, r);
		for(int a = 0; a < BLK; a++) {
			fill_former_body(k, r);		/** former calculations */
			if(dir(r) == TOP) {
				fill_go_down(k, r);		/** shift right the vectors */
			} else {
				fill_go_right(k, r);	/** shift left */
			}
			fill_latter_body(k, r);		/** latter calculations */
		}
		fill_end(k, r);
	} while(fill_check_term(k, r));
	fill_finish(k, r);					/** store vectors and coordinates to memory */

	bench_end(fill);
	return(CONT);
}
#if 0
/**
 * @fn prep_next_mem
 * @brief malloc the next memory, set pointers, and copy the content
 */
inline uint8_t *
DECLARE_FUNC(BASE, SUFFIX, prep_next_mem)(
	struct sea_local_context *this)
{
	struct sea_local_context register *k = this;

	/** malloc the next dp, dr memory */
	k->size *= 2;
	k->dp.ep = (k->dp.sp = k->pdp = (uint8_t *)malloc(k->size)) + k->size;
	/** copy the content to be passed */

	return(p);
}
#endif
/**
 * @fn chain
 * @brief chain to the next fill-in function
 */
inline int32_t
DECLARE_FUNC(BASE, SUFFIX, _chain)(
	struct sea_local_context *this,
	uint8_t *pdp,
	int32_t stat)
{
	int32_t ret = SEA_SUCCESS;
	struct sea_local_context register *k = this;

	/** retrieve the next function */
	int32_t (*cfn)(struct sea_local_context *) = NULL;
	switch(stat) {
		case MEM:   cfn = CALL_FUNC_GLOBAL(BASE, SUFFIX); break;
		case CHAIN: cfn = func_next(k, CALL_FUNC_GLOBAL(BASE, SUFFIX)); break;
		case CAP:   cfn = k->f->cap; break;
		case TERM:  cfn = NULL; break;
		default:    cfn = NULL; break;
	}
	if(cfn != NULL) {
		/** go forward */
		if(stat == MEM) {
			// uint8_t *p = CALL_FUNC(BASE, SUFFIX, prep_next_mem)(k);
			ret = cfn(k);
//			free(p);
		} else {
			ret = cfn(k);	/** chain without malloc */
		}
	} else {
		/** turn and go back (band reached the corner of the matrix) */
		if(k->alg == NW) {
			/** load terminal coordinate to (mi, mj) and (mp, mq) */
			debug("set terminal");
			search_terminal(k, pdp);
		}
	}
	return(ret);
}

/**
 * @fn search
 * @brief search the max score
 */
inline int32_t
DECLARE_FUNC(BASE, SUFFIX, _search)(
	struct sea_local_context *this,
	uint8_t *pdp)
{
	/** k contains the context after chaining */
	struct sea_local_context register *k = this;

	bench_start(search);

	if(k->alg != NW) {
		debug("check re-search is needed: t1.max(%d), t2.max(%d)", t.max, co->max);
		/** compare max of the current block with the global max */
		if(search_trigger(k, pdp)) {
			search_max_score(k, pdp);
			debug("check if replace is needed");
			if(_ivec(pdp, max) > k->max - (_ivec(pdp, p) > k->mp)) {
				wr_alloc(k->l, _ivec(pdp, p));
				store_coord(k, pdp);
			}
		}
	}

	bench_end(search);
	return(0);
}

/**
 * @fn trace
 * @brief traceback until the given start position
 */
inline int32_t
DECLARE_FUNC(BASE, SUFFIX, _trace)(
	struct sea_local_context *this,
	uint8_t *pdp)
{
	struct sea_local_context register *k = this;

	bench_start(trace);

	trace_decl(k, r, pdp);
	trace_init(k, r, pdp);
	while(trace_check_term(k, r, pdp)) {
		trace_body(k, r, pdp);
	}
	trace_finish(k, r, pdp);

	bench_end(trace);
	return(SEA_SUCCESS);
}

/**
 * @fn (base)
 * @brief base function of the dp
 */
int32_t
DECLARE_FUNC_GLOBAL(BASE, SUFFIX)(
	struct sea_local_context *this)
{
	debug("entry point: (%p)", CALL_FUNC_GLOBAL(BASE, SUFFIX));
	struct sea_local_context register *k = this;

	/** save dp matrix pointer, direction array pointer, and p-coordinate */
	uint8_t *pdp = k->pdp;

	/** fill_in */
	int32_t stat = CALL_FUNC(BASE, SUFFIX, _fill)(k, pdp);

	/** chain */
	CALL_FUNC(BASE, SUFFIX, _chain)(k, pdp, stat);

	/** search */
	CALL_FUNC(BASE, SUFFIX, _search)(k, pdp);

	/** trace */
	if(k->do_trace) {
		stat = CALL_FUNC(BASE, SUFFIX, _trace)(k, pdp);
	}
	k->pdp = pdp;
	return(stat);
}

/**
 * end of dp.c
 */
