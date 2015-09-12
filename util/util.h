
/*
 * @file util.h
 *
 * @brief a collection of utilities
 */

#ifndef _UTIL_H_INCLUDED
#define _UTIL_H_INCLUDED

#include "../sea.h"
#include "log.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>

/**
 * structs
 */

/**
 * @struct sea_pos
 *
 * @brief (internal) holds a pair of (x, y)-coordinate and (p, q)-coordinate.
 */
struct sea_pos {
	int64_t i, j, p, q;
};

/**
 * @struct sea_mem
 *
 * @brief (internal) sequence reader
 */
struct sea_mem {
	uint8_t *sp, *ep;
};

/**
 * @struct sea_reader
 *
 * @brief (internal) abstract sequence reader
 * sizeof(struct sea_reader) == 24
 */
struct sea_reader {
	uint8_t *p;
	uint8_t (*pop)(
		uint8_t const *p,
		int64_t pos);
	uint8_t b;
	uint8_t _pad[7];
};

/**
 * @struct sea_writer
 *
 * @brief (internal) abstract sequence writer
 * sizeof(struct sea_writer) == 80
 */
struct sea_writer {
	uint8_t *p;
	int64_t pos, size, len;
	int64_t (*init)(
		uint8_t *ptr,
		int64_t fpos,
		int64_t rpos);
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
	int64_t (*finish)(
		uint8_t *ptr,
		int64_t pos);
//	uint8_t _pad[16];
};

/**
 * @struct sea_joint_head
 *
 * @brief (internal) traceback start coordinate (coordinate at the beginning of the band) container
 * sizeof(struct sea_joint_head) == 32
 */
struct sea_joint_head {
	int64_t p, q, i;			/*!< (24) path start coordinate */
	int32_t score;				/*!< (4) score at (p, q) */
	uint8_t _pad[4];			/*!< (4) padding */
};

#define _head(pdp, member)		((struct sea_joint_head *)pdp)->member

/**
 * @struct sea_joint_tail
 *
 * @brief (internal) init vector container.
 * sizeof(struct sea_joint_tail) == 32
 */
struct sea_joint_tail {
	int64_t p, i;				/*!< (16) terminal coordinate */
	uint8_t *v;					/*!< (8) pointer to the initial vector (pv) */
	uint8_t bpc;				/*!< (1) bit per cell (4 / 8 / 16 / 32) */
	uint8_t d2;					/*!< (1) previous direction (copy of the last r.d2) */
	uint8_t _pad[6];			/*!< (6) */
};

#define _tail(pdp, member) 		((struct sea_joint_tail *)(pdp) - 1)->member
#define DEF_VEC_LEN				( 32 )		/** default vector length */

/**
 * @struct sea_local_context
 *
 * @brief (internal) local constant container.
 * sizeof(struct sea_local_context) == 256
 */
struct sea_local_context {
	struct sea_aln_funcs const *f;

	int8_t m;					/*!< a dynamic programming cost: match */
	int8_t x;					/*!< a dynamic programming cost: mismatch */
	int8_t gi;					/*!< a dynamic programming cost: gap open (in the affine-gap cost) or gap cost per base (in the linear-gap cost) */
	int8_t ge;					/*!< a dynamic programming cost: gap extension */

	int8_t k;					/*!< heuristic search length stretch ratio. (default is k = 4) */
	int8_t bw;					/*!< the width of the band. */
	int16_t tx;					/*!< xdrop threshold. see sea_init for more details */
	int32_t min;				/*!< (in) lower bound of the score */
	int32_t alg;				/*!< algorithm flag (same as (ctx->flags) & SEA_FLAG_MASK_ALG) */

	uint8_t *pdp;				/*!< dynamic programming matrix */
	uint8_t *tdp;				/*!< the end of dp matrix */
	uint8_t *pdr;				/*!< direction array */
	uint8_t *tdr;				/*!< the end of direction array */
	int64_t asp, bsp;			/*!< the start position on the sequences */
	int64_t aep, bep;			/*!< the end position on the sequences */
	int64_t size;				/*!< default malloc size */

	int8_t do_trace;			/*!< do traceback if nonzero */
	uint8_t _pad[3];

	int32_t max;				/*!< (inout) current maximum score */
	int64_t mp, mq, mi;			/*!< maximum score position */

	struct sea_reader a, b;		/*!< (in) sequence readers */
	struct sea_writer l;		/*!< (inout) alignment writer */

//	uint8_t _pad2[24];
};

/**
 * @struct sea_aln_funcs
 *
 * @brief (internal) a struct which holds alignment function pointers.
 */
struct sea_aln_funcs {
	int32_t (*twig)(			/*!< diag 8bit */
		struct sea_local_context *this);
	int32_t (*branch)(			/*!< diag 16bit */
		struct sea_local_context *this);
	int32_t (*trunk)(			/*!< diag 32bit */
		struct sea_local_context *this);
	int32_t (*balloon)(			/*!< 32bit balloon algorithm */
		struct sea_local_context *this);
	int32_t (*bulge)(			/*!< 32bit bulge (guided balloon) algorithm */
		struct sea_local_context *this);
	int32_t (*cap)(				/*!< 32bit cap algorithm */
		struct sea_local_context *this);
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
	struct sea_local_context k;
	struct sea_joint_tail jt;
	int8_t pv[16];
	int8_t cv[16];
	/** constants */
	struct sea_writer fw, rv;
	struct sea_aln_funcs dynamic, guided;
	/** flags */
	int32_t flags;		/*!< a bitfield of option flags */
};

/**
 * function declarations
 * naive, twig, branch, trunk, balloon, bulge, cap
 */
int32_t naive_linear_dynamic(struct sea_local_context *this);
int32_t naive_affine_dynamic(struct sea_local_context *this);
int32_t naive_linear_guided(struct sea_local_context *this);
int32_t naive_affine_guided(struct sea_local_context *this);

int32_t twig_linear_dynamic(struct sea_local_context *this);
int32_t twig_affine_dynamic(struct sea_local_context *this);
int32_t twig_linear_guided(struct sea_local_context *this);
int32_t twig_affine_guided(struct sea_local_context *this);

int32_t branch_linear_dynamic(struct sea_local_context *this);
int32_t branch_affine_dynamic(struct sea_local_context *this);
int32_t branch_linear_guided(struct sea_local_context *this);
int32_t branch_affine_guided(struct sea_local_context *this);

int32_t trunk_linear_dynamic(struct sea_local_context *this);
int32_t trunk_affine_dynamic(struct sea_local_context *this);
int32_t trunk_linear_guided(struct sea_local_context *this);
int32_t trunk_affine_guided(struct sea_local_context *this);

int32_t balloon_linear_dynamic(struct sea_local_context *this);
int32_t balloon_affine_dynamic(struct sea_local_context *this);
int32_t balloon_linear_guided(struct sea_local_context *this);
int32_t balloon_affine_guided(struct sea_local_context *this);

int32_t bulge_linear_dynamic(struct sea_local_context *this);
int32_t bulge_affine_dynamic(struct sea_local_context *this);
int32_t bulge_linear_guided(struct sea_local_context *this);
int32_t bulge_affine_guided(struct sea_local_context *this);

int32_t cap_linear_dynamic(struct sea_local_context *this);
int32_t cap_affine_dynamic(struct sea_local_context *this);
int32_t cap_linear_guided(struct sea_local_context *this);
int32_t cap_affine_guided(struct sea_local_context *this);

/**
 * io functions
 */

/**
 * @fn _pop_ascii
 * @brief retrieve an ascii character from ((uint8_t *)p)[pos].
 * @detail implemented in `io.s'.
 */
uint8_t _pop_ascii(uint8_t const *p, int64_t pos);

/**
 * @fn _pop_4bit
 * @brief retrieve a 4-bit encoded base from ((uint8_t *)p)[pos].
 * @detail implemented in `io.s'.
 */
uint8_t _pop_4bit(uint8_t const *p, int64_t pos);

/**
 * @fn _pop_2bit
 * @brief retrieve a 2-bit encoded base from ((uint8_t *)p)[pos].
 * @detail implemented in `io.s'.
 */
uint8_t _pop_2bit(uint8_t const *p, int64_t pos);

/**
 * @fn _pop_4bit8packed
 * @brief retrieve a packed 4-bit encoded base from ((uint8_t *)p)[pos/2].
 * @detail implemented in `io.s'.
 */
uint8_t _pop_4bit8packed(uint8_t const *p, int64_t pos);

/**
 * @fn _pop_2bit8packed
 * @brief retrieve a packed 2-bit encoded base from ((uint8_t *)p)[pos/4].
 * @detail implemented in `io.s'.
 */
uint8_t _pop_2bit8packed(uint8_t const *p, int64_t pos);

/**
 * @fn _pushm_ascii_f, pushx_ascii_f, _pushi_ascii_f, _pushd_ascii_f
 * @brief push a match (mismatch, ins, del) character to p[pos]
 * @detail implemented in `io.s'.
 * @return the next pos.
 */
/** forward writer */
int64_t _init_ascii_f(uint8_t *p, int64_t fpos, int64_t rpos);
int64_t _pushm_ascii_f(uint8_t *p, int64_t pos);
int64_t _pushx_ascii_f(uint8_t *p, int64_t pos);
int64_t _pushi_ascii_f(uint8_t *p, int64_t pos);
int64_t _pushd_ascii_f(uint8_t *p, int64_t pos);
int64_t _finish_ascii_f(uint8_t *p, int64_t pos);

/** reverse writer */
int64_t _init_ascii_r(uint8_t *p, int64_t fpos, int64_t rpos);
int64_t _pushm_ascii_r(uint8_t *p, int64_t pos);
int64_t _pushx_ascii_r(uint8_t *p, int64_t pos);
int64_t _pushi_ascii_r(uint8_t *p, int64_t pos);
int64_t _pushd_ascii_r(uint8_t *p, int64_t pos);
int64_t _finish_ascii_r(uint8_t *p, int64_t pos);

/**
 * @fn _pushm_cigar_f, _pushx_cigar_f, _pushi_cigar_f, _pushd_cigar_f
 * @brief append a match (mismatch, ins, del) to p[pos].
 * @detail implemented in `io.s'.
 * @return the next pos.
 */
/** forward writer */
int64_t _init_cigar_f(uint8_t *p, int64_t fpos, int64_t rpos);
int64_t _pushm_cigar_f(uint8_t *p, int64_t pos);
int64_t _pushx_cigar_f(uint8_t *p, int64_t pos);
int64_t _pushi_cigar_f(uint8_t *p, int64_t pos);
int64_t _pushd_cigar_f(uint8_t *p, int64_t pos);
int64_t _finish_cigar_f(uint8_t *p, int64_t pos);

/** reverse writer */
int64_t _init_cigar_r(uint8_t *p, int64_t fpos, int64_t rpos);
int64_t _pushm_cigar_r(uint8_t *p, int64_t pos);
int64_t _pushx_cigar_r(uint8_t *p, int64_t pos);
int64_t _pushi_cigar_r(uint8_t *p, int64_t pos);
int64_t _pushd_cigar_r(uint8_t *p, int64_t pos);
int64_t _finish_cigar_r(uint8_t *p, int64_t pos);

/**
 * @fn _pushm_dir_f, _pushx_dir_f, _pushi_dir_f, _pushd_dir_f
 * @brief append a direction string to p[pos].
 * @detail implemented in `io.s'.
 * @return the next pos.
 */
/** forward writer */
int64_t _init_dir_f(uint8_t *p, int64_t fpos, int64_t rpos);
int64_t _pushm_dir_f(uint8_t *p, int64_t pos);
int64_t _pushx_dir_f(uint8_t *p, int64_t pos);
int64_t _pushi_dir_f(uint8_t *p, int64_t pos);
int64_t _pushd_dir_f(uint8_t *p, int64_t pos);
int64_t _finish_dir_f(uint8_t *p, int64_t pos);

/** reverse writer */
int64_t _init_dir_r(uint8_t *p, int64_t fpos, int64_t rpos);
int64_t _pushm_dir_r(uint8_t *p, int64_t pos);
int64_t _pushx_dir_r(uint8_t *p, int64_t pos);
int64_t _pushi_dir_r(uint8_t *p, int64_t pos);
int64_t _pushd_dir_r(uint8_t *p, int64_t pos);
int64_t _finish_dir_r(uint8_t *p, int64_t pos);

/**
 * flags
 */
#define SW 								( SEA_SW )
#define NW 								( SEA_NW )
#define SEA 							( SEA_SEA )
#define XSEA 							( SEA_XSEA )

/**
 * coordinate conversion macros
 */
#define cox(p, q, band)				( ((p)>>1) - (q) )
#define coy(p, q, band)				( (((p)+1)>>1) + (q) )
#define cop(x, y, band)				( (x) + (y) )
#define coq(x, y, band) 			( ((y)-(x))>>1 )
#define INSIDE(x, y, p, q, band)	( (COX(p, q, band) < (x)) && (COY(p, q, band) < (y)) )

/**
 * @enum _STATE
 */
enum _STATE {
	CONT 	= 0,
	MEM 	= 1,
	CHAIN 	= 2,
	ALT 	= 3,
	CAP 	= 4,
	TERM 	= 5,
	SEARCH  = 6
};


/**
 * direction determiner constants
 */
/**
 * @enum _DIR
 * @brief constants for direction flags.
 */
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
 * @macro XCUT
 * @brief an macro for xdrop termination.
 */
#define XCUT(d,m,x) 	( ((d) + (x) > (m) ? (d) : SEA_CELL_MIN )

/**
 * benchmark macros
 */
#ifdef BENCH
#include <sys/time.h>

/**
 * @struct _bench
 * @brief benchmark variable container
 */
struct _bench {
	struct timeval s;		/** start */
	int64_t a;				/** accumulator */
};
typedef struct _bench bench_t;

/**
 * @macro bench_init
 */
#define bench_init(b) { \
	memset(&(b).s, 0, sizeof(struct timeval)); \
	(b).a = 0; \
}

/**
 * @macro bench_start
 */
#define bench_start(b) { \
	gettimeofday(&(b).s, NULL); \
}

/**
 * @macro bench_end
 */
#define bench_end(b) { \
	struct timeval e; \
	gettimeofday(&e, NULL); \
	(b).a += ( (e.tv_sec  - (b).s.tv_sec ) * 1000000000 \
	         + (e.tv_usec - (b).s.tv_usec) * 1000); \
}

/**
 * @macro bench_get
 */
#define bench_get(b) ( \
	(b).a \
)

#else /* #ifdef BENCH */

/** disable bench */
struct _bench {};
typedef struct _bench bench_t;
#define bench_init(b) 		;
#define bench_start(b) 		;
#define bench_end(b)		;
#define bench_get(b)		( 0LL )

#endif /* #ifdef BENCH */

#endif /* #ifndef _UTIL_H_INCLUDED */

/*
 * end of util.h
 */
