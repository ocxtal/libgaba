
/*
 * @file util.h
 *
 * @brief a collection of utilities
 */

#ifndef _UTIL_H_INCLUDED
#define _UTIL_H_INCLUDED

#include "../include/sea.h"
#include <stdio.h>

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

/**
 * Constants representing algorithms
 *
 * Notice: This constants must be consistent with the sea_flags_alg in sea.h.
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
 * @macro wr_init
 * @brief initialize an alignment writer instance.
 */
#define wr_init(w, f) { \
	(w).p = NULL; \
	(w).pos = 0; \
	(w).pushm = (f)->pushm; \
	(w).pushx = (f)->pushx; \
	(w).pushi = (f)->pushi; \
	(w).pushd = (f)->pushd; \
}

/**
 * @macro wr_alloc
 * @brief allocate a memory for the alignment string.
 */
#define wr_alloc(w, s) { \
	if((w).p != NULL) { \
		free((w).p); (w).p = NULL; \
	} \
	(w).p = malloc(sizeof(struct sea_result) + sizeof(uint8_t) * s); \
	(w).pos = s; \
	(w).size = s; \
}

/**
 * @macro wr_pushm, wr_pushx, wr_pushi, wr_pushd
 * @brief push an alignment character
 */
#define wr_pushm(w) { \
	(w).pos = (w).pushm((w).p, (w).pos); \
}
#define wr_pushx(w) { \
	(w).pos = (w).pushx((w).p, (w).pos); \
}
#define wr_pushi(w) { \
	(w).pos = (w).pushi((w).p, (w).pos); \
}
#define wr_pushd(w) { \
	(w).pos = (w).pushd((w).p, (w).pos); \
}

/**
 * @macro wr_finish
 * @brief finish the instance (move the content of the array to the front)
 */
#define wr_finish(w) { \
	int64_t i, j; \
	for(i = (w).pos, j = 0; i < (w).size; i++, j++) { \
		(w).p[j] = (w).p[i]; \
	} \
	(w).p[j] = 0; \
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
 * @fn _pushm_ascii, pushx_ascii, _pushi_ascii, _pushd_ascii
 * @brief push a match (mismatch, ins, del) character to p[pos]
 * @detail implemented in `io.s'.
 * @return the next pos.
 */
int64_t _pushm_ascii(uint8_t *p, int64_t pos);
int64_t _pushx_ascii(uint8_t *p, int64_t pos);
int64_t _pushi_ascii(uint8_t *p, int64_t pos);
int64_t _pushd_ascii(uint8_t *p, int64_t pos);

/**
 * @fn _pushm_cigar, _pushx_cigar, _pushi_cigar, _pushd_cigar
 * @brief append a match (mismatch, ins, del) to p[pos].
 * @detail implemented in `io.s'.
 * @return the next pos.
 */
int64_t _pushm_cigar(uint8_t *p, int64_t pos);
int64_t _pushx_cigar(uint8_t *p, int64_t pos);
int64_t _pushi_cigar(uint8_t *p, int64_t pos);
int64_t _pushd_cigar(uint8_t *p, int64_t pos);

/**
 * @fn _pushm_dir, _pushx_dir, _pushi_dir, _pushd_dir
 * @brief append a direction string to p[pos].
 * @detail implemented in `io.s'.
 * @return the next pos.
 */
int64_t _pushm_dir(uint8_t *p, int64_t pos);
int64_t _pushx_dir(uint8_t *p, int64_t pos);
int64_t _pushi_dir(uint8_t *p, int64_t pos);
int64_t _pushd_dir(uint8_t *p, int64_t pos);

/**
 * @fn _read
 */
int32_t static inline
_read(void *ptr, int64_t pos, size_t size)
{
	switch(size) {
		case 1: return((int32_t)(((int8_t *)ptr)[pos]));
		case 2: return((int32_t)(((int16_t *)ptr)[pos]));
		case 4: return((int32_t)(((int32_t *)ptr)[pos]));
		case 8: return((int32_t)(((int64_t *)ptr)[pos]));
		default: return 0;
	}
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
	(k.f->twig == ptr) ? k.f->branch : ((k.f->trunk == ptr) ? k.f->balloon : k.f->trunk) \
)

/**
 * @macro func_alternative
 */
#define func_alternative(k, ptr) ( \
	(k.f->trunk == ptr) ? k.f->balloon : k.f->trunk \
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
 * color outputs
 */
#define RED(x)			"\x1b[31m" \
						x \
						"\x1b[39m"
#define GREEN(x)		"\x1b[32m" \
						x \
						"\x1b[39m"
#define YELLOW(x)		"\x1b[33m" \
						x \
						"\x1b[39m"
#define BLUE(x)			"\x1b[34m" \
						x \
						"\x1b[39m"
#define MAGENTA(x)		"\x1b[35m" \
						x \
						"\x1b[39m"
#define CYAN(x)			"\x1b[36m" \
						x \
						"\x1b[39m"
#define WHITE(x)		"\x1b[37m" \
						x \
						"\x1b[39m"

/**
 * @macro dbprintf
 */
#define dbprintf(fmt, ...) { \
	fprintf(stderr, fmt, __VA_ARGS__); \
}

/**
 * @macro debug
 */
#define debug(...) { \
	debug_impl(__VA_ARGS__, ""); \
}
#define debug_impl(fmt, ...) { \
	dbprintf("[%s] %s(%d) " fmt "%s\n", __FILE__, __func__, __LINE__, __VA_ARGS__); \
}

/**
 * @macro log
 */
#define log(...) { \
	log_impl(__VA_ARGS__, ""); \
}
#define log_impl(fmt, ...) { \
	dbprintf("[%s] " fmt "%s\n", __func__, __VA_ARGS__); \
}

#endif /* #ifndef _UTIL_H_INCLUDED */

/*
 * end of util.h
 */
