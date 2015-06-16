
/*
 * @file util.h
 *
 * @brief a collection of utilities
 */

#ifndef _UTIL_H_INCLUDED
#define _UTIL_H_INCLUDED

#include "../include/sea.h"
#include "log.h"
#include <stdio.h>
#include <stdint.h>

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
	void *sp, *ep;
};

/**
 * @struct sea_reader
 *
 * @brief (internal) abstract sequence reader
 */
struct sea_reader {
	uint8_t *p;
	uint8_t (*pop)(
		uint8_t const *p,
		int64_t pos);
	uint8_t b;
};

/**
 * @struct sea_writer
 *
 * @brief (internal) abstract sequence writer
 */
struct sea_writer {
	uint8_t *p;
	int64_t pos, size;
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
};

/**
 * @struct sea_ivec
 *
 * @brief (internal) init vector container
 */
struct sea_ivec {
	void *cv, *pv;				/*!< pointers to the initial vectors (should be allocated in the stack by fixed length array, or alloca) */
	int8_t clen, plen;			/*!< the lengths of the vectors */
	int8_t size;				/*!< the size of a cell in the vector */
};

/**
 * @struct sea_coords
 * @brief (internal) a set of local-nonvolatile variables. (need write back / caller saved values)
 */
struct sea_coords {
	int32_t max;				/*!< (inout) current maximum score */
	int64_t mi, mj, mp, mq;		/*!< maximum score position */
	int64_t i, j, p, q;			/*!< temporary */
	struct sea_writer l;		/*!< (inout) alignment writer */
};

/**
 * @struct sea_process
 *
 * @brief (internal) a set of local-volatile variables. (no need to write back)
 */
struct sea_process {
	struct sea_ivec v;
	struct sea_reader a, b;		/*!< (in) sequence readers */
	struct sea_mem dp, dr;		/*!< (ref) a dynamic programming matrix */
	void *pdp;					/*!< dynamic programming matrix */
	uint8_t *pdr;				/*!< direction array */
	int64_t asp, bsp;			/*!< the start position on the sequences */
	int64_t aep, bep;			/*!< the end position on the sequences */
	int64_t alim, blim;			/*!< the limit coordinate of the band */
	int64_t size;				/*!< default malloc size */
};

/**
 * @struct sea_consts
 *
 * @brief (internal) local constant container.
 */
struct sea_consts {
	struct sea_aln_funcs const *f;

	int8_t m;			/*!< a dynamic programming cost: match */
	int8_t x;			/*!< a dynamic programming cost: mismatch */
	int8_t gi;			/*!< a dynamic programming cost: gap open (in the affine-gap cost) or gap cost per base (in the linear-gap cost) */
	int8_t ge;			/*!< a dynamic programming cost: gap extension */

	int8_t k;			/*!< heuristic search length stretch ratio. (default is k = 4) */
	int8_t bw;			/*!< the width of the band. */
	int16_t tx;			/*!< xdrop threshold. see sea_init for more details */
	int16_t tc;			/*!< chain threshold */
	int16_t tb; 		/*!< balloon termination threshold. */
	int32_t min;		/*!< (in) lower bound of the score */
	uint32_t alg;		/*!< algorithm flag (same as (ctx->flags) & SEA_FLAG_MASK_ALG) */

	size_t isize;		/*!< initial matsize */
	size_t memaln;		/*!< memory alignment size (default: 32) */
};

/**
 * @struct sea_aln_funcs
 *
 * @brief (internal) a struct which holds alignment function pointers.
 */
struct sea_aln_funcs {
	int32_t (*twig)(			/*!< diag 8bit */
		struct sea_consts const *ctx,
		struct sea_process *proc,
		struct sea_coords *co);
	int32_t (*branch)(			/*!< diag 16bit */
		struct sea_consts const *ctx,
		struct sea_process *proc,
		struct sea_coords *co);
	int32_t (*trunk)(			/*!< diag 32bit */
		struct sea_consts const *ctx,
		struct sea_process *proc,
		struct sea_coords *co);
	int32_t (*balloon)(			/*!< 32bit balloon algorithm */
		struct sea_consts const *ctx,
		struct sea_process *proc,
		struct sea_coords *co);
	int32_t (*bulge)(			/*!< 32bit bulge (guided balloon) algorithm */
		struct sea_consts const *ctx,
		struct sea_process *proc,
		struct sea_coords *co);
	int32_t (*cap)(				/*!< 32bit cap algorithm */
		struct sea_consts const *ctx,
		struct sea_process *proc,
		struct sea_coords *co);
};

/**
 * @struct sea_io_funcs
 * @brief (internal) a struct which holds io function pointers.
 */
struct sea_io_funcs {
	uint8_t (*popa)(			/*!< retrieve a character from sequence a */
		uint8_t const *ptr,
		int64_t pos);
	uint8_t (*popb)(
		uint8_t const *ptr,
		int64_t pos);
	int64_t (*init)(			/*!< forward alnwriter */
		uint8_t *ptr,
		int64_t fpos,
		int64_t rpos);
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
	int64_t (*finish)(
		uint8_t *ptr,
		int64_t pos);
};

/**
 * @struct sea_context
 *
 * @brief (API) an algorithmic context.
 *
 * @sa sea_init, sea_clean
 */
struct sea_context {
	struct sea_aln_funcs dynamic, guided;
	struct sea_io_funcs fw, rv;
	struct sea_ivec v;
	struct sea_consts k;
	uint32_t flags;		/*!< a bitfield of option flags */
};


/**
 * function declarations
 * naive, twig, branch, trunk, balloon, bulge, cap
 */
int32_t
naive_linear_dynamic(
	struct sea_consts const *ctx,
	struct sea_process *proc,
	struct sea_coords *co);
int32_t
naive_affine_dynamic(
	struct sea_consts const *ctx,
	struct sea_process *proc,
	struct sea_coords *co);
int32_t
naive_linear_guided(
	struct sea_consts const *ctx,
	struct sea_process *proc,
	struct sea_coords *co);
int32_t
naive_affine_guided(
	struct sea_consts const *ctx,
	struct sea_process *proc,
	struct sea_coords *co);

int32_t
twig_linear_dynamic(
	struct sea_consts const *ctx,
	struct sea_process *proc,
	struct sea_coords *co);
int32_t
twig_affine_dynamic(
	struct sea_consts const *ctx,
	struct sea_process *proc,
	struct sea_coords *co);
int32_t
twig_linear_guided(
	struct sea_consts const *ctx,
	struct sea_process *proc,
	struct sea_coords *co);
int32_t
twig_affine_guided(
	struct sea_consts const *ctx,
	struct sea_process *proc,
	struct sea_coords *co);

int32_t
branch_linear_dynamic(
	struct sea_consts const *ctx,
	struct sea_process *proc,
	struct sea_coords *co);
int32_t
branch_affine_dynamic(
	struct sea_consts const *ctx,
	struct sea_process *proc,
	struct sea_coords *co);
int32_t
branch_linear_guided(
	struct sea_consts const *ctx,
	struct sea_process *proc,
	struct sea_coords *co);
int32_t
branch_affine_guided(
	struct sea_consts const *ctx,
	struct sea_process *proc,
	struct sea_coords *co);

int32_t
trunk_linear_dynamic(
	struct sea_consts const *ctx,
	struct sea_process *proc,
	struct sea_coords *co);
int32_t
trunk_affine_dynamic(
	struct sea_consts const *ctx,
	struct sea_process *proc,
	struct sea_coords *co);
int32_t
trunk_linear_guided(
	struct sea_consts const *ctx,
	struct sea_process *proc,
	struct sea_coords *co);
int32_t
trunk_affine_guided(
	struct sea_consts const *ctx,
	struct sea_process *proc,
	struct sea_coords *co);

int32_t
balloon_linear_dynamic(
	struct sea_consts const *ctx,
	struct sea_process *proc,
	struct sea_coords *co);
int32_t
balloon_affine_dynamic(
	struct sea_consts const *ctx,
	struct sea_process *proc,
	struct sea_coords *co);
int32_t
balloon_linear_guided(
	struct sea_consts const *ctx,
	struct sea_process *proc,
	struct sea_coords *co);
int32_t
balloon_affine_guided(
	struct sea_consts const *ctx,
	struct sea_process *proc,
	struct sea_coords *co);

int32_t
bulge_linear_dynamic(
	struct sea_consts const *ctx,
	struct sea_process *proc,
	struct sea_coords *co);
int32_t
bulge_affine_dynamic(
	struct sea_consts const *ctx,
	struct sea_process *proc,
	struct sea_coords *co);
int32_t
bulge_linear_guided(
	struct sea_consts const *ctx,
	struct sea_process *proc,
	struct sea_coords *co);
int32_t
bulge_affine_guided(
	struct sea_consts const *ctx,
	struct sea_process *proc,
	struct sea_coords *co);

int32_t
cap_linear_dynamic(
	struct sea_consts const *ctx,
	struct sea_process *proc,
	struct sea_coords *co);
int32_t
cap_affine_dynamic(
	struct sea_consts const *ctx,
	struct sea_process *proc,
	struct sea_coords *co);
int32_t
cap_linear_guided(
	struct sea_consts const *ctx,
	struct sea_process *proc,
	struct sea_coords *co);
int32_t
cap_affine_guided(
	struct sea_consts const *ctx,
	struct sea_process *proc,
	struct sea_coords *co);

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
#define SEA 							( SEA_SEA )
#define XSEA 							( SEA_XSEA )
#define NW 								( SEA_NW )

/**
 * coordinate conversion macros
 */
#define	ADDR(p, q, band)			( (band)*(p)+(q)+(band)/2 )
#define ADDRI(x, y, band) 			( ADDR(COP(x, y, band), COQ(x, y, band), band) )
#define COX(p, q, band)				( ((p)>>1) - (q) )
#define COY(p, q, band)				( (((p)+1)>>1) + (q) )
#define COP(x, y, band)				( (x) + (y) )
#define COQ(x, y, band) 			( ((y)-(x))>>1 )
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
 * sequence reader implementations
 */

/**
 * @macro rd_init
 * @brief initialize a sequence reader instance.
 */
#define rd_init(r, fp, base) { \
	(r).p = (void *)base; \
	(r).b = 0; \
	(r).pop = fp; \
}

/**
 * @macro rd_fetch
 * @brief fetch a decoded base into r.b.
 */
#define rd_fetch(r, pos) { \
	(r).b = (r).pop((r).p, pos); \
}

/**
 * @macro rd_cmp
 * @brief compare two fetched bases.
 * @return true if two bases are the same, false otherwise.
 */
#define rd_cmp(r1, r2)	( (r1).b == (r2).b )

/**
 * @macro rd_decode
 * @brief get a cached char
 */
#define rd_decode(r)	( (r).b )
/**
 * @macro rd_close
 * @brief fill the instance with zero.
 */
#define rd_close(r) { \
	(r).p = NULL; \
	(r).spos = 0; \
	(r).b = 0; \
	(r).pop = NULL; \
}

/**
 * alignment writer implementations
 */

/**
 * @enum _ALN_CHAR
 * @brief alignment character, used in ascii output format.
 */
enum _ALN_CHAR {
	MCHAR = 'M',
	XCHAR = 'X',
	ICHAR = 'I',
	DCHAR = 'D'
};

/**
 * @enum _ALN_DIR
 * @brief the direction of alignment string.
 */
enum _ALN_DIR {
	ALN_FW = 0,
	ALN_RV = 1
};

/**
 * @macro wr_init
 * @brief initialize an alignment writer instance.
 */
#define wr_init(w, f) { \
	(w).p = NULL; \
	(w).init = (f).init; \
	(w).pushm = (f).pushm; \
	(w).pushx = (f).pushx; \
	(w).pushi = (f).pushi; \
	(w).pushd = (f).pushd; \
	(w).finish = (f).finish; \
}

/**
 * @macro wr_alloc
 * @brief allocate a memory for the alignment string.
 */
#define wr_alloc(w, s) { \
	if((w).p != NULL) { \
		free((w).p); (w).p = NULL; \
	} \
	(w).size = sizeof(sea_res_t) + sizeof(uint8_t) * (s); \
	(w).p = malloc((w).size); \
	(w).pos = (w).init((w).p, (w).size, sizeof(sea_res_t)); \
}

/**
 * @macro wr_pushm, wr_pushx, wr_pushi, wr_pushd
 * @brief push an alignment character
 */
#define wr_pushm(w) { \
	(w).pos = (w).pushm((w).p, (w).pos); \
	debug("pushm: %c, pos(%lld)", (w).p[(w).pos], (w).pos); \
}
#define wr_pushx(w) { \
	(w).pos = (w).pushx((w).p, (w).pos); \
	debug("pushx: %c, pos(%lld)", (w).p[(w).pos], (w).pos); \
}
#define wr_pushi(w) { \
	(w).pos = (w).pushi((w).p, (w).pos); \
	debug("pushi: %c, pos(%lld)", (w).p[(w).pos], (w).pos); \
}
#define wr_pushd(w) { \
	(w).pos = (w).pushd((w).p, (w).pos); \
	debug("pushd: %c, pos(%lld)", (w).p[(w).pos], (w).pos); \
}

/**
 * @macro wr_finish
 * @brief finish the instance (move the content of the array to the front)
 */
#define wr_finish(w) { \
	int64_t p = (w).finish((w).p, (w).pos); \
	if(p <= (w).pos) { \
		(w).size = (w).size - p - 1; \
		(w).pos = p; \
	} else { \
		(w).size = p - sizeof(sea_res_t) - 1; \
		(w).pos = sizeof(sea_res_t); \
	} \
}

/**
 * @macro wr_close
 * @brief free malloc'd memory and fill the instance with zero.
 */
#define wr_close(w) { \
	if((w).p != NULL) { \
		free((w).p); (w).p = NULL; \
	} \
	(w).pos = 0; \
	(w).pushm = (w).pushx = (w).pushi = (w).pushd = NULL; \
}

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
#define PUSHQ(x, y) { \
	VEC_CHAR_SHIFT_L(y, y); \
	VEC_CHAR_INSERT_LSB(y, x); \
}

#define PUSHT(x, y) { \
	VEC_CHAR_SHIFT_R(y, y); \
	VEC_CHAR_INSERT_MSB(y, x); \
}

/**
 * string concatenation macros
 */

/**
 * @macro JOIN2
 * @brief a macro which takes two string and concatenates them.
 */
#define JOIN2(i,j)						i##j

/**
 * @macro JOIN3
 * @brief a macro which takes three string and concatenates them.
 */
#define JOIN3(i,j,k)					i##j##k

/**
 * @macro JOIN4
 * @brief a macro which takes four string and concatenates them.
 */
#define JOIN4(i,j,k,l)					i##j##k##l

/**
 * @macro Q
 * @brief string quotation
 */
#define Q(a)							#a

/**
 * @macro QUOTE
 * @brief an wrapper of Q
 */
#define QUOTE(a)						Q(a)

/**
 * function name composition macros.
 */


/**
 * @macro HEADER_WITH_SUFFIX
 * @brief an wrapper macro of JOIN2
 */
#define HEADER_WITH_SUFFIX(a,b)			JOIN2(a,b)

/**
 * @macro FUNC_WITH_SUFFIX
 * @brief an wrapper macro of JOIN2, for the use of function name composition.
 */
#define FUNC_WITH_SUFFIX(a,b)			JOIN2(a,b)

/**
 * @macro LABEL_WITH_SUFFIX
 * @breif an wrapper of JOIN3
 */
#define LABEL_WITH_SUFFIX(a,b,c)		JOIN3(a,b,c)

/**
 * @macro DECLARE_FUNC
 * @brief a function declaration macro for static (local in a file) functions.
 */
#define DECLARE_FUNC(file, opt) 		static FUNC_WITH_SUFFIX(file, opt)

/**
 * @macro DECLARE_FUNC_GLOBAL
 * @brief a function declaration macro for global functions.
 */
#define DECLARE_FUNC_GLOBAL(file, opt)	FUNC_WITH_SUFFIX(file, opt)

/**
 * @macro CALL_FUNC
 * @brief a function call macro, which is an wrap of a function name composition macro.
 */
#define CALL_FUNC(file, opt)			FUNC_WITH_SUFFIX(file, opt)

/**
 * @macro func_next
 */
#define func_next(k, ptr) ( \
	(k.f->twig == ptr) ? k.f->branch : k.f->trunk \
)

/**
 * @macro func_alt
 */
#define func_alt(k, ptr) ( \
	(k.f->balloon == ptr) ? k.f->trunk : k.f->balloon \
)

/**
 * @macro coord_save_m
 * @brief save current (i, j)-coordinate to (mi, mj)
 */
#define coord_save_m(t) { \
	t.mi = t.i; t.mj = t.j; t.mp = t.p; t.mq = t.q; \
}

/**
 * @macro coord_load_m
 * @brief load current (mi, mj)-coordinate to (i, j)
 */
#define coord_load_m(t) { \
	t.i = t.mi; t.j = t.mj; t.p = t.mp; t.q = t.mq; \
}

/**
 * @macro LABEL
 * @brief a label declaration macro.
 */
#define LABEL(file, opt, label) 		LABEL_WITH_SUFFIX(file, opt, label)

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
 * @fn _read
 */
int32_t static inline
_read_impl(void *ptr, int64_t pos, size_t size)
{
	switch(size) {
		case 1: return((int32_t)(((int8_t *)ptr)[pos]));
		case 2: return((int32_t)(((int16_t *)ptr)[pos]));
		case 4: return((int32_t)(((int32_t *)ptr)[pos]));
		case 8: return((int32_t)(((int64_t *)ptr)[pos]));
		default: return 0;
	}
}

int32_t static inline
_read(void *ptr, int64_t pos, size_t size)
{
	int32_t r = _read_impl(ptr, pos, size);
	debug("_read: r(%d) at %p, %lld, %d", r, ptr, pos, (int32_t)size);
	return(r);
}

#endif /* #ifndef _UTIL_H_INCLUDED */

/*
 * end of util.h
 */
