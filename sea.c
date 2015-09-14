
/**
 * @file sea.c
 *
 * @brief a top level implementation of the libsea C API
 *
 * @author Hajime Suzuki
 * @date 12/29/2014
 * @license Apache v2
 */

#include <stdlib.h>				/* for malloc / free */
#include <string.h>				/* memset */
#include <ctype.h>				/* isdigit */
#include "sea.h"				/* definitions of public APIs and structures */
#include "util/util.h"			/* definitions of internal utility functions */
#include "arch/arch_util.h"		/* architecture-dependent utilities */
// #include "build/config.h"

#if HAVE_ALLOCA_H
 	#include <alloca.h>
#endif

#ifdef BENCH
	bench_t fill, search, trace;	/** global benchmark variables */
#endif

#if 0
struct sea_aln_funcs const aln_table[3][2] = {
	{
		{NULL, NULL, NULL, NULL, NULL, NULL},
		{NULL, NULL, NULL, NULL, NULL, NULL}
	},
	{
		{twig_linear_dynamic, branch_linear_dynamic, trunk_linear_dynamic, balloon_linear_dynamic, balloon_linear_dynamic, cap_linear_dynamic},
		{twig_linear_guided, branch_linear_guided, trunk_linear_guided, balloon_linear_guided, balloon_linear_guided, cap_linear_guided}
	},
	{
		{twig_affine_dynamic, branch_affine_dynamic, trunk_affine_dynamic, balloon_affine_dynamic, balloon_affine_dynamic, cap_affine_dynamic},
		{twig_affine_guided, branch_affine_guided, trunk_affine_guided, balloon_affine_guided, balloon_affine_guided, cap_affine_guided}
	}
};
#endif

struct sea_aln_funcs const aln_table[3][2] = {
	{
		{NULL, NULL, NULL, NULL, NULL, NULL},
		{NULL, NULL, NULL, NULL, NULL, NULL}
	},
	{
		{twig_linear_dynamic, branch_linear_dynamic, trunk_linear_dynamic, NULL, NULL, NULL},
		{twig_linear_guided, branch_linear_guided, trunk_linear_guided, NULL, NULL, NULL}
	},
	{
		{NULL, NULL, NULL, NULL, NULL, NULL},
		{NULL, NULL, NULL, NULL, NULL, NULL}
	}
};

struct sea_reader const rd_table[8] = {
	{NULL, NULL, 0},
	{NULL, _pop_ascii, 0},
	{NULL, _pop_4bit, 0},
	{NULL, _pop_2bit, 0},
	{NULL, _pop_4bit8packed, 0},
	{NULL, _pop_2bit8packed, 0},
	{NULL, NULL, 0},
	{NULL, NULL, 0},
};

struct sea_writer const wr_table[4][2] = {
	{
		{NULL, 0, 0, 0, NULL, NULL, NULL},
		{NULL, 0, 0, 0, NULL, NULL, NULL}
	},
	{
		{NULL, 0, 0, 0, _init_ascii_f, _push_ascii_f, _finish_ascii_f},
		{NULL, 0, 0, 0, _init_ascii_r, _push_ascii_r, _finish_ascii_r}
	},
	{
		{NULL, 0, 0, 0, _init_cigar_f, _push_cigar_f, _finish_cigar_f},
		{NULL, 0, 0, 0, _init_cigar_r, _push_cigar_r, _finish_cigar_r}
	},
	{
		{NULL, 0, 0, 0, _init_dir_f, _push_dir_f, _finish_dir_f},
		{NULL, 0, 0, 0, _init_dir_r, _push_dir_r, _finish_dir_r}
	}
};

/**
 * @fn sea_aligned_malloc
 *
 * @brief (internal) an wrapper of posix_memalign
 *
 * @param[in] size : size of memory in bytes.
 * @param[in] align : alignment size.
 *
 * @return a pointer to the allocated memory.
 */
void *sea_aligned_malloc(size_t size, size_t align)
{
	void *ptr = NULL;
	posix_memalign(&ptr, align, size);
	debug("posix_memalign(%p)", ptr);
	return(ptr);
}

/**
 * @fn sea_aligned_free
 *
 * @brief (internal) free memory which is allocated by sea_aligned_malloc
 *
 * @param[in] ptr : a pointer to the memory to be freed.
 */
void sea_aligned_free(void *ptr)
{
	free(ptr);
	return;
}

/**
 * @macro AMALLOC
 *
 * @brief an wrapper macro of alloca or sea_aligned_malloc
 */
#define ALLOCA_THRESH_SIZE		( 5*1024*1024 )		/** 5MB */

#if HAVE_ALLOCA_H
	#define AMALLOC(ptr, size, align) { \
	 	if((size) > ALLOCA_THRESH_SIZE) { \
	 		(ptr) = sea_aligned_malloc(size, align); \
	 	} else { \
	 		(ptr) = alloca(size); (ptr) = (void *)((int64_t)((ptr)+(align)-1) & ~((align)-1)); \
	 		debug("alloca(%p)", ptr); \
	 	} \
	}
#else
	#define AMALLOC(ptr, size, align) { \
		(ptr) = sea_aligned_malloc(size, align); \
	}
#endif /* #if HAVE_ALLOCA_H */

/**
 * @macro AFREE
 *
 * @breif (internal) an wrapper macro of alloca or sea_aligned_malloc
 */
#if HAVE_ALLOCA_H
	#define AFREE(ptr, size) { \
		if((size) > ALLOCA_THRESH_SIZE) { \
			sea_aligned_free(ptr); \
		} \
	}
#else
	#define AFREE(ptr, size) { \
		sea_aligned_free(ptr); \
	}
#endif /* #if HAVE_ALLOCA_H */

/**
 * @fn sea_init_flags
 *
 * @brief (internal) initialize flags
 */
static inline
int32_t
sea_init_flags(
	int32_t flags)
{
	debug("initializing flags");

	if((flags & SEA_FLAGS_MASK_ALG) == 0) {
		debug("invalid algorithm flag");
		return SEA_ERROR_UNSUPPORTED_ALG;
	}

	/** default: affine-gap cost */
	if((flags & SEA_FLAGS_MASK_COST) == 0) {
		flags = (flags & ~SEA_FLAGS_MASK_COST) | SEA_AFFINE_GAP_COST;
	}

	/** default input sequence format: ASCII */
	if((flags & SEA_FLAGS_MASK_SEQ_A) == 0) {
		flags = (flags & ~SEA_FLAGS_MASK_SEQ_A) | SEA_SEQ_A_ASCII;
	}
	if((flags & SEA_FLAGS_MASK_SEQ_B) == 0) {
		flags = (flags & ~SEA_FLAGS_MASK_SEQ_B) | SEA_SEQ_B_ASCII;
	}

	/** default output format: direction string.
	 * (yields RDRDRDRD for the input pair (AAAA and AAAA).
	 * use SEA_ALN_CIGAR for cigar string.)
	 */
	if((flags & SEA_FLAGS_MASK_ALN) == 0) {
		flags = (flags & ~SEA_FLAGS_MASK_ALN) | SEA_ALN_ASCII;
	}

	return(flags);
}

/**
 * @fn sea_init_local_context
 *
 * @brief (internal) check arguments and fill proper values (or default values) to sea_context.
 *
 * @param[ref] ctx : a pointer to blank context.
 * @param[in] flags : option flags. see flags.h for more details.
 * @param[in] len : maximal alignment length. give 0 if you are not sure. (len >= 0)
 * @param[in] m : match award in the Dynamic Programming. (m >= 0)
 * @param[in] x :  mismatch cost. (x < m)
 * @param[in] gi : gap open cost. (or just gap cost in the linear-gap cost) (2gi <= x)
 * @param[in] ge : gap extension cost. (ge <= 0) valid only in the affine-gap cost. the total penalty of the gap with length L is gi + (L - 1)*ge.
 * @param[in] xdrop : xdrop threshold. (xdrop > 0) valid only in the seed-and-extend alignment. the extension is terminated when the score S meets S < max - xdrop.
 *
 * @return SEA_SUCCESS if proper values are set, corresponding error number if failed.
 *
 * @sa sea_init, sea_init_fp
 */
static inline
int32_t
sea_init_local_context(
	struct sea_context *ctx,
	struct sea_local_context *k,
	int8_t m,
	int8_t x,
	int8_t gi,
	int8_t ge,
	int32_t tx)
{
	int32_t flags = ctx->flags;
	int32_t popa_idx, popb_idx;

	debug("initializing local context");

	/** clear context */
	memset(k, 0, sizeof(struct sea_local_context));

	/** check if DP cost values are proper. the cost values must satisfy m >= 0, x < m, 2*gi <= x, ge <= 0. */
	if(m < 0 || x >= m || 2*gi > x || ge > 0) {
		debug("invalid costs");
		return SEA_ERROR_INVALID_COST;
	}

	/** if linear-gap option is specified, set unused value (gap extension cost) zero. */
	if((flags & SEA_FLAGS_MASK_COST) == SEA_LINEAR_GAP_COST) {
		ge = 0;
	}
	
	/* push scores to context */
	k->m = m;
	k->x = x;
	k->gi = gi;
	k->ge = ge;

	/* check if thresholds are proper */
	if(tx < 0) {
		debug("invalid xdrop threshold");
		return SEA_ERROR_INVALID_ARGS;
	}
	k->tx = tx;

	k->bw = 32;			/** fixed!! */
	if((flags & SEA_FLAGS_MASK_ALG) == SEA_SW) {
		k->min = 0;
	} else {
		k->min = INT32_MIN + 10;
	}
	k->alg = flags & SEA_FLAGS_MASK_ALG;

	/* special parameters */
	k->k = 4;						/** search length stretch ratio: default is 4 */
	k->size = ALLOCA_THRESH_SIZE;	/** initial matsize = 5M */
	k->do_trace = 1;

	/* sequence reader */
	popa_idx = (ctx->flags & SEA_FLAGS_MASK_SEQ_A) >> SEA_FLAGS_POS_SEQ_A;
	popb_idx = (ctx->flags & SEA_FLAGS_MASK_SEQ_B) >> SEA_FLAGS_POS_SEQ_B;
	if(popa_idx <= 0 || popa_idx >= 8 || popb_idx <= 0 || popb_idx >= 8) {
		debug("invalid input sequence format flag");
		return SEA_ERROR_INVALID_ARGS;
	}
	k->a = rd_table[popa_idx];
	k->b = rd_table[popb_idx];

	/* sequence writer */
	k->l = ctx->fw;

	return SEA_SUCCESS;
}

/**
 * @fn sea_init_joint_tail
 *
 * @brief initialize joint_tail
 */
static inline
int32_t
sea_init_joint_tail(
	struct sea_context *ctx,
	struct sea_joint_tail *jt)
{
	int i;

	debug("initializing joint_tail");

	jt->p = 1;				/** (p, q) = (1, 0) */
	jt->i = ctx->k.bw/2;	/** (i, j) = (0, 0) */
	jt->v = (uint8_t *)ctx->pv;
	jt->bpc = 8;			/** 8bit */
	jt->d2 = SEA_TOP<<2 | SEA_LEFT;

	#define _Q(x)		( (x) - ctx->k.bw/4 )
	for(i = 0; i < ctx->k.bw/2; i++) {
		ctx->pv[i] = -ctx->k.gi + (_Q(i) < 0 ? -_Q(i) : _Q(i)+1) * (2 * ctx->k.gi - ctx->k.m);
		ctx->cv[i] =              (_Q(i) < 0 ? -_Q(i) : _Q(i)  ) * (2 * ctx->k.gi - ctx->k.m);
	}
	#undef _Q

	return SEA_SUCCESS;
}

/**
 * @fn sea_init
 *
 * @brief constructs and initializes an alignment context.
 *
 * @param[in] flags : option flags. see flags.h for more details.
 * @param[in] len : maximal alignment length. len must hold len > 0.
 * @param[in] m : match award in the Dynamic Programming. (m >= 0)
 * @param[in] x :  mismatch cost. (x < m)
 * @param[in] gi : gap open cost. (or just gap cost in the linear-gap cost) (2gi <= x)
 * @param[in] ge : gap extension cost. (ge <= 0) valid only in the affine-gap cost. the total penalty of the gap with length L is gi + (L - 1)*ge.
 * @param[in] xdrop : xdrop threshold. (xdrop > 0) valid only in the seed-and-extend alignment. the extension is terminated when the score S meets S < max - xdrop.
 *
 * @return a pointer to sea_context structure.
 *
 * @sa sea_free, sea_sea
 */
sea_t *sea_init(
	int32_t flags,
	int8_t m,
	int8_t x,
	int8_t gi,
	int8_t ge,
	int32_t tx)
{
	struct sea_context *ctx = NULL;
	int32_t cost_idx, aln_idx;
	int32_t error_label = SEA_ERROR;

	debug("initializing context");

	/** malloc sea_context */
	ctx = (struct sea_context *)sea_aligned_malloc(sizeof(struct sea_context), 32);
	if(ctx == NULL) {
		debug("out of mem");
		error_label = SEA_ERROR_OUT_OF_MEM;
		goto _sea_init_error_handler;
	}
	memset(ctx, 0, sizeof(struct sea_context));

	/** check flags */
	if((ctx->flags = sea_init_flags(flags)) < SEA_SUCCESS) {
		debug("failed to init flags");
		error_label = ctx->flags;
		goto _sea_init_error_handler;
	}

	/** initialize alignment functions */
	cost_idx = (ctx->flags & SEA_FLAGS_MASK_COST) >> SEA_FLAGS_POS_COST;
	if(cost_idx <= 0 || cost_idx >= 3) {
		debug("invalid cost flag");
		error_label = SEA_ERROR_INVALID_ARGS;
		goto _sea_init_error_handler;
	}
	ctx->dynamic = aln_table[cost_idx][0];
	ctx->guided = aln_table[cost_idx][1];

	/** set alignment writer functions */
	aln_idx = (ctx->flags & SEA_FLAGS_MASK_ALN) >> SEA_FLAGS_POS_ALN;
	if(aln_idx <= 0 || aln_idx >= 4) {
		debug("invalid alignment string flag");
		error_label = SEA_ERROR_INVALID_ARGS;
		goto _sea_init_error_handler;
	}
	ctx->fw = wr_table[aln_idx][0];
	ctx->rv = wr_table[aln_idx][1];

	/** initialize local context */
	if((error_label = sea_init_local_context(ctx, &ctx->k, m, x, gi, ge, tx)) != SEA_SUCCESS) {
		debug("failed to initialize local context template");
		goto _sea_init_error_handler;
	}

	/** initialize init vector */
	if((error_label = sea_init_joint_tail(ctx, &ctx->jt)) != SEA_SUCCESS) {
		debug("failed to initialize joint_tail template");
		goto _sea_init_error_handler;
	}

	return((sea_t *)ctx);

_sea_init_error_handler:
	if(ctx != NULL) {
		free(ctx);
	}
	return((sea_t *)NULL);
}

/**
 * @fn sea_align_intl
 *
 * @brief (internal) the body of sea_align function.
 */
static inline
struct sea_result *
sea_align_intl(
	struct sea_context const *ctx,
	void const *a,
	int64_t asp,
	int64_t aep,
	void const *b,
	int64_t bsp,
	int64_t bep,
	uint8_t const *guide,
	int64_t glen,
	int32_t dir)
{
	struct sea_result *r = NULL;
	int32_t error_label = SEA_ERROR;

	debug("enter sea_align");
	/* check if ctx points to valid context */
	if(ctx == NULL) {
		debug("invalid context: ctx(%p)", ctx);
		error_label = SEA_ERROR_INVALID_CONTEXT;
		goto _sea_error_handler;
	}

	/* check if the pointers, start position values, extension length values are proper. */
	if(a == NULL || b == NULL || asp < 0 || aep < asp || bsp < 0 || bep < bsp) {
		debug("invalid args: a(%p), apos(%lld), alen(%lld), b(%p), bpos(%lld), blen(%lld)",
			a, asp, aep, b, bsp, bep);
		error_label = SEA_ERROR_INVALID_ARGS;
		goto _sea_error_handler;
	}

	/** if one of the length is zero, returns with score = 0 and aln = "". */
	if(asp == aep || bsp == bep) {
		goto _sea_null_result;
	}

	/** malloc memory */
	int64_t mem_size = ctx->k.size;
	uint8_t *base = NULL;
	AMALLOC(base, mem_size, 32);
	if(base == NULL) {
		error_label = SEA_ERROR_OUT_OF_MEM;
		goto _sea_error_handler;
	}

	/** initialize local context */
	debug("sizeof(struct sea_local_context) (%lu)", sizeof(struct sea_local_context));
	int64_t const base_size = sizeof(struct sea_local_context)
							+ sizeof(struct sea_joint_tail);
	struct sea_local_context *k = (struct sea_local_context *)base;
	_aligned_block_memcpy(k, &ctx->k, base_size);
	k->pdp = base + base_size;
	k->tdp = base + mem_size;
	k->asp = asp;
	k->bsp = bsp;
	k->aep = aep;
	k->bep = bep;

	/** initialize io */
	if(dir == ALN_FW) {
		_aligned_block_memcpy(&k->l, &ctx->fw, sizeof(struct sea_writer));
	} else {
		_aligned_block_memcpy(&k->l, &ctx->rv, sizeof(struct sea_writer));
	}

	/** initialize alignment function pointers and direction array */
	if(guide == NULL) {
		debug("dynamic band");
		k->f = &ctx->dynamic;
		k->pdr = NULL;
		k->tdr = NULL;
	} else {
		debug("guided band");
		k->f = &ctx->guided;
		k->pdr = (uint8_t *)guide;
		k->tdr = (uint8_t *)guide + glen;
	}

	/** set pointers */
	k->a.p = (uint8_t *)a;			/** sequence a */
	k->b.p = (uint8_t *)b;			/** sequence b */

	/** set coordinate */
	_tail(k->pdp, i) += asp;

	/* do alignment */
	if((error_label = k->f->twig(k)) != SEA_SUCCESS) {
		goto _sea_error_handler;					/** when the path went out of the band */
	}

	/* finishing */
	debug("t.l.p(%p)", k->l.p);
	if(k->l.p == NULL) {
		goto _sea_null_result;
	}	
	debug("finishing: r(%p), size(%lld), pos(%lld)", k->l.p, k->l.size, k->l.pos);
	wr_finish(k->l);
	r = (struct sea_result *)k->l.p;

	r->aln = (uint8_t *)k->l.p + k->l.pos;
	r->slen = k->l.size;
	r->plen = k->l.len;
	r->a = a;
	r->b = b;
	r->apos = _head(k->pdp, i);
	r->bpos = _head(k->pdp, p) - (_head(k->pdp, i) - k->asp) + k->bsp;
	r->alen = k->mi - r->apos;
	r->blen = (k->mp - (k->mi - k->asp) + k->bsp) - r->bpos;
	r->score = k->max;

	/* clean DP matrix */
	AFREE(base, mem_size);
	return(r);

_sea_null_result:
	error_label = 0;	/** == SEA_SUCCESS */
_sea_error_handler:
	r = (struct sea_result *)malloc(sizeof(struct sea_result) + 1);
	r->a = a; r->apos = asp; r->alen = 0;
	r->b = b; r->bpos = bsp; r->blen = 0;
	r->slen = 0;
	r->plen = 0;
	r->score = error_label;
	r->aln = (void *)(r + 1); *((uint8_t *)r->aln) = '\0';
	return(r);
}

/**
 * @fn sea_align
 *
 * @brief (API) alignment function. 
 *
 * @param[ref] ctx : a pointer to an alignment context structure. must be initialized with sea_init function.
 * @param[in] a : a pointer to the query sequence a. see seqreader.h for more details of query sequence formatting.
 * @param[in] apos : the alignment start position on a. (0 <= apos < length(sequence a)) (or search start position in the Smith-Waterman algorithm). the alignment includes the position apos.
 * @param[in] alen : the extension length on a. (0 < alen) (to be exact, alen is search area length in the Smith-Waterman algorithm. the maximum extension length in the seed-and-extend alignment algorithm. the length of the query a to be aligned in the Needleman-Wunsch algorithm.)
 * @param[in] b : a pointer to the query sequence b.
 * @param[in] bpos : the alignment start position on b. (0 <= bpos < length(sequence b))
 * @param[in] blen : the extension length on b. (0 < blen)
 *
 * @return an pointer to the sea_result structure.
 *
 * @sa sea_init
 */
sea_res_t *sea_align(
	sea_t const *ctx,
	void const *a,
	int64_t asp,
	int64_t aep,
	void const *b,
	int64_t bsp,
	int64_t bep,
	uint8_t const *guide,
	int64_t glen)
{
	return((sea_res_t *)sea_align_intl(
		(struct sea_context const *)ctx,
		a, asp, aep,
		b, bsp, bep,
		guide, glen, ALN_FW));
}

/**
 * @fn sea_align_f
 * @brief (API) the same as sea_align.
 */
sea_res_t *sea_align_f(
	sea_t const *ctx,
	void const *a,
	int64_t asp,
	int64_t aep,
	void const *b,
	int64_t bsp,
	int64_t bep,
	uint8_t const *guide,
	int64_t glen)
{
	return((sea_res_t *)sea_align_intl(
		(struct sea_context const *)ctx,
		a, asp, aep,
		b, bsp, bep,
		guide, glen, ALN_FW));
}

/**
 * @fn sea_align_r
 * @brief (API) the reverse variant of sea_align.
 */
sea_res_t *sea_align_r(
	sea_t const *ctx,
	void const *a,
	int64_t asp,
	int64_t aep,
	void const *b,
	int64_t bsp,
	int64_t bep,
	uint8_t const *guide,
	int64_t glen)
{
	return((sea_res_t *)sea_align_intl(
		(struct sea_context const *)ctx,
		a, asp, aep,
		b, bsp, bep,
		guide, glen, ALN_RV));
}

/** high-level APIs for constructing seed-and-extend local alignment algorithms */
/**
 * @fn sea_align_checkpoint
 * @brief (API) do checkpoint alignment
 */
sea_res_t *sea_align_checkpoint(
	sea_t const *ctx,
	void const *a,
	void const *b,
	struct sea_checkpoint *cp)
{
	return NULL;
}

/**
 * @sea_align_finish
 * @brief (API) do finishing (the guided Smith-Waterman) alignment
 */
sea_res_t *sea_align_finish(
	sea_t const *ctx,
	void const *a,
	void const *b,
	uint8_t const *guide,
	int64_t glen)
{
	return NULL;
}

/**
 * @fn isclip
 * @brief (internal) check if a is a clip character.
 */
static inline
int isclip(char c)
{
	return(c == SEA_CLIP_HARD || c == SEA_CLIP_SOFT);
}

/**
 * @fn sea_add_clips
 * @brief add soft / hard clips at the ends of the cigar string.
 */
void sea_add_clips(
	sea_t const *ctx,
	sea_res_t *aln,
	int64_t hlen,
	int64_t tlen,
	char type)
{
	uint8_t *p;
	char buf[32];
	int64_t blen;

	if(ctx == NULL || aln == NULL || aln->slen <= 0) {
		return;
	}
	/** check if aln->aln is a ciger string */
	if(!isalnum(aln->aln[0])) {
		return;
	}
	/** check if the alignment already has clips */
	p = aln->aln;
	while(isdigit(*p)) { p++; }
	if(isclip(*p) || isclip(aln->aln[aln->slen-1])) {
		return;
	}
	/** add clips */
	if(hlen > 0) {
		/** add head clip */
		aln->slen += sprintf((char *)aln->aln + aln->slen, "%lld%c", hlen, type);
	}
	if(tlen > 0) {
		/** add tail clip */
		blen = sprintf(buf, "%lld%c", tlen, type);
		aln->aln -= blen;
		aln->slen += blen;
		strncpy((char *)aln->aln, buf, blen);
	}
	/** adjust apos, alen, bpos, and blen */
	if(type == SEA_CLIP_SOFT) {
		aln->apos -= hlen;
		aln->bpos -= hlen;
		aln->alen += (hlen + tlen);
		aln->blen += (hlen + tlen);
	}
	return;
}

/**
 * @fn sea_get_error_num
 *
 * @brief extract error number from a context or a result
 *
 * @param[ref] ctx : a pointer to an alignment context structure. can be NULL.
 * @param[ref] aln : a pointer to a result structure. can be NULL.
 *
 * @return error number, defined in sea_error
 */
int sea_get_error_num(
	sea_t const *ctx,
	sea_res_t const *aln)
{
	int32_t error_label = SEA_SUCCESS;
	if((struct sea_result *)aln != NULL) {
		error_label = aln->score;
	}
	return error_label;
}

/**
 * @fn sea_aln_free
 *
 * @brief clean up sea_result structure
 *
 * @param[in] aln : an pointer to sea_result structure.
 *
 * @return none.
 *
 * @sa sea_sea
 */
void sea_aln_free(
	sea_t const *ctx,
	sea_res_t *aln)
{
	if((struct sea_result *)aln != NULL) {
		free((struct sea_result *)aln);
		return;
	}
	return;
}

/**
 * @fn sea_close
 *
 * @brief clean up the alignment context structure.
 *
 * @param[in] ctx : a pointer to the alignment structure.
 *
 * @return none.
 *
 * @sa sea_init
 */
void sea_close(
	sea_t *ctx)
{
	if((struct sea_context *)ctx != NULL) {
		free((struct sea_context *)ctx);
		return;
	}
	return;
}

/*
 * end of sea.c
 */
