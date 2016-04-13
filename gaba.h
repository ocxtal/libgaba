

/**
 * @file gaba.h
 *
 * @brief C header of the libgaba (libsea3) API
 *
 * @author Hajime Suzuki
 * @date 2014/12/29
 * @license Apache v2
 *
 * @detail
 * a header for libgaba (libsea3): a fast banded seed-and-extend alignment library.
 * 
 * from C:
 * Include this header file as #include <gaba.h>. This will enable you to
 * use all the APIs in the gaba_init, and gaba_align form.
 *
 * from C++:
 * Include this header as #include <gaba.h>. The C APIs are wrapped with
 * namespace sea and the C++ class AlignmentContext and AlignmentResult
 * are added. See example.cpp for the detail of the usage in C++.
 */

#ifndef _GABA_H_INCLUDED
#define _GABA_H_INCLUDED

#include <stdlib.h>		/** NULL and size_t */
#include <stdint.h>		/** uint8_t, int32_t, int64_t */

#if 0
/**
 * @enum gaba_flags_band_width
 */
enum gaba_flags_band_width {
	GABA_NARROW 			= 16,
	GABA_WIDE			= 32
};

/**
 * @enum gaba_flags_band_type
 */
enum gaba_flags_band_type {
	GABA_DYNAMIC 		= 1,
	GABA_GUIDED			= 2
};
#endif

/**
 * @enum gaba_flags_format
 *
 * @brief (API) constants of the sequence format option.
 */
enum gaba_flags_format {
	GABA_ASCII 			= 1,
	GABA_4BIT 			= 2,
	GABA_2BIT 			= 3,
	GABA_4BIT8PACKED 	= 4,
	GABA_2BIT8PACKED 	= 5,
	GABA_1BIT64PACKED 	= 6
};

/**
 * @enum gaba_flags_dir
 */
enum gaba_flags_dir {
	GABA_FW_ONLY			= 1,
	GABA_FW_RV		 	= 2
};

/**
 * @enum gaba_flags_aln_format
 *
 * @brief (API) constants of the alignment format option.
 */
enum gaba_flags_aln_format {
	GABA_STR 			= 1,
	GABA_CIGAR			= 2,
	GABA_DIR 			= 3
};

/**
 * @enum gaba_error
 *
 * @brief (API) error flags. see gaba_init function and status member in the gaba_result structure for more details.
 */
enum gaba_error {
	GABA_SUCCESS 				=  0,	/*!< success!! */
	GABA_TERMINATED				=  1,	/*!< (internal code) success */
	GABA_ERROR 					= -1,	/*!< unknown error */
	/** errors which occur in an alignment function */
	GABA_ERROR_INVALID_MEM 		= -2,	/*!< invalid pointer to memory */
	GABA_ERROR_INVALID_CONTEXT 	= -3,	/*!< invalid pointer to the alignment context */
	GABA_ERROR_OUT_OF_BAND 		= -4,	/*!< traceback failure. using wider band may resolve this type of error. */
	GABA_ERROR_OUT_OF_MEM 		= -5,	/*!< out of memory error. mostly caused by exessively long queries. */
	GABA_ERROR_OVERFLOW 			= -6, 	/*!< cell overflow error */
	GABA_ERROR_INVALID_ARGS 		= -7,	/*!< inproper input arguments. */
	/** errors which occur in an initialization function */
	GABA_ERROR_UNSUPPORTED_ALG 	= -8,	/*!< unsupported combination of algorithm and processor options. use naive implementations instead. */
	GABA_ERROR_INVALID_COST 		= -9	/*!< invalid alignment cost */
};

/**
 * @enum gaba_direction
 */
enum gaba_direction {
	GABA_UE_LEFT = 0x00,							/** 0b00 */
	GABA_UE_TOP  = 0x01,							/** 0b01 */
	GABA_LE_LEFT = GABA_UE_LEFT<<1,				/** 0b00 */
	GABA_LE_TOP  = GABA_UE_TOP<<1,				/** 0b10 */
	GABA_LEFT    = GABA_UE_LEFT | GABA_LE_LEFT,	/** 0b00 */
	GABA_TOP     = GABA_UE_TOP | GABA_LE_TOP		/** 0b11 */
};

/**
 * @enum gaba_checkpoint_type
 */
enum gaba_checkpoint_type {
	GABA_CP_UPWARD = 1,
	GABA_CP_DOWNWARD = 2,
	GABA_CP_3PRIME = GABA_CP_UPWARD,
	GABA_CP_5PRIME = GABA_CP_DOWNWARD,
	GABA_CP_BEGIN = GABA_CP_DOWNWARD,
	GABA_CP_END = GABA_CP_UPWARD,
	GABA_CP_CHECKPOINT = GABA_CP_UPWARD | GABA_CP_DOWNWARD
};

/**
 * @enum gaba_clip_type
 */
enum gaba_clip_type {
	GABA_CLIP_SOFT = 'S',
	GABA_CLIP_HARD = 'H'
};

/**
 * @struct gaba_score_s
 * @brief score container
 */
struct gaba_score_s {
	int8_t score_sub[4][4];
	int8_t score_gi_a, score_ge_a;
	int8_t score_gi_b, score_ge_b;
};
typedef struct gaba_score_s gaba_score_t;

/**
 * @struct gaba_params_s
 * @brief input parameters of gaba_init
 */
struct gaba_params_s {

	/** dp options */
	// uint8_t band_width;			/** wide (32) or narrow (16) */
	// uint8_t band_type;			/** dynamic or guided */
	uint8_t _pad[3];

	/** input options */
	uint8_t seq_a_format;
	uint8_t seq_a_direction;
	uint8_t seq_b_format;
	uint8_t seq_b_direction;

	/** output options */
	uint8_t aln_format;
	int16_t head_margin;		/** margin at the head of gaba_res_t */
	int16_t tail_margin;		/** margin at the tail of gaba_res_t */

	/** score parameters */
	int32_t xdrop;
	gaba_score_t const *score_matrix;

	/** reserved */
	uint8_t _reserved[8];
};
typedef struct gaba_params_s gaba_params_t;

/**
 * @macro GABA_PARAMS
 * @brief utility macro for gaba_init, see example on header.
 */
#define GABA_PARAMS(...)			( &((struct gaba_params_s) { __VA_ARGS__ }) )

/**
 * @macro GABA_SCORE_SIMPLE
 * @brief utility macro for constructing score parameters.
 */
#define GABA_SCORE_SIMPLE(m, x, gi, ge) ( \
	&((gaba_score_t const) { \
		.score_sub = { \
			{m, -(x), -(x), -(x)}, \
			{-(x), m, -(x), -(x)}, \
			{-(x), -(x), m, -(x)}, \
			{-(x), -(x), -(x), m} \
		}, \
		.score_gi_a = gi, \
		.score_ge_a = ge, \
		.score_gi_b = gi, \
		.score_ge_b = ge \
	}) \
)

/**
 * @struct gaba_seq_pair_s
 * @brief ref-read pair
 */
struct gaba_seq_pair_s {
	void const *a, *b;			/** (16) */
	uint64_t alen, blen;		/** (16) */
};
typedef struct gaba_seq_pair_s gaba_seq_pair_t;
#define gaba_build_seq_pair(_a, _alen, _b, _blen) ( \
	(struct gaba_seq_pair_s){ \
		.a = (_a), \
		.b = (_b), \
		.alen = (_alen), \
		.blen = (_blen) \
	} \
)

/**
 * @type gaba_t
 *
 * @brief (API) an alias to `struct gaba_context_s'.
 */
typedef struct gaba_context_s gaba_t;

/**
 * @struct gaba_section_s
 *
 * @brief section container, a tuple of (id, length, head position).
 */
struct gaba_section_s {
	int32_t id;					/** (4) section id */
	uint32_t len;				/** (4) length of the  seq */
	uint64_t base;				/** (8) base position of the head of the seq */
};
typedef struct gaba_section_s gaba_section_t;
#define gaba_build_section(_id, _base, _len) ( \
	(struct gaba_section_s){ \
		.id = (_id), \
		.base = (_base), \
		.len = (_len) \
	} \
)

/**
 * @type gaba_dp_t
 *
 * @brief an alias to `struct gaba_dp_context_s`.
 */
typedef struct gaba_dp_context_s gaba_dp_t;

/**
 * @struct gaba_fill_s
 */
struct gaba_fill_s {
	uint8_t _pad1[8];

	/* coordinates */
	int64_t psum;				/** (8) global p-coordinate of the tail of the section */
	int32_t p;					/** (4) local p-coordinate of the tail of the section */
	uint32_t ssum;				/** (4) */

	/* status and max scores */
	int64_t max;				/** (8) max */
	uint32_t status;			/** (4) */

	uint8_t _pad2[60];
};
typedef struct gaba_fill_s gaba_fill_t;

/**
 * @struct gaba_path_section_s
 */
struct gaba_path_section_s {
	struct gaba_section_s a;		/** (16) base section a */
	struct gaba_section_s b;		/** (16) base section b */
	uint32_t apos, bpos;		/** (8) pos in the section */
	uint32_t alen, blen;		/** (8) length of the segment */
};
typedef struct gaba_path_section_s gaba_path_section_t;

/**
 * @struct gaba_path_s
 */
struct gaba_path_s {
	uint32_t len;				/** (4) path length (= array bit length) */
	uint32_t rem;				/** (4) remainder at the head of the path */
	uint32_t array[];			/** () path array */
};
typedef struct gaba_path_s gaba_path_t;

/**
 * @struct gaba_result_s
 */
struct gaba_result_s {
	struct gaba_path_section_s const *sec;
	struct gaba_path_s const *path;
	int64_t score;
	uint32_t slen, reserved;
};
typedef struct gaba_result_s gaba_result_t;

/**
 * @fn gaba_init
 * @brief (API) gaba_init new API
 */
gaba_t *gaba_init(gaba_params_t const *params);

/**
 * @fn gaba_clean
 *
 * @brief (API) clean up the alignment context structure.
 *
 * @param[in] ctx : a pointer to the alignment structure.
 *
 * @return none.
 *
 * @sa gaba_init
 */
void gaba_clean(
	gaba_t *ctx);

/**
 * @fn gaba_dp_init
 */
struct gaba_dp_context_s *gaba_dp_init(
	gaba_t const *ctx,
	gaba_seq_pair_t const *p);

/**
 * @fn gaba_dp_flush
 *
 * @brief flush stack
 */
void gaba_dp_flush(
	gaba_dp_t *this,
	gaba_seq_pair_t const *p);

/**
 * @fn gaba_dp_clean
 */
void gaba_dp_clean(
	gaba_dp_t *this);

/**
 * @fn gaba_dp_fill_root
 */
gaba_fill_t *gaba_dp_fill_root(
	gaba_dp_t *this,
	gaba_section_t const *a,
	uint32_t apos,
	gaba_section_t const *b,
	uint32_t bpos);

/**
 * @fn gaba_dp_fill
 * @brief fill dp matrix inside section pairs
 */
gaba_fill_t *gaba_dp_fill(
	gaba_dp_t *this,
	gaba_fill_t const *prev_sec,
	gaba_section_t const *a,
	gaba_section_t const *b);

/**
 * @fn gaba_dp_merge
 */
gaba_fill_t *gaba_dp_merge(
	gaba_dp_t *this,
	gaba_fill_t const *sec_list,
	uint64_t tail_list_len);

/**
 * @struct gaba_clip_params_s
 */
struct gaba_clip_params_s {
	char seq_a_head_type;
	char seq_a_tail_type;
	char seq_b_head_type;
	char seq_b_tail_type;
};
typedef struct gaba_clip_params_s gaba_clip_params_t;

/**
 * @macro GABA_CLIP_PARAMS
 */
#define GABA_CLIP_PARAMS(...)		( &((struct gaba_clip_params_s) { __VA_ARGS__ }) )
#define GABA_CLIP_NONE				( NULL )

/**
 * @type gaba_result_writer
 * @brief pointer to putchar-compatible writer
 */
typedef int (*gaba_result_writer)(int c);

/**
 * @fn gaba_dp_trace
 *
 * @brief generate alignment result string
 */
gaba_result_t *gaba_dp_trace(
	gaba_dp_t *this,
	gaba_fill_t const *fw_tail,
	gaba_fill_t const *rv_tail,
	gaba_clip_params_t const *clip);

#if 0
/**
 * @fn gaba_align_semi_global
 */
gaba_res_t *gaba_align_semi_global(
	gaba_t const *ctx,
	gaba_seq_pair_t const *p,
	gaba_checkpoint_t const *cp,
	uint64_t cplen,
	uint8_t const *guide,
	uint64_t glen);

/**
 * @fn gaba_align_global
 */
gaba_res_t *gaba_align_global(
	gaba_t const *ctx,
	gaba_seq_pair_t const *p,
	gaba_checkpoint_t const *cp,
	uint64_t cplen,
	uint8_t const *guide,
	uint64_t glen);

/**
 * @fn gaba_get_error_num
 *
 * @brief (API) extract error number from a context or a result
 *
 * @param[ref] ctx : a pointer to an alignment context structure. can be NULL.
 * @param[ref] aln : a pointer to a result structure. can be NULL.
 *
 * @return error number, defined in gaba_error
 */
int32_t gaba_get_error_num(
	gaba_t const *ctx,
	gaba_res_t const *aln);

/**
 * @fn gaba_aln_free
 *
 * @brief (API) clean up gaba_result structure
 *
 * @param[in] aln : an pointer to gaba_result structure.
 *
 * @return none.
 *
 * @sa gaba_sea
 */
void gaba_aln_free(
	gaba_t const *ctx,
	gaba_res_t *aln);
#endif

#endif  /* #ifndef _GABA_H_INCLUDED */

/*
 * end of gaba.h
 */
