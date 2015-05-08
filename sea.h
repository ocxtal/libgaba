

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

/**
 * If this file is included from C++ source code, wrap content of this
 * file with namespace sea.
 */
#ifdef __cplusplus
namespace sea {
#endif

/** deprecated */
/* fixme: use config.h instead of hard coding */
#include <stdint.h>
typedef 	int64_t 	sea_int_t;			/**< sea_int_t: for general integer variables. must be signed, 32-bit integer or larger. */
#define SEA_INT_MIN 	( INT64_MIN )
#define SEA_INT_MAX		( INT64_MAX )

typedef 	int32_t 	sea_cell_t;			/**< sea_cell_t: for cell values in DP matrices. must be signed, 32-bit integer. */
#define SEA_CELL_MIN 	( INT32_MIN + 10 )	/** fixme: the margin must not be fixed */
#define SEA_CELL_MAX	( INT32_MAX - 10 )	/** fixme: the margin must not be fixed */

typedef 	int8_t		sea_sint_t;			/** sea_sint_t: for small values */
#define SEA_SINT_MIN 	( INT8_MIN )
#define SEA_SINT_MAX 	( INT8_MAX )

/**
 * @brief flags : option flag bit fields
 *
 * @detail
 * option flags are used in the sea_init API and some more internal functions
 * to represent options of alignment functions. the flags are represented in 
 * 32-bit length bit fields. the meaning of each field is following:
 *
 *  31 .. 20, 19 .. 18, 17 .. 14, 13 .. 10, 9 ..... 6,
 * +--------------------------------------------------+
 * | unused |   PROC  |   ALN   |   SEQ   | BANDWIDTH |
 * +--------------------------------------------------+
 *
 *  5 .. 4, 3 .. 2, 1 .. 0
 * +----------------------+
 * |  DP  |  COST |  ALG  |
 * +----------------------+
 *
 * The meaning of each field is:
 * PROC (19..18)	processor options. use this option when offload computation to GPUs.
 * ALN (17..14)		the output formatting.
 * SEQ (13..10)		the input sequence formatting.
 * BANDWIDTH (9..6) the band width in the banded dynamic programming.
 * DP (5..4)		the dynamic programming option. select the banded DP or the full DP.
 * COST (3..2)		cost option. select one from the linear-gap cost, the affine-gap cost, and the unit cost.
 * ALG (1..0)		algorithm option. select one from the Smith-Waterman (local), the Needleman-Wunsch (global), the seed-and-extend alignment.
 */

/**
 * @enum sea_flags_pos
 *
 * @brief (internal) positions of option flag bit field.
 */
enum sea_flags_pos {
	SEA_FLAGS_POS_ALG 		= 0,
	SEA_FLAGS_POS_COST 		= 3,
	SEA_FLAGS_POS_DP 		= 5,
	SEA_FLAGS_POS_POLICY 	= 8,
	SEA_FLAGS_POS_BANDWIDTH = 10,
	SEA_FLAGS_POS_SEQ 		= 14,
	SEA_FLAGS_POS_ALN 		= 16,
	SEA_FLAGS_POS_PROC 		= 18
};

/**
 * @enum sea_flags_mask
 *
 * @brief (internal) bit-field masks
 */
enum sea_flags_mask {
	SEA_FLAGS_MASK_ALG 		= 0x07<<SEA_FLAGS_POS_ALG,
	SEA_FLAGS_MASK_COST		= 0x03<<SEA_FLAGS_POS_COST,
	SEA_FLAGS_MASK_DP		= 0x07<<SEA_FLAGS_POS_DP,
	SEA_FLAGS_MASK_POLICY	= 0x03<<SEA_FLAGS_POS_POLICY,
	SEA_FLAGS_MASK_BANDWIDTH= 0x0f<<SEA_FLAGS_POS_BANDWIDTH,
	SEA_FLAGS_MASK_SEQ		= 0x03<<SEA_FLAGS_POS_SEQ,
	SEA_FLAGS_MASK_ALN		= 0x03<<SEA_FLAGS_POS_ALN,
	SEA_FLAGS_MASK_PROC 	= 0x03<<SEA_FLAGS_POS_PROC
};

/**
 * @enum sea_flags_alg
 *
 * @brief (API) constants of algorithm option.
 */
enum sea_flags_alg {
	SEA_SW 					= 1<<SEA_FLAGS_POS_ALG,
	SEA_SEA					= 2<<SEA_FLAGS_POS_ALG,
	SEA_XSEA				= 3<<SEA_FLAGS_POS_ALG,
	SEA_NW 					= 4<<SEA_FLAGS_POS_ALG
};

/**
 * @enum sea_flags_cost
 *
 * @brief (API) constants of cost option.
 */
enum sea_flags_cost {
	SEA_AFFINE_GAP_COST 	= 1<<SEA_FLAGS_POS_COST,
	SEA_LINEAR_GAP_COST 	= 2<<SEA_FLAGS_POS_COST,
	SEA_UNIT_COST 			= 3<<SEA_FLAGS_POS_COST
};

/**
 * @enum sea_flags_dp
 *
 * @brief (API) constants of the DP option.
 */
enum sea_flags_dp {
	SEA_FULL				= 1<<SEA_FLAGS_POS_DP,
	SEA_BANDED				= 2<<SEA_FLAGS_POS_DP,
	SEA_DYNAMIC				= 3<<SEA_FLAGS_POS_DP,
	SEA_GUIDED				= 4<<SEA_FLAGS_POS_DP
};

/**
 * @enum sea_flags_policy
 *
 * @brief (API) constants of the search policy, exact or heuristic.
 */
enum sea_flags_policy {
	SEA_EXACT 				= 1<<SEA_FLAGS_POS_POLICY,
	SEA_HEURISTIC			= 2<<SEA_FLAGS_POS_POLICY
};

/**
 * @enum sea_flags_bandwidth
 *
 * @brief (API) constants of the bandwidth option.
 */
enum sea_flags_bandwidth {
	SEA_BANDWIDTH_16 		= 1<<SEA_FLAGS_POS_BANDWIDTH,
	SEA_BANDWIDTH_32 		= 2<<SEA_FLAGS_POS_BANDWIDTH,
	SEA_BANDWIDTH_64 		= 3<<SEA_FLAGS_POS_BANDWIDTH,
	SEA_BANDWIDTH_128 		= 4<<SEA_FLAGS_POS_BANDWIDTH
};

/**
 * @enum sea_flags_seq
 *
 * @brief (API) constants of the sequence format option.
 */
enum sea_flags_seq {
	SEA_SEQ_ASCII 			= 1<<SEA_FLAGS_POS_SEQ,
	SEA_SEQ_4BIT 			= 2<<SEA_FLAGS_POS_SEQ,
	SEA_SEQ_2BIT 			= 3<<SEA_FLAGS_POS_SEQ,
	SEA_SEQ_4BIT8PACKED 	= 4<<SEA_FLAGS_POS_SEQ,
	SEA_SEQ_2BIT8PACKED 	= 5<<SEA_FLAGS_POS_SEQ,
	SEA_SEQ_1BIT64PACKED 	= 6<<SEA_FLAGS_POS_SEQ
};

/**
 * @enum sea_flags_aln
 *
 * @brief (API) constants of the alignment format option.
 */
enum sea_flags_aln {
	SEA_ALN_ASCII 			= 1<<SEA_FLAGS_POS_ALN,
	SEA_ALN_CIGAR			= 2<<SEA_FLAGS_POS_ALN,
	SEA_ALN_DIR 			= 3<<SEA_FLAGS_POS_ALN
};

/**
 * @enum sea_error
 *
 * @brief (API) error flags. see sea_init function and status member in the sea_result structure for more details.
 */
enum sea_error {
	SEA_SUCCESS 				=  0,	/*!< success!! */
	SEA_TERMINATED				=  1,	/*!< success (internal code) */
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
 * @struct sea_result
 *
 * @brief (API) a structure containing an alignment result.
 *
 * @sa sea_sea, sea_aln_free
 */
struct sea_result {
	void const *a; 			/*!< a pointer to a sequence a. */
	void const *b;			/*!< a pointer to a sequence b. */
	void *aln;				/*!< a pointer to a alignment result. */
	int32_t score;			/*!< an alignment score. */
	int64_t len;			/*!< alignment length. (the length of a content of aln) used as an errno container if error occured. */
	int64_t apos;			/*!< alignment start position on a. */
	int64_t alen;			/*!< alignment length on a. the alignment interval is a[apos]..a[apos+alen-1] */
	int64_t bpos;			/*!< alignment start position on b. */
	int64_t blen;			/*!< alignment length on b. the alignment interval is b[bpos]..b[bpos+blen-1] */
	struct sea_context *ctx;/*!< a pointer to a alignment context structure. */
};

/**
 * @type sea_result_t
 *
 * @brief (deprecated) an alias to `struct sea_result'.
 */
typedef struct sea_result 		sea_result_t;

/**
 * @type sea_res_t
 *
 * @brief (API) an alias to `struct sea_result'.
 */
typedef struct sea_result sea_res_t;

/**
 * @struct sea_pos
 *
 * @brief (internal) holds a pair of (x, y)-coordinate and (p, q)-coordinate.
 */
struct sea_pos {
	int64_t i, j, p, q;
};

typedef struct sea_pos sea_pos_t;

/**
 * @struct sea_mem
 *
 * @brief sequence reader
 */
struct sea_mem {
	void *sp, *ep;
};

typedef struct sea_mem sea_mem_t;

/**
 * @struct sea_reader
 *
 * @brief abstract sequence reader
 */
struct sea_reader {
	uint8_t *p;
	int64_t spos;
	uint8_t (*pop)(
		uint8_t const *p,
		int64_t pos);
	uint8_t b;
};

/**
 * @struct sea_writer
 *
 * @brief abstract sequence writer
 */
struct sea_writer {
	uint8_t *p;
	int64_t pos, size;
	int64_t (*pushm)(
		uint8_t *p,
		int64_t pos);
	int64_t (*pushx)(
		uint8_t *p,
		int64_t pos);
	int64_t (*pushi)(
		uint8_t *p,
		int64_t pos);
	int64_t (*pushd)(
		uint8_t *p,
		int64_t pos);
};

/**
 * @struct sea_ivec
 *
 * @brief init vector container
 */
struct sea_ivec {
	void *cv, *pv;				/*!< pointers to the initial vectors (should be allocated in the stack by fixed length array, or alloca) */
	int8_t clen, plen;			/*!< the lengths of the vectors */
	int8_t size;				/*!< the size of a cell in the vector */
};

typedef struct sea_ivec sea_ivec_t;

/**
 * @struct sea_process
 *
 * @brief (internal) an individual alignment context.
 */
struct sea_process {
	struct sea_ivec v;
	struct sea_reader a, b;		/*!< (in) sequence readers */
	struct sea_writer l;		/*!< (inout) alignment writer */
	struct sea_mem dp, dr;		/*!< (ref) a dynamic programming matrix */
	void *pdp;					/*!< dynamic programming matrix */
	uint8_t *pdr;				/*!< direction array */
	int64_t alen, blen;			/*!< the length of sequences */
	int64_t alim, blim;			/*!< the limit coordinate of the band */
	int64_t i, j, p, q;			/*!< temporary */
	int64_t mi, mj, mp, mq;		/*!< maximum score position */
	int64_t size;				/*!< default malloc size */
	int32_t bw;					/*!< bandwidth */
	int32_t max;				/*!< (inout) current maximum score */
};

/**
 * @type sea_proc_t
 *
 * @brief (internal) an alias to `struct sea_process'.
 */
typedef struct sea_process sea_proc_t;

/**
 * @struct sea_funcs
 *
 * @brief (internal) a struct which holds function pointers.
 */
struct sea_funcs {
	int32_t (*twig)(			/*!< diag 8bit */
		struct sea_context const *ctx,
		struct sea_process *proc);
	int32_t (*branch)(			/*!< diag 16bit */
		struct sea_context const *ctx,
		struct sea_process *proc);
	int32_t (*trunk)(			/*!< diag 32bit */
		struct sea_context const *ctx,
		struct sea_process *proc);
	int32_t (*bulge)(			/*!< 32bit bulge (guided balloon) algorithm */
		struct sea_context const *ctx,
		struct sea_process *proc);
	int32_t (*balloon)(			/*!< 32bit balloon algorithm */
		struct sea_context const *ctx,
		struct sea_process *proc);
	int32_t (*cap)(				/*!< 32bit cap algorithm */
		struct sea_context const *ctx,
		struct sea_process *proc);
	uint8_t (*popa)(			/*!< retrieve a character from sequence a */
		uint8_t const *ptr,
		int64_t pos);
	uint8_t (*popb)(
		uint8_t const *ptr,
		int64_t pos);
	int64_t (*pushm)(
		uint8_t *ptr,
		int64_t pos);
	int64_t (*pushx)(
		uint8_t *ptr,
		int64_t pos);
	int64_t (*pushi)(
		uint8_t *ptr,
		int64_t pos);
	int64_t (*pushd)(
		uint8_t *ptr,
		int64_t pos);
};

typedef struct sea_funcs sea_funcs_t;

/**
 * @struct sea_context
 *
 * @brief (API) an algorithmic context.
 *
 * @sa sea_init, sea_clean
 */
struct sea_context {
	struct sea_funcs *f;

	uint32_t flags;		/*!< a bitfield of option flags */
	int8_t m;			/*!< a dynamic programming cost: match */
	int8_t x;			/*!< a dynamic programming cost: mismatch */
	int8_t g;			/*!< same as gi (redundant) */
	int8_t gi;			/*!< a dynamic programming cost: gap open (in the affine-gap cost) or gap cost per base (in the linear-gap cost) */
	int8_t ge;			/*!< a dynamic programming cost: gap extension */
	int8_t mask;		/*!< a dynamic programming cost: mask length (in the mask-match algorithm) */
	
	int8_t k;			/*!< heuristic search length stretch ratio. (default is k = 4) */
	int32_t tx;			/*!< xdrop threshold. see sea_init for more details */
	int32_t tc;			/*!< chain threshold */
	int32_t tb; 		/*!< balloon termination threshold. */
	int32_t bw;			/*!< the width of the band. */
	int32_t min;		/*!< (in) lower bound of the score */
	uint32_t alg;		/*!< algorithm flag (same as (ctx->flags) & SEA_FLAG_MASK_ALG) */

	int32_t isize;		/*!< initial matsize */
	int32_t memaln;		/*!< memory alignment size (default: 32) */
};

/**
 * @type sea_context_t
 *
 * @brief (deprecated) an alias to `struct sea_context *'.
 */
typedef struct sea_context *	sea_context_t;

/**
 * @type sea_ctx_t
 *
 * @brief (API) an alias to `struct sea_context'.
 */
typedef struct sea_context sea_ctx_t;


#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */

/**
 * @fn sea_init
 *
 * @brief (API) constructs and initializes an alignment context.
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
sea_ctx_t *sea_init(
	int32_t flags,
	int8_t m,
	int8_t x,
	int8_t gi,
	int8_t ge,
	int32_t xdrop);

/**
 * @fn sea_init_fp
 *
 * @brief (internal) constructs and initializes an alignment context.
 *
 * @param[in] flags : option flags.
 * @param[in] sea_sea : a pointer to an alignment function.
 * @param[in] sea_matsize : a pointer to a function which calculates a size of a matrix.
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
sea_ctx_t *sea_init_fp(
	int32_t flags,
	int32_t (*sea_sea)(			/*!< diag 8bit */
		struct sea_context *ctx,
		struct sea_process *proc),
//	int32_t (*sea_sea)(
//		sea_res_t *aln,
//		sea_ctx_t *ctx,
//		sea_proc_t val),
//	sea_int_t (*sea_matsize)(
//		sea_int_t alen,
//		sea_int_t blen,
//		sea_int_t bandwidth),
	int8_t m,
	int8_t x,
	int8_t gi,
	int8_t ge,
	int32_t xdrop);

/**
 * @fn sea_align
 *
 * @brief (API) alignment function. 
 *
 * @param[ref] ctx : a pointer to an alignment context structure. must be initialized with sea_init function.
 * @param[in] a : a pointer to the query sequence a. see seqreader.h for more details of query sequence formatting.
 * @param[in] apos : the alignment start position in a. (0 <= apos < length(sequence a)) (or search start position in the Smith-Waterman algorithm). the alignment includes the position apos.
 * @param[in] alen : the extension length in a. (0 < alen) (to be exact, alen is search area length in the Smith-Waterman algorithm. the maximum extension length in the seed-and-extend alignment algorithm. the length of the query a to be aligned in the Needleman-Wunsch algorithm.)
 * @param[in] b : a pointer to the query sequence b.
 * @param[in] bpos : the alignment start position in b. (0 <= bpos < length(sequence b))
 * @param[in] blen : the extension length in b. (0 < blen)
 *
 * @return a pointer to the sea_result structure.
 *
 * @sa sea_init
 */
sea_res_t *sea_align(
	sea_ctx_t *ctx,
	void const *a,
	int64_t apos,
	int64_t alen,
	void const *b,
	int64_t bpos,
	int64_t blen);

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
	sea_ctx_t *ctx,
	sea_res_t *aln);

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
	sea_res_t *aln);

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
	sea_ctx_t *ctx);

#ifdef __cplusplus
}		/* end of extern "C" */
#endif  /* #ifdef __cplusplus */

/**
 * this __cplusplus section corresponds to the __cplusplus section at
 * the top of this file. the former section contains a declaration of
 * the beginning of namespace sea, and this section contains the end of
 * the namespace and the additional C++ classes which wrap C APIs.
 */
#ifdef __cplusplus

#include <string>

/**
 * @class AlignmentResult
 *
 * @brief (API) an wrapper class of struct sea_aln.
 */
class AlignmentResult {
private:
	sea_res_t *_aln;
public:
	/**
	 * constructor and destructor
	 */
	AlignmentResult(
		sea_res_t *aln) {
		_aln = aln;
		if(_aln == NULL) {
			throw (int)SEA_ERROR;
		}
		if(_aln->len < 0) {
			throw (int)(_aln->len);
		}
	}
	~AlignmentResult(void) {
		sea_aln_free(_aln);			
	}
	/**
	 * setters and getters
	 *
	 * sequence a
	 */
	void const *a(void) const {
		return(_aln->a);
	}
	void const *a(void const *n) {
		_aln->a = n;
		return(_aln->a);
	}

	sea_int_t &apos(void) const {
		return(_aln->apos);
	}
	sea_int_t &apos(sea_int_t const &n) {
		_aln->apos = n;
		return(_aln->apos);
	}

	sea_int_t &alen(void) const {
		return(_aln->alen);
	}
	sea_int_t &alen(sea_int_t const &n) {
		_aln->alen = n;
		return(_aln->alen);
	}

	/**
	 * sequence b
	 */
	void const *b(void) const {
		return(_aln->b);
	}
	void const *b(void const *n) {
		_aln->b = n;
		return(_aln->b);
	}

	sea_int_t &bpos(void) const {
		return(_aln->bpos);
	}
	sea_int_t &bpos(sea_int_t const &n) {
		_aln->bpos = n;
		return(_aln->bpos);
	}

	sea_int_t &blen(void) const {
		return(_aln->blen);
	}
	sea_int_t &blen(sea_int_t const &n) {
		_aln->blen = n;
		return(_aln->blen);
	}

	/**
	 * alignment string, score, and length
	 */
	void *aln(void) const {
		return(_aln->aln);
	}
	void *aln(void *n) {
		_aln->aln = n;
		return(_aln->aln);
	}
	sea_int_t &score(void) const {
		return(_aln->score);
	}
	sea_int_t &score(sea_int_t const &n) {
		_aln->score = n;
		return(_aln->score);
	}
	sea_int_t &len(void) const {
		return(_aln->len);
	}
	sea_int_t &len(sea_int_t const &n) {
		_aln->len = n;
		return(_aln->len);
	}
};

/**
 * @class AlignmentContext
 *
 * @brief (API) an wrapper class of sea_init function and sea_ctx_t
 */
class AlignmentContext {
private:
	sea_ctx_t *_ctx;

public:
	/**
	 * constructor. see sea.h for details of parameters.
	 */
	AlignmentContext(
		sea_int_t flags,
		sea_int_t len,
		sea_int_t m,
		sea_int_t x,
		sea_int_t gi,
		sea_int_t ge,
		sea_int_t xdrop) {
		_ctx = sea_init(flags, len, m, x, gi, ge, xdrop);
		if(_ctx == NULL) {
			throw (int)SEA_ERROR_OUT_OF_MEM;
		}
		if(_ctx->error_label != SEA_SUCCESS) {
			throw (int)(_ctx->error_label);
		}
	}
	/**
	 * destructor, calls sea_aln_free
	 */
	~AlignmentContext(void) {
		if(_ctx != NULL) {
			sea_clean(_ctx);
			_ctx = NULL;
		}
	}
	/**
	 * alignment functions. see sea.h
	 */
	AlignmentResult align(
		void const *a, sea_int_t apos, sea_int_t alen,
		void const *b, sea_int_t bpos, sea_int_t blen) {
		return(AlignmentResult(
			sea_align(_ctx, a, apos, alen, b, bpos, blen)));
	}

	AlignmentResult align(
		std::string a, sea_int_t apos, sea_int_t alen,
		std::string b, sea_int_t bpos, sea_int_t blen) {
		return(AlignmentResult(
			sea_align(_ctx, a.c_str(), apos, alen, b.c_str(), bpos, blen)));
	}

	AlignmentResult align(
		std::string a,
		std::string b) {
		return(AlignmentResult(
			sea_align(_ctx, a.c_str(), 0, a.length(), b.c_str(), 0, b.length())));
	}
};

}		/* end of namespace sea */
#endif

#endif  /* #ifndef _SEA_H_INCLUDED */

/*
 * end of sea.h
 */
