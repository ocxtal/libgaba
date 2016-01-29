
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
_static_assert(sizeof(struct sea_seq_pair_s) == 32);
_static_assert(sizeof(struct sea_checkpoint_s) == 16);
_static_assert(sizeof(struct sea_section_s) == 32);
_static_assert(sizeof(struct sea_score_s) == 20);
_static_assert(sizeof(struct sea_params_s) == 32);

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
#define SEA_MEM_ALIGN_SIZE		( 16 )
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
struct sea_chain_status_s;
struct sea_dp_context_s;
struct sea_graph_context_s;

/**
 * @union sea_dir_u
 */
union sea_dir_u {
	struct sea_dir_dynamic {
		int32_t acc;			/** (4) accumulator (v[0] - v[BW-1]) */
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
 * @struct sea_mask_pair_s
 */
struct sea_mask_pair_s {
	mask_t h;					/** (4) horizontal mask vector */
	mask_t v;					/** (4) vertical mask vector */
};
_static_assert(sizeof(struct sea_mask_pair_s) == 8);

/**
 * @struct sea_diff_vec_s
 */
struct sea_diff_vec_s {
	int8_t dh[MAX_BW];			/** (32) */
	int8_t dv[MAX_BW];			/** (32) */
	int8_t de[MAX_BW];			/** (32) */
	int8_t df[MAX_BW];			/** (32) */
};
_static_assert(sizeof(struct sea_diff_vec_s) == 128);

/**
 * @struct sea_block_s
 */
struct sea_block_s {
	struct sea_mask_pair_s mask[MAX_BLK];	/** (256) mask vectors */
	union sea_dir_u dir;				/** (8) direction array */
	int64_t offset;						/** (8) large offset */
	struct sea_diff_vec_s diff;			/** (128) diff vectors */
	struct sea_small_delta_s sd;		/** (64) small delta */
};
struct sea_phantom_block_s {
	struct sea_mask_pair_s mask[2];		/** (16) */
	union sea_dir_u dir;				/** (8) */
	int64_t offset;						/** (8) */
	struct sea_diff_vec_s diff; 		/** (128) */
	struct sea_small_delta_s sd;		/** (64) */
};
_static_assert(sizeof(struct sea_block_s) == 464);
_static_assert(sizeof(struct sea_phantom_block_s) == 224);
#define SEA_BLOCK_PHANTOM_OFFSET	( offsetof(struct sea_block_s, mask[30]) )
#define SEA_BLOCK_PHANTOM_SIZE		( sizeof(struct sea_block_s) - SEA_BLOCK_PHANTOM_OFFSET )
#define _phantom_block(x)			( (struct sea_block_s *)((uint8_t *)(x) - SEA_BLOCK_PHANTOM_OFFSET) )
#define _last_block(x)				( (struct sea_block_s *)(x) - 1 )
_static_assert(SEA_BLOCK_PHANTOM_OFFSET == 240);
_static_assert(SEA_BLOCK_PHANTOM_SIZE == sizeof(struct sea_phantom_block_s));

/**
 * @struct sea_joint_head_s
 *
 * @brief (internal) traceback start coordinate (coordinate at the beginning of the band) container
 * sizeof(struct sea_joint_head_s) == 16
 */
struct sea_joint_head_s {
	uint32_t p;					/** (4) trace start p-coordinate */
	uint32_t q;					/** (4) trace start q-coordinate */
	struct sea_joint_tail_s *tail;/** (8) tail of the previous section */
};
_static_assert(sizeof(struct sea_joint_head_s) == 16);
#define _head(pdp, member)		((struct sea_joint_head_s *)pdp)->member

/**
 * @struct sea_joint_tail_s
 *
 * @brief (internal) init vector container.
 * sizeof(struct sea_joint_tail_s) == 96
 */
struct sea_joint_tail_s {
	/* middle deltas */
	struct sea_middle_delta_s *v;	/** (8) pointer to the middle delta vectors */

	/* misc */
	// uint64_t size;				/** (8) size of section in bytes */
	int32_t max;				/** (4) max */
	uint32_t p; 				/** (4) local p-coordinate of the tail */
	uint32_t mp, mq;			/** (8) max local-(p, q) */
	uint64_t psum;				/** (8) global p-coordinate of the tail */

	/* prefetched sequence buffer */
	uint8_t wa[MAX_BW];			/** (32) prefetched sequence */
	uint8_t wb[MAX_BW];			/** (32) prefetched sequence */
};
_static_assert(sizeof(struct sea_joint_tail_s) == 96);
#define _tail(pdp, member) 		((struct sea_joint_tail_s *)pdp)->member

/**
 * @struct sea_merge_tail_s
 */
struct sea_merge_tail_s {
	uint8_t tail_idx[2][MAX_BW];	/** (64) array of index of joint_tail */

	struct sea_middle_delta_s *v;	/** (8) */

	uint32_t max;				/** (4) max */
	uint32_t p; 				/** (4) local p-coordinate of the tail */
	uint32_t mp, mq;			/** (8) max local-(p, q) */
	uint64_t psum;				/** (8) global p-coordinate of the tail */

	/* prefetched sequence buffer */
	uint8_t wa[MAX_BW];			/** (32) prefetched sequence */
	uint8_t wb[MAX_BW];			/** (32) prefetched sequence */
};
_static_assert(sizeof(struct sea_merge_tail_s) == 160);

/**
 * @struct sea_chain_status_s
 * @brief pair of status and pdp
 */
struct sea_chain_status_s {
	void *ptr;
	int32_t stat;
};

/**
 * @struct sea_section_pair_s
 * @brief concatenated two section: [posa1, lima1) ~ [posa2, lima2) and [posb1, limb1) ~ [posb2, limb2)
 * sizeof(struct sea_section_pair_s) == 80
 */
struct sea_section_pair_s {
	struct sea_section_s body;	/** (32) */
	struct sea_section_s tail;	/** (32) */
	uint64_t limp;				/** (8) */
	uint64_t _pad;				/** (8) */
};
_static_assert(sizeof(struct sea_section_pair_s) == 80);
#define set_sec_pair(sec, pa1, la1, pb1, lb1, pa2, la2, pb2, lb2) { \
	(sec)->body.asp = (pa1); (sec)->body.bsp = (pb1); \
	(sec)->body.aep = (la1); (sec)->body.bep = (lb1); \
	(sec)->tail.asp = (pa2); (sec)->tail.bsp = (pb2); \
	(sec)->tail.aep = (la2); (sec)->tail.bep = (lb2); \
}
#define sea_build_section_pair(_sec1, _sec2, _p) ( \
	(struct sea_section_pair_s) { \
		.body = (_sec1), \
		.tail = (_sec2), \
		.limp = (_p) \
	} \
)

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
	uint64_t acnt, bcnt;				/** (16) */
	struct sea_section_pair_s s;		/** (80) */
	uint8_t _pad1[MAX_BLK];				/** (32) */
	/** 128, 128 */

	/** 64byte aligned */
	uint8_t bufa[MAX_BW + MAX_BLK];		/** (64) */
	uint8_t _pad2[2 * MAX_BLK];			/** (64) */
	/** 128, 256 */

	/** 64byte aligned */
	uint8_t bufb[MAX_BW + MAX_BLK];		/** (64) */
	uint8_t _pad3[MAX_BLK];				/** (32) */
	struct sea_seq_pair_s p;			/** (32) */
	/** 128, 384 */
};
_static_assert(sizeof(struct sea_reader_work_s) == 384);

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

/**
 * @struct sea_dp_work_s
 */
struct sea_dp_work_s {
	uint8_t _pad[64];
};
_static_assert(sizeof(struct sea_dp_work_s) == 64);

/**
 * @struct sea_aln_funcs_s
 */
struct sea_aln_funcs_s {
	/** narrow, wide */
	struct sea_chain_status_s (*fill)(
		struct sea_dp_context_s *this,
		struct sea_joint_tail_s *tail,
		struct sea_section_pair_s *sec);
	struct sea_chain_status_s (*merge)(
		struct sea_dp_context_s *this,
		struct sea_joint_tail_s *tail_list,
		uint64_t tail_num);
	struct sea_chain_status_s (*trace)(
		struct sea_dp_context_s *this,
		struct sea_joint_tail_s *tail,
		struct sea_joint_tail_s *prev_tail);
};
_static_assert(sizeof(struct sea_aln_funcs_s) == 24);

/**
 * @struct sea_score_vec_s
 */
struct sea_score_vec_s {
	int8_t sbv[16];				/** (16) substitution matrix */
	int8_t geav[16];			/** (16) gap penalty offset on seq a */
	int8_t gebv[16];			/** (16) gap penalty offset on seq b */
	int8_t giav[16];			/** (16) gap penalty offset on seq a */
	int8_t gibv[16];			/** (16) gap penalty offset on seq b */
};
_static_assert(sizeof(struct sea_score_vec_s) == 80);
_static_assert_offset(struct sea_score_s, score_sub, struct sea_score_vec_s, sbv, 0);
_static_assert_offset(struct sea_score_s, score_gi_a, struct sea_score_vec_s, geav, 0);

/**
 * @struct sea_dp_context_s
 *
 * @brief (internal) container for dp implementations
 * sizeof(struct sea_dp_context_s) == 512
 */
struct sea_dp_context_s {
	/** individually stored on init */

	/** 64byte aligned */
	uint8_t *stack_top;			/** (8) dynamic programming matrix */
	uint8_t *stack_end;			/** (8) the end of dp matrix */
	uint8_t const *pdr;			/** (8) direction array */
	uint8_t const *tdr;			/** (8) the end of direction array */
	// uint8_t _pad1[8];			/** (8) */
	struct sea_joint_tail_s *tail;	/** (8) */
	struct sea_writer_work_s ll;	/** (24) */
	/** 64, 64 */

	/** 64byte aligned */
	struct sea_reader_work_s rr;	/** (384) */
	/** 384, 448 */

	/** loaded on init */
	/** 64byte aligned */
	struct sea_reader_s r;		/** (16) sequence readers */
	struct sea_writer_s l;		/** (16) alignment writer */

	struct sea_score_vec_s scv;	/** (80) substitution matrix and gaps */

	int32_t tx;					/** (4) xdrop threshold */

	// int32_t max;				/** (4) current maximum score */
	// struct sea_joint_tail_s *m_tail;	/** (8) */

	int32_t mem_cnt;			/** (4) */
	uint64_t mem_size;			/** (8) malloc size */
	/** 128, 576 */

	/** 64byte aligned */
	#define SEA_MEM_ARRAY_SIZE		( 13 )
	uint8_t *mem_array[SEA_MEM_ARRAY_SIZE];		/** (104) */
	struct sea_aln_funcs_s fn;	/** (24) function pointers */

	/** 128, 704 */
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
	struct sea_dp_context_s k;		/** (704) */
	/** 704, 704 */
	
	/** 64byte aligned */
	struct sea_middle_delta_s md;	/** (64) */
	/** 64, 768 */

	/** 64byte aligned */
	/** phantom vectors */
	struct sea_phantom_block_s blk;	/** (224) */
	struct sea_joint_tail_s tail;	/** (96) */
	/** 320, 1088 */

	/** constants */
	/** 64byte aligned */
	struct sea_writer_s rv;			/** (16) reverse writer */
	struct sea_writer_s fw;			/** (16) forward writer */

	/** params */
	struct sea_params_s params;		/** (32) */
	/** 64, 1152 */

	/** 64byte aligned */
};
_static_assert(sizeof(struct sea_context_s) == 1152);

/**
 * function declarations
 * naive, twig, branch, trunk
 */
struct sea_chain_status_s wide_dynamic_fill(struct sea_dp_context_s *this, struct sea_joint_tail_s *tail, struct sea_section_pair_s *sec);
struct sea_chain_status_s wide_guided_fill(struct sea_dp_context_s *this, struct sea_joint_tail_s *tail, struct sea_section_pair_s *sec);
struct sea_chain_status_s wide_dynamic_trace(struct sea_dp_context_s *this, struct sea_joint_tail_s *tail, struct sea_section_pair_s *sec);
struct sea_chain_status_s wide_guided_trace(struct sea_dp_context_s *this, struct sea_joint_tail_s *tail, struct sea_section_pair_s *sec);
struct sea_chain_status_s narrow_dynamic_fill(struct sea_dp_context_s *this, struct sea_joint_tail_s *tail, struct sea_section_pair_s *sec);
struct sea_chain_status_s narrow_guided_fill(struct sea_dp_context_s *this, struct sea_joint_tail_s *tail, struct sea_section_pair_s *sec);
struct sea_chain_status_s narrow_dynamic_trace(struct sea_dp_context_s *this, struct sea_joint_tail_s *tail, struct sea_section_pair_s *sec);
struct sea_chain_status_s narrow_guided_trace(struct sea_dp_context_s *this, struct sea_joint_tail_s *tail, struct sea_section_pair_s *sec);

/**
 * coordinate conversion macros
 */
#define cox(p, q, band)				( ((p)>>1) - (q) )
#define coy(p, q, band)				( (((p)+1)>>1) + (q) )
#define cop(x, y, band)				( (x) + (y) )
#define coq(x, y, band) 			( ((y)-(x))>>1 )
#define rev(pos, len)				( 2 * (len) - (pos) )

/**
 * @enum _STATE
 */
enum _STATE {
	CONT 	= 0,
	MEM 	= 1,
	CHAIN 	= 2,
	CAP 	= 3,
	TERM 	= 4
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
