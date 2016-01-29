
/**
 * @file sea.c
 *
 * @brief libsea3 API implementations
 *
 * @author Hajime Suzuki
 * @date 2016/1/11
 * @license Apache v2
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "sea.h"
#include "util/util.h"
#include "arch/arch.h"

/**
 * @macro _force_inline
 * @brief inline directive for gcc-compatible compilers
 */
#define _force_inline	inline

/* benchmark accumulators */
#ifdef BENCH
static
bench_t fill, search, trace;
#endif

/* constants */
#define SEA_INIT_STACK_SIZE			( (uint64_t)32 * 1024 * 1024 )

/**
 * @val dynamic, guided
 */
static
struct sea_aln_funcs_s dynamic[2] = {
	{ NULL, NULL, NULL },
	{ wide_dynamic_fill, NULL, NULL }
	// {narrow_dynamic_fill, narrow_dynamic_merge, narrow_dynamic_trace},
	// {wide_dynamic_fill, wide_dynamic_merge, wide_dynamic_trace}
};
static
struct sea_aln_funcs_s guided[2] = {
	{ NULL, NULL, NULL },
	{ NULL, NULL, NULL }
	// {narrow_guided_fill, narrow_guided_merge, narrow_guided_trace},
	// {wide_guided_fill, wide_guided_merge, wide_guided_trace}
};

static _force_inline
int8_t extract_max(int8_t const vector[][4])
{
	int8_t *v = (int8_t *)vector;
	int8_t max = -128;
	for(int i = 0; i < 16; i++) {
		max = (v[i] > max) ? v[i] : max;
	}
	return(max);
}

/**
 * @fn sea_init_restore_default_params
 */
static _force_inline
void sea_init_restore_default_params(
	struct sea_params_s *params)
{
	#define restore(_name, _default) { \
		params->_name = ((uint64_t)(params->_name) == 0) ? (_default) : (params->_name); \
	}
	restore(band_width, 		SEA_WIDE);
	restore(band_type, 			SEA_DYNAMIC);
	restore(seq_a_format, 		SEA_ASCII);
	restore(seq_a_direction, 	SEA_FW_ONLY);
	restore(seq_b_format, 		SEA_ASCII);
	restore(seq_b_direction, 	SEA_FW_ONLY);
	restore(aln_format, 		SEA_ASCII);
	restore(head_margin, 		0);
	restore(tail_margin, 		0);
	restore(xdrop, 				100);
	restore(score_matrix, 		SEA_SCORE_SIMPLE(1, 1, 1, 1));
	return;
}

/**
 * @fn sea_init_create_score_vector
 */
static _force_inline
struct sea_score_vec_s sea_init_create_score_vector(
	struct sea_score_s const *score_matrix)
{
	#define broadcast(x) { \
		(x), (x), (x), (x), \
		(x), (x), (x), (x), \
		(x), (x), (x), (x), \
		(x), (x), (x), (x) \
	}

	int8_t *v = (int8_t *)score_matrix->score_sub;
	struct sea_score_vec_s sc;
	for(int i = 0; i < 16; i++) {
		sc.sbv[i] = v[i];
		sc.geav[i] = -score_matrix->score_ge_a;
		sc.gebv[i] = -score_matrix->score_ge_b;
		sc.giav[i] = -score_matrix->score_gi_a;
		sc.gibv[i] = -score_matrix->score_gi_b;
	}
	return(sc);
}

/**
 * @fn sea_init_create_dir_dynamic
 */
static _force_inline
union sea_dir_u sea_init_create_dir_dynamic(
	struct sea_score_s const *score_matrix)
{
	return((union sea_dir_u) {
		.dynamic = {
			0,				/* zero independent of scoreing schemes */
			0x80000000		/* (0, 0) -> (0, 1) */
		}
	});
}

/**
 * @fn sea_init_create_small_delta
 */
static _force_inline
struct sea_small_delta_s sea_init_create_small_delta(
	struct sea_score_s const *score_matrix)
{
	int8_t max = extract_max(score_matrix->score_sub);
	int8_t diff_a = max + score_matrix->score_ge_a;
	int8_t diff_b = -score_matrix->score_ge_b;

	struct sea_small_delta_s sd;
	for(int i = 0; i < BW/2; i++) {
		sd.delta[i] = diff_a;
		sd.delta[BW/2 + i] = diff_b;
		sd.max[i] = 0;
		sd.max[BW/2 + i] = -diff_b;
	}
	return(sd);
}

/**
 * @fn sea_init_create_middle_delta
 */
static _force_inline
struct sea_middle_delta_s sea_init_create_middle_delta(
	struct sea_score_s const *score_matrix)
{
	int8_t max = extract_max(score_matrix->score_sub);
	int16_t coef_a = -max - 2*score_matrix->score_ge_a;
	int16_t coef_b = -max - 2*score_matrix->score_ge_b;
	int16_t ofs_a = -score_matrix->score_gi_a;
	int16_t ofs_b = -score_matrix->score_gi_b;

	struct sea_middle_delta_s md;
	for(int i = 0; i < BW/2; i++) {
		md.delta[i] = ofs_a + coef_a * (BW/2 - i);
		md.delta[BW/2 + i] = ofs_b + coef_b * i;
	}
	md.delta[BW/2] = 0;
	return(md);
}

/**
 * @fn sea_init_create_diff_vectors
 */
static _force_inline
struct sea_diff_vec_s sea_init_create_diff_vectors(
	struct sea_score_s const *score_matrix)
{
	int8_t max = extract_max(score_matrix->score_sub);
	int8_t drop_dh = 0;
	int8_t raise_dh = max + 2*score_matrix->score_ge_b;
	int8_t drop_dv = 0;
	int8_t raise_dv = max + 2*score_matrix->score_ge_a;
	int8_t drop_de = -score_matrix->score_gi_a + score_matrix->score_ge_a;
	int8_t drop_df = -score_matrix->score_gi_b + score_matrix->score_ge_b;

	struct sea_diff_vec_s diff;
	for(int i = 0; i < BW/2; i++) {
		diff.dh[i] = drop_dh;
		diff.dh[BW/2 + i] = raise_dh;
		diff.dv[i] = raise_dv;
		diff.dv[BW/2 + i] = drop_dv;
		diff.de[i] = drop_de;
		diff.df[i] = drop_df;
	}
	return(diff);
}

/**
 * @fn sea_init
 */
sea_t *sea_init(
	struct sea_params_s const *params)
{
	/* sequence reader table */
	void (*const rd_table[3][7])(
		uint8_t *dst,
		uint8_t const *src,
		uint64_t idx,
		uint64_t src_len,
		uint64_t copy_len) = {
		[SEA_FW_ONLY] = {
			[SEA_ASCII] = _load_ascii_fw,
			[SEA_4BIT] = _load_4bit_fw,
			[SEA_2BIT] = _load_2bit_fw,
			[SEA_4BIT8PACKED] = _load_4bit8packed_fw,
			[SEA_2BIT8PACKED] = _load_2bit8packed_fw
		},
		[SEA_FW_RV] = {
			[SEA_ASCII] = _load_ascii_fr,
			[SEA_4BIT] = _load_4bit_fr,
			[SEA_2BIT] = _load_2bit_fr,
			[SEA_4BIT8PACKED] = _load_4bit8packed_fr,
			[SEA_2BIT8PACKED] = _load_2bit8packed_fr
		}
	};
	/* alignment writer table */
	struct sea_writer_s wr_fw_table[4] = {
		[SEA_STR] = {
			.push = _push_ascii_f,
			.type = WR_ASCII,
			.fr = WR_FW
		},
		[SEA_CIGAR] = {
			.push = _push_cigar_f,
			.type = WR_CIGAR,
			.fr = WR_FW
		},
		[SEA_DIR] = {
			.push = _push_dir_f,
			.type = WR_DIR,
			.fr = WR_FW
		}
	};
	struct sea_writer_s wr_rv_table[4] = {
		[SEA_STR] = {
			.push = _push_ascii_r,
			.type = WR_ASCII,
			.fr = WR_RV
		},
		[SEA_CIGAR] = {
			.push = _push_cigar_r,
			.type = WR_CIGAR,
			.fr = WR_RV
		},
		[SEA_DIR] = {
			.push = _push_dir_r,
			.type = WR_DIR,
			.fr = WR_RV
		}
	};

	if(params == NULL) {
		debug("params must not be NULL");
		return(NULL);
	}

	/* copy params to local stack */
	struct sea_params_s params_intl = *params;

	/* restore defaults */
	sea_init_restore_default_params(&params_intl);

	/* malloc sea_context */
	struct sea_context_s *ctx = (struct sea_context_s *)sea_aligned_malloc(
		sizeof(struct sea_context_s),
		SEA_MEM_ALIGN_SIZE);
	if(ctx == NULL) {
		return(NULL);
	}

	/* fill context */
	*ctx = (struct sea_context_s) {
		/* template */
		.k = (struct sea_dp_context_s) {
			.stack_top = NULL,						/* filled on dp init */
			.stack_end = NULL,						/* filled on dp init */
			.pdr = NULL,							/* filled on dp init */
			.tdr = NULL,							/* filled on dp init */

			.ll = (struct sea_writer_work_s) { 0 },	/* work: no need to init */
			.rr = (struct sea_reader_work_s) { 0 },	/* work: no need to init */
			.l = (struct sea_writer_s)(ctx->rv),	/* reverse writer (default on init) */
			.r = (struct sea_reader_s) {
				.loada = rd_table
					[params_intl.seq_a_direction]
					[params_intl.seq_a_format],		/* seq a reader */
				.loadb = rd_table
					[params_intl.seq_b_direction]
					[params_intl.seq_b_format]		/* seq b reader */
			},

			.scv = sea_init_create_score_vector(params_intl.score_matrix),
			.tx = params_intl.xdrop,
			// .max = 0,								/* at (0, 0) */
			// .m_tail = &ctx->tail,					/* at (0, 0) */

			.mem_cnt = 0,
			.mem_size = SEA_INIT_STACK_SIZE,
			.mem_array = { 0 },		/* NULL */

			.fn = ((params_intl.band_type == SEA_GUIDED) ? guided : dynamic)[0],
		},
		.md = sea_init_create_middle_delta(params_intl.score_matrix),
		.blk = (struct sea_phantom_block_s) {
			.mask = {
				{ 0x00000000, 0x00000000 },
				{ 0x0000ffff, 0xffff0000 }
			},
			.dir = sea_init_create_dir_dynamic(params_intl.score_matrix),
			.offset = 0,
			.diff = sea_init_create_diff_vectors(params_intl.score_matrix),
			.sd = sea_init_create_small_delta(params_intl.score_matrix)
		},
		.tail = (struct sea_joint_tail_s) {
			.v = &ctx->md,
			.p = 2,
			.mp = 0,
			.mq = 0,
			.psum = 2,
			.wa = { 0 },
			.wb = { 0 }
		},
		.rv = wr_rv_table[params_intl.aln_format],
		.fw = wr_fw_table[params_intl.aln_format],
		.params = params_intl
	};

	return((sea_t *)ctx);
}

/**
 * @fn sea_dp_init
 */
struct sea_dp_context_s *sea_dp_init(
	struct sea_context_s const *ctx,
	struct sea_seq_pair_s const *p,
	uint8_t const *guide,
	uint64_t glen)
{
	/* malloc stack memory */
	struct sea_dp_context_s *this = (struct sea_dp_context_s *)sea_aligned_malloc(
		ctx->k.mem_size,
		SEA_MEM_ALIGN_SIZE);
	if(this == NULL) {
		debug("failed to malloc memory");
		return(NULL);
	}

	/* init seq pointers */
	_aligned_block_memcpy(
		&this->rr.p,
		p,
		sizeof(struct sea_seq_pair_s));

	/* load template */
	_aligned_block_memcpy(
		(uint8_t *)this + SEA_DP_CONTEXT_LOAD_OFFSET,
		(uint8_t *)&ctx->k + SEA_DP_CONTEXT_LOAD_OFFSET,
		SEA_DP_CONTEXT_LOAD_SIZE);

	/* init stack pointers */
	this->stack_top = (uint8_t *)this + sizeof(struct sea_dp_context_s);
	this->stack_end = (uint8_t *)this + this->mem_size;
	this->pdr = guide;
	this->tdr = guide + glen;

	return(this);
}

/**
 * @fn sea_dp_add_stack
 */
int32_t sea_dp_add_stack(
	struct sea_dp_context_s *this)
{
	uint8_t *ptr = (uint8_t *)sea_aligned_malloc(
		(this->mem_size *= 2),
		SEA_MEM_ALIGN_SIZE);
	if(ptr == NULL) {
		this->mem_size /= 2;
		return(SEA_ERROR_OUT_OF_MEM);
	}
	this->mem_array[this->mem_cnt++] = this->stack_top = ptr;
	this->stack_end = this->stack_top + this->mem_size;
	return(SEA_SUCCESS);
}

/**
 * @fn sea_dp_malloc
 */
void *sea_dp_malloc(
	struct sea_dp_context_s *this,
	uint64_t size)
{
	/* roundup */
	uint64_t const align_size = 16;
	size += (align_size - 1);
	size &= ~(align_size - 1);

	/* malloc */
	if((this->stack_end - this->stack_top) < size) {
		if(this->mem_size < size) { this->mem_size = size; }
		if(sea_dp_add_stack(this) != SEA_SUCCESS) {
			return(NULL);
		}
	}
	this->stack_top += size;
	return((void *)(this->stack_top - size));
}

/**
 * @fn sea_dp_free
 */
void sea_dp_free(
	struct sea_dp_context_s *this,
	void *ptr)
{
	/* nothing to do */
	return;
}

/**
 * @fn sea_dp_clean
 */
void sea_dp_clean(
	struct sea_dp_context_s *this)
{
	if(this == NULL) {
		return;
	}

	for(uint64_t i = 0; i < SEA_MEM_ARRAY_SIZE; i++) {
		sea_aligned_free(this->mem_array[i]);
	}
	sea_aligned_free(this);
	return;
}

/**
 * @fn sea_align_dynamic
 */
struct sea_result *sea_align_dynamic(
	struct sea_context_s const *ctx,
	struct sea_seq_pair_s const *seq,
	struct sea_checkpoint_s const *cp,
	uint64_t cplen)
{
	return(NULL);
}

/**
 * @fn sea_align_guided
 */
struct sea_result *sea_align_guided(
	struct sea_context_s const *ctx,
	struct sea_seq_pair_s const *seq,
	struct sea_checkpoint_s const *cp,
	uint64_t cplen,
	uint8_t const *guide,
	uint64_t glen)
{
	return(NULL);
}

/**
 * end of sea.c
 */
