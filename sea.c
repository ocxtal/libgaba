
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
#include "include/sea.h"		/* definitions of public APIs and structures */
#include "util/util.h"			/* definitions of internal utility functions */
// #include "build/config.h"

#if HAVE_ALLOCA_H
 	#include <alloca.h>
#endif

/**
 * function declarations
 * naive, twig, branch, trunk, balloon, bulge, cap
 */
sea_int_t
(*func_table)(
	struct sea_context const *ctx,
	struct sea_process *proc)[3][3][7] = {
	{
		{NULL, NULL, NULL, NULL, NULL, NULL, NULL},
		{NULL, NULL, NULL, NULL, NULL, NULL, NULL},
		{NULL, NULL, NULL, NULL, NULL, NULL, NULL}
	},
	{
		{NULL, NULL, NULL, NULL, NULL, NULL},
		{naive_linear_dynamic, twig_linear_dynamic, branch_linear_dynamic, trunk_linear_dynamic, balloon_linear_dynamic, NULL, NULL},
		{naive_linear_guided, twig_linear_guided, branch_linear_guided, trunk_linear_guided, balloon_linear_guided, NULL, NULL}
	},
	{
		{NULL, NULL, NULL, NULL, NULL, NULL},
		{naive_affine_dynamic, twig_affine_dynamic, branch_affine_dynamic, trunk_affine_dynamic, balloon_affine_dynamic, NULL, NULL},
		{naive_affine_guided, twig_affine_guided, branch_affine_guided, trunk_affine_guided, balloon_affine_guided, NULL, NULL}
	}
};

/**
 * @fn aligned_malloc
 *
 * @brief an wrapper of posix_memalign
 *
 * @param[in] size : size of memory in bytes.
 * @param[in] align : alignment size.
 *
 * @return a pointer to the allocated memory.
 */
void *aligned_malloc(size_t size, size_t align)
{
	void *ptr;
	posix_memalign(&ptr, align, size);
	return(ptr);
}

/**
 * @fn aligned_free
 *
 * @brief free memory which is allocated by aligned_malloc
 *
 * @param[in] ptr : a pointer to the memory to be freed.
 */
void aligned_free(void *ptr)
{
	free(ptr);
	return;
}

/**
 * @macro AMALLOC
 *
 * @brief an wrapper macro of alloca or aligned_malloc
 */
#define ALLOCA_THRESH_SIZE		( 1000000 )		/** 1MB */

#if HAVE_ALLOCA_H
	#define AMALLOC(ptr, size, align) { \
	 	if((size) > ALLOCA_THRESH_SIZE) { \
	 		(ptr) = aligned_malloc(size, align); \
	 	} else { \
	 		(ptr) = alloca(size); (ptr) = (((ptr)+(align)-1) / (align)) * (align); \
	 	} \
	}
#else
	#define AMALLOC(ptr, size, align) { \
		(ptr) = aligned_malloc(size, align); \
	}
#endif /* #if HAVE_ALLOCA_H */

/**
 * @macro AFREE
 *
 * @breif an wrapper macro of alloca or aligned_malloc
 */
#if HAVE_ALLOCA_H
	#define AFREE(ptr, size) { \
	 	if((size) > ALLOCA_THRESH_SIZE) { \
	 		aligned_free(ptr); \
	 	} \
	}
#else
	#define AFREE(ptr, size) { \
		aligned_free(ptr); \
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
static sea_int_t
sea_init_flags_vals(
	struct sea_context *ctx,
	int32_t flags,
	int8_t m,
	int8_t x,
	int8_t gi,
	int8_t ge,
	int32_t tx,
	int32_t tc,
	int32_t tb)
{
	sea_int_t int_flags = flags;		/* internal copy of the flags */
	sea_int_t error_label = SEA_ERROR;

	/** default: affine-gap cost */
	if((int_flags & SEA_FLAGS_MASK_COST) == 0) {
		int_flags = (int_flags & ~SEA_FLAGS_MASK_COST) | SEA_AFFINE_GAP_COST;
	}

	/** default: banded DP */
	if((int_flags & SEA_FLAGS_MASK_DP) == 0) {
		int_flags = (int_flags & ~SEA_FLAGS_MASK_DP) | SEA_DYNAMIC;
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
		int_flags = (int_flags & ~SEA_FLAGS_MASK_ALN) | SEA_ALN_DIR;
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
	ctx->m = m;
	ctx->x = x;
	ctx->gi = gi;
	ctx->ge = ge;

	/* check if thresholds are proper */
	if(tx < 0 || tc < 0 || tb < 0) {
		return SEA_ERROR_INVALID_ARGS;
	}
	ctx->tx = tx;
	ctx->tc = tc;
	ctx->tb = tb;

	ctx->bw = 32;			/** fixed!! */

	if((int_flags & SEA_FLAGS_MASK_ALG) == SEA_SW) {
		ctx->min = 0;
	} else {
		ctx->min = SEA_CELL_MIN;
	}
	ctx->alg = int_flags & SEA_FLAGS_MASK_ALG;

	/* special parameters */
	ctx->mask = 0;			/** for the mask-match algorithm */
	ctx->k = 4;				/** search length stretch ratio: default is 4 */
	ctx->isize = ALLOCA_THRESH_SIZE;	/** inital matsize = 1M */
	ctx->memaln = 256;	/** (MAGIC) memory alignment size */

	/* push flags to the context */
	ctx->flags = int_flags;

	error_label = SEA_SUCCESS;
	return(error_label);
}

/**
 * @fn sea_init_io
 *
 * @brief initialize sea_io_funcs structure.
 *
 * @param[ref] io : a pointer to struct sea_io_funcs.
 * @param[in] flags : option flags.
 *
 * @return SEA_SUCCESS if proper pointers are set, corresponding error number if failed.
 */
sea_int_t sea_init_io(
	struct sea_io_funcs *io,
	sea_int_t flags)
{
	sea_int_t error_label = SEA_ERROR;

	switch(flags & SEA_FLAGS_MASK_SEQ) {
		default:
			io->_pop = NULL;
			break;
	}
	switch(flags & SEA_FLAGS_MASK_ALN) {
		default:
			io->_pushm = NULL;
			io->_pushx = NULL;
			io->_pushi = NULL;
			io->_pushd = NULL;
			break;
	}
	error_label = SEA_SUCCESS;
	return(error_label);
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
struct sea_context *sea_init(
	int32_t flags,
	int8_t m,
	int8_t x,
	int8_t gi,
	int8_t ge,
	int32_t tx,
	int32_t tc,
	int32_t tb)
{
	struct sea_context *ctx = NULL;
	sea_int_t error_label = SEA_ERROR;


	/** malloc sea_context */
	ctx = (struct sea_context *)malloc(sizeof(struct sea_context));
	if(ctx == NULL) {
		error_label = SEA_ERROR_OUT_OF_MEM;
		goto _sea_init_error_handler;
	}

	/** init value fields of sea_context */
	if((error_label = sea_init_flags_vals(&ctx, flags, m, x, gi, ge, tx, tc, tb)) != SEA_SUCCESS) {
		goto _sea_init_error_handler;
	}

	/* check if error_label is success */
	if(error_label != SEA_SUCCESS) {
		goto _sea_init_error_handler;
	}

	/**
	 * initialize sea_funcs.
	 */
	ctx.f = (struct sea_funcs *)malloc(sizeof(struct sea_funcs));
	if(ctx.f == NULL) {
		error_label = SEA_ERROR_OUT_OF_MEM;
		goto _sea_init_error_handler;
	}

	/**
	 * initialize alignment functions
	 */
	sea_int_t cost_idx = (ctx.flags & SEA_FLAGS_MASK_COST) >> SEA_FLAGS_POS_COST;
	sea_int_t dp_idx = (ctx.flags & SEA_FLAGS_MASK_DP) >> SEA_FLAGS_POS_DP;

	ctx->f.twig    = func_table[cost_idx][dp_idx][1];
	ctx->f.branch  = func_table[cost_idx][dp_idx][2];
	ctx->f.trunk   = func_table[cost_idx][dp_idx][3];
	ctx->f.balloon = func_table[cost_idx][dp_idx][4];
	ctx->f.bulge   = func_table[cost_idx][dp_idx][5];
	ctx->f.cap     = func_table[cost_idx][dp_idx][6];

	if(ctx->f.twig == NULL || ctx->f.branch == NULL || ctx->f.trunk == NULL
		|| ctx->f.balloon == NULL || ctx->f.bulge == NULL || ctx->f.cap == NULL) {
		error_label = SEA_ERROR_INVALID_ARGS;
		goto _sea_init_error_handler;
	}

	/**
	 * set seq a reader
	 */
	switch(ctx.flags & SEA_FLAGS_MASK_SEQ_A) {
		case SEA_SEQ_ASCII:
			ctx->f.popa = _pop_ascii;
			break;
		case SEA_SEQ_4BIT:
			ctx->f.popa = _pop_4bit;
			break;
		case SEA_SEQ_2BIT:
			ctx->f.popa = _pop_2bit;
			break;
		case SEA_SEQ_4BIT8PACKED:
			ctx->f.popa = _pop_4bit8packed;
			break;
		case SEA_SEQ_2BIT8PACKED:
			ctx->f.popa = _pop_2bit8packed;
			break;
		default:
			ctx->f.popa = NULL;
			break;
	}
	if(ctx->f.popa == NULL) {
		error_label = SEA_ERROR_INVALID_ARGS;
		goto _sea_init_error_handler;
	}

	/**
	 * seq b
	 */
	switch((ctx.flags & SEA_FLAGS_MASK_SEQ_B) >> (SEA_FLAGS_POS_SEQ_B - SEA_FLAGS_POS_SEQ_A)) {
		case SEA_SEQ_ASCII:
			ctx->f.popb = _pop_ascii;
			break;
		case SEA_SEQ_4BIT:
			ctx->f.popb = _pop_4bit;
			break;
		case SEA_SEQ_2BIT:
			ctx->f.popb = _pop_2bit;
			break;
		case SEA_SEQ_4BIT8PACKED:
			ctx->f.popb = _pop_4bit8packed;
			break;
		case SEA_SEQ_2BIT8PACKED:
			ctx->f.popb = _pop_2bit8packed;
			break;
		default:
			ctx->f.popb = NULL;
			break;
	}
	if(ctx->f.popb == NULL) {
		error_label = SEA_ERROR_INVALID_ARGS;
		goto _sea_init_error_handler;
	}

	switch((ctx.flags & SEA_FLAGS_MASK_ALN)) {
		case SEA_ALN_ASCII:
			f.pushm = _pushm_ascii;
			f.pushx = _pushx_ascii;
			f.pushi = _pushi_ascii;
			f.pushd = _pushd_ascii;
			break;
		case SEA_ALN_CIGAR:
			f.pushm = _pushm_cigar;
			f.pushx = _pushx_cigar;
			f.pushi = _pushi_cigar;
			f.pushd = _pushd_cigar;
			break;
		case SEA_ALN_DIR:
			f.pushm = _pushm_dir;
			f.pushx = _pushx_dir;
			f.pushi = _pushi_dir;
			f.pushd = _pushd_dir;
			break;
		default:
			f.pushm = NULL;
			f.pushx = NULL;
			f.pushi = NULL;
			f.pushd = NULL;
			break;
	}
	if(ctx->f.pushm == NULL || ctx->f.pushx == NULL
		|| ctx->f.pushi == NULL || ctx->f.pushd == NULL) {
		error_label = SEA_ERROR_INVALID_ARGS;
		goto _sea_init_error_handler;
	}

	ctx->error_label = SEA_SUCCESS;
	return ctx;

_sea_init_error_handler:
	if(ctx != NULL) {
		if(ctx.f != NULL) {
			free(ctx.f);
			ctx.f = NULL;
		}
		memset(ctx, 0, sizeof(struct sea_context));

		/** set error label (the value can be retrieved with sea_get_error) */
		ctx->error_label = error_label;
	}
	return ctx;
}

/**
 * @fn sea_align
 *
 * @brief alignment function. 
 *
 * @param[ref] ctx : a pointer to an alignment context structure. must be initialized with sea_init function.
 * @param[in] a : a pointer to the query sequence a. see seqreader.h for more details of query sequence formatting.
 * @param[in] apos : the alignment start position in a. (0 <= apos < length(sequence a)) (or search start position in the Smith-Waterman algorithm). the alignment includes the position apos.
 * @param[in] alen : the extension length in a. (0 < alen) (to be exact, alen is search area length in the Smith-Waterman algorithm. the maximum extension length in the seed-and-extend alignment algorithm. the length of the query a to be aligned in the Needleman-Wunsch algorithm.)
 * @param[in] b : a pointer to the query sequence b.
 * @param[in] bpos : the alignment start position in b. (0 <= bpos < length(sequence b))
 * @param[in] blen : the extension length in b. (0 < blen)
 *
 * @return an pointer to the sea_result structure.
 *
 * @sa sea_init
 */
struct sea_result *sea_align(
	sea_ctx_t *ctx,
	void const *a,
	sea_int_t apos,
	sea_int_t alen,
	void const *b,
	sea_int_t bpos,
	sea_int_t blen)
{
	sea_int_t i;
	sea_int_t bw = ctx->param.bandwidth;
	sea_int_t m  = ctx->param.m,
			  gi = ctx->param.gi;
	sea_int_t alnsize;					/** size of struct sea_result and alignment string */
//	sea_int_t matsize;					/** size of dynamic programming matrix */
//	sea_int_t const masize = 256;		/** size of memory alignment */
	struct sea_result *aln = NULL;
	struct sea_vals val;
	sea_int_t error_label = SEA_ERROR;

	/* check if ctx points to valid context */
	if(ctx == NULL) {
		error_label = SEA_ERROR_INVALID_CONTEXT;
		goto _sea_error_handler;
	}
	/* check if the pointers, start position values, extension length values are proper. */
	if(a == NULL || b == NULL || apos < 0 || alen < 0 || bpos < 0 || blen < 0) {
		error_label = SEA_ERROR_INVALID_ARGS;
		goto _sea_error_handler;
	}

	/* calculate the size of sea_result (a structure to hold alignment results) */
	alnsize = sizeof(char) * (alen + blen + ctx->param.bandwidth + 1) + sizeof(struct sea_result);

	/* malloc memory of sea_result */
	aln = (struct sea_result *)malloc(alnsize);
	if(aln == NULL) {
		error_label = SEA_ERROR_OUT_OF_MEM;
		goto _sea_error_handler;
	}
	/* clean up sea_result */
	aln->aln = (void *)((struct sea_result *)aln + 1);
	((char *)(aln->aln))[0] = 0;					/** aln->aln[0] = '\0'; */
	aln->score = 0;
	aln->a = a; aln->apos = apos; aln->alen = alen;
	aln->b = b; aln->bpos = bpos; aln->blen = blen;
	aln->len = alnsize;
	aln->ctx = ctx;

	/**
	 * if one of the length is zero, returns with score = 0 and aln = "".
	 */
	if(alen == 0 || blen == 0) {
		aln->alen = aln->blen = aln->len = 0;
		return aln;
	}

	/* initialize sea_vals */
	AMALLOC(val.init, 2*bw, 1);
	#define _Q(x)		( (x) - bw/2 )
	for(i = 0; i < bw; i++) {
		val.init[i]    = -gi + (_Q(i) < 0 ? -_Q(i) : _Q(i)+1) * (2 * gi - m);
		val.init[i+bw] =       (_Q(i) < 0 ? -_Q(i) : _Q(i)  ) * (2 * gi - m);
	}
	#undef _Q

	/* malloc memory of the DP matrix */
//	matsize = ctx->fp._sea_matsize(alen, blen, bw);
//	matsize = ((matsize + masize - 1) / masize) * masize;
	AMALLOC(val.mat, ctx->param.init_matsize, ctx->param.memaln);
	if(val.mat == NULL) {
		error_label = SEA_ERROR_OUT_OF_MEM;
		goto _sea_error_handler;
	}
	val.matsize = ctx->param.init_matsize;
	val.spos.i = val.spos.j = 0;					/** (0, 0) */
	val.spos.p = val.spos.q = 0;					/** (0, 0) */
	val.epos.i = val.epos.j = 0;
	val.epos.p = val.epos.q = 0;
	val.max = 0;									/** score at (0, 0) */

	/* do alignment */
//	error_label = ctx->fp._sea_sea(aln, ctx->param, mat);
	if(ctx->diag8._sea_sea != NULL) {
		error_label = ctx->diag8._sea_sea(aln, ctx, val); /** chain: diag8 -> diag16 -> diag32 */
	} else {
		error_label = ctx->naive._sea_sea(aln, ctx, val); /** chain disabled */
	}
	if(error_label != SEA_SUCCESS) {
		goto _sea_error_handler;					/** when the path went out of the band */
	}

	/* clean DP matrix */
	AFREE(val.mat, 2*ctx->param.init_matsize);
	AFREE(val.init, bw);

	return aln;

_sea_error_handler:
	if(aln != NULL) {
//		aln->a = aln->b = aln->aln = NULL;
		((char *)(aln->aln))[0] = 0;
		aln->len = error_label;
		aln->score = 0;
		aln->apos = aln->bpos = aln->alen = aln->blen = 0;
	}
	return aln;
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
sea_int_t sea_get_error_num(
	sea_ctx_t *ctx,
	struct sea_result *aln)
{
	sea_int_t error_label = SEA_SUCCESS;
	if(ctx != NULL) {
		error_label = ctx->error_label;
	}
	if(aln != NULL) {
		error_label = aln->len;
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
	struct sea_result *aln)
{
	if(aln != NULL) {
		free(aln);
		return;
	}
	return;
}

/**
 * @fn sea_clean
 *
 * @brief clean up the alignment context structure.
 *
 * @param[in] ctx : a pointer to the alignment structure.
 *
 * @return none.
 *
 * @sa sea_init
 */
void sea_clean(
	struct sea_context *ctx)
{
	if(ctx != NULL) {
		free(ctx);
		return;
	}
	return;
}

/*
 * end of sea.c
 */
