

/**
 * @file sea.h
 *
 * @brief C header of the libsea API
 *
 * @author Hajime Suzuki
 * @date 2014/12/29
 * @license Apache v2
 *
 * @detail
 * a header for libsea: a fast banded seed-and-extend alignment library.
 * 
 * from C:
 * Include this header file as #include <sea.h>. This will enable you to
 * use all the APIs in the sea_init, and sea_align form.
 *
 * from C++:
 * Include this header as #include <sea.h>. The C APIs are wrapped with
 * namespace sea and the C++ class AlignmentContext and AlignmentResult
 * are added. See example.cpp for the detail of the usage in C++.
 */

#ifndef _SEA_H_INCLUDED
#define _SEA_H_INCLUDED

#include <stdlib.h>		/** NULL and size_t */
#include <stdint.h>		/** uint8_t, int32_t, int64_t */

#if 0
/**
 * @enum sea_flags_band_width
 */
enum sea_flags_band_width {
	SEA_NARROW 			= 16,
	SEA_WIDE			= 32
};

/**
 * @enum sea_flags_band_type
 */
enum sea_flags_band_type {
	SEA_DYNAMIC 		= 1,
	SEA_GUIDED			= 2
};
#endif

/**
 * @enum sea_flags_format
 *
 * @brief (API) constants of the sequence format option.
 */
enum sea_flags_format {
	SEA_ASCII 			= 1,
	SEA_4BIT 			= 2,
	SEA_2BIT 			= 3,
	SEA_4BIT8PACKED 	= 4,
	SEA_2BIT8PACKED 	= 5,
	SEA_1BIT64PACKED 	= 6
};

/**
 * @enum sea_flags_dir
 */
enum sea_flags_dir {
	SEA_FW_ONLY			= 1,
	SEA_FW_RV		 	= 2
};

/**
 * @enum sea_flags_aln_format
 *
 * @brief (API) constants of the alignment format option.
 */
enum sea_flags_aln_format {
	SEA_STR 			= 1,
	SEA_CIGAR			= 2,
	SEA_DIR 			= 3
};

/**
 * @enum sea_error
 *
 * @brief (API) error flags. see sea_init function and status member in the sea_result structure for more details.
 */
enum sea_error {
	SEA_SUCCESS 				=  0,	/*!< success!! */
	SEA_TERMINATED				=  1,	/*!< (internal code) success */
	SEA_ERROR 					= -1,	/*!< unknown error */
	/** errors which occur in an alignment function */
	SEA_ERROR_INVALID_MEM 		= -2,	/*!< invalid pointer to memory */
	SEA_ERROR_INVALID_CONTEXT 	= -3,	/*!< invalid pointer to the alignment context */
	SEA_ERROR_OUT_OF_BAND 		= -4,	/*!< traceback failure. using wider band may resolve this type of error. */
	SEA_ERROR_OUT_OF_MEM 		= -5,	/*!< out of memory error. mostly caused by exessively long queries. */
	SEA_ERROR_OVERFLOW 			= -6, 	/*!< cell overflow error */
	SEA_ERROR_INVALID_ARGS 		= -7,	/*!< inproper input arguments. */
	/** errors which occur in an initialization function */
	SEA_ERROR_UNSUPPORTED_ALG 	= -8,	/*!< unsupported combination of algorithm and processor options. use naive implementations instead. */
	SEA_ERROR_INVALID_COST 		= -9	/*!< invalid alignment cost */
};

/**
 * @enum sea_direction
 */
enum sea_direction {
	SEA_UE_LEFT = 0x00,							/** 0b00 */
	SEA_UE_TOP  = 0x01,							/** 0b01 */
	SEA_LE_LEFT = SEA_UE_LEFT<<1,				/** 0b00 */
	SEA_LE_TOP  = SEA_UE_TOP<<1,				/** 0b10 */
	SEA_LEFT    = SEA_UE_LEFT | SEA_LE_LEFT,	/** 0b00 */
	SEA_TOP     = SEA_UE_TOP | SEA_LE_TOP		/** 0b11 */
};

/**
 * @enum sea_checkpoint_type
 */
enum sea_checkpoint_type {
	SEA_CP_UPWARD = 1,
	SEA_CP_DOWNWARD = 2,
	SEA_CP_3PRIME = SEA_CP_UPWARD,
	SEA_CP_5PRIME = SEA_CP_DOWNWARD,
	SEA_CP_BEGIN = SEA_CP_DOWNWARD,
	SEA_CP_END = SEA_CP_UPWARD,
	SEA_CP_CHECKPOINT = SEA_CP_UPWARD | SEA_CP_DOWNWARD
};

/**
 * @enum sea_clip_type
 */
enum sea_clip_type {
	SEA_CLIP_SOFT = 'S',
	SEA_CLIP_HARD = 'H'
};

/**
 * @struct sea_score_s
 * @brief score container
 */
struct sea_score_s {
	int8_t score_sub[4][4];
	int8_t score_gi_a, score_ge_a;
	int8_t score_gi_b, score_ge_b;
};
typedef struct sea_score_s sea_score_t;

/**
 * @struct sea_params_s
 * @brief input parameters of sea_init
 */
struct sea_params_s {

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
	int16_t head_margin;		/** margin at the head of sea_res_t */
	int16_t tail_margin;		/** margin at the tail of sea_res_t */

	/** score parameters */
	int32_t xdrop;
	sea_score_t const *score_matrix;

	/** reserved */
	uint8_t _reserved[8];
};
typedef struct sea_params_s sea_params_t;

/**
 * @macro SEA_PARAMS
 * @brief utility macro for sea_init, see example on header.
 */
#define SEA_PARAMS(...)			( &((struct sea_params_s) { __VA_ARGS__ }) )

/**
 * @macro SEA_SCORE_SIMPLE
 * @brief utility macro for constructing score parameters.
 */
#define SEA_SCORE_SIMPLE(m, x, gi, ge) ( \
	&((sea_score_t const) { \
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
 * @struct sea_seq_pair_s
 * @brief ref-read pair
 */
struct sea_seq_pair_s {
	void const *a, *b;			/** (16) */
	uint64_t alen, blen;		/** (16) */
};
typedef struct sea_seq_pair_s sea_seq_pair_t;
#define sea_build_seq_pair(_a, _alen, _b, _blen) ( \
	(struct sea_seq_pair_s){ \
		.a = (_a), \
		.b = (_b), \
		.alen = (_alen), \
		.blen = (_blen) \
	} \
)

/**
 * @type sea_t
 *
 * @brief (API) an alias to `struct sea_context_s'.
 */
typedef struct sea_context_s sea_t;

/**
 * @struct sea_section_s
 *
 * @brief section container, a tuple of (id, length, head position).
 */
struct sea_section_s {
	uint32_t id;				/** (4) section id */
	uint32_t len;				/** (4) length of the  seq */
	uint64_t base;				/** (8) base position of the head of the seq */
};
typedef struct sea_section_s sea_section_t;
#define sea_build_section(_id, _base, _len) ( \
	(struct sea_section_s){ \
		.id = (_id), \
		.base = (_base), \
		.len = (_len) \
	} \
)

/**
 * @type sea_dp_t
 *
 * @brief an alias to `struct sea_dp_context_s`.
 */
typedef struct sea_dp_context_s sea_dp_t;

/**
 * @struct sea_fill_s
 */
struct sea_fill_s {
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
typedef struct sea_fill_s sea_fill_t;

/**
 * @struct sea_path_section_s
 */
struct sea_path_section_s {
	struct sea_section_s a;		/** (16) base section a */
	struct sea_section_s b;		/** (16) base section b */
	uint32_t apos, bpos;		/** (8) pos in the section */
	uint32_t alen, blen;		/** (8) length of the segment */
};
typedef struct sea_path_section_s sea_path_section_t;

/**
 * @struct sea_result_s
 */
struct sea_result_s {
	struct sea_path_section_s const *sec;
	uint32_t *path;
	uint32_t rem;
	uint32_t unused;
	uint32_t slen, plen;
};
typedef struct sea_result_s sea_result_t;

/**
 * @fn sea_init
 * @brief (API) sea_init new API
 */
sea_t *sea_init(sea_params_t const *params);

/**
 * @fn sea_clean
 *
 * @brief (API) clean up the alignment context structure.
 *
 * @param[in] ctx : a pointer to the alignment structure.
 *
 * @return none.
 *
 * @sa sea_init
 */
void sea_clean(
	sea_t *ctx);

/**
 * @fn sea_dp_init
 */
struct sea_dp_context_s *sea_dp_init(
	sea_t const *ctx,
	sea_seq_pair_t const *p);

/**
 * @fn sea_dp_flush
 *
 * @brief flush stack
 */
void sea_dp_flush(
	sea_dp_t *this,
	sea_seq_pair_t const *p);

/**
 * @fn sea_dp_clean
 */
void sea_dp_clean(
	sea_dp_t *this);

/**
 * @fn sea_dp_fill_root
 */
sea_fill_t *sea_dp_fill_root(
	sea_dp_t *this,
	sea_section_t const *a,
	uint32_t apos,
	sea_section_t const *b,
	uint32_t bpos);

/**
 * @fn sea_dp_fill
 * @brief fill dp matrix inside section pairs
 */
sea_fill_t *sea_dp_fill(
	sea_dp_t *this,
	sea_fill_t const *prev_sec,
	sea_section_t const *a,
	sea_section_t const *b);

/**
 * @fn sea_dp_merge
 */
sea_fill_t *sea_dp_merge(
	sea_dp_t *this,
	sea_fill_t const *sec_list,
	uint64_t tail_list_len);

/**
 * @struct sea_clip_params_s
 */
struct sea_clip_params_s {
	char seq_a_head_type;
	char seq_a_tail_type;
	char seq_b_head_type;
	char seq_b_tail_type;
};
typedef struct sea_clip_params_s sea_clip_params_t;

/**
 * @macro SEA_CLIP_PARAMS
 */
#define SEA_CLIP_PARAMS(...)		( &((struct sea_clip_params_s) { __VA_ARGS__ }) )
#define SEA_CLIP_NONE				( NULL )

/**
 * @type sea_result_writer
 * @brief pointer to putchar-compatible writer
 */
typedef int (*sea_result_writer)(int c);

/**
 * @fn sea_dp_trace
 *
 * @brief generate alignment result string
 */
sea_result_t *sea_dp_trace(
	sea_dp_t *this,
	sea_fill_t const *fw_tail,
	sea_fill_t const *rv_tail,
	sea_clip_params_t const *clip);

#if 0
/**
 * @fn sea_align_semi_global
 */
sea_res_t *sea_align_semi_global(
	sea_t const *ctx,
	sea_seq_pair_t const *p,
	sea_checkpoint_t const *cp,
	uint64_t cplen,
	uint8_t const *guide,
	uint64_t glen);

/**
 * @fn sea_align_global
 */
sea_res_t *sea_align_global(
	sea_t const *ctx,
	sea_seq_pair_t const *p,
	sea_checkpoint_t const *cp,
	uint64_t cplen,
	uint8_t const *guide,
	uint64_t glen);

/**
 * @fn sea_get_error_num
 *
 * @brief (API) extract error number from a context or a result
 *
 * @param[ref] ctx : a pointer to an alignment context structure. can be NULL.
 * @param[ref] aln : a pointer to a result structure. can be NULL.
 *
 * @return error number, defined in sea_error
 */
int32_t sea_get_error_num(
	sea_t const *ctx,
	sea_res_t const *aln);

/**
 * @fn sea_aln_free
 *
 * @brief (API) clean up sea_result structure
 *
 * @param[in] aln : an pointer to sea_result structure.
 *
 * @return none.
 *
 * @sa sea_sea
 */
void sea_aln_free(
	sea_t const *ctx,
	sea_res_t *aln);
#endif

#endif  /* #ifndef _SEA_H_INCLUDED */

/*
 * end of sea.h
 */
