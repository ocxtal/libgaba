
/*
 * @file util.h
 *
 * @brief a collection of utilities
 */

#ifndef _UTIL_H_INCLUDED
#define _UTIL_H_INCLUDED

#include "../gaba.h"
#include "log.h"
#include "bench.h"
#include "sassert.h"
#include <stdio.h>
#include <stdint.h>				/** uint32_t, uint64_t, ... */
#include <stddef.h>				/** offsetof */
#include <string.h>				/** memset, memcpy */

/** assume 64bit little-endian system */
_static_assert(sizeof(void *) == 8);

/** check size of structs declared in sea.h */
_static_assert(sizeof(struct gaba_score_s) == 20);
_static_assert(sizeof(struct gaba_params_s) == 16);
_static_assert(sizeof(struct gaba_seq_pair_s) == 32);
_static_assert(sizeof(struct gaba_section_s) == 16);
_static_assert(sizeof(struct gaba_fill_s) == 96);
_static_assert(sizeof(struct gaba_path_section_s) == 48);
_static_assert(sizeof(struct gaba_result_s) == 32);

/**
 * constants
 */
#define MAX_BW			( 32 )
#define MAX_BLK			( 32 )
#define mask_t			uint32_t

/**
 * aligned malloc
 */
static inline
void *gaba_aligned_malloc(
	size_t size,
	size_t align)
{
	void *ptr = NULL;
	if(posix_memalign(&ptr, align, size) != 0) {
		debug("posix_memalign failed");
		return(NULL);
	}
	debug("posix_memalign(%p)", ptr);
	return(ptr);
}
static inline
void gaba_aligned_free(
	void *ptr)
{
	free(ptr);
	return;
}

/**
 * structs
 */

/** forward declarations */
struct gaba_dp_context_s;

/**
 * @union gaba_dir_u
 */
union gaba_dir_u {
	struct gaba_dir_dynamic {
		int8_t acc;				/** (1) accumulator (v[0] - v[BW-1]) */
		int8_t _pad[3];			/** (3) */
		uint32_t array;			/** (4) dynamic band */
	} dynamic;
	struct gaba_dir_guided {
		uint8_t *ptr;			/** (8) guided band */
	} guided;
};
_static_assert(sizeof(union gaba_dir_u) == 8);

/**
 * @struct gaba_small_delta_s
 */
struct gaba_small_delta_s {
	int8_t delta[MAX_BW];		/** (32) small delta */
	int8_t max[MAX_BW];			/** (32) max */
};
_static_assert(sizeof(struct gaba_small_delta_s) == 64);

/**
 * @struct gaba_middle_delta_s
 */
struct gaba_middle_delta_s {
	int16_t delta[MAX_BW];		/** (64) middle delta */
};
_static_assert(sizeof(struct gaba_middle_delta_s) == 64);

/**
 * @struct gaba_mask_u
 */
union gaba_mask_u {
	vec_mask_t mask;
	uint32_t all;
};
_static_assert(sizeof(union gaba_mask_u) == 4);

/**
 * @struct gaba_mask_pair_u
 */
union gaba_mask_pair_u {
	struct gaba_mask_pair_s {
		union gaba_mask_u h;	/** (4) horizontal mask vector */
		union gaba_mask_u v;	/** (4) vertical mask vector */
	} pair;
	uint64_t all;
};
_static_assert(sizeof(union gaba_mask_pair_u) == 8);

/**
 * @struct gaba_diff_vec_s
 */
struct gaba_diff_vec_s {
	uint8_t dh[MAX_BW];			/** (32) dh in the lower 5bits, de in the higher 3bits */
	uint8_t dv[MAX_BW];			/** (32) dv in the lower 5bits, df in the higher 3bits */
};
_static_assert(sizeof(struct gaba_diff_vec_s) == 64);

/**
 * @struct gaba_char_vec_s
 */
struct gaba_char_vec_s {
	uint8_t w[MAX_BW];					/** (32) a in the lower 4bit, b in the higher 4bit */
};
_static_assert(sizeof(struct gaba_char_vec_s) == 32);

/**
 * @struct gaba_block_s
 */
struct gaba_block_s {
	union gaba_mask_pair_u mask[MAX_BLK];/** (256) mask vectors */
	struct gaba_diff_vec_s diff; 		/** (64) */
	struct gaba_small_delta_s sd;		/** (64) */
	union gaba_dir_u dir;				/** (8) */
	int64_t offset;						/** (8) */
	int32_t aridx, bridx;				/** (8) reverse index in the current section */
	uint64_t reserved;					/** (8) */
	struct gaba_char_vec_s ch;			/** (32) char vector */
};
struct gaba_phantom_block_s {
	struct gaba_diff_vec_s diff; 		/** (64) */
	struct gaba_small_delta_s sd;		/** (64) */
	union gaba_dir_u dir;				/** (8) */
	int64_t offset;						/** (8) */
	int32_t aridx, bridx;				/** (8) reverse index in the current section */
	uint64_t reserved;					/** (8) */
	struct gaba_char_vec_s ch;			/** (32) char vector */
};
_static_assert(sizeof(struct gaba_block_s) == 448);
_static_assert(sizeof(struct gaba_phantom_block_s) == 192);
#define _last_block(x)				( (struct gaba_block_s *)(x) - 1 )

/**
 * @struct gaba_joint_tail_s
 *
 * @brief (internal) init vector container.
 * sizeof(struct gaba_joint_tail_s) == 96
 */
struct gaba_joint_tail_s {
	/* middle delta */
	struct gaba_middle_delta_s const *v;	/** (8) pointer to the middle delta vectors */

	/* coordinates */
	int64_t psum;				/** (8) global p-coordinate of the tail */
	int32_t p;					/** (4) local p-coordinate of the tail */
	uint32_t ssum;				/** (4) */

	/* max scores */
	int64_t max;				/** (8) max */

	/* status */
	uint32_t stat;				/** (4) */
	uint32_t rem_len;			/** (4) */

	/* section info */
	struct gaba_joint_tail_s const *tail;/** (8) */
	uint64_t _unused;			/** (8) */
	uint32_t apos, bpos;		/** (8) pos */
	struct gaba_section_s a;	/** (16) section a */
	struct gaba_section_s b;	/** (16) section b */
};
_static_assert(sizeof(struct gaba_joint_tail_s) == 96);
_static_assert(offsetof(struct gaba_joint_tail_s, psum) == offsetof(struct gaba_fill_s, psum));
_static_assert(offsetof(struct gaba_joint_tail_s, p) == offsetof(struct gaba_fill_s, p));
_static_assert(offsetof(struct gaba_joint_tail_s, max) == offsetof(struct gaba_fill_s, max));
#define _tail(x)				( (struct gaba_joint_tail_s *)(x) )
#define _fill(x)				( (struct gaba_fill_s *)(x) )

/**
 * @struct gaba_merge_tail_s
 */
struct gaba_merge_tail_s {
	/* middle delta */
	void *null;					/** (8) must be NULL */
	// struct gaba_middle_delta_s const *v;	/** (8) pointer to the middle delta vectors */

	/* coordinates */
	int64_t psum;				/** (8) global p-coordinate of the tail */
	int32_t p;					/** (4) local p-coordinate of the tail */
	uint32_t ssum;				/** (4) */

	/* max scores */
	int64_t max;				/** (8) max */

	/* status */
	uint32_t stat;				/** (4) */
	uint32_t rem_len;			/** (4) */

	/* section info */
	struct gaba_joint_tail_s const *tail;/** (8) */
	uint64_t _unused;			/** (8) */
	uint32_t apos, bpos;		/** (8) pos */
	struct gaba_section_s a;	/** (16) section a */
	struct gaba_section_s b;	/** (16) section b */

	/* tail array */
	uint8_t tail_idx[2][MAX_BW];	/** (64) array of index of joint_tail */
};
_static_assert(sizeof(struct gaba_merge_tail_s) == 160);

/**
 * @struct gaba_reader_s
 *
 * @brief (internal) abstract sequence reader
 * sizeof(struct gaba_reader_s) == 16
 * sizeof(struct gaba_reader_work_s) == 384
 */
#if 0
struct gaba_reader_s {
	void (*loada)(				/** (8) */
		uint8_t *dst,
		uint8_t const *src,
		uint64_t pos,
		uint64_t src_len,
		uint64_t copy_len);
	void (*loadb)(				/** (8) */
		uint8_t *dst,
		uint8_t const *src,
		uint64_t pos,
		uint64_t src_len,
		uint64_t copy_len);
};
_static_assert(sizeof(struct gaba_reader_s) == 16);
#endif
struct gaba_reader_work_s {
	/** 64byte alidned */
	uint64_t atail, btail;				/** (16) tail of the current section */
	int32_t alen, blen;					/** (8) length from the tail of the current section */
	uint64_t plim;						/** (8) p limit coordinate */
	uint8_t _pad2[16];					/** (16) */
	uint64_t alim, blim;				/** (16) max index of seq array */
	/** 64, 64 */

	/** 64byte aligned */
	struct gaba_seq_pair_s p;			/** (32) */
	uint8_t bufb[MAX_BW + MAX_BLK];		/** (64) */
	uint8_t _pad[MAX_BLK];				/** (32) */
	uint8_t bufa[MAX_BW + MAX_BLK];		/** (64) */
	/** 192, 256 */
};
_static_assert(sizeof(struct gaba_reader_work_s) == 256);

/**
 * @struct gaba_writer_work_s
 */
struct gaba_writer_work_s {
	/** 64byte aligned */

	/* section information */
	struct gaba_section_s asec;		/** (16) */
	struct gaba_section_s bsec;		/** (16) */
	struct gaba_joint_tail_s const *atail;/** (8) */
	struct gaba_joint_tail_s const *btail;/** (8) */

	/** block information */
	struct gaba_joint_tail_s const *tail;/** (8) */
	struct gaba_block_s const *blk;	/** (8) */
	/** 64, 64 */

	/** 64byte aligned */
	int32_t alen, blen;				/** (8) lengths of the current section */
	int32_t aidx, bidx;				/** (8) current ridx pair */
	int32_t asidx, bsidx;			/** (8) start ridx pair */

	int64_t psum;					/** (8) */
	int32_t p;						/** (4) */
	uint32_t mask_max;				/** (4) */
	int16_t q;						/** (2) */
	int16_t len;					/** (2) */

	/* path string information */
	int16_t fw_rem;					/** (2) */
	int16_t rv_rem;					/** (2) */
	uint32_t *fw_path;				/** (8) */
	uint32_t *rv_path;				/** (8) */
	/** 64, 128 */

	/** 64byte aligned */
	/* section information */
	struct gaba_path_section_s *fw_sec;/** (8) */
	struct gaba_path_section_s *rv_sec;/** (8) */
	int32_t fw_scnt;				/** (4) */
	int32_t rv_scnt;				/** (4) */

	/* memory management */
	uint8_t *ptr;					/** (8) */
	/** 32, 160 */
};
_static_assert(sizeof(struct gaba_writer_work_s) == 160);

/**
 * @struct gaba_score_vec_s
 */
struct gaba_score_vec_s {
	// int8_t mv[16];				/** (16) match matrix */
	int8_t sb[16];				/** (16) substitution matrix (or mismatch matrix) */
	int8_t adjh[16];			/** (16) */
	int8_t adjv[16];			/** (16) */
	int8_t ofsh[16];			/** (16) */
	int8_t ofsv[16];			/** (16) */
};
_static_assert(sizeof(struct gaba_score_vec_s) == 80);
_static_assert_offset(struct gaba_score_s, score_sub, struct gaba_score_vec_s, sb, 0);
_static_assert_offset(struct gaba_score_s, score_gi_a, struct gaba_score_vec_s, adjh, 0);

/**
 * @struct gaba_dp_context_s
 *
 * @brief (internal) container for dp implementations
 * sizeof(struct gaba_dp_context_s) == 704
 */
struct gaba_dp_context_s {
	/** individually stored on init */

	/** 64byte aligned */
	uint8_t *stack_top;			/** (8) dynamic programming matrix */
	uint8_t *stack_end;			/** (8) the end of dp matrix */
	uint8_t const *pdr;			/** (8) direction array */
	uint8_t const *tdr;			/** (8) the end of direction array */
	/** 32, 32 */

	struct gaba_writer_work_s ll;	/** (160) */
	/** 160, 192 */

	/** 64byte aligned */
	struct gaba_reader_work_s rr;	/** (256) */
	/** 256, 448 */

	/** loaded on init */
	/** 64byte aligned */
	struct gaba_score_vec_s scv;/** (80) substitution matrix and gaps */
	int32_t tx;					/** (4) xdrop threshold */
	int32_t mem_cnt;			/** (4) */
	uint64_t mem_size;			/** (8) malloc size */
	struct gaba_joint_tail_s *tail;	/** (8) template of the root tail */

	/** input options */
	uint8_t seq_a_direction;	/** (1) */
	uint8_t seq_b_direction;	/** (1) */

	/** output options */
	int16_t head_margin;		/** (2) margin at the head of gaba_res_t */
	int16_t tail_margin;		/** (2) margin at the tail of gaba_res_t */
	uint16_t _pad;				/** (2) */

	#define GABA_MEM_ARRAY_SIZE		( 18 )
	uint8_t *mem_array[GABA_MEM_ARRAY_SIZE];		/** (128) */

	/** 256, 704 */
};
_static_assert(sizeof(struct gaba_dp_context_s) == 704);
#define GABA_DP_CONTEXT_LOAD_OFFSET	( offsetof(struct gaba_dp_context_s, scv) )
#define GABA_DP_CONTEXT_LOAD_SIZE	( sizeof(struct gaba_dp_context_s) - GABA_DP_CONTEXT_LOAD_OFFSET )
_static_assert(GABA_DP_CONTEXT_LOAD_OFFSET == 448);
_static_assert(GABA_DP_CONTEXT_LOAD_SIZE == 256);

/**
 * @struct gaba_context_s
 *
 * @brief (API) an algorithmic context.
 *
 * @sa gaba_init, gaba_close
 * sizeof(struct gaba_context_s) = 1472
 */
struct gaba_context_s {
	/** 64byte aligned */
	/** templates */
	struct gaba_middle_delta_s md;	/** (64) */
	/** 64, 64 */
	
	/** 64byte aligned */
	struct gaba_dp_context_s k;		/** (704) */
	/** 704, 768 */

	/** 64byte aligned */
	/** phantom vectors */
	struct gaba_phantom_block_s blk;/** (192) */
	struct gaba_joint_tail_s tail;	/** (96) */
	/** 288, 1056 */

	/** 64byte aligned */
	/** params */
	struct gaba_params_s params;	/** (16) */
	uint64_t _pad[2];				/** (16) */
	/** 32, 1088 */

	/** 64byte aligned */
};
_static_assert(sizeof(struct gaba_context_s) == 1088);

/**
 * coordinate conversion macros
 */
#define rev(pos, len)				( 2 * (len) - (pos) )
// #define rev(pos, len)				( (len) - (pos) )		/* len must be twiced on load */
#define roundup(x, base)			( ((x) + (base) - 1) & ~((base) - 1) )

/**
 * @enum _STATE
 */
enum _STATE {
	CONT 	= 0,
	UPDATE  = 0x0100,
	TERM 	= 0x0200
};
_static_assert((int32_t)CONT == (int32_t)GABA_STATUS_CONT);
_static_assert((int32_t)UPDATE == (int32_t)GABA_STATUS_UPDATE);
_static_assert((int32_t)TERM == (int32_t)GABA_STATUS_TERM);

/**
 * direction determiner constants
 */
enum _DIR {
	LEFT 	= GABA_UE_LEFT,
	TOP 	= GABA_UE_TOP
};
enum _DIR2 {
	LL = (GABA_UE_LEFT<<2) | (GABA_UE_LEFT<<0),
	LT = (GABA_UE_LEFT<<2) | (GABA_UE_TOP<<0),
	TL = (GABA_UE_TOP<<2) | (GABA_UE_LEFT<<0),
	TT = (GABA_UE_TOP<<2) | (GABA_UE_TOP<<0)
};

/**
 * string concatenation macros
 */

/**
 * @macro JOIN2
 * @brief a macro which takes two string and concatenates them.
 */
#define JOIN2(i,j)						i##j
#define JOIN3(i,j,k)					i##j##k
#define JOIN4(i,j,k,l)					i##j##k##l

/**
 * @macro Q, QUOTE
 * @brief string quotation
 */
#define Q(a)							#a
#define QUOTE(a)						Q(a)

/**
 * function name composition macros.
 */
/**
 * @macro func, func2, func3
 */
#define func(a)							a
#define func2(a,b)						JOIN2(a,b)
#define func3(a,b)						JOIN3(a,b)

/**
 * @macro label2
 * @breif an wrapper of JOIN3
 */
#define label(a)						a
#define label2(a,b)						JOIN2(a,b)
#define label3(a,b,c)					JOIN3(a,b,c)

/**
 * max and min
 */
#define MAX2(x,y) 		( (x) > (y) ? (x) : (y) )
#define MAX3(x,y,z) 	( MAX2(x, MAX2(y, z)) )
#define MAX4(w,x,y,z) 	( MAX2(MAX2(w, x), MAX2(y, z)) )

#define MIN2(x,y) 		( (x) < (y) ? (x) : (y) )
#define MIN3(x,y,z) 	( MIN2(x, MIN2(y, z)) )
#define MIN4(w,x,y,z) 	( MIN2(MIN2(w, x), MIN2(y, z)) )

#endif /* #ifndef _UTIL_H_INCLUDED */

/*
 * end of util.h
 */
