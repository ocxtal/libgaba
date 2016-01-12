
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

#ifdef BENCH
	bench_t fill, search, trace;	/** global benchmark variables */
#endif

/**
 * constants
 */
#define SEA_INIT_STACK_SIZE			( 32 * 1024 * 1024 )
#define SEA_INIT_BW					( 16 )

/**
 * @val aln_table
 * @brief fill and trace function table
 */
struct sea_aln_funcs const aln_table[2][2] = {
	{
		{
			{twig_linear_dynamic_fill, branch_linear_dynamic_fill, NULL, NULL},
			{twig_linear_dynamic_trace, branch_linear_dynamic_trace, NULL, NULL}
		},
		{
			{twig_linear_guided_fill, branch_linear_guided_fill, NULL, NULL},
			{twig_linear_guided_trace, branch_linear_guided_trace, NULL, NULL}
		}
	},
	{
		{
			{NULL, NULL, NULL, NULL},
			{NULL, NULL, NULL, NULL}
		},
		{
			{NULL, NULL, NULL, NULL},
			{NULL, NULL, NULL, NULL}
		}
	}
};

/**
 * @val rd_table
 * @brief sequence reader function table
 */
void (* const rd_table[4][8])(
	uint8_t *dst,
	uint8_t const *src,
	uint64_t pos,
	uint64_t src_len,
	uint64_t copy_len) = {
	{NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
	{NULL, _load_ascii_fw, _load_ascii_fw, _load_ascii_fw, _load_ascii_fw, _load_ascii_fw, NULL, NULL},
	{NULL, _load_ascii_fr, _load_ascii_fr, _load_ascii_fr, _load_ascii_fr, _load_ascii_fr, NULL, NULL}
//	{NULL, _load_ascii_fw, _load_4bit_fw, _load_2bit_fw, _load_4bit8packed_fw, _load_2bit8packed_fw, NULL, NULL},
//	{NULL, _load_ascii_fr, _load_4bit_fr, _load_2bit_fr, _load_4bit8packed_fr, _load_2bit8packed_fr, NULL, NULL}
};

/**
 * @val wr_table
 * @brief alignment writer function table
 */
struct sea_writer const wr_table[4][2] = {
	{
		{NULL, 0, 0},
		{NULL, 0, 0}
	},
	{
		{_push_ascii_r, WR_ASCII, WR_RV},
		{_push_ascii_f, WR_ASCII, WR_FW}
	},
	{
		{_push_cigar_r, WR_CIGAR, WR_RV},
		{_push_cigar_f, WR_CIGAR, WR_FW}
	},
	{
		{_push_dir_r, WR_DIR, WR_RV},
		{_push_dir_f, WR_DIR, WR_FW}
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
#define SEA_MEM_ALIGN		( 64 )
static inline
void *
sea_aligned_malloc(
	size_t size,
	size_t align)
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
static inline
void
sea_aligned_free(
	void *ptr)
{
	free(ptr);
	return;
}

/**
 * @fn sea_is_linear_gap_cost
 * @brief (internal)
 */
static inline
int
sea_is_linear_gap_cost(
	struct sea_context const *ctx)
{
	return(ctx->e.k.gi == ctx->e.k.ge);
}

/**
 * @fn sea_is_edit_dist_cost
 */
static inline
int
sea_is_edit_dist_cost(
	struct sea_context const *ctx)
{
	return(ctx->e.k.m == 0 && ctx->e.k.x == -2 && ctx->e.k.gi == -1 && ctx->e.k.ge == -1);
}

/**
 * @fn sea_is_unit_cost
 */
static inline
int
sea_is_unit_cost(
	struct sea_context const *ctx)
{
	return(ctx->e.k.m == 1 && ctx->e.k.x == -1 && ctx->e.k.gi == -1 && ctx->e.k.ge == -1);
}

/**
 * @fn sea_init_restore_defaults
 *
 * @brief (internal) initialize flags
 */
static inline
int32_t
sea_init_restore_defaults(
	struct sea_params *p)
{
	/** set defaults */
	#define default_value(p, field, value) { \
		if((int64_t)(p->field) == 0) { p->field = value; } \
	}

	/** input */
	default_value(p, seq_a_format, SEA_ASCII);
	default_value(p, seq_a_direction, SEA_FW_ONLY);
	default_value(p, seq_b_format, SEA_ASCII);
	default_value(p, seq_b_direction, SEA_FW_ONLY);

	/** output */
	default_value(p, aln_format, SEA_STR);
	default_value(p, head_margin, 0);
	default_value(p, tail_margin, 0);

	/** score */
	default_value(p, score_matrix, SEA_SCORE_SIMPLE(2, -3, -5, -1));
	default_value(p, xdrop, 100);

	#undef default_value
	return SEA_SUCCESS;
}

/**
 * @fn sea_init_flags
 *
 * @brief (internal) initialize flags
 */
static inline
int32_t
sea_init_flags(
	struct sea_context *ctx)
{
	debug("initializing flags");

	/** default input sequence format: ASCII */
	if(ctx->flags.sep.seq_a == 0) {
		ctx->flags.sep.seq_a = SEA_SEQ_A_ASCII>>SEA_FLAGS_POS_SEQ_A;
	}
	if(ctx->flags.sep.seq_b == 0) {
		ctx->flags.sep.seq_b = SEA_SEQ_B_ASCII>>SEA_FLAGS_POS_SEQ_B;
	}
	
	/** default  direction: forward only */
	if(ctx->flags.sep.seq_a_dir == 0) {
		ctx->flags.sep.seq_a_dir = SEA_SEQ_A_FW_ONLY>>SEA_FLAGS_POS_SEQ_A_DIR;
	}
	if(ctx->flags.sep.seq_b_dir == 0) {
		ctx->flags.sep.seq_b_dir = SEA_SEQ_B_FW_ONLY>>SEA_FLAGS_POS_SEQ_B_DIR;
	}

	/** default output format: direction string.
	 * (yields RDRDRDRD for the input pair (AAAA and AAAA).
	 * use SEA_ALN_CIGAR for cigar string.)
	 */
	if(ctx->flags.sep.aln == 0) {
		ctx->flags.sep.aln = SEA_ALN_ASCII>>SEA_FLAGS_POS_ALN;
	}

	return SEA_SUCCESS;
}

/**
 * @fn sea_init_dp_context
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
sea_init_dp_context(
	struct sea_context *ctx,
	int8_t m,
	int8_t x,
	int8_t gi,
	int8_t ge,
	int32_t tx)
{
	int32_t loada_idx, loada_dir_idx, loadb_idx, loadb_dir_idx;

	debug("initializing local context");
	/** check if DP cost values are proper. the cost values must satisfy m >= 0, x < m, 2*gi <= x, ge <= 0. */
	if(m < 0 || x >= m || 2*gi > x || ge > 0 || tx < 0) {
		debug("invalid costs");
		return SEA_ERROR_INVALID_COST;
	}
	
	/* push scores to context */
	ctx->e.k.m = m;
	ctx->e.k.x = x;
	ctx->e.k.gi = gi;
	ctx->e.k.ge = ge;
	ctx->e.k.tx = tx;

	/* misc constants */
	ctx->e.k.max = 0;
	ctx->e.k.m_tail = (uint8_t *)&ctx->jt + sizeof(struct sea_joint_tail);

	/* fill and trace functions */
	ctx->e.k.fn = &ctx->dynamic;

	/* sequence reader */
	loada_idx = ctx->flags.sep.seq_a;
	loadb_idx = ctx->flags.sep.seq_b;
	if((uint64_t)loada_idx > 6 || (uint64_t)loadb_idx > 6) {
		debug("invalid input sequence format flag");
		return SEA_ERROR_INVALID_ARGS;
	}
	loada_dir_idx = ctx->flags.sep.seq_a_dir;
	loadb_dir_idx = ctx->flags.sep.seq_b_dir;
	if((uint64_t)loada_dir_idx > 3 || (uint64_t)loadb_dir_idx > 3) {
		debug("invalid sequence direction flag");
		return SEA_ERROR_INVALID_ARGS;
	}

	ctx->e.k.r.loada = rd_table[loada_dir_idx][loada_idx];
	ctx->e.k.r.loadb = rd_table[loadb_dir_idx][loadb_idx];

	/* sequence writer */
	ctx->e.k.l = ctx->rv;		/** reverse writer for forward extension */

	return SEA_SUCCESS;
}

/**
 * @fn sea_init_graph_context
 */
static inline
int32_t
sea_init_graph_context(
	struct sea_context *ctx)
{
	ctx->e.g.ctx = NULL;
	ctx->e.g.fn = &ctx->graph;
	return SEA_SUCCESS;
}

/**
 * @fn sea_init_search_context
 */
static inline
int32_t
sea_init_search_context(
	struct sea_context *ctx,
	int8_t m,
	int8_t x,
	int8_t gi,
	int8_t ge,
	int32_t tx)
{
	int32_t error_label = SEA_SUCCESS;

	/** clear context */
	memset(&ctx->e, 0, sizeof(struct sea_search_context));

	/** init dp context */
	if((error_label = sea_init_dp_context(ctx, m, x, gi, ge, tx)) != SEA_SUCCESS) {
		goto _sea_init_pair_error_handler;
	}

	/** init graph context */
	if((error_label = sea_init_graph_context(ctx)) != SEA_SUCCESS) {
		goto _sea_init_pair_error_handler;
	}

	/** stack related */
	ctx->e.mem_size = SEA_INIT_STACK_SIZE;
	ctx->e.mem_cnt = 0;

_sea_init_pair_error_handler:
	return(error_label);
}

/**
 * @fn sea_init_joint_head
 */
static inline
int32_t
sea_init_joint_head(
	struct sea_context *ctx)
{
	ctx->jh.p = 0;
	ctx->jh.q = 0;
	ctx->jh.p_tail = NULL;
	
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
	struct sea_context *ctx)
{
	/** initial coordinates */
	ctx->jt.psum = 2;
	ctx->jt.p = 2;				/** (p, q) = (2, 0) */
	ctx->jt.mp = 0;
	ctx->jt.mq = 0;
	ctx->jt.v = (void *)ctx->root_vec;
	ctx->jt.var = 0;			/** 8bit */
	ctx->jt.d2 = SEA_LEFT | SEA_TOP<<2;

	return SEA_SUCCESS;
}

/**
 * @fn sea_init_root_vec
 */
static inline
int32_t
sea_init_root_vec(
	struct sea_context *ctx)
{
	int64_t i;
	int8_t *pv, *cv;

	/** must be consistent to twig_types.h */
	struct twig_linear_joint_vec {
		int8_t pv[16], cv[16];
		uint8_t wa[16], wb[16];
	};
	struct twig_affine_joint_vec {
		int8_t pv[3*16], cv[3*16];
		uint8_t wa[16], wb[16];
	};

	debug("initializing joint_tail");

	memset(ctx->root_vec, INT8_MIN, 512);
	if(sea_is_linear_gap_cost(ctx)) {
		struct twig_linear_joint_vec *v = (struct twig_linear_joint_vec *)ctx->root_vec;
		pv = v->pv;
		cv = v->cv;
	} else {
		struct twig_affine_joint_vec *v = (struct twig_affine_joint_vec *)ctx->root_vec;
		pv = v->pv;
		cv = v->cv;
	}

	/** fill initial vectors */
	#define _Q(x)		( (x) - SEA_INIT_BW/2 )
	for(i = 0; i < SEA_INIT_BW; i++) {
//		pv[i] = -ctx->e.k.gi + (_Q(i) < 0 ? -_Q(i) : _Q(i)+1) * (2 * ctx->e.k.gi - ctx->e.k.m);
//		cv[i] =                (_Q(i) < 0 ? -_Q(i) : _Q(i)  ) * (2 * ctx->e.k.gi - ctx->e.k.m);
		pv[i] =               (_Q(i) < 0 ? -_Q(i)   : _Q(i)) * (2 * ctx->e.k.gi - ctx->e.k.m);
		cv[i] = ctx->e.k.gi + (_Q(i) < 0 ? -_Q(i)-1 : _Q(i)) * (2 * ctx->e.k.gi - ctx->e.k.m);
		debug("pv(%d), cv(%d)", pv[i], cv[i]);
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
 * @param[in] ge : gap extension cost. (ge <= 0) treated as linear-gap cost if gi == ge. the total penalty of the gap with length L is gi + (L - 1)*ge.
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
	int32_t xdrop)
{
	struct sea_context *ctx = NULL;
	int32_t cost_idx, aln_idx;
	int32_t error_label = SEA_ERROR;

	debug("initializing context");

	/** malloc sea_context */
	ctx = (struct sea_context *)sea_aligned_malloc(sizeof(struct sea_context), SEA_MEM_ALIGN);
	if(ctx == NULL) {
		debug("failed malloc sea_context");
		error_label = SEA_ERROR_OUT_OF_MEM;
		goto _sea_init_error_handler;
	}
	memset(ctx, 0, sizeof(struct sea_context));

	/** check flags */
	ctx->flags.all = flags;
	if((error_label = sea_init_flags(ctx)) != SEA_SUCCESS) {
		debug("failed to init flags");
		goto _sea_init_error_handler;
	}

	/** initialize pair context */
	if((error_label = sea_init_search_context(ctx, m, x, gi, ge, xdrop)) != SEA_SUCCESS) {
		debug("failed to initialize pair context template");
		goto _sea_init_error_handler;
	}

	/** initialize head, tail, and root vector */
	if((error_label = sea_init_joint_head(ctx)) != SEA_SUCCESS) {
		debug("failed to initialize joint_head template");
		goto _sea_init_error_handler;
	}
	if((error_label = sea_init_joint_tail(ctx)) != SEA_SUCCESS) {
		debug("failed to initialize joint_tail template");
		goto _sea_init_error_handler;
	}
	if((error_label = sea_init_root_vec(ctx)) != SEA_SUCCESS) {
		debug("failed to initialize root_vec template");
		goto _sea_init_error_handler;
	}


	/** initialize alignment functions */
	if(sea_is_linear_gap_cost(ctx)) {
		ctx->dynamic = aln_table[0][0];
		ctx->guided = aln_table[0][1];
	} else {
		ctx->dynamic = aln_table[1][0];
		ctx->guided = aln_table[1][1];
	}

	/** set alignment writer functions */
	// aln_idx = (ctx->flags & SEA_FLAGS_MASK_ALN) >> SEA_FLAGS_POS_ALN;
	aln_idx = ctx->flags.sep.aln;
	if(aln_idx <= 0 || aln_idx >= 4) {
		debug("invalid alignment string flag");
		error_label = SEA_ERROR_INVALID_ARGS;
		goto _sea_init_error_handler;
	}
	ctx->rv = wr_table[aln_idx][0];
	ctx->fw = wr_table[aln_idx][1];

	/** set graph traverser function */
	ctx->graph = gr_table[0];

	return((sea_t *)ctx);

_sea_init_error_handler:
	if(ctx != NULL) {
		sea_aligned_free(ctx);
	}
	return((sea_t *)NULL);
}

/**
 * @fn sea_search_init
 *
 * @brief malloc working memory, load local context to it
 */
static inline
struct sea_search_context *
sea_search_init(
	struct sea_context const *ctx,
	struct sea_seq_pair const *p,
	uint8_t const *guide,
	uint64_t glen)
{
	/** malloc memory */
	struct sea_search_context *e;
	if((e = sea_aligned_malloc(ctx->e.mem_size, SEA_MEM_ALIGN)) == NULL) {
		return(NULL);
	}

	/** clear memory pointers */
	_aligned_block_memset(
		&e->mem_array,
		0,
		SEA_MEM_ARRAY_SIZE * sizeof(uint8_t *));

	/** initialize ref and read pointers */
	_aligned_block_memcpy(
		&e->k.rr.p,
		p,
		sizeof(struct sea_seq_pair));
	debug("pa(%p), alen(%llu), pb(%p), blen(%llu)", e->k.rr.p.pa, e->k.rr.p.alen, e->k.rr.p.pb, e->k.rr.p.blen);

	/** initialize constant */
	_aligned_block_memcpy(
		(uint8_t *)e + SEA_SEARCH_CONTEXT_LOAD_OFFSET,
		(uint8_t *)&ctx->e + SEA_SEARCH_CONTEXT_LOAD_OFFSET,
		SEA_SEARCH_CONTEXT_LOAD_SIZE);

	/** init stack pointers */
	e->mem_array[e->mem_cnt++] = (uint8_t *)e;

	e->k.stack_top = (uint8_t *)e + sizeof(struct sea_search_context);
	e->k.stack_end = (uint8_t *)e + e->mem_size;
	e->k.pdr = guide;
	e->k.tdr = guide + glen;
	e->k.fn = (guide == NULL) ? &ctx->dynamic : &ctx->guided;

	return(e);
}

/**
 * @fn sea_search_add_mem
 */
static inline
int32_t
sea_search_add_mem(
	struct sea_context const *ctx,
	struct sea_search_context *e)
{
	uint8_t *ptr = sea_aligned_malloc((e->mem_size *= 2), SEA_MEM_ALIGN);
	if(ptr == NULL) {
		e->mem_size /= 2;
		return SEA_ERROR_OUT_OF_MEM;
	}
	e->mem_array[e->mem_cnt++] = e->k.stack_top = ptr;
	e->k.stack_end = e->k.stack_top + e->mem_size;
	return SEA_SUCCESS;
}

/**
 * @fn sea_search_malloc
 * @brief allocate memory from the stack
 */
static inline
void *
sea_search_malloc(
	struct sea_context const *ctx,
	struct sea_search_context *e,
	uint64_t size)
{
	/** make size multiple of 16 */
	uint64_t const align_size = 16;
	size += (align_size - 1);
	size &= ~(align_size - 1);

	/** malloc */
	if((e->k.stack_end - e->k.stack_top) < size) {
		if(e->mem_size < size) { e->mem_size = size; }
		if(sea_search_add_mem(ctx, e) != SEA_SUCCESS) {
			return(NULL);
		}
	}
	e->k.stack_top += size;
	return((void *)(e->k.stack_top - size));
}

/**
 * @fn sea_search_free
 */
static inline
void
sea_search_free(
	struct sea_context const *ctx,
	struct sea_search_context *e,
	void *ptr)
{
	/** nothing to do */
	return;
}

/**
 * @fn sea_search_clean
 */
static inline
void
sea_search_clean(
	struct sea_context const *ctx,
	struct sea_search_context *e)
{
	int64_t i;
	if(e == NULL) { return; }
	for(i = 1; i < SEA_MEM_ARRAY_SIZE && e->mem_array[i] != NULL; i++) {
		sea_aligned_free(e->mem_array[i]);
	}
	sea_aligned_free(e->mem_array[0]);
	return;
}

/**
 * @fn sea_graph_init
 */
static
void *
sea_graph_init(
	struct sea_section_pair *sec,
	struct sea_seq_pair const *p,
	struct sea_checkpoint const *root,
	int32_t dir)
{
	/** reverse extension */
	if(dir == 0) {
		set_sec_pair(sec,
			rev(root->apos, p->alen), rev(0, p->alen),
			rev(root->bpos, p->blen), rev(0, p->blen),
			0xffff808080800000, 0xffff80808080ffff,		/** magic */
			0xffffffffffff0000, 0xffffffffffffffff);	/** magic */
	} else {
		set_sec_pair(sec,
			root->apos, p->alen,
			root->bpos, p->blen,
			0xffff808080800000, 0xffff80808080ffff,
			0xffffffffffff0000, 0xffffffffffffffff);
	}
	return(NULL);
}

/**
 * @fn sea_graph_next
 */
static
int32_t
sea_graph_next(
	void *ctx,
	struct sea_section_pair *sec,
	struct sea_seq_pair const *p,
	struct sea_checkpoint const *term)
{
	return SEA_TERMINATED;
}

/**
 * @fn sea_graph_clean
 */
void
sea_graph_clean(
	void *ctx)
{
	return;
}

/**
 * @fn sea_align_check_args
 *
 * @brief check args
 */
static inline
int32_t
sea_align_check_args(
	struct sea_context const *ctx,
	struct sea_seq_pair const *p)
{
	int64_t acc = 0;		/** accumulator */

	if(ctx == NULL | p == NULL) {
		debug("invalid pointers: ctx(%p), p(%p)", ctx, p);
		return SEA_ERROR_INVALID_ARGS;
	}

	acc |= (int64_t)p->pa - 1;	/** a > 0 */
	acc |= (int64_t)p->alen;	/** alen >= 0 */
	acc |= (int64_t)p->pb - 1;
	acc |= (int64_t)p->blen;

	if(acc < 0) {
		debug("invalid args: a(%p), alen(%llu), b(%p), blen(%llu)", p->pa, p->alen, p->pb, p->blen);
		return SEA_ERROR_INVALID_ARGS;
	}
	return SEA_SUCCESS;
}

/**
 * @fn sea_align_generate_root
 */
static inline
struct sea_chain_status
sea_align_generate_root(
	struct sea_context const *ctx,
	struct sea_search_context *e)
{
	struct sea_chain_status s;
	s.stat = CHAIN;
	s.ptr = (uint8_t *)&ctx->jt + sizeof(struct sea_joint_tail);
	return(s);
}

/**
 * @fn sea_align_extend
 * @brief extend between two checkpoint
 */
static inline
struct sea_chain_status
sea_align_extend(
	struct sea_context const *ctx,
	struct sea_search_context *e,
	struct sea_checkpoint const *root,
	struct sea_checkpoint const *term,
	int32_t dir)
{
	int64_t fn_idx = 0;
	struct sea_chain_status s;
	struct sea_section_pair sec;

	/** init root */
	s = sea_align_generate_root(ctx, e);
	e->g.ctx = e->g.fn->init(&sec, &e->k.rr.p, root, dir);
	while(1) {
		s = e->k.fn->fill[fn_idx](&e->k,
			s.ptr,		/** tail of the previous block */
			&sec);		/** section to fill */
		if(s.stat == MEM) {
			if((s.stat = sea_search_add_mem(ctx, e)) != SEA_SUCCESS) {
				break;
			}
		} else if(s.stat == CHAIN) {
			fn_idx++;
		} else if(s.stat == CAP) {
			if((s.stat = e->g.fn->next(e->g.ctx, &sec, &e->k.rr.p, term)) != SEA_SUCCESS) {
				break;
			}
		} else if(s.stat == TERM) {
			break;
		}
	}
	e->g.fn->clean(e->g.ctx);
	return(s);
}

/**
 * @fn sea_align_semi_global
 */
struct sea_result *
sea_align_semi_global(
	struct sea_context const *ctx,
	struct sea_seq_pair const *p,
	struct sea_checkpoint const *cp,
	uint64_t cplen,
	uint8_t const *guide,
	uint64_t glen)
{
	int64_t i;
	int32_t error_label = SEA_SUCCESS;
	struct sea_search_context *e = NULL;

	/** check arguments */
	if((error_label = sea_align_check_args(ctx, p)) != SEA_SUCCESS) {
		goto _sea_align_semi_global_error_handler;
	}

	/** extend reverse */
	if(cp == NULL) {
		return NULL;
	}

	/** init stack and extension root */
	e = sea_search_init(ctx, p, guide, glen);
//	sea_align_extend(ctx, e, cp, NULL, 0);
	for(i = 0; i < cplen; i++) {
		sea_align_extend(ctx, e, &cp[i], &cp[i+1], 1);
	}

	/** traceback */

_sea_align_semi_global_error_handler:
	sea_search_clean(ctx, e);
	return(NULL);
}

/**
 * @fn sea_align_global
 */
struct sea_result *
sea_align_global(
	struct sea_context const *ctx,
	struct sea_seq_pair const *p,
	struct sea_checkpoint const *cp,
	uint64_t cplen,
	uint8_t const *guide,
	uint64_t glen)
{
	return(NULL);
}

/**
 * @fn sea_align_finish_result
 */
static inline
struct sea_result *
sea_align_finish_result(
	struct sea_context const *ctx,
	struct sea_writer const *w,
	struct sea_writer_work *ww)
{
	return(NULL);
}

#if 0
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
	struct sea_dp_context *k = NULL;
	struct sea_result *r = NULL;
	struct sea_chain_status s;
	int32_t error_label = SEA_ERROR;
	uint8_t *pdp = NULL;

	debug("enter sea_align");
	if((error_label = sea_align_check_args(
			ctx, a, asp, aep, b, bsp, bep, guide, glen, dir))
		!= SEA_SUCCESS) {
		;
	}

	if((k = sea_align_init_local_context(
			ctx, a, asp, aep, b, bsp, bep, guide, glen, dir))
		== NULL) {
		goto _sea_align_error_handler;
	}
	s = sea_align_generate_root(ctx, k);
	s = k->f->twig(k, s.ptr);

	/* do alignment */
	if((error_label = k->f->twig(k, k->pdp)) != SEA_SUCCESS) {
		goto _sea_align_error_handler;		/** when the path went out of the band */
	}

	/* finishing */
	debug("t.l.p(%p)", k->l.p);
	if(k->l.p == NULL) {
		goto _sea_align_null_result;
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
	AFREE(k, mem_size);
	return(r);

_sea_align_null_result:
	error_label = 0;	/** == SEA_SUCCESS */
_sea_align_error_handler:
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
#endif

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
/*		if(ctx->v != NULL) {
			sea_aligned_free(ctx->v);
		}*/
		sea_aligned_free((struct sea_context *)ctx);
		return;
	}
	return;
}

/*
 * end of sea.c
 */
