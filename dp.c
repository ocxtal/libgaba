
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
#include "arch/arch_util.h"		/** architecture-dependent utilities */
#include "variant/variant.h"	/** dynamic programming variants */

extern bench_t fill, search, trace;

/**
 * @fn fill
 * @brief fill-in the matrix until the detection of termination
 */
struct sea_chain_status
func2(VARIANT_LABEL, _fill)(
	struct sea_dp_context *this,
	uint8_t *pdp,					/** tail of the previous section */
	struct sea_section_pair *sec)
{
	int64_t i;
	struct sea_dp_context register *k = this;
	int32_t stat = CONT;
 
	debug("enter fill (%p), pdp(%p)", func2(VARIANT_LABEL, _fill), pdp);

	bench_start(fill);
	fill_decl(k);					/** declare variables */
	fill_init(k, pdp, sec);			/** load vectors and coordinates to registers */

	debug("start loop");
	/** bulk fill */
	while(!fill_check_term(k)) {
		debug("p(%lld)", p);
		fill_start(k);				/** prefetch */
		for(i = 0; i < BLK/2; i++) {
			fill_body(k);			/** unroll loop: 1 */
			fill_body(k);			/** unroll loop: 2 */
		}
		fill_end(k);				/** save vectors */
	}

	/** check flags */
	debug("check flags");
	if(fill_test_xdrop(k) < 0) {
		debug("term");
		stat = TERM;  goto label2(VARIANT_LABEL, _fill_finish_label);
	} else if(fill_test_chain(k) < 0) {
		debug("chain");
		stat = CHAIN; goto label2(VARIANT_LABEL, _fill_finish_label);
	} else if(fill_test_mem(k) < 0) {
		debug("mem");
		stat = MEM;   goto label2(VARIANT_LABEL, _fill_finish_label);
	}

	/** cap fill */
	debug("cap fill");
	stat = TERM;
	int64_t t = 0;					/** termination flag */
	while(t == 0) {
		debug("p(%lld)", p);
		fill_start_cap(k);
		for(i = 0; i < BLK; i++) {
			if(fill_check_term_cap(k)) { break; }
			fill_body_cap(k);
		}
		debug("term cap");
		for(; i < BLK; t++, i++) {	/** increment termination flag */
			fill_empty_body(k);
		}
		fill_end_cap(k);
	}

	/** finish */
label2(VARIANT_LABEL, _fill_finish_label):
	fill_finish(k, pdp, sec);		/** store vectors and coordinates to memory */
	
	bench_end(fill);

	struct sea_chain_status s = {k->stack_top, stat};
	return(s);
}

/**
 * @fn trace
 * @brief traceback until the given start position
 */
struct sea_chain_status
func2(VARIANT_LABEL, _trace)(
	struct sea_dp_context *this,
	uint8_t *pdp)
{
	struct sea_dp_context register *k = this;
	uint8_t *tdp = _head(pdp, p_tail); \
	uint8_t *hdp = tdp - _tail(tdp, size); \

	bench_start(trace);

	trace_decl(k);
	trace_init(k, hdp, tdp, pdp);
	while(1) {
		if(trace_check_term(k)) { break; }
		trace_body(k);
		if(trace_check_term(k)) { break; }
		trace_body(k);
	}
	trace_finish(k, hdp);

	bench_end(trace);

	struct sea_chain_status s = {hdp, SEA_SUCCESS};
	return(s);
}

#if 0

#if 0
static inline
int32_t
func2(VARIANT_LABEL, _set_term)(
	struct sea_dp_context *this,
	uint8_t *pdp,
	int32_t stat)
{
	/** turn and go back (band reached the corner of the matrix) */
	if(k->alg == NW) {
		/** load terminal coordinate to (mi, mj) and (mp, mq) */
		set_terminal(k, pdp);
	}
	debug("set terminal p(%lld), q(%lld), i(%lld)", k->mp, k->mq, k->mi);
	_head(k->pdp, p) = k->mp;
	_head(k->pdp, q) = k->mq;
	_head(k->pdp, i) = k->mi;
	_head(k->pdp, score) = k->max;
	wr_alloc(k->l, _head(k->pdp, p));
	return(ret);
}
#endif

/**
 * @fn (base)
 * @brief base function of the dp
 */
struct sea_chain_status
func(VARIANT_LABEL)(
	struct sea_dp_context *this,
	uint8_t *pdp)
{
	debug("entry point: (%p)", func(VARIANT_LABEL));
	struct sea_dp_context register *k = this;
	struct sea_chain_status s;

	/** fill_in */
	s = func2(VARIANT_LABEL, _fill)(k, pdp);

	/** chain */
	int32_t (*cfn)(struct sea_dp_context *) = func_next(r.stat);
	uint8_t *ndp = NULL;
	if(stat == MEM || (k->tdp - k->pdp) < 32*1024) {
		k->pdp = ndp = malloc(k->size = 2 * k->size);
	}
	s = cfn(k, r.pdp);

	/** trace */
	debug("pdp(%p), k->pdp(%p)", pdp, k->pdp);
	int64_t sp = _tail(pdp, p);
	int64_t ep = _tail(r.pdp, p);
	int64_t p  = _head(k->pdp, p);
	debug("trace sp(%lld), ep(%lld), p(%lld), i(%lld)", sp, ep, p, _head(k->pdp, i));
	if((uint64_t)(p - sp) < (ep - sp)) {
		s = func2(VARIANT_LABEL, _trace)(k, r.pdp);
	} else {
		_aligned_block_memcpy(pdp, r.pdp, sizeof(struct sea_joint_head));
	}

	if(ndp != NULL) { free(ndp); }
	return(s);
}

/**
 * @fn prep_next_mem
 * @brief malloc the next memory, set pointers, and copy the content
 */
static inline
uint8_t *
func2(VARIANT_LABEL, _prep_next_mem)(
	struct sea_dp_context *this)
{
	uint8_t *p;
	/** malloc the next dp, dr memory */
	this->size *= 2;
	p = (uint8_t *)malloc(this->size);
	/** copy the content to be passed */
	_aligned_block_memcpy(p, this->pdp - sizeof(struct sea_joint_tail), sizeof(struct sea_joint_tail));
	this->pdp = p + sizeof(struct sea_joint_tail);
	return(p);
}

/**
 * @fn clean_next_mem
 */
static inline
int32_t
func2(VARIANT_LABEL, _clean_next_mem)(
	struct sea_dp_context *this,
	uint8_t *pdp,
	uint8_t *p)
{
	_aligned_block_memcpy(pdp, p + sizeof(struct sea_joint_tail), sizeof(struct sea_joint_head));
	this->size /= 2;
	this->pdp = pdp;
	return(0);
}

/**
 * @fn chain_impl
 * @brief chain to the next fill-in function
 */
static inline
int32_t
func2(VARIANT_LABEL, _chain)(
	struct sea_dp_context *this,
	uint8_t *pdp,
	int32_t stat)
{
	int32_t ret = SEA_SUCCESS;
	struct sea_dp_context register *k = this;

	/** retrieve the next function */
	int32_t (*cfn)(struct sea_dp_context *) = NULL;
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
	}
	return(ret);
}
#endif

/**
 * end of dp.c
 */
