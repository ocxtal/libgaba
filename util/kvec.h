/* The MIT License

   Copyright (c) 2008, by Attractive Chaos <attractor@live.co.uk>

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
*/

/*
  An example:

#include "kvec.h"
int main() {
	kvec_t(int) array;
	kv_init(array);
	kv_push(int, array, 10); // append
	kv_a(int, array, 20) = 5; // dynamic
	kv_A(array, 20) = 4; // static
	kv_destroy(array);
	return 0;
}
*/

/*
  2015-0307

    * add packed vector. (Hajime Suzuki)

  2008-09-22 (0.1.0):

	* The initial version.

*/

#ifndef AC_KVEC_H
#define AC_KVEC_H

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define kv_roundup32(x) (--(x), (x)|=(x)>>1, (x)|=(x)>>2, (x)|=(x)>>4, (x)|=(x)>>8, (x)|=(x)>>16, ++(x))

#define _INIT 			( 64 )

/**
 * basic vectors (kv_*)
 */
#define kvec_t(type)    struct { uint64_t n, m; type *a; }
#define kv_init(v)      ( (v).n = 0, (v).m = _INIT, (v).a = calloc((v).m, sizeof(*(v).a)) )
#define kv_destroy(v)   { free((v).a); (v).a = NULL; }
// #define kv_A(v, i)      ( (v).a[(i)] )
#define kv_pop(v)       ( (v).a[--(v).n] )
#define kv_size(v)      ( (v).n )
#define kv_max(v)       ( (v).m )

#define kv_clear(v)		( (v).n = 0 )
#define kv_resize(v, s) ( \
	(v).m = (s), (v).a = realloc((v).a, sizeof(*(v).a) * (v).m) )

#define kv_reserve(v, s) ( \
	(v).m > (s) ? 0 : ((v).m = (s), (v).a = realloc((v).a, sizeof(*(v).a) * (v).m), 0) )

#define kv_copy(v1, v0) do {								\
		if ((v1).m < (v0).n) kv_resize(v1, (v0).n);			\
		(v1).n = (v0).n;									\
		memcpy((v1).a, (v0).a, sizeof(*(v).a) * (v0).n);	\
	} while (0)												\

#define kv_push(v, x) do {									\
		if ((v).n == (v).m) {								\
			(v).m = (v).m * 2;								\
			(v).a = realloc((v).a, sizeof(*(v).a) * (v).m);	\
		}													\
		(v).a[(v).n++] = (x);								\
	} while (0)

#define kv_pushp(v) ( \
	((v).n == (v).m) ?									\
	((v).m = (v).m * 2,									\
	 (v).a = realloc((v).a, sizeof(*(v).a) * (v).m), 0)	\
	: 0), ( (v).a + ((v).n++) )

#define kv_a(v, i) ( \
	((v).m <= (size_t)(i) ? \
	((v).m = (v).n = (i) + 1, kv_roundup32((v).m), \
	 (v).a = realloc((v).a, sizeof(*(v).a) * (v).m), 0) \
	: (v).n <= (size_t)(i) ? (v).n = (i) + 1 \
	: 0), (v).a[(i)])

/** bound-unchecked accessor */
#define kv_at(v, i) ( (v).a[(i)] )
#define kv_ptr(v)  ( (v).a )

/**
 * 2-bit packed vectors (kpv_*)
 * v.m must be multiple of kpv_elems(v).
 */
#define _BITS				( 2 )

/**
 * `sizeof(*((v).a)) * 8 / _BITS' is a number of packed elements in an array element.
 */
#define kpv_elems(v)    ( sizeof(*((v).a)) * 8 / _BITS )
#define kpv_base(v, i)  ( ((i) % kpv_elems(v)) * _BITS )
#define kpv_mask(v, e)  ( (e) & ((1<<_BITS) - 1) )

#define kpvec_t(type)       struct { uint64_t n, m; type *a; }
#define kpv_init(v)         ( (v).n = 0, (v).m = _INIT * kpv_elems(v), (v).a = calloc((v).m, sizeof(*(v).a)) )
#define kpv_destroy(v)      { free((v).a); (v).a = NULL; }

// #define kpv_A(v, i) ( kpv_mask(v, (v).a[(i) / kpv_elems(v)]>>kpv_base(v, i)) )
#define kpv_pop(v)  ( (v).n--, kpv_mask(v, (v).a[(v).n / kpv_elems(v)]>>kpv_base(v, (v).n)) )
#define kpv_size(v) ( (v).n )
#define kpv_max(v)  ( (v).m )

/**
 * the length of the array is ((v).m + kpv_elems(v) - 1) / kpv_elems(v)
 */
#define kpv_amax(v)     ( ((v).m + kpv_elems(v) - 1) / kpv_elems(v) )
#define kpv_asize(v)    ( ((v).n + kpv_elems(v) - 1) / kpv_elems(v) )

#define kpv_clear(v)	( (v).n = 0 )
#define kpv_resize(v, s) ( \
	(v).m = (s), (v).a = realloc((v).a, sizeof(*(v).a) * kpv_amax(v)) )

#define kpv_reserve(v, s) ( \
	(v).m > (s) ? 0 : ((v).m = (s), (v).a = realloc((v).a, sizeof(*(v).a) * kpv_amax(v)), 0) )

#define kpv_copy(v1, v0) do {							\
		if ((v1).m < (v0).n) kpv_resize(v1, (v0).n);	\
		(v1).n = (v0).n;								\
		memcpy((v1).a, (v0).a, kpv_amax(v));				\
	} while (0)											\
/*
#define kpv_push(v, x) do {								\
		if ((v).n == (v).m) {							\
			(v).a = realloc((v).a, 2 * sizeof(*(v).a) * kpv_amax(v)); \
			memset((v).a + kpv_amax(v), 0, kpv_amax(v));	\
			(v).m = (v).m * 2;							\
		}												\
		if(kpv_base(v, (v).n) == 0) { 					\
			(v).a[(v).n / kpv_elems(v)] = 0; 			\
		}												\
		(v).a[(v).n / kpv_elems(v)] |= kpv_mask(v, x)<<kpv_base(v, (v).n); \
		(v).n++; \
	} while (0)
*/
/** 10% faster than above */
#define kpv_push(v, x) do {								\
		if ((v).n == (v).m) {							\
			(v).a = realloc((v).a, 2 * sizeof(*(v).a) * kpv_amax(v)); \
			memset((v).a + kpv_amax(v), 0, kpv_amax(v));	\
			(v).m = (v).m * 2;							\
		}												\
		(v).a[(v).n / kpv_elems(v)] = 					\
			  ((v).a[(v).n / kpv_elems(v)] & ~(kpv_mask(v, 0xff)<<kpv_base(v, (v).n))) \
			| (kpv_mask(v, x)<<kpv_base(v, (v).n)); \
		(v).n++; \
	} while (0)

/* *
 * kpv_pushp is not supported
 */
#if 0
#define kpv_pushp(v) ( \
	((v).n == (v).m)?							\
	((v).m = ((v).m? (v).m<<1 : 2),				\
	 (v).a = realloc((v).a, sizeof(*(v).a) * (v).m), 0)	\
	: 0), ((v).a + ((v).n++))
#endif

#define kpv_a(v, i) ( \
	((v).m <= (size_t)(i)? \
	((v).m = (v).n = (i) + 1, kv_roundup32((v).m), \
	 (v).a = realloc((v).a, sizeof(*(v).a) * (v).m), 0) \
	: (v).n <= (size_t)(i)? (v).n = (i) + 1 \
	: 0), kpv_mask(v, (v).a[(i) / kpv_elems(v)]>>kpv_base(v, i)) )

/** bound-unchecked accessor */
#define kpv_at(v, i) ( kpv_mask(v, (v).a[(i) / kpv_elems(v)]>>kpv_base(v, i)) )
#define kpv_ptr(v)  ( (v).a )

#endif
/**
 * end of kvec.h
 */
