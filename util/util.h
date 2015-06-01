
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
 * function declarations
 * naive, twig, branch, trunk, balloon, bulge, cap
 */
int32_t
naive_linear_dynamic(
	struct sea_context const *ctx,
	struct sea_process *proc);
int32_t
naive_affine_dynamic(
	struct sea_context const *ctx,
	struct sea_process *proc);
int32_t
naive_linear_guided(
	struct sea_context const *ctx,
	struct sea_process *proc);
int32_t
naive_affine_guided(
	struct sea_context const *ctx,
	struct sea_process *proc);

int32_t
twig_linear_dynamic(
	struct sea_context const *ctx,
	struct sea_process *proc);
int32_t
twig_affine_dynamic(
	struct sea_context const *ctx,
	struct sea_process *proc);
int32_t
twig_linear_guided(
	struct sea_context const *ctx,
	struct sea_process *proc);
int32_t
twig_affine_guided(
	struct sea_context const *ctx,
	struct sea_process *proc);

int32_t
branch_linear_dynamic(
	struct sea_context const *ctx,
	struct sea_process *proc);
int32_t
branch_affine_dynamic(
	struct sea_context const *ctx,
	struct sea_process *proc);
int32_t
branch_linear_guided(
	struct sea_context const *ctx,
	struct sea_process *proc);
int32_t
branch_affine_guided(
	struct sea_context const *ctx,
	struct sea_process *proc);

int32_t
trunk_linear_dynamic(
	struct sea_context const *ctx,
	struct sea_process *proc);
int32_t
trunk_affine_dynamic(
	struct sea_context const *ctx,
	struct sea_process *proc);
int32_t
trunk_linear_guided(
	struct sea_context const *ctx,
	struct sea_process *proc);
int32_t
trunk_affine_guided(
	struct sea_context const *ctx,
	struct sea_process *proc);

int32_t
balloon_linear_dynamic(
	struct sea_context const *ctx,
	struct sea_process *proc);
int32_t
balloon_affine_dynamic(
	struct sea_context const *ctx,
	struct sea_process *proc);
int32_t
balloon_linear_guided(
	struct sea_context const *ctx,
	struct sea_process *proc);
int32_t
balloon_affine_guided(
	struct sea_context const *ctx,
	struct sea_process *proc);

int32_t
bulge_linear_dynamic(
	struct sea_context const *ctx,
	struct sea_process *proc);
int32_t
bulge_affine_dynamic(
	struct sea_context const *ctx,
	struct sea_process *proc);
int32_t
bulge_linear_guided(
	struct sea_context const *ctx,
	struct sea_process *proc);
int32_t
bulge_affine_guided(
	struct sea_context const *ctx,
	struct sea_process *proc);

int32_t
cap_linear_dynamic(
	struct sea_context const *ctx,
	struct sea_process *proc);
int32_t
cap_affine_dynamic(
	struct sea_context const *ctx,
	struct sea_process *proc);
int32_t
cap_linear_guided(
	struct sea_context const *ctx,
	struct sea_process *proc);
int32_t
cap_affine_guided(
	struct sea_context const *ctx,
	struct sea_process *proc);

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
int64_t _init_ascii_f(uint8_t *p, int64_t pos);
int64_t _pushm_ascii_f(uint8_t *p, int64_t pos);
int64_t _pushx_ascii_f(uint8_t *p, int64_t pos);
int64_t _pushi_ascii_f(uint8_t *p, int64_t pos);
int64_t _pushd_ascii_f(uint8_t *p, int64_t pos);
int64_t _finish_ascii_f(uint8_t *p, int64_t pos);

/** reverse writer */
int64_t _init_ascii_r(uint8_t *p, int64_t pos);
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
int64_t _init_cigar_f(uint8_t *p, int64_t pos);
int64_t _pushm_cigar_f(uint8_t *p, int64_t pos);
int64_t _pushx_cigar_f(uint8_t *p, int64_t pos);
int64_t _pushi_cigar_f(uint8_t *p, int64_t pos);
int64_t _pushd_cigar_f(uint8_t *p, int64_t pos);
int64_t _finish_cigar_f(uint8_t *p, int64_t pos);

/** reverse writer */
int64_t _init_cigar_r(uint8_t *p, int64_t pos);
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
int64_t _init_dir_f(uint8_t *p, int64_t pos);
int64_t _pushm_dir_f(uint8_t *p, int64_t pos);
int64_t _pushx_dir_f(uint8_t *p, int64_t pos);
int64_t _pushi_dir_f(uint8_t *p, int64_t pos);
int64_t _pushd_dir_f(uint8_t *p, int64_t pos);
int64_t _finish_dir_f(uint8_t *p, int64_t pos);

/** reverse writer */
int64_t _init_dir_r(uint8_t *p, int64_t pos);
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
	TERM 	= 5
};


/**
 * sequence reader implementations
 */

/**
 * @macro rd_init
 * @brief initialize a sequence reader instance.
 */
#define rd_init(r, fp, base, sp) { \
	(r).p = (void *)base; \
	(r).spos = sp; \
	(r).b = 0; \
	(r).pop = fp; \
}

/**
 * @macro rd_fetch
 * @brief fetch a decoded base into r.b.
 */
#define rd_fetch(r, pos) { \
	(r).b = (r).pop((r).p, (r).spos + pos); \
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
#define wr_init(w, f, dir) { \
	(w).p = NULL; \
	(w).pos = (dir); \
	if((dir) == ALN_FW) { \
		(w).pushm = (f)->pushm_f; \
		(w).pushx = (f)->pushx_f; \
		(w).pushi = (f)->pushi_f; \
		(w).pushd = (f)->pushd_f; \
	} else { \
		(w).pushm = (f)->pushm_r; \
		(w).pushx = (f)->pushx_r; \
		(w).pushi = (f)->pushi_r; \
		(w).pushd = (f)->pushd_r; \
	} \
}

/**
 * @macro wr_alloc
 * @brief allocate a memory for the alignment string.
 */
#define wr_alloc(w, s, k) { \
	if((w).p != NULL) { \
		free((w).p); (w).p = NULL; \
	} \
	(w).size = sizeof(struct sea_result) + sizeof(uint8_t) * s; \
	(w).p = malloc((w).size); \
	if((w).pos == ALN_FW) { \
		(w).pos = (w).size; \
		(w).pos = (k).f->init_f((w).p, (w).pos); \
	} else { \
		(w).pos = sizeof(struct sea_result); \
		(w).pos = (k).f->init_r((w).p, (w).pos); \
	} \
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
#define wr_finish(w, ctx, dir) { \
	if((dir) == ALN_FW) { \
		(w).pos = (ctx)->f->finish_f((w).p, (w).pos); \
		(w).size = (w).size - (w).pos; \
	} else { \
		(w).pos = (ctx)->f->finish_r((w).p, (w).pos); \
		(w).size = (w).pos - sizeof(struct sea_result); \
		(w).pos = sizeof(struct sea_result); \
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
	LEFT 	= 0,
	TOP 	= 0x01
};

/**
 * @enum _DIR2
 */
enum _DIR2 {
	LL = (LEFT<<0) | (LEFT<<1),
	LT = (LEFT<<0) | (TOP<<1),
	TL = (TOP<<0) | (LEFT<<1),
	TT = (TOP<<0) | (TOP<<1)
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
