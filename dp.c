
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
static inline
int32_t
func2(VARIANT_LABEL, _fill)(
	struct sea_local_context *this,
	uint8_t *pdp)
{
	struct sea_local_context register *k = this;
	int32_t stat = CONT;
 
	debug("enter fill (%p), pdp(%p)", func2(VARIANT_LABEL, _fill), pdp);

	bench_start(fill);
	fill_decl(k, r, pdp);					/** declare variables */
	fill_init(k, r, pdp);					/** load vectors and coordinates to registers */

	/** bulk fill */
	while(!fill_check_term(k, r, pdp)) {
		fill_start(k, r, pdp);
		for(int a = 0; a < BLK/2; a++) {
			/** unroll loop: 1 */
			fill_former_body(k, r, pdp);	/** former calculations */
			if(dir(r) == TOP) {
				fill_go_down(k, r, pdp);	/** shift right the vectors */
			} else {
				fill_go_right(k, r, pdp);	/** shift left */
			}
			fill_latter_body(k, r, pdp);	/** latter calculations */
			
			/** unroll loop: 2 */
			fill_former_body(k, r, pdp);	/** former calculations */
			if(dir(r) == TOP) {
				fill_go_down(k, r, pdp);	/** shift right the vectors */
			} else {
				fill_go_right(k, r, pdp);	/** shift left */
			}
			fill_latter_body(k, r, pdp);	/** latter calculations */
		}
		fill_end(k, r, pdp);
	}

	/** check flags */
	debug("check flags");
	if(fill_test_xdrop(k, r, pdp) < 0) {
		stat = TERM;  goto label2(VARIANT_LABEL, _fill_finish_label);
	} else if(fill_test_chain(k, r, pdp) < 0) {
		stat = CHAIN; goto label2(VARIANT_LABEL, _fill_finish_label);
	} else if(fill_test_mem(k, r, pdp) < 0) {
		stat = MEM;   goto label2(VARIANT_LABEL, _fill_finish_label);
	}

	/** cap fill */
	debug("cap fill");
	stat = TERM;
	while(1) {
		fill_start(k, r, pdp);
		for(int a = 0; a < BLK/2; a++) {
			/** unroll loop: 1 */
			if(fill_check_term_cap(k, r, pdp)) {
				goto label2(VARIANT_LABEL, _fill_end_label);
			}
			fill_former_body(k, r, pdp);
			if(dir(r) == TOP) {
				fill_go_down(k, r, pdp);
			} else {
				fill_go_right(k, r, pdp);
			}
			fill_latter_body_cap(k, r, pdp);

			/** unroll loop: 2 */
			if(fill_check_term_cap(k, r, pdp)) {
				goto label2(VARIANT_LABEL, _fill_end_label);
			}
			fill_former_body(k, r, pdp);
			if(dir(r) == TOP) {
				fill_go_down(k, r, pdp);
			} else {
				fill_go_right(k, r, pdp);
			}
			fill_latter_body_cap(k, r, pdp);
		}
		fill_end(k, r, pdp);
	}

	/** finish */
label2(VARIANT_LABEL, _fill_end_label):
	fill_end(k, r, pdp);
label2(VARIANT_LABEL, _fill_finish_label):
	fill_finish(k, r, pdp);					/** store vectors and coordinates to memory */

	bench_end(fill);
	return(stat);
}

/**
 * @fn prep_next_mem
 * @brief malloc the next memory, set pointers, and copy the content
 */
static inline
uint8_t *
func2(VARIANT_LABEL, _prep_next_mem)(
	struct sea_local_context *this)
{
	uint8_t *p;
	/** malloc the next dp, dr memory */
	this->size *= 2;
	p = (uint8_t *)malloc(this->size);
	/** copy the content to be passed */
	memcpy(p, this->pdp - sizeof(struct sea_joint_tail), sizeof(struct sea_joint_tail));
	this->pdp = p + sizeof(struct sea_joint_tail);
	return(p);
}

/**
 * @fn clean_next_mem
 */
static inline
int32_t
func2(VARIANT_LABEL, _clean_next_mem)(
	struct sea_local_context *this,
	uint8_t *pdp,
	uint8_t *p)
{
	memcpy(pdp, p + sizeof(struct sea_joint_tail), sizeof(struct sea_joint_head));
	this->size /= 2;
	this->pdp = pdp;
	return(0);
}

/**
 * @fn chain
 * @brief chain to the next fill-in function
 */
static inline
int32_t
func2(VARIANT_LABEL, _chain)(
	struct sea_local_context *this,
	uint8_t *pdp,
	int32_t stat)
{
	int32_t ret = SEA_SUCCESS;
	struct sea_local_context register *k = this;

	/** retrieve the next function */
	int32_t (*cfn)(struct sea_local_context *) = NULL;
	switch(stat) {
		case MEM:   cfn = func(VARIANT_LABEL); break;
		case CHAIN: cfn = func_next(k, func(VARIANT_LABEL)); break;
		case TERM:  cfn = NULL; break;
		default:    cfn = NULL; break;
	}
	if(cfn != NULL) {
		/** go forward */
		if(stat == MEM || (k->tdp - k->pdp) < 4*1024) {
			pdp = this->pdp;
			uint8_t *p = func2(VARIANT_LABEL, _prep_next_mem)(k);
			ret = cfn(k);
			func2(VARIANT_LABEL, _clean_next_mem)(k, pdp, p);
		} else {
			ret = cfn(k);	/** chain without malloc */
		}
	} else {
		/** turn and go back (band reached the corner of the matrix) */
		if(k->alg == NW) {
			/** load terminal coordinate to (mi, mj) and (mp, mq) */
			debug("set terminal");
			set_terminal(k, pdp);
		}
		_head(k->pdp, p) = k->mp;
		_head(k->pdp, q) = k->mq;
		_head(k->pdp, i) = k->mi;
		wr_alloc(k->l, _head(k->pdp, p));
	}
	return(ret);
}

/**
 * @fn trace
 * @brief traceback until the given start position
 */
static inline
int32_t
func2(VARIANT_LABEL, _trace)(
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
func(VARIANT_LABEL)(
	struct sea_local_context *this)
{
	debug("entry point: (%p)", func(VARIANT_LABEL));
	struct sea_local_context register *k = this;

	/** save dp matrix pointer, direction array pointer, and p-coordinate */
	uint8_t *pdp = k->pdp;

	/** fill_in */
	int32_t stat = func2(VARIANT_LABEL, _fill)(k, pdp);

	/** chain */
	func2(VARIANT_LABEL, _chain)(k, pdp, stat);

	/** trace */
	if(k->do_trace) {
		stat = func2(VARIANT_LABEL, _trace)(k, pdp);
	}
	k->pdp = pdp;
	return(stat);
}

/**
 * end of dp.c
 */
