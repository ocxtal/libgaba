
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
#include <stdio.h>
#include <stdint.h>
#include <string.h>

/**
 * structs
 */
/**
 * @struct sea_joint_head
 *
 * @brief (internal) traceback start coordinate (coordinate at the beginning of the band) container
 * sizeof(struct sea_joint_head) == 16
 */
struct sea_joint_head {
	uint32_t p;					/** (4) trace start p-coordinate */
	uint32_t q;					/** (4) trace start q-coordinate */
	uint8_t *p_tail;			/** (8) tail of the previous section */
};

#define _head(pdp, member)		((struct sea_joint_head *)pdp)->member

/**
 * @struct sea_joint_tail
 *
 * @brief (internal) init vector container.
 * sizeof(struct sea_joint_tail) == 32
 */
struct sea_joint_tail {
	uint32_t psum;				/** (4) global p-coordinate of the tail */
	uint32_t p; 				/** (4) local p-coordinate of the tail */
	uint32_t mp, mq;			/** (8) max (p, q) */
	void *v;					/** (8) pointer to the initial vector (pv) */
	uint32_t size;				/** (4) size of section in bytes */
	uint8_t var;				/** (1) variant id */
	uint8_t d2;					/** (1) previous direction (copy of the last r.d2) */
	uint8_t _pad[2];			/** (2) */
};

#define _tail(pdp, member) 		((struct sea_joint_tail *)(pdp) - 1)->member
#define DEF_VEC_LEN				( 32 )		/** default vector length */

/**
 * @struct sea_chain_status
 * @brief pair of status and pdp
 */
struct sea_chain_status {
	uint8_t *pdp;
	int32_t stat;
};

/**
 * @struct sea_fill_section
 * @brief concatenated two section: [posa1, lima1) ~ [posa2, lima2) and [posb1, limb1) ~ [posb2, limb2)
 * sizeof(struct sea_fill_section) == 80
 */
struct sea_fill_section {
	uint64_t limp;				/** (8) */
	uint64_t _pad;				/** (8) */
	uint64_t posa1, posb1;		/** (16) */
	uint64_t posa2, posb2;		/** (16) */
	uint64_t lima1, limb1;		/** (16) */
	uint64_t lima2, limb2;		/** (16) */
};

/**
 * @struct sea_reader
 *
 * @brief (internal) abstract sequence reader
 * sizeof(struct sea_reader) == 16
 * sizeof(struct sea_reader_work) == 296
 */
struct sea_reader {
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
struct sea_reader_work {
	/** 64byte aligned */
	uint8_t bufa[64];			/** (64) */
	uint8_t _pad1[32];			/** (32) */
	uint64_t cnta, cntb;		/** (16) */
	struct sea_fill_section s;	/** (80) */
	/** 192, 192 */

	/** 64byte aligned */
	uint8_t bufb[64];			/** (64) */
	uint8_t _pad2[32];			/** (32) */
//	struct sea_align_pair p;	/** (32) */
	uint8_t *pa, *pb;			/** (16) */
	uint64_t alen, blen;		/** (16) */
	/** 128, 320 */
};

/**
 * @struct sea_writer
 *
 * @brief (internal) abstract sequence writer
 * sizeof(struct sea_writer) == 16
 * sizeof(struct sea_writer_work) == 24
 */
struct sea_writer {
	int32_t (*push)(			/** (8) */
		uint8_t *p,
		uint32_t pos,
		uint8_t c);
	uint8_t tag;				/** (1) */
	uint8_t dir;				/** (1) */
	uint8_t _pad[6];			/** (6) */
};
struct sea_writer_work {
	uint8_t *p;					/** (8) */
	uint32_t size;				/** (4) malloc size */
	uint32_t rpos;				/** (4) previous reverse head */
	uint32_t pos;				/** (4) current head */
	uint32_t len;				/** (4) length of the string */
};

/**
 * @struct sea_local_context
 *
 * @brief (internal) local constant container.
 * sizeof(struct sea_local_context) == 448
 */
struct sea_local_context {
	/** individually stored on init */
	/** 64byte aligned */
	uint8_t *stack_top;			/** (8) dynamic programming matrix */
	uint8_t *stack_end;			/** (8) the end of dp matrix */
	uint8_t *pdr;				/** (8) direction array */
	uint8_t *tdr;				/** (8) the end of direction array */
	struct sea_aln_funcs *fn;	/** (8) */
	struct sea_writer_work ll;	/** (24) */
	/** 64, 64 */

	/** loaded on init */
	/** 64byte aligned */
	struct sea_reader_work rr;	/** (320) */
	/** 320, 384 */

	/** 64byte aligned */
	int8_t m;					/** (1) match */
	int8_t x;					/** (1) mismatch */
	int8_t gi;					/** (1) gap open (in the affine-gap cost) or gap cost per base (in the linear-gap cost) */
	int8_t ge;					/** (1) gap extension */
	int32_t tx;					/** (4) xdrop threshold */
	/** 8, 392 */

	uint8_t _pad2[4];			/** (4) */
	int32_t max;				/** (4) (inout) current maximum score */
	uint8_t *m_tail;			/** (8) */
	/** 16, 408 */

	uint64_t size;				/** (8) malloc size */
	/** 8, 416 */

	struct sea_reader r;		/** (16) (in) sequence readers */
	struct sea_writer l;		/** (16) (inout) alignment writer */
	/** 32, 448 */

	/** 64byte aligned */
};
#define SEA_LOCAL_CONTEXT_LOAD_OFFSET	( offsetof(struct sea_local_context, rr.pa) )
#define SEA_LOCAL_CONTEXT_LOAD_SIZE		( sizeof(struct sea_local_context) - SEA_LOCAL_CONTEXT_LOAD_OFFSET )

/**
 * @struct sea_aln_func_fill, sea_aln_func_trace
 */
struct sea_aln_func_fill {
	struct sea_chain_status (*twig)(			/*!< diag 8bit */
		struct sea_local_context *this,
		uint8_t *pdp,
		struct sea_fill_section *sec);
	struct sea_chain_status (*branch)(			/*!< diag 16bit */
		struct sea_local_context *this,
		uint8_t *pdp,
		struct sea_fill_section *sec);
	struct sea_chain_status (*trunk)(			/*!< diag 32bit */
		struct sea_local_context *this,
		uint8_t *pdp,
		struct sea_fill_section *sec);
	struct sea_chain_status (*naive)(
		struct sea_local_context *this,
		uint8_t *pdp,
		struct sea_fill_section *sec);
};
struct sea_aln_func_trace {
	struct sea_chain_status (*twig)(			/*!< diag 8bit */
		struct sea_local_context *this,
		uint8_t *pdp);
	struct sea_chain_status (*branch)(			/*!< diag 16bit */
		struct sea_local_context *this,
		uint8_t *pdp);
	struct sea_chain_status (*trunk)(			/*!< diag 32bit */
		struct sea_local_context *this,
		uint8_t *pdp);
	struct sea_chain_status (*naive)(
		struct sea_local_context *this,
		uint8_t *pdp);
};

/**
 * @struct sea_aln_funcs
 */
struct sea_aln_funcs {
//	struct sea_aln_func_fill fill;		/** (32) */
//	struct sea_aln_func_trace trace;	/** (32) */
	struct sea_chain_status (*fill)(
		struct sea_local_context *this,
		uint8_t *pdp,
		struct sea_fill_section *sec)[4];
	struct sea_chain_status (*trace)(
		struct sea_local_context *this,
		uint8_t *pdp)[4];
};

#if 0
/**
 * @struct sea_pair_context
 * @brief forward / reverse template
 * sizeof(struct sea_pair_context) = 896
 */
struct sea_pair_context {
	struct sea_local_context kf;	/** (448) */
	struct sea_local_context kr;	/** (448) */
};
#endif

/**
 * @struct sea_flags
 */
union sea_flags {
	struct _sea_flags_st {
		uint32_t seq_a 			: 3;
		uint32_t seq_a_dir		: 2;
		uint32_t seq_b 			: 3;
		uint32_t seq_b_dir		: 2;
		uint32_t aln 			: 2;
		uint32_t _pad 			: 20;
	} sep;
	uint32_t all;
};

/**
 * @struct sea_context
 *
 * @brief (API) an algorithmic context.
 *
 * @sa sea_init, sea_close
 */
struct sea_context {
	/** templates */
	/** 64byte aligned */
	struct sea_local_context k;		/** (448) */
	/** 448, 448 */
	
	/** 64byte aligned */
	struct sea_joint_tail jt;		/** (32) */
	struct sea_joint_head jh;		/** (16) */
	/** flags */
	union sea_flags flags;			/** (4) a bitfield of option flags */
	uint8_t _pad1[12];				/** (12) */
	/** 64, 512 */

	/** joint vectors */
	/** 64byte aligned */
	uint8_t root_vec[512];			/** (512) */
	/** 512, 1024 */

	/** constants */
	/** 64byte aligned */
	struct sea_aln_funcs dynamic;	/** (64) */
	struct sea_aln_funcs guided;	/** (64) */
	/** 128, 1152 */

	/** 64byte aligned */
	struct sea_writer fw;			/** (24) */
	struct sea_writer rv;			/** (24) */
	uint8_t _pad2[16];				/** (16) */
	/** 64, 1216 */

	/** 64byte aligned */
};

/**
 * function declarations
 * naive, twig, branch, trunk
 */
struct sea_chain_status twig_linear_dynamic_fill(struct sea_local_context *this, uint8_t *pdp, struct sea_fill_section *sec);
struct sea_chain_status twig_affine_dynamic_fill(struct sea_local_context *this, uint8_t *pdp, struct sea_fill_section *sec);
struct sea_chain_status twig_linear_guided_fill(struct sea_local_context *this, uint8_t *pdp, struct sea_fill_section *sec);
struct sea_chain_status twig_affine_guided_fill(struct sea_local_context *this, uint8_t *pdp, struct sea_fill_section *sec);
struct sea_chain_status twig_linear_dynamic_trace(struct sea_local_context *this, uint8_t *pdp);
struct sea_chain_status twig_affine_dynamic_trace(struct sea_local_context *this, uint8_t *pdp);
struct sea_chain_status twig_linear_guided_trace(struct sea_local_context *this, uint8_t *pdp);
struct sea_chain_status twig_affine_guided_trace(struct sea_local_context *this, uint8_t *pdp);

struct sea_chain_status branch_linear_dynamic_fill(struct sea_local_context *this, uint8_t *pdp, struct sea_fill_section *sec);
struct sea_chain_status branch_affine_dynamic_fill(struct sea_local_context *this, uint8_t *pdp, struct sea_fill_section *sec);
struct sea_chain_status branch_linear_guided_fill(struct sea_local_context *this, uint8_t *pdp, struct sea_fill_section *sec);
struct sea_chain_status branch_affine_guided_fill(struct sea_local_context *this, uint8_t *pdp, struct sea_fill_section *sec);
struct sea_chain_status branch_linear_dynamic_trace(struct sea_local_context *this, uint8_t *pdp);
struct sea_chain_status branch_affine_dynamic_trace(struct sea_local_context *this, uint8_t *pdp);
struct sea_chain_status branch_linear_guided_trace(struct sea_local_context *this, uint8_t *pdp);
struct sea_chain_status branch_affine_guided_trace(struct sea_local_context *this, uint8_t *pdp);

struct sea_chain_status trunk_linear_dynamic_fill(struct sea_local_context *this, uint8_t *pdp, struct sea_fill_section *sec);
struct sea_chain_status trunk_affine_dynamic_fill(struct sea_local_context *this, uint8_t *pdp, struct sea_fill_section *sec);
struct sea_chain_status trunk_linear_guided_fill(struct sea_local_context *this, uint8_t *pdp, struct sea_fill_section *sec);
struct sea_chain_status trunk_affine_guided_fill(struct sea_local_context *this, uint8_t *pdp, struct sea_fill_section *sec);
struct sea_chain_status trunk_linear_dynamic_trace(struct sea_local_context *this, uint8_t *pdp);
struct sea_chain_status trunk_affine_dynamic_trace(struct sea_local_context *this, uint8_t *pdp);
struct sea_chain_status trunk_linear_guided_trace(struct sea_local_context *this, uint8_t *pdp);
struct sea_chain_status trunk_affine_guided_trace(struct sea_local_context *this, uint8_t *pdp);

struct sea_chain_status naive_linear_dynamic_fill(struct sea_local_context *this, uint8_t *pdp, struct sea_fill_section *sec);
struct sea_chain_status naive_affine_dynamic_fill(struct sea_local_context *this, uint8_t *pdp, struct sea_fill_section *sec);
struct sea_chain_status naive_linear_guided_fill(struct sea_local_context *this, uint8_t *pdp, struct sea_fill_section *sec);
struct sea_chain_status naive_affine_guided_fill(struct sea_local_context *this, uint8_t *pdp, struct sea_fill_section *sec);
struct sea_chain_status naive_linear_dynamic_trace(struct sea_local_context *this, uint8_t *pdp);
struct sea_chain_status naive_affine_dynamic_trace(struct sea_local_context *this, uint8_t *pdp);
struct sea_chain_status naive_linear_guided_trace(struct sea_local_context *this, uint8_t *pdp);
struct sea_chain_status naive_affine_guided_trace(struct sea_local_context *this, uint8_t *pdp);

/**
 * flags
 */
// #define SW 								( SEA_SW )
// #define NW 								( SEA_NW )
// #define SEA 							( SEA_SEA )
// #define XSEA 							( SEA_XSEA )

/**
 * coordinate conversion macros
 */
#define cox(p, q, band)				( ((p)>>1) - (q) )
#define coy(p, q, band)				( (((p)+1)>>1) + (q) )
#define cop(x, y, band)				( (x) + (y) )
#define coq(x, y, band) 			( ((y)-(x))>>1 )
// #define INSIDE(x, y, p, q, band)	( (COX(p, q, band) < (x)) && (COY(p, q, band) < (y)) )

/**
 * @enum _STATE
 */
enum _STATE {
	CONT 	= 0,
	MEM 	= 1,
	CHAIN 	= 2,
	TERM 	= 3
};


/**
 * direction determiner constants
 */
/**
 * @enum _DIR
 * @brief constants for direction flags.
 */
#if 0
enum _DIR {
	LEFT 	= SEA_UE_LEFT<<2,
	TOP 	= SEA_UE_TOP<<2
};

/**
 * @enum _DIR2
 */
enum _DIR2 {
	LL = (SEA_UE_LEFT<<0) | (SEA_UE_LEFT<<2),
	LT = (SEA_UE_LEFT<<0) | (SEA_UE_TOP<<2),
	TL = (SEA_UE_TOP<<0) | (SEA_UE_LEFT<<2),
	TT = (SEA_UE_TOP<<0) | (SEA_UE_TOP<<2)
};
#endif
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

#if 0
/**
 * char vector shift operations
 */
#define pushq(x, y) { \
	vec_char_shift_l(y, y); \
	vec_char_insert_lsb(y, x); \
}

#define pusht(x, y) { \
	vec_char_shift_r(y, y); \
	vec_char_insert_msb(y, x); \
}
#endif

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

#if 0
/**
 * @macro func_next
 */
#define func_next(k, ptr) ( \
	(k->f->twig == ptr) ? k->f->branch : k->f->trunk \
)

/**
 * @macro func_alt
 */
#define func_alt(k, ptr) ( \
	(k->f->balloon == ptr) ? k->f->trunk : k->f->balloon \
)

/* foreach */
#define _for(iter, cnt)		for(iter = 0; iter < cnt; iter++)

/* boolean */
#define TRUE 			( 1 )
#define FALSE 			( 0 )
#endif

/**
 * max and min
 */
#define MAX2(x,y) 		( (x) > (y) ? (x) : (y) )
#define MAX3(x,y,z) 	( MAX2(x, MAX2(y, z)) )
#define MAX4(w,x,y,z) 	( MAX2(MAX2(w, x), MAX2(y, z)) )

#define MIN2(x,y) 		( (x) < (y) ? (x) : (y) )
#define MIN3(x,y,z) 	( MIN2(x, MIN2(y, z)) )
#define MIN4(w,x,y,z) 	( MIN2(MIN2(w, x), MIN2(y, z)) )

#if 0
/**
 * @macro XCUT
 * @brief an macro for xdrop termination.
 */
#define XCUT(d,m,x) 	( ((d) + (x) > (m) ? (d) : SEA_CELL_MIN )
#endif

#endif /* #ifndef _UTIL_H_INCLUDED */

/*
 * end of util.h
 */
