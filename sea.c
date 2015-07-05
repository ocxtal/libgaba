
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
// #include "build/config.h"

#if HAVE_ALLOCA_H
 	#include <alloca.h>
#endif

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

uint8_t
(*rd_table[8])(
	uint8_t const *p,
	int64_t pos) = {
	NULL,
	_pop_ascii,
	_pop_4bit,
	_pop_2bit,
	_pop_4bit8packed,
	_pop_2bit8packed,
	NULL
};

struct sea_io_funcs const io_table[4][2] = {
	{
		{NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
		{NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL}
	},
	{
		{_pop_ascii, _pop_ascii, _init_ascii_f, _pushm_ascii_f, _pushx_ascii_f, _pushi_ascii_f, _pushd_ascii_f, _finish_ascii_f},
		{_pop_ascii, _pop_ascii, _init_ascii_r, _pushm_ascii_r, _pushx_ascii_r, _pushi_ascii_r, _pushd_ascii_r, _finish_ascii_r}
	},
	{
		{_pop_ascii, _pop_ascii, _init_cigar_f, _pushm_cigar_f, _pushx_cigar_f, _pushi_cigar_f, _pushd_cigar_f, _finish_cigar_f},
		{_pop_ascii, _pop_ascii, _init_cigar_r, _pushm_cigar_r, _pushx_cigar_r, _pushi_cigar_r, _pushd_cigar_r, _finish_cigar_r}
	},
	{
		{_pop_ascii, _pop_ascii, _init_dir_f, _pushm_dir_f, _pushx_dir_f, _pushi_dir_f, _pushd_dir_f, _finish_dir_f},
		{_pop_ascii, _pop_ascii, _init_dir_r, _pushm_dir_r, _pushx_dir_r, _pushi_dir_r, _pushd_dir_r, _finish_dir_r}
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
	void *ptr;
	posix_memalign(&ptr, align, size);
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
#define ALLOCA_THRESH_SIZE		( 1000000 )		/** 1MB */

#if HAVE_ALLOCA_H
	#define AMALLOC(ptr, size, align) { \
	 	if((size) > ALLOCA_THRESH_SIZE) { \
	 		(ptr) = sea_aligned_malloc(size, align); \
	 	} else { \
	 		(ptr) = alloca(size); (ptr) = (((ptr)+(align)-1) / (align)) * (align); \
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
 * @fn sea_init_flags_vals
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
static int32_t
sea_init_flags_vals(
	struct sea_consts *ct,
	int32_t flags,
	int8_t m,
	int8_t x,
	int8_t gi,
	int8_t ge,
	int32_t tx,
	int32_t tc,
	int32_t tb)
{
	int32_t int_flags = flags;		/* internal copy of the flags */

	if((int_flags & SEA_FLAGS_MASK_ALG) == 0) {
		return SEA_ERROR_UNSUPPORTED_ALG;
	}

	/** default: affine-gap cost */
	if((int_flags & SEA_FLAGS_MASK_COST) == 0) {
		int_flags = (int_flags & ~SEA_FLAGS_MASK_COST) | SEA_AFFINE_GAP_COST;
	}

	/** default input sequence format: ASCII */
	if((int_flags & SEA_FLAGS_MASK_SEQ_A) == 0) {
		int_flags = (int_flags & ~SEA_FLAGS_MASK_SEQ_A) | SEA_SEQ_A_ASCII;
	}
	if((int_flags & SEA_FLAGS_MASK_SEQ_B) == 0) {
		int_flags = (int_flags & ~SEA_FLAGS_MASK_SEQ_B) | SEA_SEQ_B_ASCII;
	}

	/** default output format: direction string.
	 * (yields RDRDRDRD for the input pair (AAAA and AAAA).
	 * use SEA_ALN_CIGAR for cigar string.)
	 */
	if((int_flags & SEA_FLAGS_MASK_ALN) == 0) {
		int_flags = (int_flags & ~SEA_FLAGS_MASK_ALN) | SEA_ALN_ASCII;
	}

	/** check if DP cost values are proper. the cost values must satisfy m >= 0, x < m, 2*gi <= x, ge <= 0. */
	if(m < 0 || x >= m || 2*gi > x || ge > 0) {
		return SEA_ERROR_INVALID_COST;
	}

	/** if linear-gap option is specified, set unused value (gap extension cost) zero. */
	if((int_flags & SEA_FLAGS_MASK_COST) == SEA_LINEAR_GAP_COST) {
		ge = 0;
	}
	
	/* push scores to context */
	ct->m = m;
	ct->x = x;
	ct->gi = gi;
	ct->ge = ge;

	/* check if thresholds are proper */
	if(tx < 0 || tc < 0 || tb < 0) {
		return SEA_ERROR_INVALID_ARGS;
	}
	ct->tx = tx;
	ct->tc = tc;
	ct->tb = tb;

	ct->bw = 32;			/** fixed!! */
	if((int_flags & SEA_FLAGS_MASK_ALG) == SEA_SW) {
		ct->min = 0;
	} else {
		ct->min = INT32_MIN + 10;
	}
	ct->alg = int_flags & SEA_FLAGS_MASK_ALG;

	/* special parameters */
//	ct->mask = 0;			/** for the mask-match algorithm */
	ct->k = 4;				/** search length stretch ratio: default is 4 */
	ct->isize = ALLOCA_THRESH_SIZE;	/** initial matsize = 1M */
	ct->memaln = 256;	/** (MAGIC) memory alignment size */

	/* push flags to the context */
//	ct->flags = int_flags;

	return(int_flags);
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
	int32_t tx,
	int32_t tc,
	int32_t tb)
{
	int32_t i;
	struct sea_context *ctx = NULL;
	int32_t error_label = SEA_ERROR;


	/** malloc sea_context */
	ctx = (struct sea_context *)malloc(sizeof(struct sea_context));
	if(ctx == NULL) {
		error_label = SEA_ERROR_OUT_OF_MEM;
		goto _sea_init_error_handler;
	}
	memset(ctx, 0, sizeof(struct sea_context));

	/** init value fields of sea_context */
	if((ctx->flags = sea_init_flags_vals(&ctx->k, flags, m, x, gi, ge, tx, tc, tb)) < SEA_SUCCESS) {
		error_label = ctx->flags;
		goto _sea_init_error_handler;
	}

	/**
	 * initialize alignment functions
	 */
	int32_t cost_idx = (ctx->flags & SEA_FLAGS_MASK_COST) >> SEA_FLAGS_POS_COST;
	if(cost_idx <= 0 || cost_idx >= 3) {
		error_label = SEA_ERROR_INVALID_ARGS;
		goto _sea_init_error_handler;
	}

	ctx->dynamic = aln_table[cost_idx][0];
	ctx->guided = aln_table[cost_idx][1];

	/**
	 * set alignment writer functions
	 */
	int32_t aln_idx = (ctx->flags & SEA_FLAGS_MASK_ALN) >> SEA_FLAGS_POS_ALN;
	if(aln_idx <= 0 || aln_idx >= 4) {
		error_label = SEA_ERROR_INVALID_ARGS;
		goto _sea_init_error_handler;
	}
	ctx->fw = io_table[aln_idx][0];
	ctx->rv = io_table[aln_idx][1];

	/**
	 * set seq a reader functions
	 */
	int32_t popa_idx = (ctx->flags & SEA_FLAGS_MASK_SEQ_A) >> SEA_FLAGS_POS_SEQ_A;
	int32_t popb_idx = (ctx->flags & SEA_FLAGS_MASK_SEQ_B) >> SEA_FLAGS_POS_SEQ_B;
	if(popa_idx <= 0 || popa_idx >= 8 || popb_idx <= 0 || popb_idx >= 8) {
		error_label = SEA_ERROR_INVALID_ARGS;
		goto _sea_init_error_handler;
	}
	ctx->fw.popa = ctx->rv.popa = rd_table[popa_idx];
	ctx->fw.popb = ctx->rv.popb = rd_table[popb_idx];

	/**
	 * initialize init vector
	 */
	ctx->v.pv = (uint8_t *)malloc(32*sizeof(uint8_t));
	ctx->v.cv = ctx->v.pv + ctx->k.bw/2;
	ctx->v.clen = ctx->v.plen = ctx->k.bw/2;
	ctx->v.size = sizeof(int8_t);

	#define _Q(x)		( (x) - ctx->k.bw/4 )
	for(i = 0; i < ctx->k.bw/2; i++) {
		debug("pv: i(%d)", i);
		((int8_t *)ctx->v.pv)[i] = -ctx->k.gi + (_Q(i) < 0 ? -_Q(i) : _Q(i)+1) * (2 * ctx->k.gi - ctx->k.m);
		debug("cv: i(%d)", i);
		((int8_t *)ctx->v.cv)[i] =              (_Q(i) < 0 ? -_Q(i) : _Q(i)  ) * (2 * ctx->k.gi - ctx->k.m);
	}
	#undef _Q

	return((sea_t *)ctx);

_sea_init_error_handler:
	if(ctx != NULL) {
		if(ctx->v.pv != NULL) {
			free(ctx->v.pv); ctx->v.pv = NULL;
		}
		free(ctx);
	}
	return((sea_t *)NULL);
}

/**
 * @fn sea_align_intl
 *
 * @brief (internal) the body of sea_align function.
 */
static
struct sea_result *sea_align_intl(
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
	struct sea_consts k = ctx->k;
	struct sea_process c;
	struct sea_coords t;
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

	/**
	 * if one of the length is zero, returns with score = 0 and aln = "".
	 */
	if(asp == aep || bsp == bep) {
		r = (struct sea_result *)malloc(sizeof(struct sea_result) + 1);
		r->a = a; r->apos = asp; r->alen = 0;
		r->b = b; r->bpos = bsp; r->blen = 0;
		r->slen = 0;
		r->plen = 0;
		r->score = 0;
		r->aln = (void *)(r + 1); *((uint8_t *)r->aln) = '\0';
		return(r);
	}

	/**
	 * initialize local context
	 */

	/** initialize init vector */
	debug("initialize vector");
	c.v = ctx->v;

	/** initialize coordinates */
	debug("initialize coordinates");
	t.i = asp + k.bw/4;								/** the top-right cell of the lane */
	t.j = bsp - k.bw/4;
	t.p = 0; //COP(asp, bsp, ctx->k.bw);
	t.q = 0;
	t.mi = asp;
	t.mj = bsp;
	t.mp = 0; //COP(asp, bsp, ctx->k.bw);
	t.mq = 0;
	c.asp = asp;
	c.bsp = bsp;
	c.aep = aep;
	c.bep = bep;
	c.alim = c.aep - k.bw/2;
	c.blim = c.bep - k.bw/2;

	/** initialize io */
	if(dir == ALN_FW) {
		rd_init(c.a, ctx->fw.popa, a);
		rd_init(c.b, ctx->fw.popb, b);
		wr_init(t.l, ctx->fw);
	} else {
		rd_init(c.a, ctx->rv.popa, a);
		rd_init(c.b, ctx->rv.popb, b);
		wr_init(t.l, ctx->rv);
	}

	/** initialize dynamic programming memory */
	c.size = k.isize;
	AMALLOC(c.dp.sp, c.size, k.memaln);
	if(c.dp.sp == NULL) {
		error_label = SEA_ERROR_OUT_OF_MEM;
		goto _sea_error_handler;
	}
	c.dp.ep = (c.pdp = c.dp.sp) + c.size;


	/** initialize alignment function pointers and direction array */
	if(guide == NULL) {
		debug("initialize dynamic band direction array");
		k.f = &ctx->dynamic;
		c.dr.sp = malloc(c.size);
		if(c.dr.sp == NULL) {
			error_label = SEA_ERROR_OUT_OF_MEM;
			goto _sea_error_handler;
		}
		c.dr.ep = (c.pdr = c.dr.sp) + c.size;
		c.pdr[0] = LEFT;		/** initial vector */
	} else {
		debug("initialize guided band direction array");
		k.f = &ctx->guided;
		c.pdr = (uint8_t *)guide;
		c.dr.ep = (c.dr.sp = NULL) + glen;
	}

	/** initialize max */
	t.max = 0;

	/* do alignment */
	error_label = k.f->twig(&k, &c, &t);
	if(error_label != SEA_SUCCESS) {
		goto _sea_error_handler;					/** when the path went out of the band */
	}

	/* finishing */
	debug("t.l.p(%p)", t.l.p);
	if(t.l.p == NULL) {
		/** when returned without traceback */
		debug("set default (i, j) coordinates");
		t.i = t.mi = asp; t.j = t.mj = bsp;
		wr_alloc(t.l, 100);
	}	
	debug("finishing: r(%p), size(%lld), pos(%lld)", t.l.p, t.l.size, t.l.pos);
	wr_finish(t.l, t.mp);
	r = (struct sea_result *)t.l.p;

	r->aln = (uint8_t *)t.l.p + t.l.pos;
	r->slen = t.l.size;
	r->plen = t.l.len;
	r->a = a;
	r->b = b;
	r->apos = t.i;
	r->bpos = t.j;
	r->alen = t.mi - t.i;
	r->blen = t.mj - t.j;
	r->score = t.max;

	/* clean DP matrix */
	AFREE(c.dp.sp, k.isize);
	if(guide == NULL) {
		free(c.dr.sp);
	}

	return(r);

_sea_error_handler:
	if(t.l.p == NULL) {
		wr_alloc(t.l, 1);
	}
	r = (struct sea_result *)t.l.p;
	memset(r, 0, sizeof(struct sea_result));
	r->score = error_label;
	r->aln = (void *)(r + 1);
	*((uint8_t *)r->aln) = '\0';
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
		(struct sea_context *)ctx,
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
		(struct sea_context *)ctx,
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
		(struct sea_context *)ctx,
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
		if(((struct sea_context *)ctx)->v.pv != NULL) {
			free(((struct sea_context *)ctx)->v.pv);
			((struct sea_context *)ctx)->v.pv = NULL;
		}
		free((struct sea_context *)ctx);
		return;
	}
	return;
}

/*
 * end of sea.c
 */
