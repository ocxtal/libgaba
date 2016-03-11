
/*
 * @file util.h
 *
 * @brief a collection of utilities
 */

#ifndef _UTIL_H_INCLUDED
#define _UTIL_H_INCLUDED

#include "../sea.h"
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
_static_assert(sizeof(struct sea_score_s) == 20);
_static_assert(sizeof(struct sea_params_s) == 32);
_static_assert(sizeof(struct sea_seq_pair_s) == 32);
_static_assert(sizeof(struct sea_section_s) == 16);
_static_assert(sizeof(struct sea_fill_s) == 96);
_static_assert(sizeof(struct sea_path_section_s) == 48);
_static_assert(sizeof(struct sea_result_s) == 32);

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
void *sea_aligned_malloc(
	size_t size,
	size_t align)
{
	void *ptr = NULL;
	posix_memalign(&ptr, align, size);
	debug("posix_memalign(%p)", ptr);
	return(ptr);
}
static inline
void sea_aligned_free(
	void *ptr)
{
	if(ptr != NULL) { free(ptr); }
	return;
}

/**
 * structs
 */

/** forward declarations */
struct sea_dp_context_s;

#if 0
/**
 * @struct sea_position_s
 */
struct sea_position_s {
	int64_t apos, bpos;			/** (16) */
};
_static_assert(sizeof(struct sea_position_s) == 16);
_static_assert(offsetof(struct sea_position_s, apos) == offsetof(struct sea_section_s, apos));
_static_assert(offsetof(struct sea_position_s, bpos) == offsetof(struct sea_section_s, bpos));
#endif

/**
 * @union sea_dir_u
 */
union sea_dir_u {
	struct sea_dir_dynamic {
		int8_t acc;				/** (1) accumulator (v[0] - v[BW-1]) */
		int8_t _pad[3];			/** (3) */
		uint32_t array;			/** (4) dynamic band */
	} dynamic;
	struct sea_dir_guided {
		uint8_t *ptr;			/** (8) guided band */
	} guided;
};
_static_assert(sizeof(union sea_dir_u) == 8);

/**
 * @struct sea_small_delta_s
 */
struct sea_small_delta_s {
	int8_t delta[MAX_BW];		/** (32) small delta */
	int8_t max[MAX_BW];			/** (32) max */
};
_static_assert(sizeof(struct sea_small_delta_s) == 64);

/**
 * @struct sea_middle_delta_s
 */
struct sea_middle_delta_s {
	int16_t delta[MAX_BW];		/** (64) middle delta */
};
_static_assert(sizeof(struct sea_middle_delta_s) == 64);

/**
 * @struct sea_mask_u
 */
union sea_mask_u {
	vec_mask_t mask;
	uint32_t all;
};
_static_assert(sizeof(union sea_mask_u) == 4);

/**
 * @struct sea_mask_pair_u
 */
union sea_mask_pair_u {
	struct sea_mask_pair_s {
		union sea_mask_u h;		/** (4) horizontal mask vector */
		union sea_mask_u v;		/** (4) vertical mask vector */
	} pair;
	uint64_t all;
};
_static_assert(sizeof(union sea_mask_pair_u) == 8);

/**
 * @struct sea_diff_vec_s
 */
struct sea_diff_vec_s {
	uint8_t dh[MAX_BW];			/** (32) dh in the lower 5bits, de in the higher 3bits */
	uint8_t dv[MAX_BW];			/** (32) dv in the lower 5bits, df in the higher 3bits */
};
_static_assert(sizeof(struct sea_diff_vec_s) == 64);

/**
 * @struct sea_char_vec_s
 */
struct sea_char_vec_s {
	uint8_t w[MAX_BW];					/** (32) a in the lower 4bit, b in the higher 4bit */
};
_static_assert(sizeof(struct sea_char_vec_s) == 32);

/**
 * @struct sea_block_s
 */
struct sea_block_s {
	union sea_mask_pair_u mask[MAX_BLK];/** (256) mask vectors */
	union sea_dir_u dir;				/** (8) direction array */
	int64_t offset;						/** (8) large offset */
	struct sea_diff_vec_s diff;			/** (64) diff vectors */
	struct sea_small_delta_s sd;		/** (64) small delta */
	struct sea_char_vec_s ch;			/** (32) char vector */
	int32_t aridx, bridx;				/** (8) reverse index on the current section */
	union sea_mask_pair_u last_mask;	/** (8) corresponds to (blk + 1)->mask[-1] */
};
struct sea_phantom_block_s {
	union sea_dir_u dir;				/** (8) */
	int64_t offset;						/** (8) */
	struct sea_diff_vec_s diff; 		/** (64) */
	struct sea_small_delta_s sd;		/** (64) */
	struct sea_char_vec_s ch;			/** (32) char vector */
	int32_t aridx, bridx;				/** (8) reverse index on the current section */
	union sea_mask_pair_u last_mask;	/** (8) corresponds to (blk + 1)->mask[-1] */
};
_static_assert(sizeof(struct sea_block_s) == 448);
_static_assert(sizeof(struct sea_phantom_block_s) == 192);
#define SEA_BLOCK_PHANTOM_OFFSET	( offsetof(struct sea_block_s, dir) )
#define SEA_BLOCK_PHANTOM_SIZE		( offsetof(struct sea_phantom_block_s, sd) + offsetof(struct sea_small_delta_s, max) )
#define SEA_BLOCK_ZERO_OFFSET		( SEA_BLOCK_PHANTOM_OFFSET + SEA_BLOCK_PHANTOM_SIZE )
#define SEA_BLOCK_ZERO_SIZE			( sizeof(struct sea_small_delta_s) - offsetof(struct sea_small_delta_s, max) )
#define _phantom_block(x)			( (struct sea_block_s *)((uint8_t *)(x) + sizeof(struct sea_joint_head_s) - SEA_BLOCK_PHANTOM_OFFSET) )
#define _last_block(x)				( (struct sea_block_s *)(x) - 1 )
#define _block(x)					( (struct sea_block_s *)(x) )
_static_assert(SEA_BLOCK_PHANTOM_OFFSET == 256);
_static_assert(SEA_BLOCK_PHANTOM_SIZE == 112);

/**
 * @struct sea_joint_head_s
 *
 * @brief (internal) traceback start coordinate (coordinate at the beginning of the band) container
 * sizeof(struct sea_joint_head_s) == 32
 */
struct sea_joint_head_s {
	int32_t p;					/** (4) trace start p-coordinate */
	int32_t q;					/** (4) trace start q-coordinate */
	struct sea_joint_tail_s const *tail;/** (8) tail of the previous section */
};
_static_assert(sizeof(struct sea_joint_head_s) == 16);
#define _phantom_head(x)		( (struct sea_joint_head_s *)((uint8_t *)(x) + SEA_BLOCK_PHANTOM_OFFSET) - 1 )

/**
 * @struct sea_joint_tail_s
 *
 * @brief (internal) init vector container.
 * sizeof(struct sea_joint_tail_s) == 96
 */
struct sea_joint_tail_s {
	/* middle delta */
	struct sea_middle_delta_s const *v;	/** (8) pointer to the middle delta vectors */

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
	struct sea_joint_tail_s const *tail;/** (8) */
	uint64_t _unused;			/** (8) */
	uint32_t apos, bpos;		/** (8) pos */
	struct sea_section_s a;		/** (16) section a */
	struct sea_section_s b;		/** (16) section b */
};
_static_assert(sizeof(struct sea_joint_tail_s) == 96);
_static_assert(offsetof(struct sea_joint_tail_s, psum) == offsetof(struct sea_fill_s, psum));
_static_assert(offsetof(struct sea_joint_tail_s, p) == offsetof(struct sea_fill_s, p));
_static_assert(offsetof(struct sea_joint_tail_s, max) == offsetof(struct sea_fill_s, max));
#define _tail(x)				( (struct sea_joint_tail_s *)(x) )
#define _fill(x)				( (struct sea_fill_s *)(x) )
#define SEA_JOINT_TAIL_COPY_SIZE	( offsetof(struct sea_joint_tail_s, _unused) )
_static_assert(SEA_JOINT_TAIL_COPY_SIZE == 48);

/**
 * @struct sea_merge_tail_s
 */
struct sea_merge_tail_s {
	/* middle delta */
	void *null;					/** (8) must be NULL */
	// struct sea_middle_delta_s const *v;	/** (8) pointer to the middle delta vectors */

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
	struct sea_joint_tail_s const *tail;/** (8) */
	uint64_t _unused;			/** (8) */
	uint32_t apos, bpos;		/** (8) pos */
	struct sea_section_s a;		/** (16) section a */
	struct sea_section_s b;		/** (16) section b */

	/* tail array */
	uint8_t tail_idx[2][MAX_BW];	/** (64) array of index of joint_tail */
};
_static_assert(sizeof(struct sea_merge_tail_s) == 160);

/**
 * @struct sea_reader_s
 *
 * @brief (internal) abstract sequence reader
 * sizeof(struct sea_reader_s) == 16
 * sizeof(struct sea_reader_work_s) == 384
 */
struct sea_reader_s {
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
_static_assert(sizeof(struct sea_reader_s) == 16);
struct sea_reader_work_s {
	/** 64byte alidned */
	uint64_t atail, btail;				/** (16) tail of the current section */
	int32_t alen, blen;					/** (8) length from the tail of the current section */
	uint64_t plim;						/** (8) p limit coordinate */
	uint8_t _pad2[32];					/** (64) */
	/** 64, 64 */

	/** 64byte aligned */
	struct sea_seq_pair_s p;			/** (32) */
	uint8_t bufb[MAX_BW + MAX_BLK];		/** (64) */
	uint8_t _pad[MAX_BLK];				/** (32) */
	uint8_t bufa[MAX_BW + MAX_BLK];		/** (64) */
	/** 192, 256 */
};
_static_assert(sizeof(struct sea_reader_work_s) == 256);

#if 0
/**
 * @struct sea_writer_s
 *
 * @brief (internal) abstract sequence writer
 * sizeof(struct sea_writer_s) == 12
 * sizeof(struct sea_writer_work_s) == 24
 */
struct sea_writer_s {
	uint64_t (*push)(			/** (8) */
		uint8_t *p,
		uint64_t dst,
		uint64_t src,
		uint8_t c);
	uint8_t type;				/** (1) */
	uint8_t fr;					/** (1) */
	uint8_t _pad[6];			/** (6) */
};
_static_assert(sizeof(struct sea_writer_s) == 16);
struct sea_writer_work_s {
	uint8_t *p;					/** (8) */
	uint32_t size;				/** (4) malloc size */
	uint32_t rpos;				/** (4) previous reverse head */
	uint32_t pos;				/** (4) current head */
	uint32_t len;				/** (4) length of the string */
};
_static_assert(sizeof(struct sea_writer_work_s) == 24);
#endif

/**
 * @struct sea_writer_work_s
 */
#if 0
struct sea_writer_work_s {
	struct sea_path_section_s *fw_sec;
	struct sea_path_section_s *rv_sec;
	int32_t fw_scnt;
	int32_t rv_scnt;
	void *ptr;
};
_static_assert(sizeof(struct sea_writer_work_s) == 32);
#endif
struct sea_writer_work_s {
	/** 64byte aligned */

	/* section information */
	struct sea_section_s asec;		/** (16) */
	struct sea_section_s bsec;		/** (16) */
	struct sea_joint_tail_s const *atail;/** (8) */
	struct sea_joint_tail_s const *btail;/** (8) */

	/** block information */
	struct sea_joint_tail_s const *tail;/** (8) */
	struct sea_block_s const *blk;	/** (8) */
	/** 64, 64 */

	/** 64byte aligned */
	uint32_t a_update_mask, b_update_mask;/** (8) update is required if 0xffff */
	int32_t alen, blen;				/** (8) lengths of the current section */
	int32_t aridx, bridx;			/** (8) current ridx pair */
	int32_t asridx, bsridx;			/** (8) start ridx pair */

	uint32_t mask_max;				/** (4) */
	int32_t psum;					/** (4) */
	int32_t p;						/** (4) */
	int8_t q;						/** (1) */
	uint8_t len;					/** (1) */

	/* path string information */
	uint8_t fw_rem;					/** (1) */
	uint8_t rv_rem;					/** (1) */
	uint32_t *fw_path;				/** (8) */
	uint32_t *rv_path;				/** (8) */
	/** 64, 128 */

	/** 64byte aligned */
	/* section information */
	struct sea_path_section_s *fw_sec;/** (8) */
	struct sea_path_section_s *rv_sec;/** (8) */
	int32_t fw_scnt;				/** (4) */
	int32_t rv_scnt;				/** (4) */

	/* memory management */
	uint8_t *ptr;					/** (8) */
	/** 32, 160 */
};
_static_assert(sizeof(struct sea_writer_work_s) == 160);

/**
 * @struct sea_score_vec_s
 */
struct sea_score_vec_s {
	// int8_t mv[16];				/** (16) match matrix */
	int8_t sb[16];				/** (16) substitution matrix (or mismatch matrix) */
	int8_t adjh[16];			/** (16) */
	int8_t adjv[16];			/** (16) */
	int8_t ofsh[16];			/** (16) */
	int8_t ofsv[16];			/** (16) */
};
_static_assert(sizeof(struct sea_score_vec_s) == 80);
_static_assert_offset(struct sea_score_s, score_sub, struct sea_score_vec_s, sb, 0);
_static_assert_offset(struct sea_score_s, score_gi_a, struct sea_score_vec_s, adjh, 0);

/**
 * @struct sea_dp_context_s
 *
 * @brief (internal) container for dp implementations
 * sizeof(struct sea_dp_context_s) == 704
 */
struct sea_dp_context_s {
	/** individually stored on init */

	/** 64byte aligned */
	uint8_t *stack_top;			/** (8) dynamic programming matrix */
	uint8_t *stack_end;			/** (8) the end of dp matrix */
	uint8_t const *pdr;			/** (8) direction array */
	uint8_t const *tdr;			/** (8) the end of direction array */
	/** 32, 32 */

	struct sea_writer_work_s ll;	/** (160) */
	/** 160, 192 */

	/** 64byte aligned */
	struct sea_reader_work_s rr;	/** (256) */
	/** 256, 448 */

	/** loaded on init */
	/** 64byte aligned */
	struct sea_reader_s r;		/** (16) sequence readers */

	struct sea_score_vec_s scv;	/** (80) substitution matrix and gaps */
	int32_t tx;					/** (4) xdrop threshold */
	int32_t mem_cnt;			/** (4) */
	uint64_t mem_size;			/** (8) malloc size */
	struct sea_joint_tail_s *tail;	/** (8) template of the root tail */

	#define SEA_MEM_ARRAY_SIZE		( 17 )
	uint8_t *mem_array[SEA_MEM_ARRAY_SIZE];		/** (128) */

	/** 256, 704 */
};
_static_assert(sizeof(struct sea_dp_context_s) == 704);
#define SEA_DP_CONTEXT_LOAD_OFFSET	( offsetof(struct sea_dp_context_s, r) )
#define SEA_DP_CONTEXT_LOAD_SIZE	( sizeof(struct sea_dp_context_s) - SEA_DP_CONTEXT_LOAD_OFFSET )
_static_assert(SEA_DP_CONTEXT_LOAD_OFFSET == 448);
_static_assert(SEA_DP_CONTEXT_LOAD_SIZE == 256);

/**
 * @struct sea_context_s
 *
 * @brief (API) an algorithmic context.
 *
 * @sa sea_init, sea_close
 * sizeof(struct sea_context_s) = 1472
 */
struct sea_context_s {
	/** 64byte aligned */
	/** templates */
	struct sea_middle_delta_s md;	/** (64) */
	/** 64, 64 */
	
	/** 64byte aligned */
	struct sea_dp_context_s k;		/** (704) */
	/** 704, 768 */

	/** 64byte aligned */
	/** phantom vectors */
	struct sea_block_s blk;			/** (448) */
	struct sea_joint_tail_s tail;	/** (96) */
	/** 544, 1312 */

	/** 64byte aligned */
	/** params */
	struct sea_params_s params;		/** (32) */
	/** 32, 1344 */

	/** 64byte aligned */
};
_static_assert(sizeof(struct sea_context_s) == 1344);
#define SEA_DP_ROOT_SIZE 		( sizeof(struct sea_joint_tail_s) + sizeof(struct sea_phantom_block_s) )
#define SEA_DP_ROOT_TAIL_OFFSET	( sizeof(struct sea_phantom_block_s) )
#define SEA_DP_ROOT_LOAD_SIZE	( SEA_JOINT_TAIL_COPY_SIZE + sizeof(struct sea_phantom_block_s) )
_static_assert(SEA_DP_ROOT_SIZE == 288);
_static_assert(SEA_DP_ROOT_TAIL_OFFSET == 192);
_static_assert(SEA_DP_ROOT_LOAD_SIZE == 240);

/**
 * coordinate conversion macros
 */
#if 0
#define cox(p, q, band)				( ((p)>>1) - (q) )
#define coy(p, q, band)				( (((p)+1)>>1) + (q) )
#define cop(x, y, band)				( (x) + (y) )
#define coq(x, y, band) 			( ((y)-(x))>>1 )
#endif
#define rev(pos, len)				( 2 * (len) - (pos) )
#define roundup(x, base)			( ((x) + (base) - 1) & ~((base) - 1) )

/**
 * @enum _STATE
 */
enum _STATE {
	CONT 	= 1,
	TERM 	= 0
};

/**
 * direction determiner constants
 */
enum _DIR {
	LEFT 	= SEA_UE_LEFT,
	TOP 	= SEA_UE_TOP
};
enum _DIR2 {
	LL = (SEA_UE_LEFT<<2) | (SEA_UE_LEFT<<0),
	LT = (SEA_UE_LEFT<<2) | (SEA_UE_TOP<<0),
	TL = (SEA_UE_TOP<<2) | (SEA_UE_LEFT<<0),
	TT = (SEA_UE_TOP<<2) | (SEA_UE_TOP<<0)
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
