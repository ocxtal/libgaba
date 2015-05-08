
/*
 * @file util.h
 *
 * @brief a collection of utilities
 */

#ifndef _UTIL_H_INCLUDED
#define _UTIL_H_INCLUDED

#include "sea.h"

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
 * function name composition macros.
 */

/**
 * @macro FUNC_WITH_SUFFIX
 * @brief an wrapper macro of JOIN2, for the use of function name composition.
 */
#define FUNC_WITH_SUFFIX(a,b)			JOIN2(a,b)

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
#define LABEL(file, label) 				FUNC_WITH_SUFFIX(file, label)

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

#endif /* #ifndef _UTIL_H_INCLUDED */

/*
 * end of util.h
 */
