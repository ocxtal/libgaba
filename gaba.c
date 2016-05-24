
/**
 * @file gaba.c
 *
 * @brief libgaba (libsea3) API implementations
 *
 * @author Hajime Suzuki
 * @date 2016/1/11
 * @license Apache v2
 */

/* import unittest */
#define UNITTEST_UNIQUE_ID			33
#include  "unittest.h"


#include <stdio.h>				/* sprintf in dump_path */
#include <stdint.h>				/* uint32_t, uint64_t, ... */
#include <stddef.h>				/* offsetof */
#include <string.h>				/* memset, memcpy */
#include "gaba.h"
#include "arch/arch.h"


/* aliasing vector macros */
#define _VECTOR_ALIAS_PREFIX	v32i8
#include "arch/vector_alias.h"

/* import utils */
// #include "util.h"
#include "bench.h"
#include "sassert.h"


/* input sequence bitwidth option (2bit or 4bit) */
#ifdef BIT
#  if !(BIT == 2 || BIT == 4)
#    error "BIT must be 2 or 4."
#  endif
#else
#  define BIT 						4
// #  define BIT 						2
#endif

/* gap penalty model (linear or affine) */
#define LINEAR 						1
#define AFFINE						2

#ifdef MODEL
#  if !(MODEL == LINEAR || MODEL == AFFINE)
#    error "MODEL must be LINEAR (1) or AFFINE (2)."
#  endif
#else
#  define MODEL 					AFFINE
// #  define MODEL 					LINEAR
#endif

/* constants */
#define BW_BASE						( 5 )
#define BW 							( 0x01<<BW_BASE )
#define BLK_BASE					( 5 )
#define BLK 						( 0x01<<BLK_BASE )
#define MAX_BW						( 32 )
#define MAX_BLK						( 32 )
#define mask_t						uint32_t


#define MIN_BULK_BLOCKS				( 32 )
#define MEM_ALIGN_SIZE				( 32 )		/* 32byte aligned for AVX2 environments */
#define MEM_INIT_SIZE				( (uint64_t)32 * 1024 * 1024 )
#define MEM_MARGIN_SIZE				( 2048 )
#define PSUM_BASE					( 1 )


/**
 * @macro _likely, _unlikely
 * @brief branch prediction hint for gcc-compatible compilers
 */
#define _likely(x)		__builtin_expect(!!(x), 1)
#define _unlikely(x)	__builtin_expect(!!(x), 0)

/**
 * @macro _force_inline
 * @brief inline directive for gcc-compatible compilers
 */
#define _force_inline	inline
// #define _force_inline


/** assume 64bit little-endian system */
_static_assert(sizeof(void *) == 8);

/** check size of structs declared in gaba.h */
_static_assert(sizeof(struct gaba_score_s) == 20);
_static_assert(sizeof(struct gaba_params_s) == 16);
_static_assert(sizeof(struct gaba_section_s) == 16);
_static_assert(sizeof(struct gaba_fill_s) == 64);
_static_assert(sizeof(struct gaba_path_section_s) == 32);
_static_assert(sizeof(struct gaba_result_s) == 32);


/* forward declarations */
static int32_t gaba_dp_add_stack(struct gaba_dp_context_s *this, uint64_t size);
static void *gaba_dp_malloc(struct gaba_dp_context_s *this, uint64_t size);
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
	struct gaba_middle_delta_s const *md;/** (8) pointer to the middle delta vectors */
	struct gaba_char_vec_s ch;			/** (32) char vector */
};
struct gaba_phantom_block_s {
	struct gaba_diff_vec_s diff; 		/** (64) */
	struct gaba_small_delta_s sd;		/** (64) */
	union gaba_dir_u dir;				/** (8) */
	int64_t offset;						/** (8) */
	int32_t aridx, bridx;				/** (8) reverse index in the current section */
	struct gaba_middle_delta_s const *md;/** (8) pointer to the middle delta vectors */
	struct gaba_char_vec_s ch;			/** (32) char vector */
};
_static_assert(sizeof(struct gaba_block_s) == 448);
_static_assert(sizeof(struct gaba_phantom_block_s) == 192);
#define _last_block(x)				( (struct gaba_block_s *)(x) - 1 )

/**
 * @struct gaba_joint_tail_s
 *
 * @brief (internal) init vector container.
 * sizeof(struct gaba_joint_tail_s) == 64
 */
struct gaba_joint_tail_s {
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
	uint32_t apos, bpos;		/** (8) pos */
	uint32_t alen, blen;		/** (8) len */
	uint32_t aid, bid;			/** (8) id */
};
_static_assert(sizeof(struct gaba_joint_tail_s) == 64);
_static_assert(offsetof(struct gaba_joint_tail_s, psum) == offsetof(struct gaba_fill_s, psum));
_static_assert(offsetof(struct gaba_joint_tail_s, p) == offsetof(struct gaba_fill_s, p));
_static_assert(offsetof(struct gaba_joint_tail_s, max) == offsetof(struct gaba_fill_s, max));
#define _tail(x)				( (struct gaba_joint_tail_s *)(x) )
#define _fill(x)				( (struct gaba_fill_s *)(x) )

/**
 * @struct gaba_merge_tail_s
 */
struct gaba_merge_tail_s {
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
	uint32_t apos, bpos;		/** (8) pos */
	uint32_t alen, blen;		/** (8) len */
	uint32_t aid, bid;			/** (8) id */

	/* tail array */
	uint8_t tail_idx[2][MAX_BW];	/** (64) array of index of joint_tail */
};
_static_assert(sizeof(struct gaba_merge_tail_s) == 128);

/**
 * @struct gaba_reader_work_s
 *
 * @brief (internal) abstract sequence reader
 * sizeof(struct gaba_reader_s) == 16
 * sizeof(struct gaba_reader_work_s) == 384
 */
struct gaba_reader_work_s {
	/** 64byte alidned */
	uint8_t const *alim, *blim;			/** (16) max index of seq array */
	uint8_t const *atail, *btail;		/** (16) tail of the current section */
	int32_t alen, blen;					/** (8) lengths of the current section */
	uint32_t aid, bid;					/** (8) ids */
	uint64_t plim;						/** (8) p limit coordinate */
	uint64_t _pad;						/** (8) */
	/** 64, 64 */

	/** 64byte aligned */
	uint8_t bufa[MAX_BW + MAX_BLK];		/** (64) */
	uint8_t bufb[MAX_BW + MAX_BLK];		/** (64) */
	/** 128, 192 */
};
_static_assert(sizeof(struct gaba_reader_work_s) == 192);

/**
 * @struct gaba_writer_work_s
 */
struct gaba_writer_work_s {
	/* reserved to avoid collision with alim and blim */
	uint8_t const *alim, *blim;			/** (16) unused in writer */

	/* section info */
	struct gaba_path_section_s *sec;	/** (8) section base pointer */
	uint32_t fw_sec_idx, rv_sec_idx;	/** (8) current section indices */
	uint32_t tail_sec_idx;				/** (4) tail index */

	/* path string info */
	int16_t fw_rem, rv_rem;				/** (4) */
	uint32_t *fw_path, *rv_path;		/** (16) */
	uint32_t *tail_path;				/** (8) */
	/** 64, 64 */

	/** 64byte aligned */
	/** block pointers */
	struct gaba_joint_tail_s const *tail;/** (8) current tail */
	struct gaba_block_s const *blk;		/** (8) current block */

	/** section info */
	struct gaba_joint_tail_s const *atail;/** (8) */
	struct gaba_joint_tail_s const *btail;/** (8) */

	/** lengths and ids */
	int32_t alen, blen;					/** (8) section lengths */
	uint32_t aid, bid;					/** (8) */

	/* indices */
	int32_t aidx, bidx;					/** (8) indices of the current trace */
	int32_t asidx, bsidx;				/** (8) base indices of the current trace */
	/** 64, 128 */

	/** 64byte aligned */
	/* p-coordinates */
	int64_t psum;						/** (8) */
	int32_t p;							/** (4) */
	int32_t q;							/** (4) */

	/* path length info */
	uint32_t *spath;					/** (8) */
	uint32_t srem;						/** (4) */
	uint32_t pspos;						/** (4) */

	uint64_t _pad2[4];					/** (32) */
};
_static_assert(sizeof(struct gaba_writer_work_s) == 192);

/**
 * @struct gaba_score_vec_s
 */
struct gaba_score_vec_s {
	int8_t v1[16];
	int8_t v2[16];
	int8_t v3[16];
	int8_t v4[16];
	int8_t v5[16];

	#if 0
	int8_t adjh[16];			/** (16) */
	int8_t adjv[16];			/** (16) */
	int8_t ofsh[16];			/** (16) */
	int8_t ofsv[16];			/** (16) */
	int8_t sb[16];				/** (16) substitution matrix (or mismatch matrix) */
	#endif
};
_static_assert(sizeof(struct gaba_score_vec_s) == 80);

/**
 * @struct gaba_dp_context_s
 *
 * @brief (internal) container for dp implementations
 * sizeof(struct gaba_dp_context_s) == 640
 */
struct gaba_dp_context_s {
	/** individually stored on init */

	/** 64byte aligned */
	union gaba_work_s {
		struct gaba_writer_work_s l;	/** (192) */
		struct gaba_reader_work_s r;	/** (192) */
	} w;
	/** 192, 192 */

	/** 64byte aligned */
	/** loaded on init */
	struct gaba_score_vec_s scv;		/** (80) substitution matrix and gaps */
	/** 80, 272 */

	uint8_t *stack_top;					/** (8) dynamic programming matrix */
	uint8_t *stack_end;					/** (8) the end of dp matrix */
	/** 16, 288 */

	int16_t tx;							/** (2) xdrop threshold */
	uint16_t mem_cnt;					/** (2) */

	/** output options */
	int16_t head_margin;				/** (2) margin at the head of gaba_res_t */
	int16_t tail_margin;				/** (2) margin at the tail of gaba_res_t */

	#define GABA_MEM_ARRAY_SIZE		( 11 )
	uint8_t *mem_array[GABA_MEM_ARRAY_SIZE];/** (88) */
	/** 96, 384 */

	/** phantom vectors */
	/** 64byte aligned */
	struct gaba_phantom_block_s blk;	/** (192) */
	/** 192, 576 */

	/** 64byte aligned */
	struct gaba_joint_tail_s tail;		/** (64) */
	/** 64, 640 */
};
_static_assert(sizeof(struct gaba_dp_context_s) == 640);
#define GABA_DP_CONTEXT_LOAD_OFFSET	( offsetof(struct gaba_dp_context_s, scv) )
#define GABA_DP_CONTEXT_LOAD_SIZE	( sizeof(struct gaba_dp_context_s) - GABA_DP_CONTEXT_LOAD_OFFSET )
_static_assert(GABA_DP_CONTEXT_LOAD_OFFSET == 192);
_static_assert(GABA_DP_CONTEXT_LOAD_SIZE == 448);

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
	struct gaba_dp_context_s k;		/** (640) */
	/** 640, 704 */

	/** 64byte aligned */
	struct gaba_params_s params;	/** (16) */
	uint64_t _pad[6];				/** (48) */
	/** 64byte aligned */
};
_static_assert(sizeof(struct gaba_context_s) == 768);

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
 * coordinate conversion macros
 */
#define _rev(pos, len)				( (len) + (uint64_t)(len) - (uint64_t)(pos) )
#define _roundup(x, base)			( ((x) + (base) - 1) & ~((base) - 1) )

/**
 * max and min
 */
#define MAX2(x,y) 		( (x) > (y) ? (x) : (y) )
#define MAX3(x,y,z) 	( MAX2(x, MAX2(y, z)) )
#define MAX4(w,x,y,z) 	( MAX2(MAX2(w, x), MAX2(y, z)) )

#define MIN2(x,y) 		( (x) < (y) ? (x) : (y) )
#define MIN3(x,y,z) 	( MIN2(x, MIN2(y, z)) )
#define MIN4(w,x,y,z) 	( MIN2(MIN2(w, x), MIN2(y, z)) )


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



/* direction macros */
#define DYNAMIC 		( 1 )
#if DYNAMIC
#define direction_prefix 		dynamic_

/* direction determiners for the dynamic band algorithms */
/**
 * @macro _dir_fetch
 */
#define _dir_fetch(_dir) { \
	(_dir).dynamic.array <<= 1; \
	(_dir).dynamic.array |= (uint32_t)((_dir).dynamic.acc < 0); \
	debug("fetched dir(%x), %s", (_dir).dynamic.array, _dir_is_down(dir) ? "go down" : "go right"); \
}
/**
 * @macro _dir_update
 * @brief update direction determiner for the next band
 */
#define _dir_update(_dir, _vector, _sign) { \
	(_dir).dynamic.acc += (_sign) * (_ext(_vector, 0) - _ext(_vector, BW-1)); \
	/*debug("acc(%d), (%d, %d)", (_dir).dynamic.acc, _ext(_vector, 0), _ext(_vector, BW-1));*/ \
}
/**
 * @macro _dir_adjust_remainder
 * @brief adjust direction array when termination is detected in the middle of the block
 */
#define _dir_adjust_remainder(_dir, _filled_count) { \
	debug("adjust remainder, array(%x), shifted array(%x)", \
		(_dir).dynamic.array, \
		(_dir).dynamic.array<<(BLK - (_filled_count))); \
	(_dir).dynamic.array <<= (BLK - (_filled_count)); \
}
/**
 * @macro _dir_test_bound, _dir_test_bound_cap
 * @brief test if the bound of the direction array is invaded
 */
#define _dir_test_bound(_dir, k, p) 		( /* nothing to do */ )
#define _dir_test_bound_cap(_dir, k, p)		( /* nothing to do */ )
/**
 * @macro _dir_is_down, _dir_is_right
 * @brief direction indicator (_dir_is_down returns true if dir == down)
 */
#define _dir_is_down(_dir)					( (_dir).dynamic.array & 0x01 )
#define _dir_is_right(_dir)					( !_dir_is_down(_dir) )
/**
 * @macro _dir_load
 */
#define _dir_load(_blk, _local_idx) ({ \
	union gaba_dir_u _dir = (_blk)->dir; \
	debug("load dir idx(%d), mask(%x), shifted mask(%x)", (int32_t)_local_idx, _dir.dynamic.array, _dir.dynamic.array>>(BLK - (_local_idx) - 1)); \
	_dir.dynamic.array >>= (BLK - (_local_idx) - 1); \
	_dir; \
})
/**
 * @macro _dir_bcnt
 */
#define _dir_bcnt(_dir) ( \
	popcnt((_dir).dynamic.array) \
)
/**
 * @macro _dir_windback
 */
#define _dir_windback(_dir) { \
	(_dir).dynamic.array >>= 1; \
}

#else

#define direction_prefix 		guided_

/* direction determiners for the guided band algorithms */
#define _dir_fetch(_dir)					{ /* nothing to do */ }
#define _dir_update(_dir, _vector) { \
	(_dir).guided.ptr++; \
}
#define _dir_adjust_remainder(_dir, i)		{ /* nothing to do */ }
#define _dir_test_bound(_dir, k, p) ( \
	((k)->tdr - (_dir).guided.ptr + 2) - (p) - BLK \
)
#define _dir_test_bound_cap(_dir, k, p) ( \
	((k)->tdr - (_dir).guided.ptr + 2) - (p) \
)
#define _dir_is_down(_dir)					( *(_dir).guided.ptr != 0 )
#define _dir_is_right(_dir)					( *(_dir).guided.ptr == 0 )
#define _dir_load(_blk, _local_idx) ({ \
	union gaba_dir_u _dir = (_blk)->dir; \
	_dir.guided.ptr -= (BLK - (_local_idx)); \
	_dir; \
})
#define _dir_windback(_dir) { \
	(_dir).guided.ptr--; \
}

#endif

/**
 * seqreader macros
 */
#define _rd_bufa_base(k)		( (k)->w.r.bufa + BLK + BW )
#define _rd_bufb_base(k)		( (k)->w.r.bufb )
#define _rd_bufa(k, pos, len)	( _rd_bufa_base(k) - (pos) - (len) )
#define _rd_bufb(k, pos, len)	( _rd_bufb_base(k) + (pos) )
#define _lo64(v)		_ext_v2i64(v, 0)
#define _hi64(v)		_ext_v2i64(v, 1)
#define _lo32(v)		_ext_v2i32(v, 0)
#define _hi32(v)		_ext_v2i32(v, 1)

/**
 * @fn transpose_section_pair
 */
static _force_inline
struct gaba_trans_section_s {
	v2i32_t id;
	v2i32_t len;
	v2i64_t base;
} transpose_section_pair(
	v2i64_t a,
	v2i64_t b)
{
	_print_v2i64(a);
	_print_v2i64(b);

	/* transpose */
	v2i32_t id_len_a = _cast_v2i64_v2i32(a);
	v2i32_t id_len_b = _cast_v2i64_v2i32(b);

	_print_v2i32(id_len_a);
	_print_v2i32(id_len_b);

	v2i32_t id = _lo_v2i32(id_len_a, id_len_b);
	v2i32_t len = _hi_v2i32(id_len_a, id_len_b);
	v2i64_t base = _hi_v2i64(a, b);

	_print_v2i32(id);
	_print_v2i32(len);
	_print_v2i64(base);

	return((struct gaba_trans_section_s){
		.id = id,
		.len = len,
		.base = base
	});
}

/*
 * @fn fill_load_section
 */
static _force_inline
void fill_load_section(
	struct gaba_dp_context_s *this,
	struct gaba_section_s const *a,
	struct gaba_section_s const *b,
	uint64_t plim)
{
	/* load current section lengths */
	struct gaba_trans_section_s c = transpose_section_pair(
		_loadu_v2i64(a), _loadu_v2i64(b));

	/* convert current section to (pos of the end, len) */
	v2i64_t c_tail = _add_v2i64(c.base, _cvt_v2i32_v2i64(c.len));

	_print_v2i32(c.id);
	_print_v2i32(c.len);
	_print_v2i64(c_tail);

	/* store sections */
	_store_v2i64(&this->w.r.atail, c_tail);
	_store_v2i32(&this->w.r.alen, c.len);
	_store_v2i32(&this->w.r.aid, c.id);
	this->w.r.plim = plim;

	return;
}

/**
 * @struct gaba_joint_block_s
 * @brief result container for block fill functions
 */
struct gaba_joint_block_s {
	struct gaba_block_s *blk;
	int64_t p;
	int32_t stat;
};

/**
 * @fn fill_load_seq_a
 */
static _force_inline
void fill_load_seq_a(
	struct gaba_dp_context_s *this,
	uint8_t const *pos,
	uint64_t len)
{
	#if BIT == 2
		if(pos < this->w.r.alim) {
			debug("reverse fetch a: pos(%p), len(%llu)", pos, len);
			/* reverse fetch: 2 * alen - (2 * alen - pos) + (len - 32) */
			vec_t a = _loadu(pos + (len - BW));
			_print(a);
			_print(_swap(a));
			_storeu(_rd_bufa(this, BW, len), _swap(a));
		} else {
			debug("forward fetch a: pos(%p), len(%llu)", pos, len);
			/* take complement */
			vec_t const mask = _set(0x03);

			/* forward fetch: 2 * alen - pos */
			vec_t a = _loadu(_rev(pos, this->w.r.alim));
			_storeu(_rd_bufa(this, BW, len), _xor(a, mask));
		}
	#else /* BIT == 4 */
		if(pos < this->w.r.alim) {
			debug("reverse fetch a: pos(%p), len(%llu)", pos, len);
			/* reverse fetch: 2 * alen - (2 * alen - pos) + (len - 32) */
			vec_t a = _loadu(pos + (len - BW));
			_print(a);
			_print(_swap(a));
			_storeu(_rd_bufa(this, BW, len), _swap(a));
		} else {
			debug("forward fetch a: pos(%p), len(%llu)", pos, len);
			/* take complement */
			uint8_t const comp[16] __attribute__(( aligned(16) )) = {
				0x00, 0x08, 0x04, 0x0c, 0x02, 0x0a, 0x06, 0x0e,
				0x01, 0x09, 0x05, 0x0d, 0x03, 0x0b, 0x07, 0x0f
			};
			vec_t const cv = _bc_v16i8(_load_v16i8(comp));

			/* forward fetch: 2 * alen - pos */
			vec_t a = _loadu(_rev(pos, this->w.r.alim));
			_storeu(_rd_bufa(this, BW, len), _shuf(a, cv));
		}
	#endif
	return;
}

/**
 * @fn fill_load_seq_b
 */
static _force_inline
void fill_load_seq_b(
	struct gaba_dp_context_s *this,
	uint8_t const *pos,
	uint64_t len)
{
	#if BIT == 2
		if(pos < this->w.r.blim) {
			debug("forward fetch b: pos(%p), len(%llu)", pos, len);
			/* forward fetch: pos */
			vec_t b = _loadu(pos);
			_storeu(_rd_bufb(this, BW, len), _shl(b, 2));
		} else {
			debug("reverse fetch b: pos(%p), len(%llu)", pos, len);
			/* take complement */
			vec_t const mask = _set(0x03);

			/* reverse fetch: 2 * blen - pos + (len - 32) */
			vec_t b = _loadu(_rev(pos, this->w.r.blim) + (len - BW));
			_print(b);
			_print(_shl(_swap(b), 2));
			_storeu(_rd_bufb(this, BW, len), _shl(_swap(_xor(b, mask)), 2));
		}
	#else /* BIT == 4 */
		if(pos < this->w.r.blim) {
			debug("forward fetch b: pos(%p), len(%llu)", pos, len);
			/* forward fetch: pos */
			vec_t b = _loadu(pos);
			_storeu(_rd_bufb(this, BW, len), b);
		} else {
			debug("reverse fetch b: pos(%p), len(%llu)", pos, len);
			/* take complement */
			uint8_t const comp[16] __attribute__(( aligned(16) )) = {
				0x00, 0x08, 0x04, 0x0c, 0x02, 0x0a, 0x06, 0x0e,
				0x01, 0x09, 0x05, 0x0d, 0x03, 0x0b, 0x07, 0x0f
			};
			vec_t const cv = _bc_v16i8(_load_v16i8(comp));

			/* reverse fetch: 2 * blen - pos + (len - 32) */
			vec_t b = _loadu(_rev(pos, this->w.r.blim) + (len - BW));
			_storeu(_rd_bufb(this, BW, len), _shuf(_swap(b), cv));
		}
	#endif
	return;
}

/**
 * @fn fill_bulk_fetch
 *
 * @brief fetch 32bases from current section
 */
static _force_inline
void fill_bulk_fetch(
	struct gaba_dp_context_s *this,
	struct gaba_block_s *blk)
{
	/* load sequence from previous block */
	vec_t const mask = _set(0x0f);
	vec_t w = _load(&(blk - 1)->ch.w);
	vec_t a = _and(mask, w);
	vec_t b = _and(mask, _shr(w, 4));

	_print(w);
	_print(a);
	_print(b);

	debug("atail(%p), aridx(%d)", this->w.r.atail, (blk-1)->aridx);
	debug("btail(%p), bridx(%d)", this->w.r.btail, (blk-1)->bridx);

	/* fetch seq a */
	fill_load_seq_a(this, this->w.r.atail - (blk - 1)->aridx, BLK);
	_store(_rd_bufa(this, 0, BW), a);

	/* fetch seq b */
	_store(_rd_bufb(this, 0, BW), b);
	fill_load_seq_b(this, this->w.r.btail - (blk - 1)->bridx, BLK);
	return;
}

/**
 * @fn fill_cap_fetch
 *
 * @return the length of the sequence fetched.
 */
static _force_inline
void fill_cap_fetch(
	struct gaba_dp_context_s *this,
	struct gaba_block_s *blk)
{
	/* const */
	v2i32_t const z = _zero_v2i32();
	v2i32_t const tot = _set_v2i32(BLK);

	/* load lengths */
	v2i32_t ridx = _load_v2i32(&(blk - 1)->aridx);

	/* clip len with max load length */
	v2i32_t len = _max_v2i32(_min_v2i32(ridx, tot), z);

	/* load sequence from previous block */
	vec_t const mask = _set(0x0f);
	vec_t w = _load(&(blk - 1)->ch.w);
	vec_t a = _and(mask, w);
	vec_t b = _and(mask, _shr(w, 4));

	_print(w);
	_print(a);
	_print(b);

	/* fetch seq a */
	fill_load_seq_a(this, this->w.r.atail - _lo32(ridx), _lo32(len));
	_store(_rd_bufa(this, 0, BW), a);

	/* fetch seq b */
	_store(_rd_bufb(this, 0, BW), b);
	fill_load_seq_b(this, this->w.r.btail - _hi32(ridx), _hi32(len));
	return;
}

/**
 * @macro fill_init_fetch
 * @brief similar to cap fetch, updating ridx and rem
 */
static _force_inline
struct gaba_joint_block_s fill_init_fetch(
	struct gaba_dp_context_s *this,
	struct gaba_phantom_block_s *blk,
	struct gaba_joint_tail_s const *prev_tail,
	v2i32_t ridx)
{
	/* restore (brem, arem) */
	int64_t prem = -prev_tail->psum;
	int64_t arem = (prem + 1) / 2;
	int64_t brem = prem / 2;
	v2i32_t rem = _seta_v2i32(brem, arem);

	/* calc the next rem */
	v2i32_t const z = _zero_v2i32();
	v2i32_t const ofs = _seta_v2i32(1, 0);
	v2i32_t nrem = _max_v2i32(_sub_v2i32(rem, ridx), z);
	v2i32_t rrem = _sub_v2i32(_swap_v2i32(nrem), ofs);
	nrem = _max_v2i32(nrem, rrem);

	/* cap fetch */
	v2i32_t len = _sub_v2i32(rem, nrem);

	/* load sequence from the previous block */ {
		struct gaba_block_s *prev_blk = _last_block(prev_tail);
		vec_t const mask = _set(0x0f);
		vec_t w = _load(&prev_blk->ch.w);
		vec_t a = _and(mask, w);
		vec_t b = _and(mask, _shr(w, 4));

		debug("prev_blk(%p), prev_tail(%p)", prev_blk, prev_tail);
		_print(w);
		_print(a);
		_print(b);

		/* fetch seq a */
		fill_load_seq_a(this, this->w.r.atail - _lo32(ridx), _lo32(len));
		_store(_rd_bufa(this, 0, BW), a);

		/* fetch seq b */
		_store(_rd_bufb(this, 0, BW), b);
		fill_load_seq_b(this, this->w.r.btail - _hi32(ridx), _hi32(len));
	}

	/* store char vector to the current block */ {
		vec_t a = _loadu(_rd_bufa(this, _lo32(len), BW));
		vec_t b = _loadu(_rd_bufb(this, _hi32(len), BW));
		_store(&blk->ch.w, _or(a, _shl(b, 4)));

		_print(a);
		_print(b);
	}

	/* adjust and store ridx */
	_print_v2i32(ridx);
	_print_v2i32(len);
	ridx = _sub_v2i32(ridx, len);
	_store_v2i32(&blk->aridx, ridx);
	_print_v2i32(ridx);
	debug("blk(%p), p(%d), stat(%x)", blk + 1, _lo32(len) + _hi32(len), _mask_v2i32(_eq_v2i32(ridx, z)));

	return((struct gaba_joint_block_s){
		.blk = (struct gaba_block_s *)(blk + 1),
		.p = _lo32(len) + _hi32(len),
		.stat = (_mask_v2i32(_eq_v2i32(ridx, z)) == V2I32_MASK_00) ? CONT : UPDATE
	});
}

/**
 * @macro fill_restore_fetch
 * @brief fetch sequence from existing block
 */
static _force_inline
void fill_restore_fetch(
	struct gaba_dp_context_s *this,
	struct gaba_block_s const *blk)
{
	vec_t const mask = _set(0x0f);

	/* calc cnt */
	v2i32_t curr_len = _load_v2i32(&blk->aridx);
	v2i32_t prev_len = _load_v2i32(&(blk - 1)->aridx);
	v2i32_t cnt = _sub_v2i32(prev_len, curr_len);

	/* from the current block */
	vec_t cw = _load(&blk->ch.w);
	vec_t ca = _and(mask, cw);
	vec_t cb = _and(mask, _shr(cw, 4));
	_storeu(_rd_bufa(this, _lo32(cnt), BW), ca);
	_storeu(_rd_bufb(this, _hi32(cnt), BW), cb);

	/* from the previous block */
	vec_t pw = _load(&(blk - 1)->ch.w);
	vec_t pa = _and(mask, pw);
	vec_t pb = _and(mask, _shr(pw, 4));
	_store(_rd_bufa(this, 0, BW), pa);
	_store(_rd_bufb(this, 0, BW), pb);

	_print(pa);
	_print(pb);

	return;
}

/**
 * @macro fill_update_section
 */
static _force_inline
v2i32_t fill_update_section(
	struct gaba_dp_context_s *this,
	struct gaba_block_s *blk,
	v2i32_t cnt)
{
	/* update pos and ridx */
	v2i32_t ridx = _load_v2i32(&(blk - 1)->aridx);
	ridx = _sub_v2i32(ridx, cnt);
	_print_v2i32(ridx);

	/* store ridx to block */
	_store_v2i32(&blk->aridx, ridx);

	vec_t a = _loadu(_rd_bufa(this, _lo32(cnt), BW));
	vec_t b = _loadu(_rd_bufb(this, _hi32(cnt), BW));
	_store(&blk->ch.w, _or(a, _shl(b, 4)));

	_print(a);
	_print(b);
	return(ridx);
}

/**
 * @fn fill_create_phantom_block
 * @brief create joint_head on the stack to start block extension
 */
static _force_inline
struct gaba_joint_block_s fill_create_phantom_block(
	struct gaba_dp_context_s *this,
	struct gaba_joint_tail_s const *prev_tail)
{
	debug("create head prev_tail(%p), p(%d), psum(%lld), ssum(%d)",
		prev_tail, prev_tail->p, prev_tail->psum, prev_tail->ssum);

	/* init working stack */
	struct gaba_phantom_block_s *blk = (struct gaba_phantom_block_s *)this->stack_top;
	struct gaba_block_s *pblk = _last_block(prev_tail);

	/* copy phantom vectors from the previous fragment */
	// _memcpy_blk_aa(&blk->diff, &pblk->diff, offsetof(struct gaba_phantom_block_s, sd.max));
	_memcpy_blk_aa(&blk->diff, &pblk->diff, offsetof(struct gaba_phantom_block_s, dir));

	/* fill max vector with zero */
	// _store(&blk->sd.max, _zero());
	// _store(&blk->sd.max, _set(-128));

	/* copy remaining */
	blk->dir = pblk->dir;
	blk->offset = pblk->offset;

	/* calc ridx */
	v2i32_t ridx = _sub_v2i32(
		_load_v2i32(&this->w.r.alen),
		_load_v2i32(&prev_tail->apos));
	_print_v2i32(ridx);

	/* check if init fetch is needed */
	if(prev_tail->psum >= 0) {
		/* store index on the current section */
		_store_v2i32(&blk->aridx, ridx);
		
		/* copy char vectors from prev_tail */
		_store(&blk->ch, _load(&pblk->ch));

		return((struct gaba_joint_block_s){
			.blk = (struct gaba_block_s *)(blk + 1),
			.p = 0,
			.stat = CONT
		});
	} else {
		/* init fetch */
		return(fill_init_fetch(this, blk, prev_tail, ridx));
	}
}

/**
 * @fn fill_create_tail
 * @brief create joint_tail at the end of the blocks
 */
static _force_inline
struct gaba_joint_tail_s *fill_create_tail(
	struct gaba_dp_context_s *this,
	struct gaba_joint_tail_s const *prev_tail,
	struct gaba_block_s *blk,
	int64_t p,
	int32_t stat)
{
	debug("create tail, p(%lld)", p);

	/* create joint_tail */
	struct gaba_joint_tail_s *tail = (struct gaba_joint_tail_s *)blk;
	this->stack_top = (void *)(tail + 1);	/* write back stack_top */
	(blk - 1)->md = _last_block(prev_tail)->md;	/* copy middle delta pointer */

	/* update p */
	int64_t prev_psum = prev_tail->psum;
	int32_t np = (prev_psum < 0) ? MAX2(p + prev_psum, 0) : p;

	/* save misc to joint_tail */
	tail->psum = p + prev_tail->psum;
	tail->p = np;
	debug("p(%lld), prev_tail->psum(%lld), prev_tail->p(%d), tail->psum(%lld), tail->p(%d)",
		p, prev_tail->psum, prev_tail->p, tail->psum, tail->p);
	tail->ssum = prev_tail->ssum + 1;
	tail->tail = prev_tail;					/* to treat tail chain as linked list */
	tail->rem_len = 0;

	/* search max section */
	v32i16_t sd = _cvt_v32i8_v32i16(_load(&(blk - 1)->sd.max));
	v32i16_t md = _load_v32i16(_last_block(prev_tail)->md);
	_print_v32i16(sd);
	_print_v32i16(md);

	/* extract max */
	md = _add_v32i16(md, sd);
	int16_t max = _hmax_v32i16(md);
	_print_v32i16(md);

	/* store */
	// tail->mask_max.mask = mask_max;
	tail->max = max + (blk - 1)->offset;

	debug("offset(%lld)", (blk - 1)->offset);
	debug("max(%d)", _hmax_v32i16(md));
	// debug("mask_max(%u)", tail->mask_max.all);

	/* store section lengths */
	v2i32_t const z = _zero_v2i32();
	v2i32_t ridx = _load_v2i32(&(blk - 1)->aridx);
	v2i32_t len = _load_v2i32(&this->w.r.alen);
	_print_v2i32(ridx);
	_print_v2i32(len);
	_print_v2i32(_sel_v2i32(
		_eq_v2i32(ridx, z),
		z, _sub_v2i32(len, ridx)));
	_store_v2i32(&tail->apos, _sel_v2i32(_eq_v2i32(ridx, z),
		z, _sub_v2i32(len, ridx)));
	_store_v2i32(&tail->alen, len);

	/* store section ids */
	v2i32_t id = _load_v2i32(&this->w.r.aid);
	_store_v2i32(&tail->aid, id);

	/* store status */
	tail->stat = stat | _mask_v2i32(_eq_v2i32(ridx, z));
	return(tail);
}

/**
 * @macro _match
 * @brief alias to sequence matcher macro
 */
#ifndef _match
#  if BIT == 2
#    define _match		_or 		/* for 2bit encoded */
#  else
#    define _match		_and		/* for 4bit encoded */
#  endif
#endif /* _match */


/**
 * @macro _fill_load_contexts
 * @brief load vectors onto registers
 */
#if MODEL == LINEAR
#define _fill_load_contexts(_blk) \
	debug("blk(%p)", (_blk)); \
	/* load sequence buffer offset */ \
	uint8_t const *aptr = _rd_bufa(this, 0, BW); \
	uint8_t const *bptr = _rd_bufb(this, 0, BW); \
	/* load mask pointer */ \
	union gaba_mask_pair_u *mask_ptr = (_blk)->mask; \
	/* load vector registers */ \
	vec_t register dh = _load(_pv(((_blk) - 1)->diff.dh)); \
	vec_t register dv = _load(_pv(((_blk) - 1)->diff.dv)); \
	_print(_add(dh, _load_ofsh(this->scv))); \
	_print(_add(dv, _load_ofsv(this->scv))); \
	/* load delta vectors */ \
	vec_t register delta = _load(_pv(((_blk) - 1)->sd.delta)); \
	vec_t register max = _load(_pv(((_blk) - 1)->sd.max)); \
	_print(max); \
	_print_v32i16(_add_v32i16(_cvt_v32i8_v32i16(delta), _load_v32i16(_last_block(&this->tail)->md))); \
	/* load direction determiner */ \
	union gaba_dir_u dir = ((_blk) - 1)->dir; \
	/* load large offset */ \
	int64_t offset = ((_blk) - 1)->offset; \
	debug("offset(%lld)", offset);
#else
#define _fill_load_contexts(_blk) \
	debug("blk(%p)", (_blk)); \
	/* load sequence buffer offset */ \
	uint8_t const *aptr = _rd_bufa(this, 0, BW); \
	uint8_t const *bptr = _rd_bufb(this, 0, BW); \
	/* load mask pointer */ \
	union gaba_mask_pair_u *mask_ptr = (_blk)->mask; \
	/* load vector registers */ \
	vec_t const mask = _set(0x07); \
	vec_t register dh = _load(_pv(((_blk) - 1)->diff.dh)); \
	vec_t register dv = _load(_pv(((_blk) - 1)->diff.dv)); \
	vec_t register de = _and(mask, dh); \
	vec_t register df = _and(mask, dv); \
	dh = _shr(_andn(mask, dh), 3); \
	dv = _shr(_andn(mask, dv), 3); \
	de = _add(dv, de); \
	df = _add(dh, df); \
	dh = _sub(_zero(), dh); \
	_print(_add(dh, _load_ofsh(this->scv))); \
	_print(_add(dv, _load_ofsv(this->scv))); \
	_print(_sub(_sub(de, dv), _load_adjh(this->scv))); \
	_print(_sub(_add(df, dh), _load_adjv(this->scv))); \
	/* load delta vectors */ \
	vec_t register delta = _load(_pv(((_blk) - 1)->sd.delta)); \
	vec_t register max = _load(_pv(((_blk) - 1)->sd.max)); \
	_print(max); \
	_print_v32i16(_add_v32i16(_cvt_v32i8_v32i16(delta), _load_v32i16(_last_block(&this->tail)->md))); \
	/* load direction determiner */ \
	union gaba_dir_u dir = ((_blk) - 1)->dir; \
	/* load large offset */ \
	int64_t offset = ((_blk) - 1)->offset; \
	debug("offset(%lld)", offset);
#endif

/**
 * @macro _fill_body
 * @brief update vectors
 */
#if MODEL == LINEAR
#define _fill_body() { \
	vec_t register t = _match(_loadu(aptr), _loadu(bptr)); \
	/*t = _shuf(_load_sc(this, sb), t);*/ \
	t = _shuf(_load_sb(this->scv), t); \
	_print(t); \
	t = _max(dh, t); \
	t = _max(dv, t); \
	mask_ptr->pair.h.mask = _mask(_eq(t, dv)); \
	mask_ptr->pair.v.mask = _mask(_eq(t, dh)); \
	debug("mask(%llx)", mask_ptr->all); \
	mask_ptr++; \
	vec_t _dv = _sub(t, dh); \
	dh = _sub(t, dv); \
	dv = _dv; \
	_print(_add(dh, _load_ofsh(this->scv))); \
	_print(_add(dv, _load_ofsv(this->scv))); \
}
#else
#define _fill_body() { \
	vec_t register t = _match(_loadu(aptr), _loadu(bptr)); \
	_print(_loadu(aptr)); \
	_print(_loadu(bptr)); \
	/*t = _shuf(_load_sc(this, sb), t);*/ \
	t = _shuf(_load_sb(this->scv), t); \
	_print(t); \
	t = _max(de, t); \
	t = _max(df, t); \
	mask_ptr->pair.h.mask = _mask(_eq(t, de)); \
	mask_ptr->pair.v.mask = _mask(_eq(t, df)); \
	debug("mask(%llx)", mask_ptr->all); \
	mask_ptr++; \
	/*df = _sub(_max(_add(df, _load_sc(this, adjv)), t), dv);*/ \
	df = _sub(_max(_add(df, _load_adjv(this->scv)), t), dv); \
	dv = _sub(dv, t); \
	/*de = _add(_max(_add(de, _load_sc(this, adjh)), t), dh);*/ \
	de = _add(_max(_add(de, _load_adjh(this->scv)), t), dh); \
	t = _add(dh, t); \
	dh = dv; dv = t; \
	_print(_add(dh, _load_ofsh(this->scv))); \
	_print(_add(dv, _load_ofsv(this->scv))); \
	_print(_sub(_sub(de, dv), _load_adjh(this->scv))); \
	_print(_sub(_add(df, dh), _load_adjv(this->scv))); \
}
#endif /* MODEL */

/**
 * @macro _fill_update_delta
 * @brief update small delta vector and max vector
 */
#define _fill_update_delta(_op, _vector, _offset, _sign) { \
	delta = _op(delta, _add(_vector, _offset)); \
	max = _max(max, delta); \
	_dir_update(dir, _vector, _sign); \
	_print_v32i16(_add_v32i16(_set_v32i16(offset), _add_v32i16(_cvt_v32i8_v32i16(delta), _load_v32i16(_last_block(&this->tail)->md)))); \
	_print_v32i16(_add_v32i16(_set_v32i16(offset), _add_v32i16(_cvt_v32i8_v32i16(max), _load_v32i16(_last_block(&this->tail)->md)))); \
}

/**
 * @macro _fill_right, _fill_down
 * @brief wrapper of _fill_body and _fill_update_delta
 */
#define _fill_right_update_ptr() { \
	aptr--;				/* increment sequence buffer pointer */ \
}
#define _fill_right_windback_ptr() { \
	aptr++; \
}
#if MODEL == LINEAR
#define _fill_right() { \
	dh = _bsl(dh, 1);	/* shift left dh */ \
	_fill_body();		/* update vectors */ \
	/*_fill_update_delta(_add, dh, _load_sc(this, ofsh), 1);*/ \
	_fill_update_delta(_add, dh, _load_ofsh(this->scv), 1); \
}
#else
#define _fill_right() { \
	dh = _bsl(dh, 1);	/* shift left dh */ \
	df = _bsl(df, 1);	/* shift left df */ \
	_fill_body();		/* update vectors */ \
	/*_fill_update_delta(_sub, dh, _load_sc(this, ofsh), -1);*/ \
	_fill_update_delta(_sub, dh, _load_ofsh(this->scv), -1); \
}
#endif /* MODEL */
#define _fill_down_update_ptr() { \
	bptr++;				/* increment sequence buffer pointer */ \
}
#define _fill_down_windback_ptr() { \
	bptr--; \
}
#if MODEL == LINEAR
#define _fill_down() { \
	dv = _bsr(dv, 1);	/* shift right dv */ \
	_fill_body();		/* update vectors */ \
	/*_fill_update_delta(_add, dv, _load_sc(this, ofsv), 1);*/ \
	_fill_update_delta(_add, dv, _load_ofsv(this->scv), 1); \
}
#else
#define _fill_down() { \
	dv = _bsr(dv, 1);	/* shift right dv */ \
	de = _bsr(de, 1);	/* shift right de */ \
	_fill_body();		/* update vectors */ \
	/*_fill_update_delta(_add, dv, _load_sc(this, ofsv), 1);*/ \
	_fill_update_delta(_add, dv, _load_ofsv(this->scv), 1); \
}
#endif /* MODEL */

/**
 * @macro _fill_update_offset
 * @brief update offset and max vector, reset the small delta
 */
#define _fill_update_offset() { \
	int8_t _cd = _ext(delta, BW/2); \
	/*int8_t _cd = (_ext(delta, BW/4) + _ext(delta, 3*BW/4))>>1;*/ \
	offset += _cd; \
	delta = _sub(delta, _set(_cd)); \
	max = _sub(max, _set(_cd)); \
	/*debug("_cd(%d), offset(%lld)", _cd, offset);*/ \
}

/**
 * @macro _fill_store_vectors
 * @brief store vectors at the end of the block
 */
#if MODEL == LINEAR
#define _fill_store_vectors(_blk) ({ \
	/* store diff vectors */ \
	_store(_pv((_blk)->diff.dh), dh); \
	_store(_pv((_blk)->diff.dv), dv); \
	_print(dh); \
	_print(dv); \
	/* store delta vectors */ \
	_store(_pv((_blk)->sd.delta), delta); \
	_store(_pv((_blk)->sd.max), max); \
	/* store direction array */ \
	(_blk)->dir = dir; \
	/* store large offset */ \
	(_blk)->offset = offset; \
	/* calc cnt */ \
	uint64_t acnt = _rd_bufa(this, 0, BW) - aptr; \
	uint64_t bcnt = bptr - _rd_bufb(this, 0, BW); \
	_seta_v2i32(bcnt, acnt); \
})
#else
#define _fill_store_vectors(_blk) ({ \
	/* store diff vectors */ \
	de = _sub(de, dv); \
	df = _add(df, dh); \
	dh = _sub(_zero(), dh); \
	_print(dh); \
	_print(dv); \
	_print(de); \
	_print(df); \
	dh = _shl(dh, 3); \
	dv = _shl(dv, 3); \
	_print(dh); \
	_print(dv); \
	_store(_pv((_blk)->diff.dh), _add(dh, de)); \
	_store(_pv((_blk)->diff.dv), _add(dv, df)); \
	_print(_add(dh, de)); \
	_print(_add(dv, df)); \
	/* store delta vectors */ \
	_store(_pv((_blk)->sd.delta), delta); \
	_store(_pv((_blk)->sd.max), max); \
	/* store direction array */ \
	(_blk)->dir = dir; \
	/* store large offset */ \
	(_blk)->offset = offset; \
	/* calc cnt */ \
	uint64_t acnt = _rd_bufa(this, 0, BW) - aptr; \
	uint64_t bcnt = bptr - _rd_bufb(this, 0, BW); \
	/*_print_v2i32(_seta_v2i32(bcnt, acnt));*/ \
	_seta_v2i32(bcnt, acnt); \
})
#endif

/**
 * @fn fill_test_xdrop
 * @brief returns negative if terminate-condition detected
 */
static _force_inline
int64_t fill_test_xdrop(
	struct gaba_dp_context_s const *this,
	struct gaba_block_s const *blk)
{
	return(this->tx - blk->sd.max[BW/2]);
}

/**
 * @fn fill_bulk_test_seq_bound
 * @brief returns negative if ij-bound (for the bulk fill) is invaded
 */
static _force_inline
int64_t fill_bulk_test_seq_bound(
	struct gaba_dp_context_s const *this,
	struct gaba_block_s const *blk)
{
	debug("test(%lld, %lld), len(%d, %d)",
		(int64_t)blk->aridx - BW,
		(int64_t)blk->bridx - BW,
		blk->aridx, blk->bridx);
	return(((int64_t)blk->aridx - BW)
		 | ((int64_t)blk->bridx - BW));
}

/**
 * @fn fill_cap_test_seq_bound
 * @brief returns negative if ij-bound (for the cap fill) is invaded
 */
#define _fill_cap_test_seq_bound_init(_blk) \
	uint8_t *alim = _rd_bufa(this, ((_blk) - 1)->aridx, BW); \
	uint8_t *blim = _rd_bufb(this, ((_blk) - 1)->bridx, BW);
#define _fill_cap_test_seq_bound() ( \
	((int64_t)aptr - (int64_t)alim) | ((int64_t)blim - (int64_t)bptr) \
)

/**
 * @fn fill_bulk_test_p_bound
 * @brief returns negative if p-bound (for the bulk fill) is invaded
 */
static _force_inline
int64_t fill_bulk_test_p_bound(
	struct gaba_dp_context_s const *this,
	int64_t p)
{
	return(this->w.r.plim - p - BW);
}
static _force_inline
int64_t fill_cap_test_p_bound(
	struct gaba_dp_context_s const *this,
	int64_t p)
{
	return(this->w.r.plim - p);
}

/**
 * @fn fill_bulk_block
 * @brief fill a block
 */
static _force_inline
void fill_bulk_block(
	struct gaba_dp_context_s *this,
	struct gaba_block_s *blk)
{
	/* fetch sequence */
	fill_bulk_fetch(this, blk);

	/* load vectors onto registers */
	debug("blk(%p)", blk);
	_fill_load_contexts(blk);
	/**
	 * @macro _fill_block
	 * @brief an element of unrolled fill-in loop
	 */
	#define _fill_block(_direction, _label, _jump_to) { \
		_dir_fetch(dir); \
		if(_unlikely(!_dir_is_##_direction(dir))) { \
			goto _linear_fill_##_jump_to; \
		} \
		_linear_fill_##_label: \
		_fill_##_direction##_update_ptr(); \
		_fill_##_direction(); \
		if(--i == 0) { break; } \
	}

	/* update diff vectors */
	int64_t i = BLK;
	while(1) {					/* 4x unrolled loop */
		_fill_block(down, d1, r1);
		_fill_block(right, r1, d2);
		_fill_block(down, d2, r2);
		_fill_block(right, r2, d1);
	}

	/* update seq offset */
	_fill_update_offset();

	/* store vectors */
	v2i32_t cnt = _fill_store_vectors(blk);

	/* update section */
	fill_update_section(this, blk, cnt);

	return;
}

/**
 * @fn fill_bulk_predetd_blocks
 * @brief fill <blk_cnt> contiguous blocks without ij-bound test
 */
static _force_inline
struct gaba_joint_block_s fill_bulk_predetd_blocks(
	struct gaba_dp_context_s *this,
	struct gaba_block_s *blk,
	uint64_t blk_cnt)
{
	int32_t stat = CONT;
	uint64_t bc = 0;
	for(bc = 0; bc < blk_cnt; bc++) {
		/* check xdrop termination */
		if(fill_test_xdrop(this, blk - 1) < 0) {
			stat = TERM; break;
		}

		/* bulk fill */
		debug("blk(%p)", blk);
		fill_bulk_block(this, blk++);
	}
	return((struct gaba_joint_block_s){
		.blk = blk,
		.p = (int64_t)bc * BLK,
		.stat = stat
	});
}

/**
 * @fn fill_bulk_seq_bounded
 * @brief fill blocks with ij-bound test
 */
static _force_inline
struct gaba_joint_block_s fill_bulk_seq_bounded(
	struct gaba_dp_context_s *this,
	struct gaba_block_s *blk)
{
	int32_t stat = CONT;

	/* init local coordinate */
	int64_t p = 0;

	/* bulk fill loop */
	while(1) {
		/* check termination */
		if((fill_test_xdrop(this, blk - 1)
		  | fill_bulk_test_seq_bound(this, blk - 1)) < 0) {
			break;
		}
		
		/* bulk fill */
		debug("blk(%p)", blk);
		fill_bulk_block(this, blk++);
		
		/* update p-coordinate */
		p += BLK;
	}
	if(fill_test_xdrop(this, blk - 1) < 0) { stat = TERM; }
	return((struct gaba_joint_block_s){
		.blk = blk,
		.p = p,
		.stat = stat 		/* < 0 if stat == TERM */
	});
}

/**
 * @fn fill_bulk_seq_p_bounded
 * @brief fill blocks with ij-bound and p-bound test
 */
static _force_inline
struct gaba_joint_block_s fill_bulk_seq_p_bounded(
	struct gaba_dp_context_s *this,
	struct gaba_block_s *blk)
{
	int32_t stat = CONT;

	/* init local coordinate */
	int64_t p = 0;

	/* bulk fill loop */
	while(1) {
		/* check termination */
		if((fill_test_xdrop(this, blk - 1)
		  | fill_bulk_test_seq_bound(this, blk - 1)
		  | fill_bulk_test_p_bound(this, p)) < 0) {
			break;
		}
		
		/* bulk fill */
		debug("blk(%p)", blk);
		fill_bulk_block(this, blk++);
		
		/* update p-coordinate */
		p += BLK;
	}
	if(fill_test_xdrop(this, blk - 1) < 0) { stat = TERM; }
	return((struct gaba_joint_block_s){
		.blk = blk,
		.p = p,
		.stat = stat 		/* < 0 if stat == TERM */
	});
}

/**
 * @fn fill_cap_seq_bounded
 * @brief fill blocks with cap test
 */
static _force_inline
struct gaba_joint_block_s fill_cap_seq_bounded(
	struct gaba_dp_context_s *this,
	struct gaba_block_s *blk)
{
	int32_t stat = CONT;
	int64_t p = 0;

	while(1) {
		/* check xdrop termination */
		if(fill_test_xdrop(this, blk - 1) < 0) {
			stat = TERM; goto _fill_cap_seq_bounded_finish;
		}
		/* fetch sequence */
		fill_cap_fetch(this, blk);

		/* vectors on registers inside this block */ {
			_fill_cap_test_seq_bound_init(blk);
			_fill_load_contexts(blk);

			/* update diff vectors */
			uint64_t i = 0;
			for(i = 0; i < BLK; i++) {
				/* determine direction */
				_dir_fetch(dir);

				if(_dir_is_right(dir)) {
					/* update sequence coordinate and then check term */
					_fill_right_update_ptr();
					
					if(_fill_cap_test_seq_bound() < 0) {
						_fill_right_windback_ptr();
						_dir_windback(dir);
						break;
					}

					/* update band */
					_fill_right();
				} else {
					/* update sequence coordinate and then check term */
					_fill_down_update_ptr();
					
					if(_fill_cap_test_seq_bound() < 0) {
						_fill_down_windback_ptr();
						_dir_windback(dir);
						break;
					}

					/* update band */
					_fill_down();
				}
			}
			/* adjust dir remainder */
			_dir_adjust_remainder(dir, i);

			/* update seq offset */
			_fill_update_offset();
			
			/* store mask and vectors */
			v2i32_t cnt = _fill_store_vectors(blk);

			/* update section */
			fill_update_section(this, blk, cnt);

			/* update block pointer and p-coordinate */
			blk += (i != 0); p += i;

			/* break if not filled full length */
			if(i != BLK) { stat = UPDATE; break; }
		}
	}

	debug("blk(%p), p(%lld), stat(%x)", blk, p, stat);

_fill_cap_seq_bounded_finish:;
	return((struct gaba_joint_block_s){
		.blk = blk,
		.p = p,
		.stat = stat
	});
}

/**
 * @fn fill_cap_seq_p_bounded
 * @brief fill blocks with cap test
 */
static _force_inline
struct gaba_joint_block_s fill_cap_seq_p_bounded(
	struct gaba_dp_context_s *this,
	struct gaba_block_s *blk)
{
	int32_t stat = CONT;
	int64_t p = 0;

	while(1) {
		/* check xdrop termination */
		if(fill_test_xdrop(this, blk - 1) < 0) {
			stat = TERM; goto _fill_cap_seq_bounded_finish;
		}
		/* fetch sequence */
		fill_cap_fetch(this, blk);

		/* vectors on registers inside this block */ {
			_fill_cap_test_seq_bound_init(blk);
			_fill_load_contexts(blk);

			/* update diff vectors */
			uint64_t i = 0;
			for(i = 0; i < BLK; i++) {
				/* determine direction */
				_dir_fetch(dir);

				if(_dir_is_right(dir)) {
					/* update sequence coordinate and then check term */
					_fill_right_update_ptr();
					
					if((_fill_cap_test_seq_bound()
					  | fill_cap_test_p_bound(this, ++p)) < 0) {
						_fill_right_windback_ptr();
						_dir_windback(dir);
						break;
					}

					/* update band */
					_fill_right();
				} else {
					/* update sequence coordinate and then check term */
					_fill_down_update_ptr();
					
					if((_fill_cap_test_seq_bound()
					  | fill_cap_test_p_bound(this, ++p)) < 0) {
						_fill_down_windback_ptr();
						_dir_windback(dir);
						break;
					}

					/* update band */
					_fill_down();
				}
			}
			/* adjust dir remainder */
			_dir_adjust_remainder(dir, i);
			
			/* update seq offset */
			_fill_update_offset();
			
			/* store mask and vectors */
			v2i32_t cnt = _fill_store_vectors(blk);

			/* update section */
			fill_update_section(this, blk, cnt);

			/* update block pointer and p-coordinate */
			blk += (i != 0);

			/* break if not filled full length */
			if(i != BLK) { stat = UPDATE; break; }
		}
	}

	debug("blk(%p), p(%lld), stat(%x)", blk, p, stat);

_fill_cap_seq_bounded_finish:;
	return((struct gaba_joint_block_s){
		.blk = blk,
		.p = p,
		.stat = stat
	});
}

/**
 * @fn calc_max_bulk_blocks_mem
 * @brief calculate maximum number of blocks (limited by stack size)
 */
static _force_inline
uint64_t calc_max_bulk_blocks_mem(
	struct gaba_dp_context_s const *this)
{
	uint64_t const rem = sizeof(struct gaba_joint_tail_s)
					   + 3 * sizeof(struct gaba_block_s);
	uint64_t mem_size = this->stack_end - this->stack_top;
	return((mem_size - rem) / sizeof(struct gaba_block_s) / BLK);
}

/**
 * @fn calc_max_bulk_blocks_seq
 * @brief calculate maximum number of blocks (limited by seq bounds)
 */
static _force_inline
uint64_t calc_max_bulk_blocks_seq_blk(
	struct gaba_dp_context_s const *this,
	struct gaba_block_s const *blk)
{
	uint64_t max_bulk_p = MIN2(
		(blk - 1)->aridx,
		(blk - 1)->bridx);
	return(max_bulk_p / BLK);
}
static _force_inline
uint64_t calc_max_bulk_blocks_seq_tail(
	struct gaba_dp_context_s const *this,
	struct gaba_joint_tail_s const *tail)
{
	uint64_t max_bulk_p = MIN2(
		this->w.r.alen - tail->apos,
		this->w.r.blen - tail->bpos);
	return(max_bulk_p / BLK);
}

/**
 * @fn calc_max_bulk_blocks_seq_p
 * @brief calculate maximum number of blocks (limited by seq bounds)
 */
static _force_inline
uint64_t calc_max_bulk_blocks_seq_p_blk(
	struct gaba_dp_context_s const *this,
	struct gaba_block_s const *blk,
	int64_t psum)
{
	uint64_t max_bulk_p = MIN3(
		(blk - 1)->aridx,
		(blk - 1)->bridx,
		this->w.r.plim - psum);
	return(max_bulk_p / BLK);
}
static _force_inline
uint64_t calc_max_bulk_blocks_seq_p_tail(
	struct gaba_dp_context_s const *this,
	struct gaba_joint_tail_s const *tail,
	int64_t psum)
{
	uint64_t max_bulk_p = MIN3(
		this->w.r.alen - tail->apos,
		this->w.r.blen - tail->bpos,
		this->w.r.plim - psum);
	return(max_bulk_p / BLK);
}

/**
 * @fn fill_mem_bounded
 * @brief fill <blk_cnt> contiguous blocks without seq bound tests, adding head and tail
 */
static _force_inline
struct gaba_joint_tail_s *fill_mem_bounded(
	struct gaba_dp_context_s *this,
	struct gaba_joint_tail_s const *prev_tail,
	uint64_t blk_cnt)
{
	struct gaba_joint_block_s h = fill_create_phantom_block(this, prev_tail);
	struct gaba_joint_block_s b = fill_bulk_predetd_blocks(this, h.blk, blk_cnt);
	return(fill_create_tail(this, prev_tail, b.blk, h.p + b.p, b.stat));
}

/**
 * @fn fill_seq_bounded
 * @brief fill blocks with seq bound tests, adding head and tail
 */
static _force_inline
struct gaba_joint_tail_s *fill_seq_bounded(
	struct gaba_dp_context_s *this,
	struct gaba_joint_tail_s const *prev_tail)
{
	struct gaba_joint_block_s stat = fill_create_phantom_block(this, prev_tail);
	int64_t psum = stat.p;

	/* check if term detected in init fetch */
	if(stat.stat != CONT) {
		goto _fill_seq_bounded_finish;
	}

	/* calculate block size */
	uint64_t seq_bulk_blocks = calc_max_bulk_blocks_seq_blk(this, stat.blk);
	while(seq_bulk_blocks > MIN_BULK_BLOCKS) {
		/* bulk fill without ij-bound test */
		psum += (stat = fill_bulk_predetd_blocks(this, stat.blk, seq_bulk_blocks)).p;
		if(stat.stat != CONT) {
			goto _fill_seq_bounded_finish;	/* skip cap */
		}
		seq_bulk_blocks = calc_max_bulk_blocks_seq_blk(this, stat.blk);
	}

	/* bulk fill with ij-bound test */
	psum += (stat = fill_bulk_seq_bounded(this, stat.blk)).p;

	if(stat.stat != CONT) {
		goto _fill_seq_bounded_finish;	/* skip cap */
	}

	/* cap fill (without p-bound test) */
	psum += (stat = fill_cap_seq_bounded(this, stat.blk)).p;

_fill_seq_bounded_finish:;
	return(fill_create_tail(this, prev_tail, stat.blk, psum, stat.stat));
}

/**
 * @fn fill_seq_p_bounded
 * @brief fill blocks with seq bound tests, adding head and tail
 */
static _force_inline
struct gaba_joint_tail_s *fill_seq_p_bounded(
	struct gaba_dp_context_s *this,
	struct gaba_joint_tail_s const *prev_tail)
{
	struct gaba_joint_block_s stat = fill_create_phantom_block(this, prev_tail);
	int64_t psum = stat.p;

	/* check if term detected in init fetch */
	if(stat.stat != CONT) {
		goto _fill_seq_p_bounded_finish;
	}

	/* calculate block size */
	uint64_t seq_bulk_blocks = calc_max_bulk_blocks_seq_p_blk(this, stat.blk, psum);
	while(seq_bulk_blocks > MIN_BULK_BLOCKS) {
		/* bulk fill without ij-bound test */
		psum += (stat = fill_bulk_predetd_blocks(this, stat.blk, seq_bulk_blocks)).p;
		if(stat.stat != CONT) {
			goto _fill_seq_p_bounded_finish;	/* skip cap */
		}
		seq_bulk_blocks = calc_max_bulk_blocks_seq_p_blk(this, stat.blk, psum);
	}

	/* bulk fill with ij-bound test */
	psum += (stat = fill_bulk_seq_p_bounded(this, stat.blk)).p;

	if(stat.stat != CONT) {
		goto _fill_seq_p_bounded_finish;	/* skip cap */
	}

	/* cap fill (without p-bound test) */
	psum += (stat = fill_cap_seq_p_bounded(this, stat.blk)).p;

_fill_seq_p_bounded_finish:;
	return(fill_create_tail(this, prev_tail, stat.blk, psum, stat.stat));
}

/**
 * @fn fill_section_seq_bounded
 * @brief fill dp matrix inside section pairs
 */
static _force_inline
struct gaba_joint_tail_s *fill_section_seq_bounded(
	struct gaba_dp_context_s *this,
	struct gaba_joint_tail_s const *prev_tail,
	struct gaba_section_s const *a,
	struct gaba_section_s const *b)
{
	/* init section and restore sequence reader buffer */
	fill_load_section(this, a, b, INT64_MAX);

	/* init tail pointer */
	struct gaba_joint_tail_s *tail = _tail(prev_tail);

	/* calculate block sizes */
	uint64_t mem_bulk_blocks = calc_max_bulk_blocks_mem(this);
	uint64_t seq_bulk_blocks = calc_max_bulk_blocks_seq_tail(this, tail);

	/* extra large bulk fill (with stack allocation) */
	while(_unlikely(mem_bulk_blocks < seq_bulk_blocks)) {
		if(mem_bulk_blocks > MIN_BULK_BLOCKS) {
			if((tail = fill_mem_bounded(this, tail, mem_bulk_blocks))->stat != CONT) {
				return(tail);
			}

			/* fill-in area has changed */
			seq_bulk_blocks = calc_max_bulk_blocks_seq_tail(this, tail);
		}

		/* malloc the next stack and set pointers */
		gaba_dp_add_stack(this, 0);

		/* stack size has changed */
		mem_bulk_blocks = calc_max_bulk_blocks_mem(this);
	}

	debug("v(%p), psum(%lld), p(%d)", _last_block(tail)->md, tail->psum, tail->p);

	/* bulk fill with seq bound check */
	return(fill_seq_bounded(this, tail));
}

/**
 * @fn fill_section_seq_p_bounded
 * @brief fill dp matrix inside section pairs
 */
static _force_inline
struct gaba_joint_tail_s *fill_section_seq_p_bounded(
	struct gaba_dp_context_s *this,
	struct gaba_joint_tail_s const *prev_tail,
	struct gaba_section_s const *a,
	struct gaba_section_s const *b,
	int64_t plim)
{
	/* init section and restore sequence reader buffer */
	fill_load_section(this, a, b, plim);

	/* init tail pointer */
	struct gaba_joint_tail_s *tail = _tail(prev_tail);

	/* calculate block sizes */
	uint64_t mem_bulk_blocks = calc_max_bulk_blocks_mem(this);
	uint64_t seq_bulk_blocks = calc_max_bulk_blocks_seq_p_tail(this, tail, 0);

	/* extra large bulk fill (with stack allocation) */
	while(_unlikely(mem_bulk_blocks < seq_bulk_blocks)) {
		if(mem_bulk_blocks > MIN_BULK_BLOCKS) {
			if((tail = fill_mem_bounded(this, tail, mem_bulk_blocks))->stat != CONT) {
				return(tail);
			}

			/* fill-in area has changed */
			seq_bulk_blocks = calc_max_bulk_blocks_seq_p_tail(this, tail, 0);
		}

		/* malloc the next stack and set pointers */
		gaba_dp_add_stack(this, 0);

		/* stack size has changed */
		mem_bulk_blocks = calc_max_bulk_blocks_mem(this);
	}

	debug("v(%p), psum(%lld), p(%d)", _last_block(tail)->md, tail->psum, tail->p);

	/* bulk fill with seq bound check */
	return(fill_seq_p_bounded(this, tail));
}

/**
 * @fn gaba_dp_fill_root
 *
 * @brief build_root API
 */
struct gaba_fill_s *gaba_dp_fill_root(
	struct gaba_dp_context_s *this,
	struct gaba_section_s const *a,
	uint32_t apos,
	struct gaba_section_s const *b,
	uint32_t bpos)
{
	/* store section info */
	this->tail.apos = apos;
	this->tail.bpos = bpos;
	return(_fill(fill_section_seq_bounded(this, &this->tail, a, b)));
}

/**
 * @fn gaba_dp_fill
 *
 * @brief fill API
 */
struct gaba_fill_s *gaba_dp_fill(
	struct gaba_dp_context_s *this,
	struct gaba_fill_s const *prev_sec,
	struct gaba_section_s const *a,
	struct gaba_section_s const *b)
{
	struct gaba_joint_tail_s const *tail = _tail(prev_sec);

	debug("%s", dump(tail, sizeof(struct gaba_joint_tail_s)));

	#if 0
	/* check if the tail is merge_tail */
	if(tail->v == NULL) {
		/* tail is merge_tail */
		if(tail->rem_len > 0) {
			#if 0
			/* aとbをclipする */
			ca = clip_to_bw(a);
			cb = clip_to_bw(b);

			/* バンドに対応するseqが揃うまでfillを続ける */
			new_tail = gaba_dp_smalloc(sizeof(struct gaba_merge_tail_s));
			for(i = 0; i < tail->num_tail; i++) {
				new_tail->tail[i] = fill_section_seq_bounded(
					this, tail->tail[i], ca, cb);
			}

			/* rem_lenを更新し、0になったらpを揃えるモードに移る */
			if(tail->rem_len == 0) {
				tail->prem = ...;
			}
			#endif
		} else {
			#if 0
			/* pが揃うまでfillを続ける */
			new_tail = gaba_dp_smalloc(sizeof(struct gaba_merge_tail_s));
			for(i = 0; i < tail->num_tail; i++) {
				if(tail->prem[i] != tail->p) {
					new_tail->tail[i] = fill_section_seq_p_bounded(
						this, tail->tail[i], a, b, tail->prem[i]);
				} else {
					new_tail->tail[i] = tail->tail[i];
				}
			}

			/* pが揃ったかチェック */
			if(p_is_aligned) {
				return(merge(this, tail));
			}
			#endif
		}
		#if 0
		/* merge-tail内のsectionとmaxの情報を更新する */
		tail->max = calc_max();
		tail->p = ...;
		#endif
	} else {
		/* tail is not merge_tail */
		return(_fill(fill_section_seq_bounded(this, tail, a, b)));
	}
	#endif

	return(_fill(fill_section_seq_bounded(this, tail, a, b)));

	/* never reach here */
	return(NULL);
}

/**
 * @fn trace_load_max_mask
 */
struct trace_max_mask_s {
	vec_t max;
	int64_t offset;
	uint32_t mask_max;
};
static _force_inline
struct trace_max_mask_s trace_load_max_mask(
	struct gaba_dp_context_s *this,
	struct gaba_joint_tail_s const *tail)
{
	struct gaba_block_s *blk = _last_block(tail);

	debug("p(%d), psum(%lld), ssum(%d), offset(%lld)",
		tail->p, tail->psum, tail->ssum, blk->offset);

	/* load max vector, create mask */
	vec_t max = _load(&blk->sd.max);
	int64_t offset = blk->offset;
	uint32_t mask_max = ((union gaba_mask_u){
		.mask = _mask_v32i16(_eq_v32i16(
			_set_v32i16(tail->max - offset),
			_add_v32i16(_load_v32i16(_last_block(tail)->md), _cvt_v32i8_v32i16(max))))
	}).all;
	debug("mask_max(%x)", mask_max);
	_print_v32i16(_set_v32i16(tail->max - offset));
	_print_v32i16(_add_v32i16(_load_v32i16(_last_block(tail)->md), _cvt_v32i8_v32i16(max)));

	return((struct trace_max_mask_s){
		.max = max,
		.offset = offset,
		.mask_max = mask_max
	});
}

/**
 * @fn trace_detect_max_block
 */
struct trace_max_block_s {
	vec_t max;
	struct gaba_block_s *blk;
	int32_t p;
	uint32_t mask_max;
};
static _force_inline
struct trace_max_block_s trace_detect_max_block(
	struct gaba_dp_context_s *this,
	struct gaba_joint_tail_s const *tail,
	int64_t offset,
	uint32_t mask_max,
	vec_t max)
{
	/* scan blocks backward */
	struct gaba_block_s *blk = _last_block(tail);
	int32_t p = tail->p;

	/* b must be sined integer, in order to detect negative index. */
	for(int32_t b = (p - 1)>>BLK_BASE; b >= 0; b--, blk--) {

		/* load the previous max vector and offset */
		vec_t prev_max = _load(&(blk - 1)->sd.max);
		int64_t prev_offset = (blk - 1)->offset;

		/* adjust offset */
		max = _add(max, _set(offset - prev_offset));

		/* take mask */
		uint32_t prev_mask_max = mask_max & ((union gaba_mask_u){
			.mask = _mask(_eq(prev_max, max))
		}).all;

		debug("scan block: b(%d), offset(%lld), mask_max(%u), prev_mask_max(%u)",
			b, offset, mask_max, prev_mask_max);

		if(prev_mask_max == 0) {
			p = b * BLK;
			break;
		}

		/* windback a block */
		max = prev_max;
		offset = prev_offset;
		mask_max = prev_mask_max;
	}

	debug("block found: blk(%p), p(%d), mask_max(%u)", blk, p, mask_max);
	return((struct trace_max_block_s){
		.max = max,
		.blk = blk,
		.p = p,
		.mask_max = mask_max
	});
}

/**
 * @fn trace_refill_block
 */
static _force_inline
void trace_refill_block(
	struct gaba_dp_context_s *this,
	union gaba_mask_u *mask_max_ptr,
	int64_t len,
	struct gaba_block_s *blk,
	vec_t compd_max)
{
	/* fetch from existing blocks */
	fill_restore_fetch(this, blk);

	/* vectors on registers inside this block */ {
		#define _fill_block_leaf(_mask_ptr) { \
			_dir_fetch(dir); \
			if(_dir_is_right(dir)) { \
				_fill_right_update_ptr(); \
				_fill_right(); \
			} else { \
				_fill_down_update_ptr(); \
				_fill_down(); \
			} \
			(_mask_ptr)++->mask = _mask(_eq(max, delta)); \
			debug("mask(%x)", ((union gaba_mask_u){ .mask = _mask(_eq(max, delta)) }).all); \
		}

		/* load contexts and overwrite max vector */
		_fill_load_contexts(blk);
		max = compd_max;		/* overwrite with compensated max vector */
		(void)offset;			/* to avoid warning */

		for(int64_t i = 0; i < len; i++) {
			_fill_block_leaf(mask_max_ptr);
		}
	}
	return;
}

/**
 * @fn trace_detect_max_pos
 */
struct trace_max_pos_s {
	int32_t p;
	int32_t q;
};
static _force_inline
struct trace_max_pos_s trace_detect_max_pos(
	struct gaba_dp_context_s *this,
	union gaba_mask_u *mask_max_ptr,
	int64_t len,
	uint32_t mask_max)
{
	for(int64_t i = 0; i < len; i++) {
		uint32_t mask_update = mask_max_ptr[i].all & mask_max;
		if(mask_update != 0) {
			debug("i(%lld), p(%lld), q(%d)",
				i, this->w.l.p + i, tzcnt(mask_update) - BW/2);

			return((struct trace_max_pos_s){
				.p = i,
				.q = tzcnt(mask_update)
			});
		}
	}

	/* not found in the block (never reaches here) */
	return((struct trace_max_pos_s){
		.p = 0,
		.q = 0
	});
}

/**
 * @fn trace_save_coordinates
 */
static _force_inline
void trace_save_coordinates(
	struct gaba_dp_context_s *this,
	struct gaba_joint_tail_s const *tail,
	struct gaba_block_s const *blk,
	int32_t p,
	int32_t q)
{
	/* store tail and block pointer */
	this->w.l.tail = tail;
	this->w.l.blk = blk;
	this->w.l.atail = tail;
	this->w.l.btail = tail;

	#if 0
	/* copy ids and lengths */
	v2i32_t len = _load_v2i32(&tail->alen);
	v2i32_t id = _load_v2i32(&tail->aid);
	_store_v2i32(&this->w.l.alen, len);
	_store_v2i32(&this->w.l.aid, id);
	#else
	/* init ids and lengths with invalid */
	_store_v2i32(&this->w.l.alen, _zero_v2i32());
	_store_v2i32(&this->w.l.aid, _set_v2i32(-1));
	#endif

	/* calc ridx */
	int64_t mask_idx = p & (BLK - 1);
	int64_t filled_count = mask_idx + 1;
	int32_t bcnt = _dir_bcnt(_dir_load(blk, mask_idx));
	int32_t acnt = filled_count - bcnt;
	v2i32_t ridx = _add_v2i32(
		_load_v2i32(&(blk - 1)->aridx),
		_seta_v2i32((BLK - 1 - q) - bcnt, q - acnt));

	debug("acnt(%d), bcnt(%d)", acnt, bcnt);
	_print_v2i32(_load_v2i32(&(blk - 1)->aridx));
	_print_v2i32(_seta_v2i32((BLK - 1 - q) - bcnt, q - acnt));
	_print_v2i32(ridx);

	/* convert to idx */
	v2i32_t len = _load_v2i32(&this->w.l.alen);
	_store_v2i32(&this->w.l.aidx, _sub_v2i32(len, ridx));
	_store_v2i32(&this->w.l.asidx, _sub_v2i32(len, ridx));

	/* i \in [0 .. BLK) */
	/* adjust global coordinates with local coordinate */
	this->w.l.psum = tail->psum - tail->p + p;
	this->w.l.p = p;
	this->w.l.q = q;
	return;
}

/**
 * @fn trace_search_max
 */
static _force_inline
void trace_search_max(
	struct gaba_dp_context_s *this,
	struct gaba_joint_tail_s const *tail)
{
	/* load max vector and create mask */
	struct trace_max_mask_s m = trace_load_max_mask(this, tail);

	/* search block */
	struct trace_max_block_s b = trace_detect_max_block(
		this, tail, m.offset, m.mask_max, m.max);

	/* refill detected block */
	int64_t len = MIN2(tail->p - b.p, BLK);
	union gaba_mask_u mask_max_arr[BLK];
	trace_refill_block(this, mask_max_arr, len, b.blk, b.max);

	/* detect pos */
	struct trace_max_pos_s l = trace_detect_max_pos(
		this, mask_max_arr, len, b.mask_max);

	/* calculate ij-coordinates */
	trace_save_coordinates(this, tail, b.blk, b.p + l.p, l.q);
	return;
}

/**
 * @fn trace_load_section_a, trace_load_section_b
 */
static _force_inline
void trace_load_section_a(
	struct gaba_dp_context_s *this)
{
	debug("load section a, idx(%d), len(%d)", this->w.l.aidx, this->w.l.atail->alen);

	/* load tail pointer (must be inited with leaf tail) */
	struct gaba_joint_tail_s const *tail = this->w.l.atail;
	int32_t len = tail->alen;
	int32_t idx = this->w.l.aidx + len;

	debug("adjust len(%d), idx(%d)", len, idx);
	while(idx <= 0) {
		for(tail = tail->tail; tail->apos != 0; tail = tail->tail) {}
		idx += (len = tail->alen);
		debug("adjust again len(%d), idx(%d)", len, idx);
	}

	/* reload finished, store section info */
	this->w.l.atail = tail;
	this->w.l.alen = len;
	this->w.l.aid = tail->aid;

	this->w.l.aidx = idx;
	this->w.l.asidx = idx;
	return;
}
static _force_inline
void trace_load_section_b(
	struct gaba_dp_context_s *this)
{
	debug("load section b, idx(%d), len(%d)", this->w.l.bidx, this->w.l.btail->blen);

	/* reload needed, load tail pointer (must be inited with leaf tail) */
	struct gaba_joint_tail_s const *tail = this->w.l.btail;
	int32_t len = tail->blen;
	int32_t idx = this->w.l.bidx + len;

	debug("adjust len(%d), idx(%d)", len, idx);
	while(idx <= 0) {
		for(tail = tail->tail; tail->bpos != 0; tail = tail->tail) {}
		idx += (len = tail->blen);
		debug("adjust again len(%d), idx(%d)", len, idx);
	}

	/* reload finished, store section info */
	this->w.l.btail = tail;
	this->w.l.blen = len;
	this->w.l.bid = tail->bid;

	this->w.l.bidx = idx;
	this->w.l.bsidx = idx;
	return;
}

/**
 * @macro _trace_load_context, _trace_forward_load_context
 */
#define _trace_load_context(t) \
	v2i32_t idx = _load_v2i32(&(t)->w.l.aidx); \
	struct gaba_block_s const *blk = (t)->w.l.blk; \
	int64_t p = (t)->w.l.p; \
	int64_t q = (t)->w.l.q; \
	int64_t diff_q = 0; \
	int64_t prev_c = 0;
#define _trace_forward_load_context(t) \
	_trace_load_context(t); \
	uint32_t *path = (t)->w.l.fw_path; \
	int64_t rem = (t)->w.l.fw_rem;
#define _trace_reverse_load_context(t) \
	_trace_load_context(t); \
	uint32_t *path = (t)->w.l.rv_path; \
	int64_t rem = (t)->w.l.rv_rem; \
	/* reverse q-coordinate in the reverse trace */ \
	debug("reverse q-coordinate q(%lld), rq(%lld)", q, (BW - 1) - q); \
	q = (BW - 1) - q;

/**
 * @macro _trace_load_block
 */
#define _trace_load_block(_local_idx) \
	union gaba_dir_u dir = _dir_load(blk, (_local_idx)); \
	union gaba_mask_pair_u const *mask_ptr = &blk->mask[(_local_idx)]; \
	uint64_t path_array = 0; \
	/* working registers need not be initialized */ \
	v2i32_t next_idx; \
	int64_t next_diff_q; \
	uint64_t next_path_array, next_prev_c;

/**
 * @macro _trace_forward_determine_direction
 */
#define _trace_forward_determine_direction(_mask) ({ \
	uint64_t _mask_h = (_mask)>>q; \
	uint64_t _mask_v = _mask_h>>BW; \
	int64_t _is_down = (0x01 & _mask_v) | prev_c; \
	next_path_array = (path_array<<1) | _is_down; \
	next_diff_q = _dir_is_down(dir) - _is_down; \
	next_prev_c = 0x01 & ~(prev_c | _mask_h | _mask_v); \
	debug("c(%lld), prev_c(%llu), p(%lld), psum(%lld), q(%lld), diff_q(%lld), down(%d), mask(%llx), path_array(%llx)", \
		_is_down, prev_c, p, this->w.l.psum - (this->w.l.p - p), q, next_diff_q, _dir_is_down(dir), _mask, path_array); \
	_is_down; \
})
#define _trace_reverse_determine_direction(_mask) ({ \
	uint64_t _mask_h = (_mask)<<q; \
	uint64_t _mask_v = _mask_h>>BW; \
	int64_t _is_right = (0x80000000 & _mask_h) | prev_c; \
	next_path_array = (path_array>>1) | _is_right; \
	next_diff_q = (int64_t)!_is_right - _dir_is_down(dir); \
	next_prev_c = 0x80000000 & ~(prev_c | _mask_h | _mask_v); \
	debug("c(%lld), prev_c(%llu), p(%lld), psum(%lld), q(%lld), diff_q(%lld), down(%d), mask(%llx), path_array(%llx)", \
		_is_right>>31, prev_c>>31, p, this->w.l.psum - (this->w.l.p - p), q, next_diff_q, _dir_is_down(dir), _mask, path_array); \
	_is_right; \
})

/**
 * @macro _trace_update_index
 */
#define _trace_update_index() { \
	q += next_diff_q; \
	diff_q = next_diff_q; \
	path_array = next_path_array; \
	prev_c = next_prev_c; \
	mask_ptr--; \
	_dir_windback(dir); \
}

/**
 * @macro _trace_forward_bulk_body
 */
#define _trace_forward_bulk_body() { \
	_trace_forward_determine_direction(mask_ptr->all); \
	_trace_update_index(); \
}
#define _trace_reverse_bulk_body() { \
	_trace_reverse_determine_direction(mask_ptr->all); \
	_trace_update_index(); \
}

/**
 * @macro _trace_forward_cap_body
 */
#define _trace_forward_cap_body() { \
	int64_t _is_down = _trace_forward_determine_direction(mask_ptr->all); \
	/* calculate the next ridx */ \
	v2i32_t const idx_down = _seta_v2i32(1, 0); \
	v2i32_t const idx_right = _seta_v2i32(0, 1); \
	next_idx = _sub_v2i32(idx, _is_down ? idx_down : idx_right); \
	_print_v2i32(next_idx); \
}
#define _trace_reverse_cap_body() { \
	int64_t _is_right = _trace_reverse_determine_direction(mask_ptr->all); \
	/* calculate the next ridx */ \
	v2i32_t const idx_down = _seta_v2i32(1, 0); \
	v2i32_t const idx_right = _seta_v2i32(0, 1); \
	next_idx = _sub_v2i32(idx, _is_right ? idx_right : idx_down); \
	_print_v2i32(next_idx); \
}

/**
 * @macro _trace_cap_update_index
 */
#define _trace_cap_update_index() { \
	_trace_update_index(); \
 	idx = next_idx; \
	p--; \
}

/**
 * @macro _trace_test_cap_loop
 *
 * @brief returns non-zero if ridx >= len
 */
#define _trace_test_cap_loop() ( \
	/* next_ridx must be precalculated */ \
	_mask_v2i32(_lt_v2i32(next_idx, _zero_v2i32())) \
)
#define _trace_test_bulk_loop() ( \
	_mask_v2i32(_lt_v2i32(idx, _set_v2i32(BW))) \
)

/** 
 * @macro _trace_bulk_update_index
 */
#define _trace_bulk_update_index() { \
	int32_t bcnt = popcnt(path_array); \
	idx = _sub_v2i32(idx, _seta_v2i32(bcnt, BLK - bcnt)); \
	debug("bulk update index p(%lld)", p); \
	_print_v2i32(idx); \
	p -= BLK; \
}

/**
 * @macro _trace_forward_update_path, _trace_backward_update_path
 */
#define _trace_forward_compensate_remainder(_traced_count) ({ \
	/* decrement cnt when invasion of boundary on seq b with diagonal path occurred */ \
	int64_t _cnt = (_traced_count) - prev_c; \
	path_array >>= prev_c; \
	p += prev_c; \
	q -= (prev_c == 0) ? 0 : diff_q; \
	idx = _add_v2i32(idx, _seta_v2i32(0, prev_c)); \
	debug("cnt(%lld), traced_count(%lld), prev_c(%lld), path_array(%llx), p(%lld)", \
		_cnt, _traced_count, prev_c, path_array, p); \
	_cnt; \
})
#define _trace_reverse_compensate_remainder(_traced_count) ({ \
	/* decrement cnt when invasion of boundary on seq b with diagonal path occurred */ \
	prev_c >>= 31; \
	int64_t _cnt = (_traced_count) - prev_c; \
	path_array <<= prev_c; \
	p += prev_c; \
	q -= (prev_c == 0) ? 0 : diff_q; \
	idx = _add_v2i32(idx, _seta_v2i32(prev_c, 0)); \
	debug("cnt(%lld), traced_count(%lld), prev_c(%lld), path_array(%llx), p(%lld)", \
		_cnt, _traced_count, prev_c, path_array, p); \
	_cnt; \
})
#define _trace_forward_update_path(_traced_count) { \
	/* adjust path_array */ \
	path_array = path_array<<(BLK - (_traced_count)); \
	/* load the previous array */ \
	uint64_t prev_array = path[0]; \
	/* write back array */ \
	path[-1] = (uint32_t)(path_array<<(BLK - rem)); \
	path[0] = (uint32_t)(prev_array | (path_array>>rem)); \
	debug("prev_array(%llx), path_array(%llx), path[-1](%x), path[0](%x), blk(%p)", \
		prev_array, path_array, path[-1], path[0], blk); \
	/* update path and remainder */ \
	path -= (rem + (_traced_count)) / BLK; \
	debug("path_adv(%d)", ((rem + (_traced_count)) & BLK) != 0 ? 1 : 0); \
	rem = (rem + (_traced_count)) & (BLK - 1); \
	debug("i(%lld), rem(%lld)", (int64_t)(_traced_count), rem); \
}
#define _trace_reverse_update_path(_traced_count) { \
	/* adjust path_array */ \
	path_array = (uint64_t)(~((uint32_t)path_array))>>(BLK - (_traced_count)); \
	/* load previous array */ \
	uint64_t prev_array = path[0]; \
	/* write back array */ \
	path[0] = (uint32_t)(prev_array | (path_array<<rem)); \
	path[1] = (uint32_t)(path_array>>(BLK - rem)); \
	debug("prev_array(%llx), path_array(%llx), path[0](%x), path[1](%x), blk(%p)", \
		prev_array, path_array, path[0], path[1], blk); \
	/* update path and remainder */ \
	path += (rem + (_traced_count)) / BLK; \
	debug("path_adv(%d)", ((rem + (_traced_count)) & BLK) != 0 ? 1 : 0); \
	rem = (rem + (_traced_count)) & (BLK - 1); \
	debug("i(%lld), rem(%lld)", (int64_t)(_traced_count), rem); \
}

/**
 * @macro _trace_windback_block
 */
#define _trace_windback_block() { \
	/* load the previous block */ \
	blk--; \
}

/**
 * @macro _trace_forward_save_context
 */
#define _trace_save_context(t) { \
	(t)->w.l.blk = blk; \
	_store_v2i32(&(t)->w.l.aidx, idx); \
	(t)->w.l.psum -= (t)->w.l.p - p; \
	(t)->w.l.p = p; \
	(t)->w.l.q = q; \
	debug("p(%lld), psum(%lld), q(%llu)", p, (t)->w.l.psum, q); \
}
#define _trace_forward_save_context(t) { \
	(t)->w.l.fw_path = path; \
	(t)->w.l.fw_rem = rem; \
	_trace_save_context(t); \
}
#define _trace_reverse_save_context(t) { \
	/* reverse q-coordinate in the reverse trace */ \
	debug("unreverse q-coordinate rq(%lld), q(%lld)", q, (BW - 1) - q); \
	q = (BW - 1) - q; \
	(t)->w.l.rv_path = path; \
	(t)->w.l.rv_rem = rem; \
	_trace_save_context(t); \
}

/**
 * @macro _trace_reload_tail
 */
#define _trace_reload_tail(t) { \
	debug("tail(%p), next tail(%p), p(%d), psum(%lld), ssum(%d)", \
		(t)->w.l.tail, (t)->w.l.tail->tail, (t)->w.l.tail->tail->p, \
		(t)->w.l.tail->tail->psum, (t)->w.l.tail->tail->ssum); \
	struct gaba_joint_tail_s const *tail = (t)->w.l.tail = (t)->w.l.tail->tail; \
	blk = _last_block(tail); \
	(t)->w.l.psum -= (t)->w.l.p; \
	p += ((t)->w.l.p = tail->p); \
	debug("updated psum(%lld), w.l.p(%d), p(%lld)", (t)->w.l.psum, (t)->w.l.p, p); \
}

/**
 * @fn trace_forward_trace
 *
 * @brief traceback path until section invasion occurs
 */
static
void trace_forward_trace(
	struct gaba_dp_context_s *this)
{
	/* load context onto registers */
	_trace_forward_load_context(this);
	debug("start traceback loop p(%lld), q(%llu)", p, q);

	while(1) {
		/* cap trace at the head */
		if(p > 0 && (p + 1) % BLK != 0) {
			int64_t loop_count = (p + 1) % BLK;		/* loop_count \in (1 .. BLK), 0 is invalid */

			/* load direction array and mask pointer */
			_trace_load_block(loop_count - 1);

			/* cap trace loop */
			for(int64_t i = 0; i < loop_count; i++) {
				_trace_forward_cap_body();

				/* check if the section invasion occurred */
				if(_trace_test_cap_loop() != 0) {
					debug("cap test failed, i(%lld)", i);
					i = _trace_forward_compensate_remainder(i);
					_trace_forward_update_path(i);
					_trace_forward_save_context(this);
					return;
				}

				_trace_cap_update_index();
			}

			/* update rem, path, and block */
			_trace_forward_update_path(loop_count);
			_trace_windback_block();
		}

		/* bulk trace */
		while(p > 0 && _trace_test_bulk_loop() == 0) {
			/* load direction array and mask pointer */
			_trace_load_block(BLK - 1);
			(void)next_idx;		/* to avoid warning */

			/* bulk trace loop */
			for(int64_t i = 0; i < BLK; i++) {
				_trace_forward_bulk_body();
			}

			/* update path and block */
			_trace_bulk_update_index();
			_trace_forward_update_path(BLK);
			_trace_windback_block();

			debug("bulk loop end p(%lld), q(%llu)", p, q);
		}

		/* bulk trace test failed, continue to cap trace */
		while(p > 0) {
			/* cap loop */
			_trace_load_block(BLK - 1);

			/* cap trace loop */
			for(int64_t i = 0; i < BLK; i++) {
				_trace_forward_cap_body();

				/* check if the section invasion occurred */
				if(_trace_test_cap_loop() != 0) {
					debug("cap test failed, i(%lld)", i);
					i = _trace_forward_compensate_remainder(i);
					_trace_forward_update_path(i);
					_trace_forward_save_context(this);
					return;		/* exit is here */
				}

				_trace_cap_update_index();
			}
			debug("bulk loop end p(%lld), q(%llu)", p, q);

			/* update path and block */
			_trace_forward_update_path(BLK);
			_trace_windback_block();
		}

		/* check psum termination */
		if(this->w.l.psum < this->w.l.p - p) {
			_trace_forward_save_context(this);
			return;
		}

		/* reload tail pointer */
		_trace_reload_tail(this);
	}
	return;
}

/**
 * @fn trace_reverse_trace
 *
 * @brief traceback path until section invasion occurs
 */
static
void trace_reverse_trace(
	struct gaba_dp_context_s *this)
{
	/* load context onto registers */
	_trace_reverse_load_context(this);
	debug("start traceback loop p(%lld), q(%llu)", p, q);

	while(1) {
		/* cap trace at the head */ {
			int64_t loop_count = (p + 1) % BLK;		/* loop_count \in (1 .. BLK), 0 is invalid */

			/* load direction array and mask pointer */
			_trace_load_block(loop_count - 1);

			/* cap trace loop */
			for(int64_t i = 0; i < loop_count; i++) {
				_trace_reverse_cap_body();

				/* check if the section invasion occurred */
				if(_trace_test_cap_loop() != 0) {
					debug("cap test failed, i(%lld)", i);
					i = _trace_reverse_compensate_remainder(i);
					_trace_reverse_update_path(i);
					_trace_reverse_save_context(this);
					return;
				}

				_trace_cap_update_index();
			}

			/* update rem, path, and block */
			_trace_reverse_update_path(loop_count);
			_trace_windback_block();
		}

		/* bulk trace */
		while(p > 0 && _trace_test_bulk_loop() == 0) {
			/* load direction array and mask pointer */
			_trace_load_block(BLK - 1);
			(void)next_idx;		/* to avoid warning */

			/* bulk trace loop */
			_trace_bulk_update_index();
			for(int64_t i = 0; i < BLK; i++) {
				_trace_reverse_bulk_body();
			}
			debug("bulk loop end p(%lld), q(%llu)", p, q);

			/* update path and block */
			_trace_reverse_update_path(BLK);
			_trace_windback_block();
		}

		/* bulk trace test failed, continue to cap trace */
		while(p > 0) {
			/* cap loop */
			_trace_load_block(BLK - 1);

			/* cap trace loop */
			for(int64_t i = 0; i < BLK; i++) {
				_trace_reverse_cap_body();

				/* check if the section invasion occurred */
				if(_trace_test_cap_loop() != 0) {
					debug("cap test failed, i(%lld)", i);
					i = _trace_reverse_compensate_remainder(i);
					_trace_reverse_update_path(i);
					_trace_reverse_save_context(this);
					return;		/* exit is here */
				}

				_trace_cap_update_index();
			}
			debug("bulk loop end p(%lld), q(%llu)", p, q);

			/* update path and block */
			_trace_reverse_update_path(BLK);
			_trace_windback_block();
		}

		/* check psum termination */
		if(this->w.l.psum < this->w.l.p - p) {
			_trace_reverse_save_context(this);
			return;
		}

		/* reload tail pointer */
		_trace_reload_tail(this);
	}
	return;
}

/**
 * @fn trace_forward_push
 */
static
void trace_forward_push(
	struct gaba_dp_context_s *this)
{
	/* load section info */
	v2i32_t id = _load_v2i32(&this->w.l.aid);
	v2i32_t idx = _load_v2i32(&this->w.l.aidx);
	v2i32_t sidx = _load_v2i32(&this->w.l.asidx);

	/* calc path length */
	int64_t plen = 32 * (this->w.l.spath - this->w.l.fw_path)
		+ (this->w.l.fw_rem - this->w.l.srem);

	/* store section info */	
	_store_v2i32(&this->w.l.sec[this->w.l.fw_sec_idx].aid, id);
	_store_v2i32(&this->w.l.sec[this->w.l.fw_sec_idx].apos, idx);
	_store_v2i32(&this->w.l.sec[this->w.l.fw_sec_idx].alen, _sub_v2i32(sidx, idx));

	/* store path length */
	this->w.l.sec[this->w.l.fw_sec_idx].plen = plen;
	this->w.l.sec[this->w.l.fw_sec_idx].ppos = 0;

	debug("push current section info a(%u, %u, %u), b(%u, %u, %u), len(%lld)",
		this->w.l.sec[this->w.l.fw_sec_idx].aid,
		this->w.l.sec[this->w.l.fw_sec_idx].apos,
		this->w.l.sec[this->w.l.fw_sec_idx].alen,
		this->w.l.sec[this->w.l.fw_sec_idx].bid,
		this->w.l.sec[this->w.l.fw_sec_idx].bpos,
		this->w.l.sec[this->w.l.fw_sec_idx].blen,
		plen);

	/* update rsidx */
	_store_v2i32(&this->w.l.asidx, idx);

	/* update path and rem */
	this->w.l.spath = this->w.l.fw_path;
	this->w.l.srem = this->w.l.fw_rem;

	/* windback pointer */
	this->w.l.fw_sec_idx--;
	return;
}

/**
 * @fn trace_reverse_push
 */
static
void trace_reverse_push(
	struct gaba_dp_context_s *this)
{
	/* push section info to section array */
	v2i32_t const mask = _set_v2i32(0x01);
	v2i32_t len = _load_v2i32(&this->w.l.alen);
	v2i32_t id = _load_v2i32(&this->w.l.aid);
	v2i32_t idx = _load_v2i32(&this->w.l.aidx);
	v2i32_t sidx = _load_v2i32(&this->w.l.asidx);

	/* calc path pos and len */
	int64_t plen = 32 * (this->w.l.rv_path - this->w.l.spath)
		+ (this->w.l.rv_rem - this->w.l.srem);
	int64_t ppos = this->w.l.pspos;

	/* store revcomped section */
	_store_v2i32(&this->w.l.sec[this->w.l.rv_sec_idx].aid, _xor_v2i32(id, mask));
	_store_v2i32(&this->w.l.sec[this->w.l.rv_sec_idx].apos, _sub_v2i32(len, sidx));
	_store_v2i32(&this->w.l.sec[this->w.l.rv_sec_idx].alen, _sub_v2i32(sidx, idx));

	/* store path length */
	this->w.l.sec[this->w.l.rv_sec_idx].plen = plen;
	this->w.l.sec[this->w.l.rv_sec_idx].ppos = ppos;

	debug("push current section info a(%u, %u, %u), b(%u, %u, %u), pos(%lld), len(%lld)",
		this->w.l.sec[this->w.l.rv_sec_idx].aid,
		this->w.l.sec[this->w.l.rv_sec_idx].apos,
		this->w.l.sec[this->w.l.rv_sec_idx].alen,
		this->w.l.sec[this->w.l.rv_sec_idx].bid,
		this->w.l.sec[this->w.l.rv_sec_idx].bpos,
		this->w.l.sec[this->w.l.rv_sec_idx].blen,
		ppos, plen);

	/* update rsidx */
	_store_v2i32(&this->w.l.asidx, idx);

	/* update path, rem, and pspos */
	this->w.l.spath = this->w.l.rv_path;
	this->w.l.srem = this->w.l.rv_rem;
	this->w.l.pspos = ppos + plen;

	/* windback pointer */
	this->w.l.rv_sec_idx++;
	return;
}

/**
 * @fn trace_forward and trace_reverse
 */
#define TRACE_FORWARD 		( 0 )
#define TRACE_REVERSE 		( 1 )
static _force_inline
void trace_generate_path(
	struct gaba_dp_context_s *this,
	struct gaba_joint_tail_s const *tail,
	uint64_t dir)
{
	debug("trace_init_start_pos tail(%p), p(%d), psum(%lld), ssum(%d)",
		tail, tail->p, tail->psum, tail->ssum);

	// if(tail->psum < 0) {
	if(tail->psum < 2) {
		debug("valid block not found, psum(%lld)", tail->psum);
		return;
	}

	/* search max block, determine pos in the block */
	trace_search_max(this, tail);

	/* initialize function pointers */
	void (*body)(struct gaba_dp_context_s *this) = (
		(dir == TRACE_FORWARD) ? trace_forward_trace : trace_reverse_trace
	);
	void (*push)(struct gaba_dp_context_s *this) = (
		(dir == TRACE_FORWARD) ? trace_forward_push : trace_reverse_push
	);

	/* initialize path length info */
	this->w.l.spath = (dir == TRACE_FORWARD) ? this->w.l.fw_path : this->w.l.rv_path;
	this->w.l.srem = 0;
	this->w.l.pspos = 0;

	/* until the pointer reaches the root of the matrix */
	while(this->w.l.psum >= 0) {
		/* update section info */
		if(this->w.l.aidx <= 0) {
			trace_load_section_a(this);
		}
		if(this->w.l.bidx <= 0) {
			trace_load_section_b(this);
		}

		/* fragment trace */
		body(this);
		debug("p(%d), psum(%lld), q(%d)", this->w.l.p, this->w.l.psum, this->w.l.q);

		/* push section info to section array */
		push(this);
	}
	return;
}

/**
 * @fn trace_init_work
 */
static _force_inline
void trace_init_work(
	struct gaba_dp_context_s *this,
	struct gaba_fill_s const *fw_tail,
	struct gaba_fill_s const *rv_tail,
	struct gaba_clip_params_s const *clip)
{
	/* calculate array lengths */
	uint64_t ssum = _tail(fw_tail)->ssum + _tail(rv_tail)->ssum;
	uint64_t psum = _roundup(_tail(fw_tail)->psum + BLK, 32)
				  + _roundup(_tail(rv_tail)->psum + BLK, 32);

	/* malloc trace working area */
	uint64_t sec_len = 2 * ssum;
	uint64_t path_len = _roundup(psum / 32, sizeof(uint32_t));
	debug("psum(%lld), path_len(%llu), sec_len(%llu)", psum, path_len, sec_len);

	/* malloc pointer */
	uint64_t path_size = sizeof(uint32_t) * path_len;
	uint64_t sec_size = sizeof(struct gaba_path_section_s) * sec_len;
	struct gaba_result_s *res = (struct gaba_result_s *)(gaba_dp_malloc(this,
		  sizeof(struct gaba_result_s) + path_size + sec_size
		+ this->head_margin + this->tail_margin) + this->head_margin
		+ sizeof(uint64_t));

	/* set section array info */
	struct gaba_path_section_s *sec_base = (struct gaba_path_section_s *)(res + 1);
	this->w.l.sec = sec_base;
	this->w.l.fw_sec_idx = sec_len - 1;
	this->w.l.rv_sec_idx = 0;
	this->w.l.tail_sec_idx = sec_len - 1;

	/* set path array info */
	this->w.l.fw_rem = 0;
	this->w.l.rv_rem = 0;

	uint32_t *path_base = (uint32_t *)&sec_base[sec_len];
	this->w.l.fw_path = path_base + path_len - 1;
	this->w.l.rv_path = path_base;
	this->w.l.tail_path = path_base + path_len - 1;

	/* clear path array */
	this->w.l.fw_path[0] = 0;
	this->w.l.fw_path[1] = 0x01;			/* terminator */
	this->w.l.fw_path[2] = 0;
	this->w.l.rv_path[0] = 0;

	return;
}

/**
 * @fn trace_concatenate_path
 */
static _force_inline
struct gaba_result_s *trace_concatenate_path(
	struct gaba_dp_context_s *this,
	struct gaba_fill_s const *fw_tail,
	struct gaba_fill_s const *rv_tail,
	struct gaba_clip_params_s const *clip)
{
	debug("fw_path(%p), rv_path(%p), fw_sec(%u), rv_sec(%u)",
		this->w.l.fw_path, this->w.l.rv_path, this->w.l.fw_sec_idx, this->w.l.rv_sec_idx);
	debug("fw_rem(%d), rv_rem(%d)", this->w.l.fw_rem, this->w.l.rv_rem);

	/* recover res pointer */
	struct gaba_result_s *res = ((struct gaba_result_s *)this->w.l.sec) - 1;

	/* store score */
	int64_t score_adj = 0;					/* score adjustment at the root is not implemented */
	res->score = fw_tail->max + rv_tail->max + score_adj;

	/* store section pointer and section length */
	res->sec = this->w.l.sec;
	res->slen = this->w.l.rv_sec_idx + (this->w.l.tail_sec_idx - this->w.l.fw_sec_idx);
	// res->reserved = this->head_margin;		/* use reserved */

	/* load section pointers */
	struct gaba_path_section_s *fw_sec = this->w.l.sec + this->w.l.fw_sec_idx;
	struct gaba_path_section_s *rv_sec = this->w.l.sec + this->w.l.rv_sec_idx;
	struct gaba_path_section_s *tail_sec = this->w.l.sec + this->w.l.tail_sec_idx;
	
	/* load base path pos */
	uint32_t ppos = this->w.l.pspos;

	/* copy forward section */
	while(fw_sec < tail_sec) {
		*rv_sec = *++fw_sec;

		/* update pos */
		rv_sec++->ppos = ppos;
		ppos += fw_sec->plen;
		debug("copy forward section, ppos(%u)", ppos);
	}

	/* push forward path and update rem */
	uint64_t fw_rem = this->w.l.fw_rem;
	uint64_t rv_rem = this->w.l.rv_rem;
	uint32_t *fw_path = this->w.l.fw_path;
	uint32_t *rv_path = this->w.l.rv_path;
	uint32_t *fw_path_base = this->w.l.tail_path;
	uint32_t *rv_path_base = (uint32_t *)&this->w.l.sec[this->w.l.tail_sec_idx + 1];

	/* concatenate the heads */
	uint64_t prev_array = *fw_path;
	uint64_t path_array = *rv_path--<<(BLK - rv_rem);

	debug("fw_path_array(%llx), rv_path_array(%llx)", prev_array, path_array);

	fw_path[0] = prev_array | (path_array>>fw_rem);
	fw_path[-1] = path_array<<(BLK - fw_rem);
	debug("path_array(%x), prev_array(%x)", fw_path[0], fw_path[-1]);

	debug("dec fw_path(%d), fw_rem(%llu), rv_rem(%llu), rem(%llu)",
		(((fw_rem + rv_rem) & BLK) != 0) ? 1 : 0,
		fw_rem, rv_rem, (fw_rem + rv_rem) & (BLK - 1));
	fw_path -= (((fw_rem + rv_rem) & BLK) != 0) ? 1 : 0;
	fw_rem = (fw_rem + rv_rem) & (BLK - 1);

	if(rv_path >= rv_path_base) {
		/* load array */
		prev_array = fw_path[0];

		while(rv_path >= rv_path_base) {
			path_array = *rv_path--;
			*fw_path-- = prev_array | (path_array>>fw_rem);
			prev_array = path_array<<(BLK - fw_rem);
			debug("rv_path(%llx), path_array(%x), prev_array(%llx)", path_array, fw_path[1], prev_array);
		}

		/* store array */
		fw_path[0] = prev_array;
	}

	/* create path info container */
	struct gaba_path_s *path = (struct gaba_path_s *)(fw_path + (fw_rem == 0)) - 1;

	/* calc path length */
	int64_t fw_path_block_len = fw_path_base - fw_path;
	int64_t path_len = 32 * fw_path_block_len + fw_rem;

	/* store path info */
	path->len = path_len;
	path->offset = (32 - fw_rem) & 31;
	res->path = path;

	debug("sec(%p), path(%p), path_array(%x, %x, %x, %x), rem(%llu), slen(%u), plen(%lld)",
		fw_sec, fw_path, fw_path[0], fw_path[1], fw_path[2], fw_path[3], fw_rem, res->slen, path_len);
	return(res);
}

/**
 * @fn gaba_dp_trace
 */
struct gaba_result_s *gaba_dp_trace(
	struct gaba_dp_context_s *this,
	struct gaba_fill_s const *fw_tail,
	struct gaba_fill_s const *rv_tail,
	struct gaba_clip_params_s const *clip)
{
	/* substitute tail if NULL */
	fw_tail = (fw_tail == NULL) ? _fill(&this->tail) : fw_tail;
	rv_tail = (rv_tail == NULL) ? _fill(&this->tail) : rv_tail;

	/* init */
	trace_init_work(this, fw_tail, rv_tail, clip);

	/* forward trace */
	trace_generate_path(this, _tail(fw_tail), TRACE_FORWARD);

	/* reverse trace */
	trace_generate_path(this, _tail(rv_tail), TRACE_REVERSE);

	/* concatenate */
	return(trace_concatenate_path(this, fw_tail, rv_tail, clip));
}

/**
 * @fn parse_load_uint64
 */
static inline
uint64_t parse_load_uint64(
	uint64_t const *ptr,
	int64_t pos)
{
	int64_t rem = pos & 63;
	uint64_t a = (ptr[pos>>6]>>rem) | ((ptr[(pos>>6) + 1]<<(63 - rem))<<1);
	debug("load arr(%llx)", a);
	return(a);
}

/**
 * @union parse_cigar_table_u
 */
union parse_cigar_table_u {
	struct parse_cigar_table_s {
		char str[2];
		uint8_t len;
		uint8_t adv;
	} table;
	uint32_t all;
};

/**
 * @fn parse_get_cigar_elem
 * @brief get cigar element of length i
 */
static _force_inline
union parse_cigar_table_u parse_get_cigar_elem(
	uint8_t i)
{
	#define e(_str) { \
		.str = { \
			[0] = (_str)[0], \
			[1] = (_str)[1], \
		}, \
		.len = strlen(_str), \
		.adv = strlen(_str) + 1 \
	}
	
	struct parse_cigar_table_s const conv_table[64] = {
		{ .str = { 0 }, .len = 0, .adv = 0 },
		/*e(""),*/ e("1"), e("2"), e("3"), e("4"), e("5"), e("6"), e("7"),
		e("8"), e("9"), e("10"), e("11"), e("12"), e("13"), e("14"), e("15"),
		e("16"), e("17"), e("18"), e("19"), e("20"), e("21"), e("22"), e("23"),
		e("24"), e("25"), e("26"), e("27"), e("28"), e("29"), e("30"), e("31"),
		e("32"), e("33"), e("34"), e("35"), e("36"), e("37"), e("38"), e("39"),
		e("40"), e("41"), e("42"), e("43"), e("44"), e("45"), e("46"), e("47"),
		e("48"), e("49"), e("50"), e("51"), e("52"), e("53"), e("54"), e("55"),
		e("56"), e("57"), e("58"), e("59"), e("60"), e("61"), e("62"), e("63"),
	};
	return((union parse_cigar_table_u){ .table = conv_table[i] });

	#undef e
}

/**
 * @fn parse_dump_match_string
 */
static _force_inline
int64_t parse_dump_match_string(
	char *buf,
	int64_t len)
{
	if(len < 64) {
		union parse_cigar_table_u c = parse_get_cigar_elem(len);
		*((uint32_t *)buf) = c.all;
		*((uint16_t *)(buf + c.table.len)) = 'M';
		return(c.table.adv);
	} else {
		int64_t l = sprintf(buf, "%lldM", len);
		return(l);
	}
}

/**
 * @fn parse_dump_gap_string
 */
static _force_inline
int64_t parse_dump_gap_string(
	char *buf,
	int64_t len,
	uint64_t type)
{
	uint64_t mask = 0ULL - type;
	uint16_t gap_ch = 'D' + ((uint16_t)mask & ('I' - 'D'));

	if(len < 64) {
		union parse_cigar_table_u c = parse_get_cigar_elem(len);
		*((uint32_t *)buf) = c.all;
		*((uint16_t *)(buf + c.table.len)) = gap_ch;
		return(c.table.adv);
	} else {
		int64_t l = sprintf(buf, "%lld%c", len, gap_ch);
		return(l);
	}
}

/**
 * @macro _parse_count_match, _parse_count_gap
 */
#define _parse_count_match(_arr) ({ \
	uint64_t _a = (_arr); \
	uint64_t m0 = _a & (_a>>1); \
	uint64_t m1 = _a | (_a<<1); \
	uint64_t m = m0 | ~m1; \
	debug("m0(%llx), m1(%llx), m(%llx), tzcnt(%u)", m0, m1, m, tzcnt(m)); \
	tzcnt(m); \
})
#define _parse_count_gap(_arr) ({ \
	uint64_t _a = (_arr); \
	uint64_t mask = 0ULL - (_a & 0x01); \
	int64_t gc = tzcnt(_a ^ mask) + (int64_t)mask; \
	debug("arr(%llx), mask(%llx), gc(%lld)", _a, mask, gc); \
	gc; \
})

/**
 * @fn gaba_dp_print_cigar
 * @brief parse path string and print cigar to file
 */
int64_t gaba_dp_print_cigar(
	gaba_dp_fprintf_t _fprintf,
	void *fp,
	uint32_t const *path,
	uint32_t offset,
	uint32_t len)
{
	int64_t clen = 0;

	/* convert path to uint64_t pointer */
	uint64_t const *p = (uint64_t const *)((uint64_t)path & ~(sizeof(uint64_t) - 1));
	int64_t lim = offset + (((uint64_t)path & sizeof(uint32_t)) ? 32 : 0) + len;
	int64_t ridx = len;

	debug("path(%p), lim(%lld), ridx(%lld)", p, lim, ridx);

	while(1) {
		int64_t rsidx = ridx;
		while(1) {
			int64_t a = MIN2(_parse_count_match(parse_load_uint64(p, lim - ridx)), ridx & ~0x01);
			if(a < 64) { ridx -= a; break; }
			ridx -= 64;
		}
		int64_t m = (rsidx - ridx)>>1;
		if(m > 0) {
			clen += _fprintf(fp, "%lldM", m);
		}
		if(ridx <= 0) { break; }

		uint64_t arr;
		int64_t g = MIN2(_parse_count_gap(arr = parse_load_uint64(p, lim - ridx)), ridx);
		if(g > 0) {
			clen += _fprintf(fp, "%u%c", g, 'D' + ((char)(0ULL - (arr & 0x01)) & ('I' - 'D')));
		}
		if((ridx -= g) <= 0) { break; }
	}
	return(clen);
}

/**
 * @fn gaba_dp_dump_cigar
 * @brief parse path string and store cigar to buffer
 */
int64_t gaba_dp_dump_cigar(
	char *buf,
	uint64_t buf_size,
	uint32_t const *path,
	uint32_t offset,
	uint32_t len)
{
	uint64_t const filled_len_margin = 5;
	char *b = buf, *blim = buf + buf_size - filled_len_margin;

	/* convert path to uint64_t pointer */
	uint64_t const *p = (uint64_t const *)((uint64_t)path & ~(sizeof(uint64_t) - 1));
	int64_t lim = offset + (((uint64_t)path & sizeof(uint32_t)) ? 32 : 0) + len;
	int64_t ridx = len;

	debug("path(%p), lim(%lld), ridx(%lld)", p, lim, ridx);

	while(1) {
		int64_t rsidx = ridx;
		while(1) {
			int64_t a = MIN2(_parse_count_match(parse_load_uint64(p, lim - ridx)), ridx & ~0x01);
			debug("a(%lld), ridx(%lld), ridx&~0x01(%lld)", a, ridx, ridx & ~0x01);
			if(a < 64) { ridx -= a; break; }
			ridx -= 64;
		}
		b += parse_dump_match_string(b, (rsidx - ridx)>>1);
		if(ridx <= 0 || b > blim) { break; }

		uint64_t arr;
		int64_t g = MIN2(_parse_count_gap(arr = parse_load_uint64(p, lim - ridx)), ridx);
		b += parse_dump_gap_string(b, g, arr & 0x01);
		if((ridx -= g) <= 0 || b > blim) { break; }
	}
	return(b - buf);
}

/**
 * @fn gaba_dp_set_qual
 */
void gaba_dp_set_qual(
	gaba_result_t *res,
	int32_t qual)
{
	res->qual = qual;
	return;
}

/**
 * @fn extract_max, extract_min
 * @brief extract max /min value from 8-bit 16-cell vector
 */
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
static _force_inline
int8_t extract_min(int8_t const vector[][4])
{
	int8_t *v = (int8_t *)vector;
	int8_t min = 127;
	for(int i = 0; i < 16; i++) {
		min = (v[i] < min) ? v[i] : min;
	}
	return(min);
}

/**
 * @fn gaba_init_restore_default_params
 */
static
struct gaba_score_s const *default_score_matrix = GABA_SCORE_SIMPLE(1, 1, 1, 1);
static _force_inline
void gaba_init_restore_default_params(
	struct gaba_params_s *params)
{
	#define restore(_name, _default) { \
		params->_name = ((uint64_t)(params->_name) == 0) ? (_default) : (params->_name); \
	}
	restore(head_margin, 		0);
	restore(tail_margin, 		0);
	restore(xdrop, 				100);
	restore(score_matrix, 		default_score_matrix);
	return;
}

/**
 * @fn gaba_init_create_score_vector
 */
static _force_inline
struct gaba_score_vec_s gaba_init_create_score_vector(
	struct gaba_score_s const *score_matrix)
{
	int8_t *v = (int8_t *)score_matrix->score_sub;
	int8_t geh = -score_matrix->score_ge_a;
	int8_t gev = -score_matrix->score_ge_b;
	int8_t gih = -score_matrix->score_gi_a;
	int8_t giv = -score_matrix->score_gi_b;
	
	int8_t sb[16] __attribute__(( aligned(16) ));
	struct gaba_score_vec_s sc;
	#if BIT == 2
		for(int i = 0; i < 16; i++) {
			sb[i] = v[i] - (geh + gih + gev + giv);
		}
	#else /* BIT == 4 */
		int8_t min = extract_min(score_matrix->score_sub);
		sb[0] = min - (geh + gih + gev + giv);
		for(int i = 1; i < 16; i++) {
			sb[i] = v[0] - (geh + gih + gev + giv);
		}
	#endif
	_store_sb(sc, _load_v16i8(sb));

	#if MODEL == LINEAR
		_store_adjh(sc, 0, 0, geh + gih, gev + giv);
		_store_adjv(sc, 0, 0, geh + gih, gev + giv);
		_store_ofsh(sc, 0, 0, geh + gih, gev + giv);
		_store_ofsv(sc, 0, 0, geh + gih, gev + giv);

		/*
		for(int i = 0; i < 16; i++) {
			sc.adjh[i] = sc.adjv[i] = 0;
			sc.ofsh[i] = geh + gih;
			sc.ofsv[i] = gev + giv;
		}
		*/
	#else
		_store_adjh(sc, -gih, -giv, -(geh + gih), gev + giv);
		_store_adjv(sc, -gih, -giv, -(geh + gih), gev + giv);
		_store_ofsh(sc, -gih, -giv, -(geh + gih), gev + giv);
		_store_ofsv(sc, -gih, -giv, -(geh + gih), gev + giv);

		/*
		for(int i = 0; i < 16; i++) {
			sc.adjh[i] = -gih;
			sc.adjv[i] = -giv;
			sc.ofsh[i] = -(geh + gih);
			sc.ofsv[i] = gev + giv;
		}
		*/
	#endif
	return(sc);
}

/**
 * @fn gaba_init_create_dir_dynamic
 */
static _force_inline
union gaba_dir_u gaba_init_create_dir_dynamic(
	struct gaba_score_s const *score_matrix)
{
	int8_t max = extract_max(score_matrix->score_sub);
	int8_t geh = -score_matrix->score_ge_a;
	int8_t gev = -score_matrix->score_ge_b;
	int8_t gih = -score_matrix->score_gi_a;
	int8_t giv = -score_matrix->score_gi_b;

	#if MODEL == LINEAR
		int16_t coef_a = -max + 2 * (geh + gih);
		int16_t coef_b = -max + 2 * (gev + giv);
		int16_t ofs_a = 0;
		int16_t ofs_b = 0;
	#else
		int16_t coef_a = -max + 2 * geh;
		int16_t coef_b = -max + 2 * gev;
		int16_t ofs_a = gih;
		int16_t ofs_b = giv;
	#endif

	int64_t acc = (ofs_a + coef_a * BW/2) - (ofs_b + coef_b * (BW/2 - 1));
	return((union gaba_dir_u) {
		.dynamic = {
			.acc = acc,
			.array = 0x80000000	/* (0, 0) -> (0, 1) (down) */
		}
	});
}

/**
 * @fn gaba_init_create_small_delta
 */
static _force_inline
struct gaba_small_delta_s gaba_init_create_small_delta(
	struct gaba_score_s const *score_matrix)
{
	int8_t relax = -128 / BW;

	struct gaba_small_delta_s sd;
	#if MODEL == LINEAR
		for(int i = 0; i < BW/2; i++) {
			// sd.delta[i] = sd.delta[BW/2 + i] = 0;
			// sd.max[i] = sd.max[BW/2 + i] = 0;

			sd.delta[i] = relax * (BW/2 - i);
			sd.delta[BW/2 + i] = relax * i;
			sd.max[i] = relax * (BW/2 - i);
			sd.max[BW/2 + i] = relax * i;
		}
	#else
		for(int i = 0; i < BW/2; i++) {
			// sd.delta[i] = sd.delta[BW/2 + i] = 0;
			// sd.max[i] = sd.max[BW/2 + i] = 0;

			sd.delta[i] = relax * (BW/2 - i);
			sd.delta[BW/2 + i] = relax * i;
			sd.max[i] = relax * (BW/2 - i);
			sd.max[BW/2 + i] = relax * i;
		}
	#endif

	return(sd);

	/*
	return((struct gaba_small_delta_s){
		.delta = { 0 },
		.max = { 0 }
	});
	*/
}

/**
 * @fn gaba_init_create_middle_delta
 */
static _force_inline
struct gaba_middle_delta_s gaba_init_create_middle_delta(
	struct gaba_score_s const *score_matrix)
{
	int8_t max = extract_max(score_matrix->score_sub);
	int8_t geh = -score_matrix->score_ge_a;
	int8_t gev = -score_matrix->score_ge_b;
	int8_t gih = -score_matrix->score_gi_a;
	int8_t giv = -score_matrix->score_gi_b;
	int8_t relax = 128 / BW;

	#if MODEL == LINEAR
		// int16_t coef_a = -max + 2 * (geh + gih);
		// int16_t coef_b = -max + 2 * (gev + giv);
		int16_t coef_a = -max + 2 * (geh + gih) + relax;
		int16_t coef_b = -max + 2 * (gev + giv) + relax;
		int16_t ofs_a = 0;
		int16_t ofs_b = 0;
	#else
		// int16_t coef_a = -max + 2*geh;
		// int16_t coef_b = -max + 2*gev;
		int16_t coef_a = -max + 2*geh + relax;
		int16_t coef_b = -max + 2*gev + relax;
		int16_t ofs_a = gih;
		int16_t ofs_b = giv;
	#endif

	struct gaba_middle_delta_s md;
	for(int i = 0; i < BW/2; i++) {
		md.delta[i] = ofs_a + coef_a * (BW/2 - i);
		md.delta[BW/2 + i] = ofs_b + coef_b * i;
	}
	md.delta[BW/2] = 0;
	return(md);
}

/**
 * @fn gaba_init_create_diff_vectors
 *
 * @detail
 * dH[i, j] = S[i, j] - S[i - 1, j]
 * dV[i, j] = S[i, j] - S[i, j - 1]
 * dE[i, j] = E[i, j] - S[i, j]
 * dF[i, j] = F[i, j] - S[i, j]
 */
#if MODEL == LINEAR
static _force_inline
struct gaba_diff_vec_s gaba_init_create_diff_vectors(
	struct gaba_score_s const *score_matrix)
{
	int8_t max = extract_max(score_matrix->score_sub);
	int8_t geh = -score_matrix->score_ge_a;
	int8_t gev = -score_matrix->score_ge_b;
	int8_t gih = -score_matrix->score_gi_a;
	int8_t giv = -score_matrix->score_gi_b;

	int8_t drop = 0;
	int8_t raise = max - (geh + gih + gev + giv);

	int8_t dh[BW] __attribute__(( aligned(16) ));
	int8_t dv[BW] __attribute__(( aligned(16) ));

	struct gaba_diff_vec_s diff __attribute__(( aligned(16) ));
	/**
	 * dh: dH[i, j] - gh
	 * dv: dV[i, j] - gv
	 */
	/* calc dh and dv */
	for(int i = 0; i < BW/2; i++) {
		dh[i] = drop;
		dh[BW/2 + i] = raise;
		dv[i] = raise;
		dv[BW/2 + i] = drop;
	}
 	dh[BW/2] = raise;
 	dv[BW/2] = raise;

	_store(&diff.dh, _load(dh));
	_store(&diff.dv, _load(dv));
	return(diff);
}
#else
static _force_inline
struct gaba_diff_vec_s gaba_init_create_diff_vectors(
	struct gaba_score_s const *score_matrix)
{
	int8_t max = extract_max(score_matrix->score_sub);
	int8_t geh = -score_matrix->score_ge_a;
	int8_t gev = -score_matrix->score_ge_b;
	int8_t gih = -score_matrix->score_gi_a;
	int8_t giv = -score_matrix->score_gi_b;

	int8_t ofs_dh = -(geh + gih);
	int8_t ofs_dv = -(gev + giv);
	int8_t ofs_de = -gih;
	int8_t ofs_df = -giv;
	
	int8_t drop_dh = geh + ofs_dh;
	int8_t raise_dh = max - gev + ofs_dh;
	int8_t drop_dv = gev + ofs_dv;
	int8_t raise_dv = max - geh + ofs_dv;
	int8_t drop_de = gih + ofs_de;
	int8_t raise_de = ofs_de;
	int8_t drop_df = giv + ofs_df;
	int8_t raise_df = ofs_df;

	int8_t dh[BW] __attribute__(( aligned(16) ));
	int8_t dv[BW] __attribute__(( aligned(16) ));
	int8_t de[BW] __attribute__(( aligned(16) ));
	int8_t df[BW] __attribute__(( aligned(16) ));

	struct gaba_diff_vec_s diff __attribute__(( aligned(16) ));
	/**
	 * dh: dH[i, j] - geh
	 * dv: dV[i, j] - gev
	 * de: dE[i, j] + gih + dV[i, j] - gev
	 * df: dF[i, j] + giv + dH[i, j] - geh
	 */
	/* calc dh and dv */
	for(int i = 0; i < BW/2; i++) {
		dh[i] = drop_dh;
		dh[BW/2 + i] = raise_dh;
		dv[i] = raise_dv;
		dv[BW/2 + i] = drop_dv;
	}
 	dh[BW/2] = raise_dh - gih;
 	dv[BW/2] = raise_dv - giv;

	/* calc de and df */
 	for(int i = 0; i < BW/2; i++) {
 		de[i] = raise_de;
 		de[BW/2 + i] = drop_de;
 		df[i] = drop_df;
 		df[BW/2 + i] = raise_df;
 	}
 	de[BW/2] = drop_de;
 	df[BW/2] = drop_df;

 	vec_t _dh = _load(dh);
 	vec_t _dv = _load(dv);
 	vec_t _de = _load(de);
 	vec_t _df = _load(df);

 	_print(_dh);
 	_print(_dv);
 	_print(_de);
 	_print(_df);

	_dh = _shl(_dh, 3);
	_dv = _shl(_dv, 3);
	_store(&diff.dh, _add(_dh, _de));
	_store(&diff.dv, _add(_dv, _df));
	_print(_add(_dh, _de));
	_print(_add(_dv, _df));

	return(diff);
}
#endif

/**
 * @fn gaba_init_create_char_vector
 */
static _force_inline
struct gaba_char_vec_s gaba_init_create_char_vector(
	void)
{
	struct gaba_char_vec_s ch;

	for(int i = 0; i < BW; i++) {
		ch.w[i] = (BIT == 2) ? 0x80 : 0;
	}
	return(ch);
}

/**
 * @fn gaba_init
 */
gaba_t *gaba_init(
	struct gaba_params_s const *params)
{
	if(params == NULL) {
		debug("params must not be NULL");
		return(NULL);
	}

	/* copy params to local stack */
	struct gaba_params_s params_intl = *params;

	/* restore defaults */
	gaba_init_restore_default_params(&params_intl);

	/* malloc gaba_context_s */
	struct gaba_context_s *ctx = (struct gaba_context_s *)gaba_aligned_malloc(
		sizeof(struct gaba_context_s),
		MEM_ALIGN_SIZE);
	if(ctx == NULL) {
		return(NULL);
	}

	/* fill context */
	*ctx = (struct gaba_context_s) {
		/* default middle delta vector */
		.md = gaba_init_create_middle_delta(params_intl.score_matrix),

		/* template */
		.k = (struct gaba_dp_context_s) {
			.stack_top = NULL,						/* stored on init */
			.stack_end = NULL,						/* stored on init */

			/* score vectors */
			.scv = gaba_init_create_score_vector(params_intl.score_matrix),
			.tx = params_intl.xdrop,

			/* input and output options */
			.head_margin = _roundup(params_intl.head_margin, MEM_ALIGN_SIZE),
			.tail_margin = _roundup(params_intl.tail_margin, MEM_ALIGN_SIZE),

			/* memory management */
			.mem_cnt = 0,
			.mem_array = { 0 },						/* NULL */

			/* phantom block at root */
			.blk = (struct gaba_phantom_block_s) {
				/* direction array */
				.dir = gaba_init_create_dir_dynamic(params_intl.score_matrix),
				
				/* offset, diffs and deltas */
				.offset = 0,
				.diff = gaba_init_create_diff_vectors(params_intl.score_matrix),
				.sd = gaba_init_create_small_delta(params_intl.score_matrix),
				.md = &ctx->md,

				/* char vectors */
				.ch = gaba_init_create_char_vector(),

				/* indices */
				.aridx = 0,
				.bridx = 0
			},

			/* tail of root section */
			.tail = (struct gaba_joint_tail_s) {
				/* coordinates */
				.psum = PSUM_BASE - BW,
				.p = 0,
				.ssum = 0,

				/* max */
				.max = 0,

				/* status */
				.stat = CONT,

				/* internals */
				.rem_len = 0,
				.tail = NULL,
				.apos = 0,
				.bpos = 0,
				.alen = 0,
				.blen = 0,
				.aid = 0xfffc,
				.bid = 0xfffd
			},
		},

		/* copy params */
		.params = *params
	};

	return((gaba_t *)ctx);
}

/**
 * @fn gaba_clean
 */
void gaba_clean(
	struct gaba_context_s *ctx)
{
	gaba_aligned_free(ctx);
	return;
}

/**
 * @fn gaba_dp_init
 */
struct gaba_dp_context_s *gaba_dp_init(
	struct gaba_context_s const *ctx,
	uint8_t const *alim,
	uint8_t const *blim)
{
	/* malloc stack memory */
	struct gaba_dp_context_s *this = (struct gaba_dp_context_s *)gaba_aligned_malloc(
		MEM_INIT_SIZE, MEM_ALIGN_SIZE);
	if(this == NULL) {
		debug("failed to malloc memory");
		return(NULL);
	}

	/* init seq lims */
	this->w.r.alim = alim;
	this->w.r.blim = blim;

	/* copy template */
	_memcpy_blk_aa(
		(uint8_t *)this + GABA_DP_CONTEXT_LOAD_OFFSET,
		(uint8_t *)&ctx->k + GABA_DP_CONTEXT_LOAD_OFFSET,
		GABA_DP_CONTEXT_LOAD_SIZE);

	/* init stack pointers */
	this->stack_top = (uint8_t *)this + sizeof(struct gaba_dp_context_s);
	this->stack_end = (uint8_t *)this + MEM_INIT_SIZE - MEM_MARGIN_SIZE;
	return(this);
}

/**
 * @fn gaba_dp_add_stack
 */
static _force_inline
int32_t gaba_dp_add_stack(
	struct gaba_dp_context_s *this,
	uint64_t size)
{
	uint8_t *ptr = this->mem_array[this->mem_cnt];
	size += MEM_MARGIN_SIZE + MEM_ALIGN_SIZE;
	size = MAX2(size, MEM_INIT_SIZE<<(this->mem_cnt + 1));

	/* if next mem block is not malloc'd, malloc and store size at the head */
	if(ptr == NULL || *((uint64_t *)ptr) < size) {
		free(ptr);
		ptr = (uint8_t *)gaba_aligned_malloc(size, MEM_ALIGN_SIZE);
		*((uint64_t *)ptr) = size;
		debug("memory block added ptr(%p)", ptr);
	} else {
		size = *((uint64_t *)ptr);
	}

	/* check errors */
	if(ptr == NULL) {
		return(GABA_ERROR_OUT_OF_MEM);
	}

	/* set stack pointers */
	this->mem_array[this->mem_cnt++] = ptr;
	this->stack_top = ptr + MEM_ALIGN_SIZE;
	this->stack_end = this->stack_top + size - MEM_MARGIN_SIZE;
	debug("stack_top(%p), stack_end(%p), size(%llu)",
		this->stack_top, this->stack_end, size);
	return(GABA_SUCCESS);
}

/**
 * @fn gaba_dp_flush
 */
void gaba_dp_flush(
	struct gaba_dp_context_s *this,
	uint8_t const *alim,
	uint8_t const *blim)
{
	/* init seq lims */
	this->w.r.alim = alim;
	this->w.r.blim = blim;

	this->stack_top = (uint8_t *)this + sizeof(struct gaba_dp_context_s);
	this->stack_end = (uint8_t *)this + MEM_INIT_SIZE - MEM_MARGIN_SIZE;

	/* clear memory */
	this->mem_cnt = 0;
	return;
}

/**
 * @fn gaba_dp_malloc
 */
static _force_inline
void *gaba_dp_malloc(
	struct gaba_dp_context_s *this,
	uint64_t size)
{
	/* roundup */
	size = _roundup(size, MEM_ALIGN_SIZE);

	/* malloc */
	debug("this(%p), stack_top(%p), size(%llu)", this, this->stack_top, size);
	if((this->stack_end - this->stack_top) < size) {
		if(gaba_dp_add_stack(this, size) != GABA_SUCCESS) {
			return(NULL);
		}
		debug("stack_top(%p)", this->stack_top);
	}
	this->stack_top += size;
	return((void *)(this->stack_top - size));
}

#if 0
/**
 * @fn gaba_dp_smalloc
 * @brief small malloc without boundary check, for joint_head, joint_tail, merge_tail, and delta vectors.
 */
static _force_inline
void *gaba_dp_smalloc(
	struct gaba_dp_context_s *this,
	uint64_t size)
{
	debug("this(%p), stack_top(%p)", this, this->stack_top);
	void *ptr = (void *)this->stack_top;
	this->stack_top += _roundup(size, MEM_ALIGN_SIZE);
	return(ptr);
}
#endif

/**
 * @fn gaba_dp_clean
 */
void gaba_dp_clean(
	struct gaba_dp_context_s *this)
{
	if(this == NULL) {
		return;
	}

	for(uint64_t i = 0; i < GABA_MEM_ARRAY_SIZE; i++) {
		gaba_aligned_free(this->mem_array[i]); this->mem_array[i] = NULL;
	}
	gaba_aligned_free(this);
	return;
}

/* unittests */
#if UNITTEST == 1

#include <string.h>
#include <sys/types.h>
#include <unistd.h>

/**
 * @fn unittest_build_context
 * @brief build context for unittest
 */
static
struct gaba_score_s const *unittest_default_score_matrix = GABA_SCORE_SIMPLE(2, 3, 5, 1);
static
void *unittest_build_context(void *params)
{
	struct gaba_score_s const *score = (params != NULL)
		? (struct gaba_score_s const *)params
		: unittest_default_score_matrix;

	/* build context */
	gaba_t *ctx = gaba_init(GABA_PARAMS(
		.xdrop = 100,
		.score_matrix = score));
	return((void *)ctx);
}

/**
 * @fn unittest_clean_context
 */
static
void unittest_clean_context(void *ctx)
{
	gaba_clean((struct gaba_context_s *)ctx);
	return;
}

/**
 * @struct unittest_seqs_s
 * @brief sequence container
 */
struct unittest_seqs_s {
	char const *a;
	char const *b;
};
#define unittest_seq_pair(_a, _b) ( \
	(void *)&((struct unittest_seqs_s const){ \
		.a = _a "GGGGGGGGGGGGGGGGGGGG", \
		.b = _b "CCCCCCCCCCCCCCCCCCCC" \
	}) \
)

/**
 * @struct unittest_sections_s
 * @brief section container
 */
struct unittest_sections_s {
	uint8_t const *a, *b;
	uint8_t const *alim, *blim;
	uint64_t alen, blen;
	struct gaba_section_s afsec, aftail, bfsec, bftail;
	struct gaba_section_s arsec, artail, brsec, brtail;
};

/**
 * @fn unittest_encode_base
 * @brief mapping IUPAC amb. to 2bit / 4bit encoding
 */
static _force_inline
uint8_t unittest_encode_base(
	char c)
{
	/* convert to upper case and subtract offset by 0x40 */
	#define _b(x)	( (x) & 0x1f )

	/* conversion tables */
	#if BIT == 2
		enum bases { A = 0x00, C = 0x01, G = 0x02, T = 0x03 };
		uint8_t const table[] = {
			[_b('A')] = A,
			[_b('C')] = C,
			[_b('G')] = G,
			[_b('T')] = T,
			[_b('U')] = T,
			[_b('N')] = A,		/* treat 'N' as 'A' */
			[_b('_')] = 0		/* sentinel */
		};
	#else /* BIT == 4 */
		enum bases { A = 0x01, C = 0x02, G = 0x04, T = 0x08 };
		uint8_t const table[] = {
			[_b('A')] = A,
			[_b('C')] = C,
			[_b('G')] = G,
			[_b('T')] = T,
			[_b('U')] = T,
			[_b('R')] = A | G,
			[_b('Y')] = C | T,
			[_b('S')] = G | C,
			[_b('W')] = A | T,
			[_b('K')] = G | T,
			[_b('M')] = A | C,
			[_b('B')] = C | G | T,
			[_b('D')] = A | G | T,
			[_b('H')] = A | C | T,
			[_b('V')] = A | C | G,
			[_b('N')] = 0,		/* treat 'N' as a gap */
			[_b('_')] = 0		/* sentinel */
		};
	#endif /* BIT */
	return(table[_b((uint8_t)c)]);

	#undef _b
}

/**
 * @fn unittest_build_seqs
 * @brief build seq_pair and sections
 */
static
void *unittest_build_seqs(void *params)
{
	struct unittest_seqs_s const *seqs = (struct unittest_seqs_s const *)params;
	char const *a = seqs->a;
	char const *b = seqs->b;

	uint64_t atot = strlen(a);
	uint64_t btot = strlen(b);
	uint32_t alen = atot - 20;
	uint32_t blen = btot - 20;

	int64_t margin = 64;
	struct unittest_sections_s *sec = malloc(
		sizeof(struct unittest_sections_s) + (atot + 1) + (btot + 1) + margin);

	/* copy sequences */
	uint8_t *ca = (uint8_t *)(sec + 1);
	uint8_t *cb = ca + atot + 1;
	uint8_t *cm = cb + btot + 1;

	for(int64_t i = 0; i < atot; i++) {
		ca[i] = unittest_encode_base(a[i]);
	}
	for(int64_t i = 0; i < btot; i++) {
		cb[i] = unittest_encode_base(b[i]);
	}
	ca[atot] = cb[btot] = '\0';
	memset(cm, 0, margin);

	/* build context */
	uint8_t const *const alim = (void const *)0x800000000000;
	uint8_t const *const blim = (void const *)0x800000000000;
	*sec = (struct unittest_sections_s){
		.a = ca,
		.b = cb,
		.alim = alim,
		.blim = blim,
		.alen = atot,
		.blen = btot,
		
		/* forward */
		.afsec = gaba_build_section(0, ca, alen),
		.aftail = gaba_build_section(2, ca + alen, 20),
		.bfsec = gaba_build_section(4, cb, blen),
		.bftail = gaba_build_section(6, cb + blen, 20),

		/* reverse */
		.arsec = gaba_build_section(1, _rev(ca + alen, alim), alen),
		.artail = gaba_build_section(3, _rev(ca + atot, alim), 20),
		.brsec = gaba_build_section(5, _rev(cb + blen, blim), blen),
		.brtail = gaba_build_section(7, _rev(cb + btot, blim), 20)
	};
	return((void *)sec);
}

/**
 * @fn unittest_clean_seqs
 */
static
void unittest_clean_seqs(void *ctx)
{
	free(ctx);
}

/**
 * @macro with_seq_pair
 * @brief constructs .param, .init, and .clean parameters
 */
#define with_seq_pair(_a, _b) \
	.params = unittest_seq_pair(_a, _b), \
	.init = unittest_build_seqs, \
	.clean = unittest_clean_seqs

/**
 * misc macros and functions for assertion
 */
#define check_tail(_t, _max, _p, _psum, _ssum) ( \
	   (_t) != NULL \
	&& (_t)->max == (_max) \
	&& (_t)->p == (_p) \
	&& (_t)->psum == (_psum) \
	&& (_t)->ssum == (_ssum) \
)
#define print_tail(_t) \
	"tail(%p), max(%lld), p(%d), psum(%lld), ssum(%u)", \
	(_t), (_t)->max, (_t)->p, (_t)->psum, (_t)->ssum
#define check_result(_r, _score, _plen, _offset, _slen) ( \
	   (_r) != NULL \
	&& (_r)->sec != NULL \
	&& (_r)->path != NULL \
	&& (_r)->path->len == (_plen) \
	&& (_r)->path->offset == (_offset) \
	&& (_r)->slen == (_slen) \
	&& (_r)->score == (_score) \
)
#define print_result(_r) \
	"res(%p), score(%lld), plen(%u), offset(%u), slen(%u)", \
	(_r), (_r)->score, (_r)->path->len, (_r)->path->offset, (_r)->slen

static
int check_path(
	struct gaba_result_s const *res,
	char const *str)
{
	int64_t plen = res->path->len, slen = strlen(str);
	uint32_t const *p = &res->path->array[_roundup(plen, 32) / 32 - 1];
	char const *s = &str[slen - 1];
	debug("%s", str);

	/* first check length */
	if(plen != slen) {
		debug("plen(%lld), slen(%lld)", plen, slen);
		return(0);
	}

	/* next compare encoded string (bit string) */
	while(plen > 0) {
		int64_t i = 0;
		uint32_t array = 0;
		for(i = 0; i < 32; i++) {
			if(plen-- == 0) { break; }
			array = (array<<1) | ((*s-- == 'D') ? 1 : 0);
			debug("%c, %x", s[1], array);
		}
		array <<= 32 - i;
		debug("path(%x), array(%x), i(%lld)", *p, array, i);
		if(*p-- != array) {
			return(0);
		}
	}
	return(1);
}

static
int check_cigar(
	struct gaba_result_s const *res,
	char const *cigar)
{
	char buf[1024];

	debug("path(%x), rem(%u), len(%u)", res->path->array[0], res->path->offset, res->path->len);

	int64_t l = gaba_dp_dump_cigar(buf, 1024,
		res->path->array, res->path->offset, res->path->len);

	debug("cigar(%s)", buf);

	/* first check length */
	if(strlen(cigar) != l) { return(0); }

	/* next compare cigar string */
	return((strcmp(buf, cigar) == 0) ? 1 : 0);
}

#define decode_path(_r) ({ \
	int64_t plen = (_r)->path->len, cnt = 0; \
	uint32_t const *path = &(_r)->path->array[_roundup(plen, 32) / 32 - 1]; \
	uint32_t path_array = *path; \
	char *p = alloca(plen) + plen; \
	*p-- = '\0'; \
	while(plen-- > 0) { \
		*p-- = (path_array & 0x80000000) ? 'D' : 'R'; \
		path_array <<= 1; \
		if(++cnt == 32) { \
			path_array = *--path; \
			cnt = 0; \
		} \
	} \
	(p + 1); \
})
#define print_path(_r)			"%s", decode_path(_r)
#define check_section(_s, _a, _apos, _alen, _b, _bpos, _blen, _ppos, _plen) ( \
	   (_s).aid == (_a).id \
	&& (_s).apos == (_apos) \
	&& (_s).alen == (_alen) \
	&& (_s).bid == (_b).id \
	&& (_s).bpos == (_bpos) \
	&& (_s).blen == (_blen) \
	&& (_s).ppos == (_ppos) \
	&& (_s).plen == (_plen) \
)
#define print_section(_s) \
	"a(%u), apos(%u), alen(%u), b(%u), bpos(%u), blen(%u), ppos(%u), plen(%u)", \
	(_s).aid, (_s).apos, (_s).alen, \
	(_s).bid, (_s).bpos, (_s).blen, \
	(_s).ppos, (_s).plen

/* global configuration of the tests */
unittest_config(
	.name = "gaba",
	.init = unittest_build_context,
	.clean = unittest_clean_context
);


/**
 * check if gaba_init returns a valid pointer to a context
 */
unittest()
{
	struct gaba_context_s const *c = (struct gaba_context_s const *)gctx;
	assert(c != NULL, "%p", c);
}

/**
 * check if unittest_build_seqs returns a valid seq_pair and sections
 */
unittest(with_seq_pair("A", "A"))
{
	struct unittest_sections_s const *s = (struct unittest_sections_s const *)ctx;

	/* check pointer */
	assert(s != NULL, "%p", s);

	/* check sequences */
	#if BIT == 2
		assert(strncmp((char const *)s->a,
			((char const [22]){ 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, '\0' }),
			22) == 0, "%s", (char const *)s->a);
		assert(strncmp((char const *)s->b,
			((char const [22]){ 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, '\0' }),
			22) == 0, "%s", (char const *)s->b);
	#else /* BIT == 4 */
		assert(strncmp((char const *)s->a,
			((char const [22]){ 1, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, '\0' }),
			22) == 0, "%s", (char const *)s->a);
		assert(strncmp((char const *)s->b,
			((char const [22]){ 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, '\0' }),
			22) == 0, "%s", (char const *)s->b);
	#endif
	assert(s->alen == 21, "%llu", s->alen);
	assert(s->blen == 21, "%llu", s->blen);

	/* check fowrard sections */
	assert(s->afsec.id == 0, "%d", s->afsec.id);
	assert(s->afsec.base == s->a + 0, "%p", s->afsec.base);
	assert(s->afsec.len == 1, "%u", s->afsec.len);

	assert(s->aftail.id == 2, "%d", s->aftail.id);
	assert(s->aftail.base == s->a + 1, "%p", s->aftail.base);
	assert(s->aftail.len == 20, "%u", s->aftail.len);

	assert(s->bfsec.id == 4, "%d", s->bfsec.id);
	assert(s->bfsec.base == s->b + 0, "%p", s->bfsec.base);
	assert(s->bfsec.len == 1, "%u", s->bfsec.len);

	assert(s->bftail.id == 6, "%d", s->bftail.id);
	assert(s->bftail.base == s->b + 1, "%p", s->bftail.base);
	assert(s->bftail.len == 20, "%u", s->bftail.len);

	/* check reverse sections */
	assert(s->arsec.id == 1, "%d", s->arsec.id);
	assert(s->arsec.base == (void const *)0x1000000000000 - (uint64_t)s->a - 1, "%p", s->arsec.base);
	assert(s->arsec.len == 1, "%u", s->arsec.len);

	assert(s->artail.id == 3, "%d", s->artail.id);
	assert(s->artail.base == (void const *)0x1000000000000 - (uint64_t)s->a - 21, "%p", s->artail.base);
	assert(s->artail.len == 20, "%u", s->artail.len);

	assert(s->brsec.id == 5, "%d", s->brsec.id);
	assert(s->brsec.base == (void const *)0x1000000000000 - (uint64_t)s->b - 1, "%p", s->brsec.base);
	assert(s->brsec.len == 1, "%u", s->brsec.len);

	assert(s->brtail.id == 7, "%d", s->brtail.id);
	assert(s->brtail.base == (void const *)0x1000000000000 - (uint64_t)s->b - 21, "%p", s->brtail.base);
	assert(s->brtail.len == 20, "%u", s->brtail.len);
}

/**
 * check if gaba_dp_init returns a vaild pointer to a dp context
 */
#define omajinai() \
	struct gaba_context_s const *c = (struct gaba_context_s const *)gctx; \
	struct unittest_sections_s const *s = (struct unittest_sections_s const *)ctx; \
	struct gaba_dp_context_s *d = gaba_dp_init(c, s->alim, s->blim);

unittest(with_seq_pair("A", "A"))
{
	omajinai();

	assert(d != NULL, "%p", d);
	gaba_dp_clean(d);
}

/**
 * check if gaba_dp_fill_root and gaba_dp_fill returns a correct score
 */
unittest(with_seq_pair("A", "A"))
{
	omajinai();

	/* fill root section */
	struct gaba_fill_s *f = gaba_dp_fill_root(d, &s->afsec, 0, &s->bfsec, 0);
	assert(f->status == 0x1ff, "%x", f->status);
	assert(check_tail(f, 0, 0, -29, 1), print_tail(f));

	/* fill again */
	f = gaba_dp_fill(d, f, &s->afsec, &s->bfsec);
	assert(f->status == 0x1ff, "%x", f->status);
	assert(check_tail(f, 0, 0, -27, 2), print_tail(f));

	/* fill tail section */
	f = gaba_dp_fill(d, f, &s->aftail, &s->bftail);
	assert(f->status == 0x1ff, "%x", f->status);
	assert(check_tail(f, 4, 13, 13, 3), print_tail(f));

	/* fill tail section again */
	f = gaba_dp_fill(d, f, &s->aftail, &s->bftail);
	#if MODEL == LINEAR
		assert(f->status == 0x1ff, "%x", f->status);
		assert(check_tail(f, 4, 40, 53, 4), print_tail(f));
	#else
		assert(f->status == 0x10f, "%x", f->status);
		assert(check_tail(f, 4, 31, 44, 4), print_tail(f));
	#endif

	gaba_dp_clean(d);
}

/* with longer sequences */
unittest(with_seq_pair("ACGTACGTACGT", "ACGTACGTACGT"))
{
	omajinai();

	/* fill root section */
	struct gaba_fill_s *f = gaba_dp_fill_root(d, &s->afsec, 0, &s->bfsec, 0);
	assert(f->status == 0x1ff, "%x", f->status);
	assert(check_tail(f, 0, 0, -7, 1), print_tail(f));

	/* fill again */
	f = gaba_dp_fill(d, f, &s->afsec, &s->bfsec);
	assert(f->status == 0x1ff, "%x", f->status);
	assert(check_tail(f, 16, 17, 17, 2), print_tail(f));

	/* fill tail section */
	f = gaba_dp_fill(d, f, &s->aftail, &s->bftail);
	assert(f->status == 0x1ff, "%x", f->status);
	assert(check_tail(f, 48, 40, 57, 3), print_tail(f));

	/* fill tail section again */
	f = gaba_dp_fill(d, f, &s->aftail, &s->bftail);
	#if MODEL == LINEAR
		assert(f->status == 0x1ff, "%x", f->status);
		assert(check_tail(f, 48, 40, 97, 4), print_tail(f));
	#else
		assert(f->status == 0x10f, "%x", f->status);
		assert(check_tail(f, 48, 31, 88, 4), print_tail(f));
	#endif

	gaba_dp_clean(d);
}

/* sequences with different lengths (consumed as mismatches) */
unittest(with_seq_pair("GAAAAAAAA", "AAAAAAAA"))
{
	omajinai();

	struct gaba_fill_s *f = gaba_dp_fill_root(d, &s->afsec, 0, &s->bfsec, 0);
	assert(f->status == 0x01ff, "%x", f->status);
	f = gaba_dp_fill(d, f, &s->afsec, &s->bfsec);
	assert(f->status == 0x01f0, "%x", f->status);
	f = gaba_dp_fill(d, f, &s->afsec, &s->bftail);
	assert(f->status == 0x010f, "%x", f->status);
	f = gaba_dp_fill(d, f, &s->aftail, &s->bftail);
	assert(f->status == 0x01f0, "%x", f->status);
	assert(check_tail(f, 22, 37, 42, 4), print_tail(f));

	gaba_dp_clean(d);
}

/* another pair with different lengths */
unittest(with_seq_pair("TTTTTTTT", "CTTTTTTTT"))
{
	omajinai();

	struct gaba_fill_s *f = gaba_dp_fill_root(d, &s->afsec, 0, &s->bfsec, 0);
	assert(f->status == 0x010f, "%x", f->status);
	f = gaba_dp_fill(d, f, &s->afsec, &s->bfsec);
	assert(f->status == 0x01f0, "%x", f->status);
	f = gaba_dp_fill(d, f, &s->afsec, &s->bfsec);
	assert(f->status == 0x010f, "%x", f->status);
	f = gaba_dp_fill(d, f, &s->aftail, &s->bfsec);
	assert(f->status == 0x01f0, "%x", f->status);

	f = gaba_dp_fill(d, f, &s->aftail, &s->bftail);
	#if MODEL == LINEAR
		assert(f->status == 0x010f, "%x", f->status);
		assert(check_tail(f, 22, 36, 42, 5), print_tail(f));
	#else
		assert(f->status == 0x010f, "%x", f->status);
		assert(check_tail(f, 22, 35, 41, 5), print_tail(f));
	#endif

	gaba_dp_clean(d);
}

/* with deletions */
unittest(with_seq_pair("GACGTACGT", "ACGTACGT"))
{
	omajinai();

	struct gaba_fill_s *f = gaba_dp_fill_root(d, &s->afsec, 0, &s->bfsec, 0);
	assert(f->status == 0x01ff, "%x", f->status);
	f = gaba_dp_fill(d, f, &s->afsec, &s->bfsec);
	assert(f->status == 0x01f0, "%x", f->status);
	f = gaba_dp_fill(d, f, &s->afsec, &s->bftail);
	assert(f->status == 0x010f, "%x", f->status);

	f = gaba_dp_fill(d, f, &s->aftail, &s->bftail);
	#if MODEL == LINEAR
		assert(f->status == 0x01f0, "%x", f->status);
		assert(check_tail(f, 20, 37, 42, 4), print_tail(f));
	#else
		assert(f->status == 0x01ff, "%x", f->status);
		assert(check_tail(f, 20, 38, 43, 4), print_tail(f));
	#endif

	gaba_dp_clean(d);
}

/* with insertions */
unittest(with_seq_pair("ACGTACGT", "GACGTACGT"))
{
	omajinai();

	struct gaba_fill_s *f = gaba_dp_fill_root(d, &s->afsec, 0, &s->bfsec, 0);
	assert(f->status == 0x010f, "%x", f->status);
	f = gaba_dp_fill(d, f, &s->afsec, &s->bfsec);
	assert(f->status == 0x01f0, "%x", f->status);
	f = gaba_dp_fill(d, f, &s->afsec, &s->bfsec);
	assert(f->status == 0x010f, "%x", f->status);
	f = gaba_dp_fill(d, f, &s->aftail, &s->bfsec);
	assert(f->status == 0x01f0, "%x", f->status);


	f = gaba_dp_fill(d, f, &s->aftail, &s->bftail);
	#if MODEL == LINEAR
		assert(f->status == 0x010f, "%x", f->status);
		assert(check_tail(f, 20, 35, 41, 5), print_tail(f));
	#else
		assert(f->status == 0x010f, "%x", f->status);
		assert(check_tail(f, 20, 36, 42, 5), print_tail(f));
	#endif

	gaba_dp_clean(d);
}

/* print_cigar test */
static
int ut_sprintf(
	void *pbuf,
	char const *fmt,
	...)
{
	va_list l;
	va_start(l, fmt);

	char *buf = *((char **)pbuf);
	int len = vsprintf(buf, fmt, l);
	*((char **)pbuf) += len;
	va_end(l);
	return(len);
}

unittest()
{
	char *buf = (char *)malloc(16384);
	char *p = buf;
	gaba_dp_print_cigar(ut_sprintf, (void *)&p, (uint32_t const []){ 0x55555555, 0, 0 }, 0, 32);
	assert(strcmp(buf, "16M") == 0, "%s", buf);

	p = buf;
	gaba_dp_print_cigar(ut_sprintf, (void *)&p, (uint32_t const []){ 0x55555555, 0x55555555, 0, 0 }, 0, 64);
	assert(strcmp(buf, "32M") == 0, "%s", buf);

	p = buf;
	gaba_dp_print_cigar(ut_sprintf, (void *)&p, (uint32_t const []){ 0x55555555, 0x55555555, 0x55555555, 0x55555555, 0, 0 }, 0, 128);
	assert(strcmp(buf, "64M") == 0, "%s", buf);

	p = buf;
	gaba_dp_print_cigar(ut_sprintf, (void *)&p, (uint32_t const []){ 0x55550000, 0x55555555, 0x55555555, 0x55555555, 0, 0 }, 16, 112);
	assert(strcmp(buf, "56M") == 0, "%s", buf);

	p = buf;
	gaba_dp_print_cigar(ut_sprintf, (void *)&p, (uint32_t const []){ 0x55555000, 0x55555555, 0x55555555, 0x55555555, 0, 0 }, 12, 116);
	assert(strcmp(buf, "58M") == 0, "%s", buf);

	p = buf;
	gaba_dp_print_cigar(ut_sprintf, (void *)&p, (uint32_t const []){ 0x55, 0, 0 }, 0, 8);
	assert(strcmp(buf, "4M") == 0, "%s", buf);

	p = buf;
	gaba_dp_print_cigar(ut_sprintf, (void *)&p, (uint32_t const []){ 0x55555000, 0x55555555, 0x55555555, 0x55 }, 12, 92);
	assert(strcmp(buf, "46M") == 0, "%s", buf);

	p = buf;
	gaba_dp_print_cigar(ut_sprintf, (void *)&p, (uint32_t const []){ 0x55550555, 0, 0 }, 0, 32);
	assert(strcmp(buf, "6M4D8M") == 0, "%s", buf);

	p = buf;
	gaba_dp_print_cigar(ut_sprintf, (void *)&p, (uint32_t const []){ 0x5555f555, 0, 0 }, 0, 32);
	assert(strcmp(buf, "6M4I8M") == 0, "%s", buf);

	p = buf;
	gaba_dp_print_cigar(ut_sprintf, (void *)&p, (uint32_t const []){ 0xaaaa0555, 0, 0 }, 0, 33);
	assert(strcmp(buf, "6M5D8M") == 0, "%s", buf);

	p = buf;
	gaba_dp_print_cigar(ut_sprintf, (void *)&p, (uint32_t const []){ 0xaaabf555, 0, 0 }, 0, 33);
	assert(strcmp(buf, "6M5I8M") == 0, "%s", buf);

	p = buf;
	gaba_dp_print_cigar(ut_sprintf, (void *)&p, (uint32_t const []){ 0xaaabf555, 0xaaaa0556, 0, 0 }, 0, 65);
	assert(strcmp(buf, "6M5I8M1I5M5D8M") == 0, "%s", buf);

	p = buf;
	gaba_dp_print_cigar(ut_sprintf, (void *)&p, (uint32_t const []){ 0xaaabf555, 0xaaaa0556, 0xaaaaaaaa, 0 }, 0, 65);
	assert(strcmp(buf, "6M5I8M1I5M5D8M") == 0, "%s", buf);

	p = buf;
	gaba_dp_print_cigar(ut_sprintf, (void *)&p, (uint32_t const []){ 0xaaabf554, 0xaaaa0556, 0xaaaaaaaa, 0 }, 0, 65);
	assert(strcmp(buf, "2D5M5I8M1I5M5D8M") == 0, "%s", buf);

	free(buf);
}

/* dump_cigar test */
unittest()
{
	uint64_t const len = 16384;
	char *buf = (char *)malloc(len);

	gaba_dp_dump_cigar(buf, len, (uint32_t const []){ 0x55555555, 0, 0 }, 0, 32);
	assert(strcmp(buf, "16M") == 0, "%s", buf);

	gaba_dp_dump_cigar(buf, len, (uint32_t const []){ 0x55555555, 0x55555555, 0, 0 }, 0, 64);
	assert(strcmp(buf, "32M") == 0, "%s", buf);

	gaba_dp_dump_cigar(buf, len, (uint32_t const []){ 0x55555555, 0x55555555, 0x55555555, 0x55555555, 0, 0 }, 0, 128);
	assert(strcmp(buf, "64M") == 0, "%s", buf);

	gaba_dp_dump_cigar(buf, len, (uint32_t const []){ 0x55550000, 0x55555555, 0x55555555, 0x55555555, 0, 0 }, 16, 112);
	assert(strcmp(buf, "56M") == 0, "%s", buf);

	gaba_dp_dump_cigar(buf, len, (uint32_t const []){ 0x55555000, 0x55555555, 0x55555555, 0x55555555, 0, 0 }, 12, 116);
	assert(strcmp(buf, "58M") == 0, "%s", buf);

	gaba_dp_dump_cigar(buf, len, (uint32_t const []){ 0x55, 0, 0 }, 0, 8);
	assert(strcmp(buf, "4M") == 0, "%s", buf);

	gaba_dp_dump_cigar(buf, len, (uint32_t const []){ 0x55555000, 0x55555555, 0x55555555, 0x55 }, 12, 92);
	assert(strcmp(buf, "46M") == 0, "%s", buf);

	gaba_dp_dump_cigar(buf, len, (uint32_t const []){ 0x55550555, 0, 0 }, 0, 32);
	assert(strcmp(buf, "6M4D8M") == 0, "%s", buf);

	gaba_dp_dump_cigar(buf, len, (uint32_t const []){ 0x5555f555, 0, 0 }, 0, 32);
	assert(strcmp(buf, "6M4I8M") == 0, "%s", buf);

	gaba_dp_dump_cigar(buf, len, (uint32_t const []){ 0xaaaa0555, 0, 0 }, 0, 33);
	assert(strcmp(buf, "6M5D8M") == 0, "%s", buf);

	gaba_dp_dump_cigar(buf, len, (uint32_t const []){ 0xaaabf555, 0, 0 }, 0, 33);
	assert(strcmp(buf, "6M5I8M") == 0, "%s", buf);

	gaba_dp_dump_cigar(buf, len, (uint32_t const []){ 0xaaabf555, 0xaaaa0556, 0, 0 }, 0, 65);
	assert(strcmp(buf, "6M5I8M1I5M5D8M") == 0, "%s", buf);

	gaba_dp_dump_cigar(buf, len, (uint32_t const []){ 0xaaabf555, 0xaaaa0556, 0xaaaaaaaa, 0 }, 0, 65);
	assert(strcmp(buf, "6M5I8M1I5M5D8M") == 0, "%s", buf);

	gaba_dp_dump_cigar(buf, len, (uint32_t const []){ 0xaaabf554, 0xaaaa0556, 0xaaaaaaaa, 0 }, 0, 65);
	assert(strcmp(buf, "2D5M5I8M1I5M5D8M") == 0, "%s", buf);

	free(buf);
}

/**
 * check if gaba_dp_trace returns a correct path
 */
/* with empty sequences */
unittest(with_seq_pair("A", "A"))
{
	omajinai();

	/* fill sections */
	struct gaba_fill_s *f = gaba_dp_fill_root(d, &s->afsec, 0, &s->bfsec, 0);

	/* forward-only traceback */
	struct gaba_result_s *r = gaba_dp_trace(d, f, NULL, NULL);
	assert(check_result(r, 0, 0, 0, 0), print_result(r));

	/* forward-reverse traceback */
	r = gaba_dp_trace(d, f, f, NULL);
	assert(check_result(r, 0, 0, 0, 0), print_result(r));

	/* section added */
	f = gaba_dp_fill(d, f, &s->afsec, &s->bfsec);

	/* forward-only traceback */
	r = gaba_dp_trace(d, f, NULL, NULL);
	assert(check_result(r, 0, 0, 0, 0), print_result(r));

	/* forward-reverse traceback */
	r = gaba_dp_trace(d, f, f, NULL);
	assert(check_result(r, 0, 0, 0, 0), print_result(r));

	gaba_dp_clean(d);
}

/* with short sequences */
unittest(with_seq_pair("A", "A"))
{
	omajinai();

	/* fill sections */
	struct gaba_fill_s *f = gaba_dp_fill_root(d, &s->afsec, 0, &s->bfsec, 0);
	f = gaba_dp_fill(d, f, &s->afsec, &s->bfsec);
	f = gaba_dp_fill(d, f, &s->aftail, &s->bftail);

	/* forward-only traceback */
	struct gaba_result_s *r = gaba_dp_trace(d, f, NULL, NULL);
	assert(check_result(r, 4, 4, 28, 2), print_result(r));
	assert(check_path(r, "DRDR"), print_path(r));
	assert(check_cigar(r, "2M"), print_path(r));
	assert(check_section(r->sec[0], s->afsec, 0, 1, s->bfsec, 0, 1, 0, 2), print_section(r->sec[0]));
	assert(check_section(r->sec[1], s->afsec, 0, 1, s->bfsec, 0, 1, 2, 2), print_section(r->sec[1]));

	/* reverse-only traceback */
	r = gaba_dp_trace(d, NULL, f, NULL);
	assert(check_result(r, 4, 4, 28, 2), print_result(r));
	assert(check_path(r, "DRDR"), print_path(r));
	assert(check_cigar(r, "2M"), print_path(r));
	assert(check_section(r->sec[0], s->arsec, 0, 1, s->brsec, 0, 1, 0, 2), print_section(r->sec[0]));
	assert(check_section(r->sec[1], s->arsec, 0, 1, s->brsec, 0, 1, 2, 2), print_section(r->sec[1]));

	/* forward-reverse traceback */
	r = gaba_dp_trace(d, f, f, NULL);
	assert(check_result(r, 8, 8, 24, 4), print_result(r));
	assert(check_path(r, "DRDRDRDR"), print_path(r));
	assert(check_cigar(r, "4M"), print_path(r));
	assert(check_section(r->sec[0], s->arsec, 0, 1, s->brsec, 0, 1, 0, 2), print_section(r->sec[0]));
	assert(check_section(r->sec[1], s->arsec, 0, 1, s->brsec, 0, 1, 2, 2), print_section(r->sec[1]));
	assert(check_section(r->sec[2], s->afsec, 0, 1, s->bfsec, 0, 1, 4, 2), print_section(r->sec[2]));
	assert(check_section(r->sec[3], s->afsec, 0, 1, s->bfsec, 0, 1, 6, 2), print_section(r->sec[3]));

	gaba_dp_clean(d);
}

/* with longer sequences */
unittest(with_seq_pair("ACGTACGTACGT", "ACGTACGTACGT"))
{
	omajinai();

	/* fill sections */
	struct gaba_fill_s *f = gaba_dp_fill_root(d, &s->afsec, 0, &s->bfsec, 0);
	f = gaba_dp_fill(d, f, &s->afsec, &s->bfsec);
	f = gaba_dp_fill(d, f, &s->aftail, &s->bftail);

	/* fw */
	struct gaba_result_s *r = gaba_dp_trace(d, f, NULL, NULL);
	assert(check_result(r, 48, 48, 16, 2), print_result(r));
	assert(check_path(r, "DRDRDRDRDRDRDRDRDRDRDRDRDRDRDRDRDRDRDRDRDRDRDRDR"), print_path(r));
	assert(check_cigar(r, "24M"), print_path(r));
	assert(check_section(r->sec[0], s->afsec, 0, 12, s->bfsec, 0, 12, 0, 24), print_section(r->sec[0]));
	assert(check_section(r->sec[1], s->afsec, 0, 12, s->bfsec, 0, 12, 24, 24), print_section(r->sec[1]));

	/* rv */
	r = gaba_dp_trace(d, NULL, f, NULL);
	assert(check_result(r, 48, 48, 16, 2), print_result(r));
	assert(check_path(r, "DRDRDRDRDRDRDRDRDRDRDRDRDRDRDRDRDRDRDRDRDRDRDRDR"), print_path(r));
	assert(check_cigar(r, "24M"), print_path(r));
	assert(check_section(r->sec[0], s->arsec, 0, 12, s->brsec, 0, 12, 0, 24), print_section(r->sec[0]));
	assert(check_section(r->sec[1], s->arsec, 0, 12, s->brsec, 0, 12, 24, 24), print_section(r->sec[1]));

	/* fw-rv */
	r = gaba_dp_trace(d, f, f, NULL);
	assert(check_result(r, 96, 96, 0, 4), print_result(r));
	assert(check_path(r,
		"DRDRDRDRDRDRDRDRDRDRDRDRDRDRDRDRDRDRDRDRDRDRDRDR"
		"DRDRDRDRDRDRDRDRDRDRDRDRDRDRDRDRDRDRDRDRDRDRDRDR"),
		print_path(r));
	assert(check_cigar(r, "48M"), print_path(r));
	assert(check_section(r->sec[0], s->arsec, 0, 12, s->brsec, 0, 12, 0, 24), print_section(r->sec[0]));
	assert(check_section(r->sec[1], s->arsec, 0, 12, s->brsec, 0, 12, 24, 24), print_section(r->sec[1]));
	assert(check_section(r->sec[2], s->afsec, 0, 12, s->bfsec, 0, 12, 48, 24), print_section(r->sec[2]));
	assert(check_section(r->sec[3], s->afsec, 0, 12, s->bfsec, 0, 12, 72, 24), print_section(r->sec[3]));

	gaba_dp_clean(d);
}

/* sequences with different lengths (consumed as mismatches) */
unittest(with_seq_pair("GAAAAAAAA", "AAAAAAAA"))
{
	omajinai();

	struct gaba_fill_s *f = gaba_dp_fill_root(d, &s->afsec, 0, &s->bfsec, 0);
	f = gaba_dp_fill(d, f, &s->afsec, &s->bfsec);
	f = gaba_dp_fill(d, f, &s->afsec, &s->bftail);
	f = gaba_dp_fill(d, f, &s->aftail, &s->bftail);

	/* fw */	
	struct gaba_result_s *r = gaba_dp_trace(d, f, NULL, NULL);
	assert(check_result(r, 22, 32, 0, 3), print_result(r));
	assert(check_path(r, "DRDRDRDRDRDRDRDRDRDRDRDRDRDRDRDR"), print_path(r));
	assert(check_cigar(r, "16M"), print_path(r));
	assert(check_section(r->sec[0], s->afsec, 0, 8, s->bfsec, 0, 8, 0, 16), print_section(r->sec[0]));
	assert(check_section(r->sec[1], s->afsec, 8, 1, s->bfsec, 0, 1, 16, 2), print_section(r->sec[1]));
	assert(check_section(r->sec[2], s->afsec, 0, 7, s->bfsec, 1, 7, 18, 14), print_section(r->sec[2]));

	/* rv */
	r = gaba_dp_trace(d, NULL, f, NULL);
	assert(check_result(r, 22, 32, 0, 3), print_result(r));
	assert(check_path(r, "DRDRDRDRDRDRDRDRDRDRDRDRDRDRDRDR"), print_path(r));
	assert(check_cigar(r, "16M"), print_path(r));
	assert(check_section(r->sec[0], s->arsec, 2, 7, s->brsec, 0, 7, 0, 14), print_section(r->sec[0]));
	assert(check_section(r->sec[1], s->arsec, 0, 1, s->brsec, 7, 1, 14, 2), print_section(r->sec[1]));
	assert(check_section(r->sec[2], s->arsec, 1, 8, s->brsec, 0, 8, 16, 16), print_section(r->sec[2]));

	/* fw-rv */
	r = gaba_dp_trace(d, f, f, NULL);
	assert(check_result(r, 44, 64, 0, 6), print_result(r));
	assert(check_path(r,
		"DRDRDRDRDRDRDRDRDRDRDRDRDRDRDRDR"
		"DRDRDRDRDRDRDRDRDRDRDRDRDRDRDRDR"),
		print_path(r));
	assert(check_cigar(r, "32M"), print_path(r));
	assert(check_section(r->sec[0], s->arsec, 2, 7, s->brsec, 0, 7, 0, 14), print_section(r->sec[0]));
	assert(check_section(r->sec[1], s->arsec, 0, 1, s->brsec, 7, 1, 14, 2), print_section(r->sec[1]));
	assert(check_section(r->sec[2], s->arsec, 1, 8, s->brsec, 0, 8, 16, 16), print_section(r->sec[2]));
	assert(check_section(r->sec[3], s->afsec, 0, 8, s->bfsec, 0, 8, 32, 16), print_section(r->sec[3]));
	assert(check_section(r->sec[4], s->afsec, 8, 1, s->bfsec, 0, 1, 48, 2), print_section(r->sec[4]));
	assert(check_section(r->sec[5], s->afsec, 0, 7, s->bfsec, 1, 7, 50, 14), print_section(r->sec[5]));

	gaba_dp_clean(d);
}

/* another pair with different lengths */
unittest(with_seq_pair("TTTTTTTT", "CTTTTTTTT"))
{
	omajinai();

	struct gaba_fill_s *f = gaba_dp_fill_root(d, &s->afsec, 0, &s->bfsec, 0);
	f = gaba_dp_fill(d, f, &s->afsec, &s->bfsec);
	f = gaba_dp_fill(d, f, &s->afsec, &s->bfsec);
	f = gaba_dp_fill(d, f, &s->aftail, &s->bfsec);
	f = gaba_dp_fill(d, f, &s->aftail, &s->bftail);

	/* fw */
	struct gaba_result_s *r = gaba_dp_trace(d, f, NULL, NULL);
	assert(check_result(r, 22, 32, 0, 3), print_result(r));
	assert(check_path(r, "DRDRDRDRDRDRDRDRDRDRDRDRDRDRDRDR"), print_path(r));
	assert(check_cigar(r, "16M"), print_path(r));
	assert(check_section(r->sec[0], s->afsec, 0, 8, s->bfsec, 0, 8, 0, 16), print_section(r->sec[0]));
	assert(check_section(r->sec[1], s->afsec, 0, 1, s->bfsec, 8, 1, 16, 2), print_section(r->sec[1]));
	assert(check_section(r->sec[2], s->afsec, 1, 7, s->bfsec, 0, 7, 18, 14), print_section(r->sec[2]));

	/* rv */
	r = gaba_dp_trace(d, NULL, f, NULL);
	assert(check_result(r, 22, 32, 0, 3), print_result(r));
	assert(check_path(r, "DRDRDRDRDRDRDRDRDRDRDRDRDRDRDRDR"), print_path(r));
	assert(check_cigar(r, "16M"), print_path(r));
	assert(check_section(r->sec[0], s->arsec, 0, 7, s->brsec, 2, 7, 0, 14), print_section(r->sec[0]));
	assert(check_section(r->sec[1], s->arsec, 7, 1, s->brsec, 0, 1, 14, 2), print_section(r->sec[1]));
	assert(check_section(r->sec[2], s->arsec, 0, 8, s->brsec, 1, 8, 16, 16), print_section(r->sec[2]));

	/* fw-rv */
	r = gaba_dp_trace(d, f, f, NULL);
	assert(check_result(r, 44, 64, 0, 6), print_result(r));
	assert(check_path(r,
		"DRDRDRDRDRDRDRDRDRDRDRDRDRDRDRDR"
		"DRDRDRDRDRDRDRDRDRDRDRDRDRDRDRDR"), print_path(r));
	assert(check_cigar(r, "32M"), print_path(r));
	assert(check_section(r->sec[0], s->arsec, 0, 7, s->brsec, 2, 7, 0, 14), print_section(r->sec[0]));
	assert(check_section(r->sec[1], s->arsec, 7, 1, s->brsec, 0, 1, 14, 2), print_section(r->sec[1]));
	assert(check_section(r->sec[2], s->arsec, 0, 8, s->brsec, 1, 8, 16, 16), print_section(r->sec[2]));
	assert(check_section(r->sec[3], s->afsec, 0, 8, s->bfsec, 0, 8, 32, 16), print_section(r->sec[3]));
	assert(check_section(r->sec[4], s->afsec, 0, 1, s->bfsec, 8, 1, 48, 2), print_section(r->sec[4]));
	assert(check_section(r->sec[5], s->afsec, 1, 7, s->bfsec, 0, 7, 50, 14), print_section(r->sec[5]));

	gaba_dp_clean(d);
}

/* with deletions */
unittest(with_seq_pair("GACGTACGT", "ACGTACGT"))
{
	omajinai();

	struct gaba_fill_s *f = gaba_dp_fill_root(d, &s->afsec, 0, &s->bfsec, 0);
	f = gaba_dp_fill(d, f, &s->afsec, &s->bfsec);
	f = gaba_dp_fill(d, f, &s->afsec, &s->bftail);
	f = gaba_dp_fill(d, f, &s->aftail, &s->bftail);

	/* fw */
	struct gaba_result_s *r = gaba_dp_trace(d, f, NULL, NULL);
	assert(check_result(r, 20, 34, 30, 2), print_result(r));
	assert(check_path(r, "RDRDRDRDRDRDRDRDRRDRDRDRDRDRDRDRDR"), print_path(r));
	assert(check_cigar(r, "1D8M1D8M"), print_path(r));
	assert(check_section(r->sec[0], s->afsec, 0, 9, s->bfsec, 0, 8, 0, 17), print_section(r->sec[0]));
	assert(check_section(r->sec[1], s->afsec, 0, 9, s->bfsec, 0, 8, 17, 17), print_section(r->sec[1]));

	/* rv */
	r = gaba_dp_trace(d, NULL, f, NULL);
	assert(check_result(r, 20, 34, 30, 2), print_result(r));
	assert(check_path(r, "DRDRDRDRDRDRDRDRRDRDRDRDRDRDRDRDRR"), print_path(r));
	assert(check_cigar(r, "8M1D8M1D"), print_path(r));
	assert(check_section(r->sec[0], s->arsec, 0, 9, s->brsec, 0, 8, 0, 17), print_section(r->sec[0]));
	assert(check_section(r->sec[1], s->arsec, 0, 9, s->brsec, 0, 8, 17, 17), print_section(r->sec[1]));

	/* fw-rv */
	r = gaba_dp_trace(d, f, f, NULL);
	/* fixme!! continuous gaps at the root must be concatenated! */
	assert(check_result(r, 40, 68, 28, 4), print_result(r));
	assert(check_path(r,
		"DRDRDRDRDRDRDRDRRDRDRDRDRDRDRDRDRR"
		"RDRDRDRDRDRDRDRDRRDRDRDRDRDRDRDRDR"), print_path(r));
	assert(check_cigar(r, "8M1D8M2D8M1D8M"), print_path(r));
	assert(check_section(r->sec[0], s->arsec, 0, 9, s->brsec, 0, 8, 0, 17), print_section(r->sec[0]));
	assert(check_section(r->sec[1], s->arsec, 0, 9, s->brsec, 0, 8, 17, 17), print_section(r->sec[1]));
	assert(check_section(r->sec[2], s->afsec, 0, 9, s->bfsec, 0, 8, 34, 17), print_section(r->sec[2]));
	assert(check_section(r->sec[3], s->afsec, 0, 9, s->bfsec, 0, 8, 51, 17), print_section(r->sec[3]));

	gaba_dp_clean(d);
}

/* with insertions */
unittest(with_seq_pair("ACGTACGT", "GACGTACGT"))
{
	omajinai();

	struct gaba_fill_s *f = gaba_dp_fill_root(d, &s->afsec, 0, &s->bfsec, 0);
	f = gaba_dp_fill(d, f, &s->afsec, &s->bfsec);
	f = gaba_dp_fill(d, f, &s->afsec, &s->bfsec);
	f = gaba_dp_fill(d, f, &s->aftail, &s->bfsec);
	f = gaba_dp_fill(d, f, &s->aftail, &s->bftail);

	/* fw */
	struct gaba_result_s *r = gaba_dp_trace(d, f, NULL, NULL);
	assert(check_result(r, 20, 34, 30, 2), print_result(r));
	assert(check_path(r, "DDRDRDRDRDRDRDRDRDDRDRDRDRDRDRDRDR"), print_path(r));
	assert(check_cigar(r, "1I8M1I8M"), print_path(r));
	assert(check_section(r->sec[0], s->afsec, 0, 8, s->bfsec, 0, 9, 0, 17), print_section(r->sec[0]));
	assert(check_section(r->sec[1], s->afsec, 0, 8, s->bfsec, 0, 9, 17, 17), print_section(r->sec[1]));

	/* rv */
	r = gaba_dp_trace(d, NULL, f, NULL);
	assert(check_result(r, 20, 34, 30, 2), print_result(r));
	assert(check_path(r, "DRDRDRDRDRDRDRDRDDRDRDRDRDRDRDRDRD"), print_path(r));
	assert(check_cigar(r, "8M1I8M1I"), print_path(r));
	assert(check_section(r->sec[0], s->arsec, 0, 8, s->brsec, 0, 9, 0, 17), print_section(r->sec[0]));
	assert(check_section(r->sec[1], s->arsec, 0, 8, s->brsec, 0, 9, 17, 17), print_section(r->sec[1]));

	/* fw-rv */
	r = gaba_dp_trace(d, f, f, NULL);
	assert(check_result(r, 40, 68, 28, 4), print_result(r));
	assert(check_path(r,
		"DRDRDRDRDRDRDRDRDDRDRDRDRDRDRDRDRD"
		"DDRDRDRDRDRDRDRDRDDRDRDRDRDRDRDRDR"), print_path(r));
	assert(check_cigar(r, "8M1I8M2I8M1I8M"), print_path(r));
	assert(check_section(r->sec[0], s->arsec, 0, 8, s->brsec, 0, 9, 0, 17), print_section(r->sec[0]));
	assert(check_section(r->sec[1], s->arsec, 0, 8, s->brsec, 0, 9, 17, 17), print_section(r->sec[1]));
	assert(check_section(r->sec[2], s->afsec, 0, 8, s->bfsec, 0, 9, 34, 17), print_section(r->sec[2]));
	assert(check_section(r->sec[3], s->afsec, 0, 8, s->bfsec, 0, 9, 51, 17), print_section(r->sec[3]));

	gaba_dp_clean(d);
}


/* cross tests */

/**
 * @struct unittest_naive_result_s
 * @brief result container
 */
struct unittest_naive_result_s {
	int32_t score;
	uint32_t path_length;
	int64_t apos, bpos;
	int64_t alen, blen;
	char *path;
};

/**
 * @fn unittest_naive_encode_a
 */
static inline
int8_t unittest_naive_encode(char a)
{
	return(0x03 & ((a>>1) ^ (a>>2)));
}

/**
 * @fn unittest_naive
 *
 * @brief naive implementation of the forward semi-global alignment algorithm
 * left-aligned gap and left-aligned deletion
 */
#if MODEL == LINEAR
static
struct unittest_naive_result_s unittest_naive(
	struct gaba_score_s const *sc,
	char const *a,
	char const *b)
{
	/* utils */
	#define _a(p, q, plen)		( (q) * ((plen) + 1) + (p) )
	#define s(p, q)		_a(p, (q), alen)
	#define m(p, q)		( \
		sc->score_sub \
			[unittest_naive_encode(a[(p) - 1])] \
			[unittest_naive_encode(b[(q) - 1])] \
	)

	/* load gap penalties */
	int8_t gia = -sc->score_gi_a;
	int8_t gib = -sc->score_gi_b;
	int8_t gea = -sc->score_ge_a;
	int8_t geb = -sc->score_ge_a;

	/* calc lengths */
	int64_t alen = strlen(a);
	int64_t blen = strlen(b);

	/* calc min */
	int8_t *v = (int8_t *)sc->score_sub;
	int64_t min = 0;
	for(int i = 0; i < 16; i++) {
		min = (v[i] < min) ? v[i] : min;
	}
	min = INT16_MIN - min - gia - gib;

	/* malloc matrix */
	int16_t *mat = (int16_t *)malloc(
		(alen + 1) * (blen + 1) * sizeof(int16_t));

	/* init */
	struct unittest_naive_maxpos_s {
		int16_t score;
		int64_t apos;
		int64_t bpos;
	};

	struct unittest_naive_maxpos_s max = { 0, 0, 0 };

	mat[s(0, 0)] = 0;
	for(int64_t i = 1; i < alen+1; i++) {
		mat[s(i, 0)] = MAX2(min, i * (gea + gia));
	}
	for(int64_t j = 1; j < blen+1; j++) {
		mat[s(0, j)] = MAX2(min, j * (geb + gib));
	}

	for(int64_t j = 1; j < blen+1; j++) {
		for(int64_t i = 1; i < alen+1; i++) {
			int16_t score = mat[s(i, j)] = MAX4(min,
				mat[s(i - 1, j - 1)] + m(i, j),
				mat[s(i - 1, j)] + gia + gea,
				mat[s(i, j - 1)] + gib + geb);
			if(score > max.score
			|| (score == max.score && (i + j) < (max.apos + max.bpos))) {
				max = (struct unittest_naive_maxpos_s){
					score, i, j
				};
			}
		}
	}
	if(max.score == 0) {
		max = (struct unittest_naive_maxpos_s){ 0, 0, 0 };
	}

	debug("max(%d), apos(%lld), bpos(%lld)", max.score, max.apos, max.bpos);

	struct unittest_naive_result_s result = {
		.score = max.score,
		.apos = max.apos,
		.bpos = max.bpos,
		.path_length = max.apos + max.bpos + 1,
		.path = (char *)malloc(max.apos + max.bpos + 1)
	};
	int64_t path_index = max.apos + max.bpos + 1;
	while(max.apos > 0 || max.bpos > 0) {
		debug("path_index(%llu), apos(%lld), bpos(%lld)", path_index, max.apos, max.bpos);
		/* M > I > D > X */
		if(max.bpos > 0
		&& mat[s(max.apos, max.bpos)] == mat[s(max.apos, max.bpos - 1)] + (geb + gib)) {
			max.bpos--;
			result.path[--path_index] = 'D';
		} else if(max.apos > 0
		&& mat[s(max.apos, max.bpos)] == mat[s(max.apos - 1, max.bpos)] + (gea + gia)) {
			max.apos--;
			result.path[--path_index] = 'R';
		} else {
			result.path[--path_index] = 'R';
			result.path[--path_index] = 'D';
			max.apos--;
			max.bpos--;
		}
	}

	result.alen = result.apos - max.apos;
	result.blen = result.bpos - max.bpos;

	result.path_length -= path_index;
	for(uint64_t i = 0; i < result.path_length; i++) {
		result.path[i] = result.path[path_index++];
	}
	result.path[result.path_length] = '\0';

	free(mat);

	#undef _a
	#undef s
	#undef m
	return(result);
}
#else /* MODEL == AFFINE */
static
struct unittest_naive_result_s unittest_naive(
	struct gaba_score_s const *sc,
	char const *a,
	char const *b)
{
	/* utils */
	#define _a(p, q, plen)		( (q) * ((plen) + 1) + (p) )
	#define s(p, q)		_a(p, 3*(q), alen)
	#define e(p, q)		_a(p, 3*(q)+1, alen)
	#define f(p, q)		_a(p, 3*(q)+2, alen)
	#define m(p, q)		( \
		sc->score_sub \
			[unittest_naive_encode(a[(p) - 1])] \
			[unittest_naive_encode(b[(q) - 1])] \
	)

	/* load gap penalties */
	int8_t gia = -sc->score_gi_a;
	int8_t gib = -sc->score_gi_b;
	int8_t gea = -sc->score_ge_a;
	int8_t geb = -sc->score_ge_a;

	/* calc lengths */
	int64_t alen = strlen(a);
	int64_t blen = strlen(b);

	/* calc min */
	int8_t *v = (int8_t *)sc->score_sub;
	int64_t min = 0;
	for(int i = 0; i < 16; i++) {
		min = (v[i] < min) ? v[i] : min;
	}
	min = INT16_MIN - min - gia - gib;

	/* malloc matrix */
	int16_t *mat = (int16_t *)malloc(
		3 * (alen + 1) * (blen + 1) * sizeof(int16_t));

	/* init */
	struct unittest_naive_maxpos_s {
		int16_t score;
		int64_t apos;
		int64_t bpos;
	};

	struct unittest_naive_maxpos_s max = { 0, 0, 0 };

	mat[s(0, 0)] = mat[e(0, 0)] = mat[f(0, 0)] = 0;
	for(int64_t i = 1; i < alen+1; i++) {
		mat[s(i, 0)] = mat[e(i, 0)] = MAX2(min, gia + i * gea);
		mat[f(i, 0)] = MAX2(min, gia + i * gea + gib - 1);
	}
	for(int64_t j = 1; j < blen+1; j++) {
		mat[s(0, j)] = mat[f(0, j)] = MAX2(min, gib + j * geb);
		mat[e(0, j)] = MAX2(min, gib + j * geb + gia - 1);
	}

	for(int64_t j = 1; j < blen+1; j++) {
		for(int64_t i = 1; i < alen+1; i++) {
			int16_t score_e = mat[e(i, j)] = MAX2(
				mat[s(i - 1, j)] + gia + gea,
				mat[e(i - 1, j)] + gea);
			int16_t score_f = mat[f(i, j)] = MAX2(
				mat[s(i, j - 1)] + gib + geb,
				mat[f(i, j - 1)] + geb);
			int16_t score = mat[s(i, j)] = MAX4(min,
				mat[s(i - 1, j - 1)] + m(i, j),
				score_e, score_f);
			if(score > max.score
			|| (score == max.score && (i + j) < (max.apos + max.bpos))) {
				max = (struct unittest_naive_maxpos_s){
					score, i, j
				};
			}
		}
	}
	if(max.score == 0) {
		max = (struct unittest_naive_maxpos_s){ 0, 0, 0 };
	}

	struct unittest_naive_result_s result = {
		.score = max.score,
		.apos = max.apos,
		.bpos = max.bpos,
		.path_length = max.apos + max.bpos + 1,
		.path = (char *)malloc(max.apos + max.bpos + 1)
	};
	int64_t path_index = max.apos + max.bpos + 1;
	while(max.apos > 0 || max.bpos > 0) {
		/* M > I > D > X */
		if(mat[s(max.apos, max.bpos)] == mat[f(max.apos, max.bpos)]) {
			max.bpos--;
			result.path[--path_index] = 'D';
		} else if(mat[s(max.apos, max.bpos)] == mat[e(max.apos, max.bpos)]) {
			max.apos--;
			result.path[--path_index] = 'R';
		} else {
			result.path[--path_index] = 'R';
			result.path[--path_index] = 'D';
			max.apos--;
			max.bpos--;
		}
	}

	result.alen = result.apos - max.apos;
	result.blen = result.bpos - max.bpos;

	result.path_length -= path_index;
	for(uint64_t i = 0; i < result.path_length; i++) {
		result.path[i] = result.path[path_index++];
	}
	result.path[result.path_length] = '\0';

	free(mat);

	#undef _a
	#undef s
	#undef e
	#undef f
	#undef m
	return(result);
}
#endif /* MODEL */

/**
 * @fn unittest_random_base
 */
static _force_inline
char unittest_random_base(void)
{
	char const table[4] = {'A', 'C', 'G', 'T'};
	return(table[rand() % 4]);
}

/**
 * @fn unittest_generate_random_sequence
 */
static
char *unittest_generate_random_sequence(
	int64_t len)
{
	char *seq;		/** a pointer to sequence */
	seq = (char *)malloc(sizeof(char) * (len + 1));

	if(seq == NULL) { return NULL; }
	for(int64_t i = 0; i < len; i++) {
		seq[i] = unittest_random_base();
	}
	seq[len] = '\0';
	return seq;
}

/**
 * @fn unittest_generate_mutated_sequence
 */
static
char *unittest_generate_mutated_sequence(
	char const *seq,
	double x,
	double d,
	int bw)
{
	int64_t wave = 0;			/** wave is q-coordinate of the alignment path */
	int64_t len = strlen(seq);
	char *mutated_seq;

	if(seq == NULL) { return NULL; }
	mutated_seq = (char *)malloc(sizeof(char) * (len + 1));
	if(mutated_seq == NULL) { return NULL; }
	for(int64_t i = 0, j = 0; i < len; i++) {
		if(((double)rand() / (double)RAND_MAX) < x) {
			mutated_seq[i] = unittest_random_base();	j++;	/** mismatch */
		} else if(((double)rand() / (double)RAND_MAX) < d) {
			if(rand() & 0x01 && wave > -bw+1) {
				mutated_seq[i] = (j < len) ? seq[j++] : unittest_random_base();
				j++; wave--;						/** deletion */
			} else if(wave < bw-2) {
				mutated_seq[i] = unittest_random_base();
				wave++;								/** insertion */
			} else {
				mutated_seq[i] = (j < len) ? seq[j++] : unittest_random_base();
			}
		} else {
			mutated_seq[i] = (j < len) ? seq[j++] : unittest_random_base();
		}
	}
	mutated_seq[len] = '\0';
	return mutated_seq;
}

/**
 * @fn unittest_add_tail
 */
char *unittest_add_tail(
	char *seq,
	char c,
	int64_t tail_len)
{
	int64_t len = strlen(seq);
	seq = realloc(seq, len + tail_len + 1);

	for(int64_t i = 0; i < tail_len; i++) {
		seq[len + i] = (c == 0) ? unittest_random_base() : c;
	}
	seq[len + tail_len] = '\0';
	return(seq);
}

/* test if the naive implementation is sane */
#define check_naive_result(_r, _score, _path) ( \
	   (_r).score == (_score) \
	&& strcmp((_r).path, (_path)) == 0 \
	&& (_r).path_length == strlen(_path) \
)
#define print_naive_result(_r) \
	"score(%lld), path(%s), len(%lld)", \
	(_r).score, (_r).path, (_r).path_length

static
char *string_pair_diff(
	char const *a,
	char const *b)
{
	int64_t len = 2 * (strlen(a) + strlen(b));
	char *base = malloc(len);
	char *ptr = base, *tail = base + len - 1;
	int64_t state = 0;

	#define push(ch) { \
		*ptr++ = (ch); \
		if(ptr == tail) { \
			base = realloc(base, 2 * len); \
			ptr = base + len; \
			tail = base + 2 * len; \
			len *= 2; \
		} \
	}
	#define push_str(str) { \
		for(int64_t i = 0; i < strlen(str); i++) { \
			push(str[i]); \
		} \
	}

	int64_t i;
	for(i = 0; i < MIN2(strlen(a), strlen(b)); i++) {
		if(state == 0 && a[i] != b[i]) {
			push_str("\x1b[31m"); state = 1;
		} else if(state == 1 && a[i] == b[i]) {
			push_str("\x1b[39m"); state = 0;
		}
		push(a[i]);
	}
	if(state == 1) { push_str("\x1b[39m"); state = 0; }
	for(; i < strlen(a); i++) { push(a[i]); }

	push('\n');
	for(int64_t i = 0; i < strlen(b); i++) {
		push(b[i]);
	}

	push('\0');
	return(base);
}
#define format_string_pair_diff(_a, _b) ({ \
	char *str = string_pair_diff(_a, _b); \
	char *copy = alloca(strlen(str) + 1); \
	strcpy(copy, str); \
	free(str); \
	copy; \
})
#define print_string_pair_diff(_a, _b)		"\n%s", format_string_pair_diff(_a, _b)

#if MODEL == LINEAR
unittest()
{
	struct gaba_context_s const *c = (struct gaba_context_s const *)gctx;
	struct gaba_score_s const *p = c->params.score_matrix;
	struct unittest_naive_result_s n;

	/* all matches */
	n = unittest_naive(p, "AAAA", "AAAA");
	assert(check_naive_result(n, 8, "DRDRDRDR"), print_naive_result(n));
	free(n.path);

	/* with deletions */
	n = unittest_naive(p, "TTTTACGTACGT", "TTACGTACGT");
	assert(check_naive_result(n, 8, "DRDRRRDRDRDRDRDRDRDRDR"), print_naive_result(n));
	free(n.path);

	/* with insertions */
	n = unittest_naive(p, "TTACGTACGT", "TTTTACGTACGT");
	assert(check_naive_result(n, 8, "DRDRDDDRDRDRDRDRDRDRDR"), print_naive_result(n));
	free(n.path);
}
#else /* MODEL == AFFINE */
unittest()
{
	struct gaba_context_s const *c = (struct gaba_context_s const *)gctx;
	struct gaba_score_s const *p = c->params.score_matrix;
	struct unittest_naive_result_s n;

	/* all matches */
	n = unittest_naive(p, "AAAA", "AAAA");
	assert(check_naive_result(n, 8, "DRDRDRDR"), print_naive_result(n));
	free(n.path);

	/* with deletions */
	n = unittest_naive(p, "TTTTACGTACGT", "TTACGTACGT");
	assert(check_naive_result(n, 13, "DRDRRRDRDRDRDRDRDRDRDR"), print_naive_result(n));
	free(n.path);

	/* with insertions */
	n = unittest_naive(p, "TTACGTACGT", "TTTTACGTACGT");
	assert(check_naive_result(n, 13, "DRDRDDDRDRDRDRDRDRDRDR"), print_naive_result(n));
	free(n.path);
}
#endif /* MODEL */


#if 1
/* cross test */
unittest()
{
	struct gaba_context_s const *c = (struct gaba_context_s const *)gctx;
	struct gaba_score_s const *p = c->params.score_matrix;

	/* seed rand */
	#ifndef SEED
	int32_t seed = getpid();
	#else
	int32_t seed = SEED;
	#endif
	srand(seed);

	// int64_t cross_test_count = 10000000;
	int64_t cross_test_count = 1000;
	for(int64_t i = 0; i < cross_test_count; i++) {
		/* generate sequences */
		char *a = unittest_generate_random_sequence(1000);
		char *b = unittest_generate_mutated_sequence(a, 0.1, 0.1, 500);

		/* add random sequences at the tail */
		a = unittest_add_tail(a, 0, 64);
		b = unittest_add_tail(b, 0, 64);

		/* add tail margin */
		a = unittest_add_tail(a, 'C', 20);
		b = unittest_add_tail(b, 'G', 20);


		/* naive */
		struct unittest_naive_result_s n = unittest_naive(p, a, b);


		/* build section */
		struct unittest_sections_s *sec = unittest_build_seqs(
			&((struct unittest_seqs_s){ .a = a, .b = b }));

		debug("seed(%d)\n%s", seed, format_string_pair_diff(a, b));

		/* generate dp context */
		struct gaba_dp_context_s *d = gaba_dp_init(c, sec->alim, sec->blim);

		/* fill section */
		struct gaba_section_s const *as = &sec->afsec;
		struct gaba_section_s const *bs = &sec->bfsec;
		struct gaba_fill_s *f = gaba_dp_fill_root(d, as, 0, bs, 0);
		struct gaba_fill_s *m = f;
		
		/* fill tail (1) */
		as = (f->status & GABA_STATUS_UPDATE_A) ? &sec->aftail : as;
		bs = (f->status & GABA_STATUS_UPDATE_B) ? &sec->bftail : bs;
		struct gaba_fill_s *t1 = gaba_dp_fill(d, f, as, bs);
		m = (t1->max > m->max) ? t1 : m;

		/* fill tail (2) */
		as = (t1->status & GABA_STATUS_UPDATE_A) ? &sec->aftail : as;
		bs = (t1->status & GABA_STATUS_UPDATE_B) ? &sec->bftail : bs;
		struct gaba_fill_s *t2 = gaba_dp_fill(d, t1, as, bs);
		m = (t2->max > m->max) ? t2 : m;

		/* check max */
		assert(m->max == n.score, "m->max(%lld), f(%lld, %u), t1->max(%lld, %u), t2->max(%lld, %u), n.score(%d)",
			m->max, f->max, f->status, t1->max, t1->status, t2->max, t2->status, n.score);
		if(m->max != n.score) {
			struct gaba_fill_s *f2 = gaba_dp_fill_root(d, &sec->afsec, 0, &sec->bfsec, 0);
			log("refill f2(%lld, %u)", f2->max, f2->status);
		}

		/* trace */
		struct gaba_result_s *r = gaba_dp_trace(d, m, NULL, NULL);

		/* check scores */
		assert(r->score == n.score, "m->max(%lld), r->score(%lld), n.score(%d)",
			m->max, r->score, n.score);
		assert(check_path(r, n.path), "\n%s\n%s",
			format_string_pair_diff(a, b),
			format_string_pair_diff(decode_path(r), n.path));

		debug("score(%lld, %d), alen(%lld), blen(%lld)\n%s",
			r->score, n.score, n.alen, n.blen,
			format_string_pair_diff(decode_path(r), n.path));

		/* cleanup */
		gaba_dp_clean(d);
		free(sec);
		free(n.path);
		free(a);
		free(b);
	}
}
#else
#if 0
"GACGCCTAATAATGTAGAAGACGGATCGTAACTTGATTGCTCGTTTTACTCATTGAGCAGCCTACAGAAATCGTGCCCCCCGACGGGACTGTAAGCCGTTAGGACATCGTTCTAGTCCTTACTTGCTCCGCCCCCGACGGGCGATCCAGCTCGGTAACAACAAACATAGTTGTACCATCTGCGAGTTTGAGACTTAAGCCGGACGCGCAAGCGGCTGGGCGGTTTAGAAGGATTTACTGACGATTTATATTCTATTCCACCAGTGCTTCAGCACTGGGCGCGTGGGATACGAACATTCAAAGCGGATACTATCCATAGTGAAGGGGCCGTGCTGGTTTAAAGAGGAGGCGTACTATATGTAGCTTGCTTTGCCGCGGGGGCAGCCAGAGGGAACCATGACAGGATAGACACCGGAGTCCCGATCAAACGTCTTCATAGTTTGCATTATCGTCAGCGTCTCGGATAGTCACAGTAGTTAAGTTACTTTCTTAGCACAATGTGTTGGAGTTGGCAATTCTCCTCAGGCCCAGTGATATCCCGGTGTTGACTAAAAGCTTGGAAACCGACTCGAACTCGTGGCCCCTCTGATTAATTCCAGCTAAGGAACGGTTTAGATGCCAAAGGGAGGATCCGTTCCAACACTGGTATACAATATACCGTCGCGACGACCTCGCGAACGGCTGAAAATATGTAGCTATGAGTCCTACTTGGGAGGGACCCAGATCACGCTGACAATTGATAGGGCTGTTGCATTACTCCCCGTCTCGACCTTCGGGGGATCCACCAGAACGTCCCCAGGCCTGAATCTCCAGTCGCTCTGAAGTAGACCCAATAGGTGAATAGGCCCCATTAGACAATAGCTCGCGAAATTTGGGAAACTGGACTCCGCGAGAATATCCGACGGTGACATTCCTATACCCGGAAGCTACCTAAAGCGTTGAGTGGACGTGACGGGCCGTCGCAATCTAAGATCAACCCTCCCGGTTTACGGACTAAGGCTAAAGACGAGTGATACCTACATTGGCGTACCCGGGCTGTGCACAGCTTTACTCGACTATCATCGCCCCCCCCCCCCCCCCCCCCC",
"GACGACCTAATAATGTACAAAACGGCTCGTATGCTAGATTCCCGTGTACTCATTGAAGCAGTCTACAGAAATCTTGCACCACCACGGGACTTACAAGCCGTCTAGGACATCGTTTCTAGTCCTTGACTTGCCCGCCCCCGACGAGCGATCAAGCTCGGTAACAACAAACATAGTTTCACCATCTAGCGACTTTGAGACTAATCCGGACGCGCAAGCGGCTGGGCGAGTTTAAGAGGATATACTGACGATTTATATTCCTTATTCCACCTAGTGATTCAGCACAGTGCGTGTGGGATCGAACATTCGAGCTTGATACTGTCCATAGTGAAGGGACCGTGACTCGTTTAAAGAGGAGGCGTACTATATGTTGCCTGTTGCCGCGGGGCAGCCAAGGGTCCCATGACACCATAAGACACGGAGTCCCGCCAAACGTCTTCATTGTTTGCATCTCGTCCGCGTCTCGGCATAGTCACTGTAGTTAAGTTACTTCTTAGCACATTTTGAGGTTGTGCAATTCTCCCTCGGCCCAGTGGTACCCGGGTTTGTGACTAATCAGCTTGGAAACCGACTCGAACTCGTGGCCCCTCTGATTAATTCCAGCTAAAGGAGACGGTTTAGATGCCCAAGGAGGATCCGTCCAACACTGGTATACAATAATACCGTAGCGACGACCCGCGACACGGATAAAATCTAGCTATGGTCCTACTTGGGAGGGACCCAATCACGGGACAATTGATAGGGCAGTTGCATTACTCCGCGTGACGCCTTCGGGGGATCCACCACAACTCCCCAGGCCTGAATCTCCAGTCGCTCTGAATTAGACCCAGTAGGCAATAGGCCCCGTTAGACAAATAGCTCGCGGAAAGTTTGGGATAACTGGAGCTCCGCGAGAATAACCGCGGTGACATTCCATATCCCGGAAGCTACCTTAAAGCCGCTGAGTGACTGTACGGGCCGTCGGCATCTGTACATCAACCCTCCCGGTTTACGGACTAAGGCTGAATGTCCCCGTAAGGCCTGTGCACTGCTCTTACTCTCCGGACTGTTTGCTCTTATAGTTGTTTGGGGGGGGGGGGGGGGGGGG"
"AACACGGCACCCACACAGTAGGCGCCGGACACTGGCGGCCATCGACCGCTGGCACTCTGTTGTGTCCGAATGTTTAGTCCAACTAAGCAGTCCGAGGTATTACCGATAAACGAGAAGGCTACGCCGTTTTCGAATCCGATGCGTCCGTGATGTGGTTTATACACGATATAGTACCCCCTGGATAGATTCCCCTCTTGGTTTTTCAGATCCTAGTCTATCTCAAGGCCTGGCCGATAGTGGTGTAATACGTCGGTGATAGAAGACGTTACATGAGAGTGATAAACTTCTTCGAGCCGCATGAACCGGGACACCAAGATAGAGAAAACTTGGATGCCGTTCGCGAGTCGTTGACCGTTGGCTACAAAATGGAGATCGGCATGATAGACGTCAGTGGGTCTTGTTCGGGAGGCGTAAGATAATTAGATGCAGATGTCCTATTCACGTAGGAGGCTAATGTGGATGAGAGCTTAGCGATGGTAGGTATGGTAAATTTGGAGTACGAGCCCACGTGAGATCGGTAGACAGCTAACGGGTTATTCCGTACCTGATAGTACGAAAGGAAGGCACGGGTGTGACCAGCCACAACTTTTCTCGACGCAGTCCTAGCCTGTGGCATCCCTGTAGAGTCGGCTCGAATAGCTGGAGTTGGTGTGTTCGCACCTTTAGCAGTCATAGTGCAGATAGATCGTTCGCGGTGCTCCCCACTTGCCCGTTCAGACCTTGGAGCGCACTTTATCAGGTTCCTGTCTGATGAGCTTGAGGTGTTTAAGTATCGAGTCCATTATAATTTGCGGTTCTGATCACCAGCATGCTCGGCCAGTCCCGTTAGGTGCTTAGTTTCTTGGGATCCTGTGCTAGCTCTATGGTCTTACATGGATGTGGTTGTAGCTGACTCTAGAGAGTCGATACTTGTAAGGGGTGGCTGAGCCCTCATTCGTACTCTAAGTGTCGTGGTGGTTTGAGCACATATTAGTAATGTTTTTAACGTTTTATGAACTGGGGAAGCTTGTTGTGGAAGTGGAGAGAGAACCTAAATCGGTCAGAATCGATGACACTATACACCACCCCCCCCCCCCCCCCCCCC",
"AACACGGCACACACACAGTAGGCGGCGGACACTATGGCGGCCCTCGACGGCTGGCACTCTGTTATGTCGGAATGTTTAGTCCAAACTAAGCAGTCCGAGGTATTCCGATCAACGCGAAGGGCTGCCCGTTTTCGAATCCGAGGCGTTCGTGATGTGTTTATACACGAATAGTACCCCTGGACAGTTCCCCTCTATGGTTTTCGGATCCTGTCTATCTACAAGGCCTGGGCCGTTGTGGTTATTACCTCGGTGATAGAAGACGTTACATGAGATGATAACACTCCTTCGCGCCGACAGGAACGGGGACAACAAGTAGATAAAACTTGGAAGCCGTTCGCGACGTCGTTGACCGTTGCCACAAAAATGGTAGATTGGCAGACAGACTCCGGGGTCTTGTTCGGGAGCGTAAGATAGTTAGATGTCAATTTTCCATTCACGAGGAGCCTAATGTGGATGAGAGTTTCGCGATGGTGGATGGTAAATTTGGCGTACGAGCCACGTGACATCGGTAGACAGCTAACATTTATTCCTACCGAAGTACGAAAGAGAAGGAGCGGTGTGACTCAGCCCAACTTTTCTCGAGCAGTCCTAGCTGTGGCATCTTGTAGAGTCGGATCCGAATAAGCTGGAGTTTGTGTGTTCGATCTTTCGCGCCAGTATTGCAGATAGAATCGTTCGACGGTGCTCCTCACTTGCCCGTCTCAACCTTGGAGCGCACTTTTTCAGGTTTGCTGTCTGATGAGCATGGAGGTGCTTAAGTATCAAGGCCTTATAATTTGCGGTGTCTAGTCACCGCAGGCTCAGTATCACCGTTAGGTGCTTGGTTTCTTGGATCTGGGCTAGCTCTAGGTCTTACACGGTTGTGGTTGTAGCTGAGCTCGATAGAGTCGATACTTGTACGGGGTGGCTGAGCCCTCATTCTTCTCTAGTGTCGTGGTGGTTTCAGCACATATTAGTAATGTTTTTAACGTCTTTAGTGAAATGTGTACCGAATCGAGCTGTGGATTCAATCCATATACAGTTTGGAGGAGATACCAGCCTCAGATTTTCAGGTATCTAAAGAAGGGGGGGGGGGGGGGGGGGG"
"GTTCACGGATACCCAGAGGATACATTCGTTTAACCTGAAAGATCATGCGGATTTGAATCGGTCAATTCCCTGTCGCCCTATACACGCCGCGTGCTAATTAGCATCCTGTGATGTTTTGTCGGGCCAGTATTTGCATGAAAACTATGGCATGGTCCCGGAAAGCGAATAAGCGTATAGCTACATAGGGAATAGGCCTTGGATCCTCGTCGGTCATTGCACCAAGCCGCATACAGCGTGATCTTAAAAGACATTAGAGCCCTCTAACACCCGAACGGTCCAGAAAGTTCATACGTAGATTCATGCAATGGCTAAAGTCCTACCATAACGCGAAAGAACGTTTGCTAAGGGTGAGCAGCGCATGTTAGTCTCCGCTAGATGTAAGACTCATGTTTTGTCGAGCATCTGAAAACATGACCCCATCAACTTTTGTTGGATTAATCGGAGAAGGTCATAAGAGGGTAGTTCATACTTACCCTCACAATAACCTTTAAGACCATCCTTGGCCTGTGGAATTCAGAACCAATTGTCACGTGCGCTTGATTAACACGATGGGAGCCTATCACTCGATGTCGCTTCAGTTGGGAGCAGGCGGGGAGCCTATACCGCTTGAGGCATCTAATACTGGCGACACCATCACTCGTTGAAGTTGGCCTGGCGGCTTCTTGTAATACATCAAAACATCGCAATTCCCTTTAGCGTGGCATCGTACATGACTATAACAAGTGCCAGTCGGGCTTGATCTAATCGGATGACGAAAGGTCCCCTTGCTCGTTAAATCGCTATTCTGGTGGTGCTCTCGCTCGGGCAGAGTCCTCCGAACGGACCCGCGATACCGTTCATCTTTAACTGGTCGTAATCCGAGGAACGCTAGTAGCGTTAGTCAAGACGACGGACATACTGTACACAGTGGAATATCCCTAGCCAATGTGAAAAAGTTCTTCAGTAGGCGGGTAGAAGCAGTACTAAATGGGCCGACCTGGGATGTTGTTTTCGTTTACCTGAGCCGGTTTTGAAGGAACGTGAACGCCACTATGTGGACCATCGTATATCCCAACACGTTTCCTCCCCCCCCCCCCCCCCCCCC",
"TTCAAAGGATACCAAGAAGATACAACCTTTAACCTGAAAGATCATGCGATTTGGATGGCTCAATTCCCTGTCCGCCCTATACACGCCCGCGTGCTATTAGCATCCTGTGAGGGTTTTGTACTCGGCCAGTATATGCATGAAAACTATGCATGGTACCGGACAAGCGAAGTAAGCGTTTGCTACTAGGAAGATTGGCCTTGGATCCTTGCTGTGACTGCACCATCCGCATGCAGCATGATCTCAATAAGACACTAGCCCCCTATAACACCCGAACGGCCAAAAGTTTCGTTAACGCAGATTCATGCCATGGTTAAAGCCTCCTACCAGAACGCGAAAGAACGTCTGCTAAGGGTGAGCAGCGCATGTTAGTCTCCGCTAGATGTAGTCTCATGTTTTGCGGAAGCATCGATACATGACCCCATAAACTTTTGTTGGATAATCGGAGAAGGTCAATAGAGGGTAGACTTAATTACCCACGACAATAACCTCTAAGACCATCCTTGGCCGTGGAATTCAAACAATTGTCACGTGCGCTTGATTAACACGATGGGAGCCGATACTCTATCGTGCCCTTCAGTTGGAGCGGGGGGGACCTATAGTCCCTTGAGGCATCTGAATAGTGGGACACCATCACTCTTGAAGTTGCCGTGGCGGGTTCCTTGTATACATAAAATCATCACAATTCCTTTAGCGTGGCTCGTACATGACTAAAAAAAGTAGCCAATCGCGGCTTGATCTAATCCGGATGACCAAACGTCCCCTTGCTCGTCTTAAATCGCTATTCGGTGGTGCCTCGCGTGCTGGCGAGTCCTCCTACACTGACCGCGATACCGTTCACTTTAACTAGTCGTATCGAGAGGAACGCTGTAGCGATTGTAAGACGAACGACATACTGTACCGTGAATATCCCGAAGCGTATCTGAAAAAGTTCTTCAGGAGCGGGTACGAACGCAGTACTAATCGCCCGACATGGGAGTTGTTTCAGTTTATCTTGTACGCCGAAGGAACAGGTCGTCGTGAAGTGTTGGAACTGCTAATCAATTAGGTACCGGGGAAGTGTAGTTGGGGGGGGGGGGGGGGGGGG"
"CCGCGTCTTCGCGGGCTTGGATCGCCAAGAGGCGTGTCCATGTTGAGCTGCTCCATAAGGGCGCGCGACTAGCCCACAGTTCTGCCCACCCCTGGGTTATTGCTGGCGTTCTGAATGTATAATGCTTAATCTCGCAGGGCCAACTGCAGAGGGCACTATCCTAATGGCGGTTGGTACCCTTTTTGTAGGTAGTGCAAACGCTTATTGAAGGTGAAAGTGTCGGGCTATATGCATTCGGGGAGATCCTCGTTCGGAGTGGTTTTCCGGGTCTGAAGAATCTCCCTATACCGACTTTCATTATTATCCTTATCACAGAGATTTGCAGCAAGTCTTGAGTTCGATCTATTGTCCGATATTTACCCTTTTAGATTTTGACGATTGAAGAGATTCCTTACAACAGTAGCCCGATCACACGACATAAAGAGATCCGAGTCGTGCGAGGTCGCCCTACGGATAACTGCTTCCATTCTCGCCCTCTCCGCCCCCATTTGCCGCTGTGGGAGCGAATTAGTAGTGTCGTCTGGGTATCCCAAATGTCTCAAGGTGCCGCGAATACCACAGAATGCAACAACGAGGTCGTGCGCGACCAAGAATTCCGACGTATTCCCGGACGTCCGCAAAATTCCATACCGAGCATCCCACACAGCCTGCCTCCGTATAAACCGATGTAGAAGCATTAAGTTCAGTGCGACCAGTTCTCTACTGGGGTCAGTGACCTGTCTGGGACTCTCCGAGCTCCCATAATAATTTCACCAATCTCTTCTGACTTGCAACCACTCTGGACAGTAGCTGAGCCAGCTTATTAGGTGCGATCTAACCTGGAAGGGTACTCCATGTGATATCCTACTCTAATTCAGGTTGGAGAGGGAATCTGGCGCTGCTCTTTTGTGCGCTCGTGGTGGACGGTCTTTAGACTATGTGCCTTATTCCGGGTGAAGACCCAGGGAAAAAACTAGGGCACGTGAGGGACCGCGAGGACCGTTATATACACACATAGGGGGAGCTGCTTATGGTAGTCCGCGATGTAAAGTTTCCGGATCTGTACCGCACCGACGGAATAGGTCCCCCCCCCCCCCCCCCCCCC",
"CCGCGGTCTTCGCGGCTTGGAGCGCCAAGAGGCAGTGTCATGATGAGCTGCCCCATAAGTTCGCTGAGCGTAGCACACAGTTCTAGCCCACCCTTGGGTTATTCCTCTGGCGTATTTGATGTATAATCCTTAATCTCGCAGGTAGTCACTGCAGTGGGCACTATCCTAATGGGGTTGGGACCCTTTTTGTAGGTAGTGCAAACGCGCTCTGTGAAGGTGTATGCTGTCGGCGATATGTATTCACGCGAGATCCTCGTGTCGGAGTGGTTTTCCGGGTCTGCAGATTTCCCTCATACTGCTTTCATTATTTTGCCTTATCACAGAATTCTGCAGCGAGTCTTGTGTTGATCTTTTGTCCTACAGTTACGTTATTAGTTTTGACGATTGAGATATTCATGACAACACGAGCCCGATCGAACGACATAAAGAGAGCCGGTCGTGCGGAGGTCTGCCCAGGGGATAACTGCTTCCCTTCTCGCCCTCTCCGCCCCCATTTGCCGTCTGTGTAGCGAATGGAATGTGCAGTCTGTGTATCCCAAATGTTTCAAGGTTCCCCGAATACCACAGAATCAACAAAGAGGTCTGCGCGACCAAGAATTCCGACGTATACCGGACGTCCGCAAAATTCCATACCGAGCATCCACACAGGCTGGCTCCGTATAAACCGATGTATAAGCATTAAGTTCAGTGCGACCAATTCTCTAATGGGGGACATACCAGTCTGGGACTTTCCCGAGCTCCCATACATCATTTCACCTATCTCGTCTGAGTTGCAACCACTCGAACAGTAGTGATTAGCTTAGTTGTGCGATCTTACCTGAATGGTACTCCATGTGTTGTCTACTCTAATTGAGCTTGGAGCGGGAACGGCGCTGCTGCTTTTCGGCGCCGGGGTGGGAACGGTCTTTAGACTAATGTGCCTTAATCCGGAGGGAGACCCAGGGAAAACAACTAAGGACGTGTGGACTGGAGACCGTTAATACCAATAGGGGGTGGACAGACATCCTCAGCTGCTTGGGACAGACTTTACAAAGGATTATTTTGCTCCGCGCCGACGCAAGCCGGGGGGGGGGGGGGGGGGGGG"
#endif
/* for debugging */
unittest(with_seq_pair(
"CCGCGTCTTCGCGGGCTTGGATCGCCAAGAGGCGTGTCCATGTTGAGCTGCTCCATAAGGGCGCGCGACTAGCCCACAGTTCTGCCCACCCCTGGGTTATTGCTGGCGTTCTGAATGTATAATGCTTAATCTCGCAGGGCCAACTGCAGAGGGCACTATCCTAATGGCGGTTGGTACCCTTTTTGTAGGTAGTGCAAACGCTTATTGAAGGTGAAAGTGTCGGGCTATATGCATTCGGGGAGATCCTCGTTCGGAGTGGTTTTCCGGGTCTGAAGAATCTCCCTATACCGACTTTCATTATTATCCTTATCACAGAGATTTGCAGCAAGTCTTGAGTTCGATCTATTGTCCGATATTTACCCTTTTAGATTTTGACGATTGAAGAGATTCCTTACAACAGTAGCCCGATCACACGACATAAAGAGATCCGAGTCGTGCGAGGTCGCCCTACGGATAACTGCTTCCATTCTCGCCCTCTCCGCCCCCATTTGCCGCTGTGGGAGCGAATTAGTAGTGTCGTCTGGGTATCCCAAATGTCTCAAGGTGCCGCGAATACCACAGAATGCAACAACGAGGTCGTGCGCGACCAAGAATTCCGACGTATTCCCGGACGTCCGCAAAATTCCATACCGAGCATCCCACACAGCCTGCCTCCGTATAAACCGATGTAGAAGCATTAAGTTCAGTGCGACCAGTTCTCTACTGGGGTCAGTGACCTGTCTGGGACTCTCCGAGCTCCCATAATAATTTCACCAATCTCTTCTGACTTGCAACCACTCTGGACAGTAGCTGAGCCAGCTTATTAGGTGCGATCTAACCTGGAAGGGTACTCCATGTGATATCCTACTCTAATTCAGGTTGGAGAGGGAATCTGGCGCTGCTCTTTTGTGCGCTCGTGGTGGACGGTCTTTAGACTATGTGCCTTATTCCGGGTGAAGACCCAGGGAAAAAACTAGGGCACGTGAGGGACCGCGAGGACCGTTATATACACACATAGGGGGAGCTGCTTATGGTAGTCCGCGATGTAAAGTTTCCGGATCTGTACCGCACCGACGGAATAGGTCCCCCCCCCCCCCCCCCCCCC",
"CCGCGGTCTTCGCGGCTTGGAGCGCCAAGAGGCAGTGTCATGATGAGCTGCCCCATAAGTTCGCTGAGCGTAGCACACAGTTCTAGCCCACCCTTGGGTTATTCCTCTGGCGTATTTGATGTATAATCCTTAATCTCGCAGGTAGTCACTGCAGTGGGCACTATCCTAATGGGGTTGGGACCCTTTTTGTAGGTAGTGCAAACGCGCTCTGTGAAGGTGTATGCTGTCGGCGATATGTATTCACGCGAGATCCTCGTGTCGGAGTGGTTTTCCGGGTCTGCAGATTTCCCTCATACTGCTTTCATTATTTTGCCTTATCACAGAATTCTGCAGCGAGTCTTGTGTTGATCTTTTGTCCTACAGTTACGTTATTAGTTTTGACGATTGAGATATTCATGACAACACGAGCCCGATCGAACGACATAAAGAGAGCCGGTCGTGCGGAGGTCTGCCCAGGGGATAACTGCTTCCCTTCTCGCCCTCTCCGCCCCCATTTGCCGTCTGTGTAGCGAATGGAATGTGCAGTCTGTGTATCCCAAATGTTTCAAGGTTCCCCGAATACCACAGAATCAACAAAGAGGTCTGCGCGACCAAGAATTCCGACGTATACCGGACGTCCGCAAAATTCCATACCGAGCATCCACACAGGCTGGCTCCGTATAAACCGATGTATAAGCATTAAGTTCAGTGCGACCAATTCTCTAATGGGGGACATACCAGTCTGGGACTTTCCCGAGCTCCCATACATCATTTCACCTATCTCGTCTGAGTTGCAACCACTCGAACAGTAGTGATTAGCTTAGTTGTGCGATCTTACCTGAATGGTACTCCATGTGTTGTCTACTCTAATTGAGCTTGGAGCGGGAACGGCGCTGCTGCTTTTCGGCGCCGGGGTGGGAACGGTCTTTAGACTAATGTGCCTTAATCCGGAGGGAGACCCAGGGAAAACAACTAAGGACGTGTGGACTGGAGACCGTTAATACCAATAGGGGGTGGACAGACATCCTCAGCTGCTTGGGACAGACTTTACAAAGGATTATTTTGCTCCGCGCCGACGCAAGCCGGGGGGGGGGGGGGGGGGGGG"))
{
	omajinai();
	struct gaba_score_s const *p = c->params.score_matrix;

	/* fill section */
	struct gaba_section_s const *as = &s->afsec;
	struct gaba_section_s const *bs = &s->bfsec;
	struct gaba_fill_s *f = gaba_dp_fill_root(d, as, 0, bs, 0);
	
	/* fill tail (1) */
	as = (f->status & GABA_STATUS_UPDATE_A) ? &s->aftail : as;
	bs = (f->status & GABA_STATUS_UPDATE_B) ? &s->bftail : bs;
	struct gaba_fill_s *t1 = gaba_dp_fill(d, f, as, bs);
	f = (t1->max > f->max) ? t1 : f;

	/* fill tail (2) */
	as = (f->status & GABA_STATUS_UPDATE_A) ? &s->aftail : as;
	bs = (f->status & GABA_STATUS_UPDATE_B) ? &s->bftail : bs;
	struct gaba_fill_s *t2 = gaba_dp_fill(d, t1, as, bs);
	f = (t2->max > f->max) ? t2 : f;
	struct gaba_result_s *r = gaba_dp_trace(d, f, NULL, NULL);

	/* naive */
	char const *a = (char const *)s->a;
	char const *b = (char const *)s->b;
	struct unittest_naive_result_s n = unittest_naive(p, a, b);

	/* check scores */
	assert(r->score == n.score, "f->max(%lld), r->score(%lld), n.score(%d)",
		f->max, r->score, n.score);
	assert(check_path(r, n.path), "\n%s\n%s",
		format_string_pair_diff(a, b),
		format_string_pair_diff(decode_path(r), n.path));

	debug("score(%lld, %d), alen(%lld), blen(%lld)\n%s",
		r->score, n.score, n.alen, n.blen,
		format_string_pair_diff(decode_path(r), n.path));

	/* cleanup */
	gaba_dp_clean(d);
}
#endif

#endif /* UNITTEST */

/**
 * end of gaba.c
 */
