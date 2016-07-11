
/**
 * @fn lmm.c
 *
 * @brief malloc with local context
 */
#ifndef _LMM_H_INCLUDED
#define _LMM_H_INCLUDED

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE		200112L
#endif

#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif

#include <stdint.h>
#include <stdlib.h>
#include <string.h>


/* roundup */
#define _lmm_cutdown(x, base)		( (x) & ~((base) - 1) )
#define _lmm_roundup(x, base)		( ((x) + (base) - 1) & ~((base) - 1) )

/* constants */
#define LMM_MIN_BASE_SIZE			( 128 )
#define LMM_DEFAULT_BASE_SIZE		( 1024 )

/* max */
#define LMM_MAX2(x,y) 		( (x) > (y) ? (x) : (y) )


/**
 * @struct lmm_s
 */
struct lmm_s {
	uint8_t need_free;
	uint8_t pad[7];
	void *ptr;
	void *lim;
};
typedef struct lmm_s lmm_t;

/**
 * @fn lmm_init
 */
static inline
lmm_t *lmm_init(
	void *base,
	size_t base_size)
{
	if(base != NULL && base_size > LMM_MIN_BASE_SIZE) {
		struct lmm_s *lmm = (struct lmm_s *)base;
		lmm->need_free = 0;
		lmm->ptr = base + sizeof(struct lmm_s);
		lmm->lim = base + _lmm_cutdown(base_size, 16);
		return((lmm_t *)lmm);
	} else {
		base_size = LMM_MAX2(base_size, LMM_DEFAULT_BASE_SIZE);
		struct lmm_s *lmm = (struct lmm_s *)malloc(base_size);
		lmm->need_free = 1;
		lmm->ptr = (void *)(lmm + 1);
		lmm->lim = (void *)lmm + base_size;
		return((lmm_t *)lmm);
	}

	/* never reaches here */
	return(NULL);
}

/**
 * @fn lmm_clean
 */
static inline
void lmm_clean(
	lmm_t *lmm)
{
	if(lmm != NULL && lmm->need_free == 1) {
		free(lmm);
	}
	return;
}

/**
 * @fn lmm_reserve_mem
 */
static inline
void *lmm_reserve_mem(
	lmm_t *lmm,
	void *ptr,
	uint64_t size)
{
	uint64_t *sp = (uint64_t *)ptr;
	size = _lmm_roundup(size, 16);
	*sp = size;
	lmm->ptr = (void *)(sp + 1) + size;
	return((void *)(sp + 1));
}

/**
 * @fn lmm_malloc
 */
static inline
void *lmm_malloc(
	lmm_t *lmm,
	size_t size)
{
	if(lmm != NULL && lmm->ptr + size + sizeof(uint64_t) < lmm->lim) {
		return(lmm_reserve_mem(lmm, lmm->ptr, size));
	} else {
		return(malloc(size));
	}
}

/**
 * @fn lmm_realloc
 */
static inline
void *lmm_realloc(
	lmm_t *lmm,
	void *ptr,
	size_t size)
{
	if(lmm == NULL) {
		return(realloc(ptr, size));
	}

	/* check if prev mem (ptr) is inside mm */
	if((void *)lmm < ptr && ptr < lmm->lim) {
		
		uint64_t prev_size = *((uint64_t *)ptr - 1);
		if(ptr + prev_size == lmm->ptr && ptr + size < lmm->lim) {
			return(lmm_reserve_mem(lmm, ptr - sizeof(uint64_t), size));
		}

		void *np = malloc(size);
		if(np == NULL) { return(NULL); }

		memcpy(np, ptr, prev_size);
		return(np);
	}

	/* pass to library realloc */
	return(realloc(ptr, size));
}

/**
 * @fn lmm_free
 */
static inline
void lmm_free(
	lmm_t *lmm,
	void *ptr)
{
	if(lmm != NULL && (void *)lmm < ptr && ptr < lmm->lim) {
		/* no need to free */
		uint64_t prev_size = *((uint64_t *)ptr - 1);
		if(ptr + prev_size == lmm->ptr) {
			lmm->ptr = ptr - sizeof(uint64_t);
		}
		return;
	}

	free(ptr);
	return;
}

/**
 * @fn lmm_strdup
 */
static inline
char *lmm_strdup(
	lmm_t *lmm,
	char const *str)
{
	int64_t len = strlen(str);
	char *s = lmm_malloc(lmm, len + 1);
	memcpy(s, str, len + 1);
	return(s);
}

/**
 * kvec.h from https://github.com/ocxtal/kvec.h
 * the original implementation of kvec.h is found at
 * https://github.com/attractivechaos/klib
 *
 * ===== start kvec.h =====
 */

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
  
  2016-0410
    * add kv_pushm

  2016-0401

    * modify kv_init to return object
    * add kv_pusha to append arbitrary type element
    * add kv_roundup
    * change init size to 256

  2015-0307

    * add packed vector. (Hajime Suzuki)

  2008-09-22 (0.1.0):

	* The initial version.

*/
// #define kv_roundup(x) (-, 32-(x), (x)|=(x)>>1, (x)|=(x)>>2, (x)|=(x)>>4, (x)|=(x)>>8, (x)|=(x)>>16, ++(x))
#define lmm_kv_roundup(x, base)			( (((x) + (base) - 1) / (base)) * (base) )


#define LMM_KVEC_INIT_SIZE 			( 256 )

/**
 * basic vectors (kv_*)
 */
#define lmm_kvec_t(type)		struct { uint64_t n, m; type *a; }
#define lmm_kv_init(lmm, v)		({ (v).n = 0; (v).m = LMM_KVEC_INIT_SIZE; (v).a = lmm_malloc((lmm), (v).m * sizeof(*(v).a)); (v); })
#define lmm_kv_destroy(lmm, v)	{ lmm_free((lmm), (v).a); (v).a = NULL; }
// #define lmm_kv_A(v, i)      ( (v).a[(i)] )
#define lmm_kv_pop(lmm, v)		( (v).a[--(v).n] )
#define lmm_kv_size(v)			( (v).n )
#define lmm_kv_max(v)			( (v).m )

#define lmm_kv_clear(lmm, v)	( (v).n = 0 )
#define lmm_kv_resize(lmm, v, s) ( \
	(v).m = (s), (v).a = lmm_realloc((lmm), (v).a, sizeof(*(v).a) * (v).m) )

#define lmm_kv_reserve(lmm, v, s) ( \
	(v).m > (s) ? 0 : ((v).m = (s), (v).a = lmm_realloc((lmm), (v).a, sizeof(*(v).a) * (v).m), 0) )

#define lmm_kv_copy(lmm, v1, v0) do {								\
		if ((v1).m < (v0).n) lmm_kv_resize(lmm, v1, (v0).n);			\
		(v1).n = (v0).n;									\
		memcpy((v1).a, (v0).a, sizeof(*(v).a) * (v0).n);	\
	} while (0)

#define lmm_kv_push(lmm, v, x) do {									\
		if ((v).n == (v).m) {								\
			(v).m = (v).m * 2;								\
			(v).a = lmm_realloc((lmm), (v).a, sizeof(*(v).a) * (v).m);	\
		}													\
		(v).a[(v).n++] = (x);								\
	} while (0)

#define lmm_kv_pushp(lmm, v) ( \
	((v).n == (v).m) ?									\
	((v).m = (v).m * 2,									\
	 (v).a = lmm_realloc((lmm), (v).a, sizeof(*(v).a) * (v).m), 0)	\
	: 0), ( (v).a + ((v).n++) )

/* lmm_kv_pusha will not check the alignment of elem_t */
#define lmm_kv_pusha(lmm, elem_t, v, x) do { \
		uint64_t size = lmm_kv_roundup(sizeof(elem_t), sizeof(*(v).a)); \
		if(sizeof(*(v).a) * ((v).m - (v).n) < size) { \
			(v).m = LMM_MAX2((v).m * 2, (v).n + (size));				\
			(v).a = lmm_realloc((lmm), (v).a, sizeof(*(v).a) * (v).m);	\
		} \
		*((elem_t *)&((v).a[(v).n])) = (x); \
		(v).n += size / sizeof(*(v).a); \
	} while(0)

#define lmm_kv_pushm(lmm, v, arr, size) do { \
		if(sizeof(*(v).a) * ((v).m - (v).n) < (uint64_t)(size)) { \
			(v).m = LMM_MAX2((v).m * 2, (v).n + (size));				\
			(v).a = lmm_realloc((lmm), (v).a, sizeof(*(v).a) * (v).m);	\
		} \
		for(uint64_t _i = 0; _i < (uint64_t)(size); _i++) { \
			(v).a[(v).n + _i] = (arr)[_i]; \
		} \
		(v).n += (uint64_t)(size); \
	} while(0)

#define lmm_kv_a(lmm, v, i) ( \
	((v).m <= (size_t)(i) ? \
	((v).m = (v).n = (i) + 1, lmm_kv_roundup((v).m, 32), \
	 (v).a = lmm_realloc((lmm), (v).a, sizeof(*(v).a) * (v).m), 0) \
	: (v).n <= (size_t)(i) ? (v).n = (i) + 1 \
	: 0), (v).a[(i)])

/** bound-unchecked accessor */
#define lmm_kv_at(v, i)			( (v).a[(i)] )
#define lmm_kv_ptr(v)			( (v).a )

/** heap queue : elements in v must be orderd in heap */
#define lmm_kv_hq_init(lmm, v)		{ lmm_kv_init(lmm, v); (v).n = 1; }
#define lmm_kv_hq_destroy(lmm, v)	lmm_kv_destroy(lmm, v)
#define lmm_kv_hq_size(v)			( lmm_kv_size(v) - 1 )
#define lmm_kv_hq_max(v)			( lmm_kv_max(v) - 1 )
#define lmm_kv_hq_clear(lmm, v)		( (v).n = 1 )

#define lmm_kv_hq_resize(lmm, v, s)		( lmm_kv_resize(lmm, v, (s) + 1) )
#define lmm_kv_hq_reserve(lmm, v, s)	( lmm_kv_reserve(lmm, v, (s) + 1) )

#define lmm_kv_hq_copy(lmm, v1, v0)		lmm_kv_copy(lmm, v1, v0)

#define lmm_kv_hq_n(v, i) ( *((int64_t *)&v.a[i]) )
#define lmm_kv_hq_push(lmm, v, x) { \
	debug("push, n(%llu), m(%llu)", (v).n, (v).m); \
	lmm_kv_push(lmm, v, x); \
	uint64_t i = (v).n - 1; \
	while(i > 1 && (lmm_kv_hq_n(v, i>>1) > lmm_kv_hq_n(v, i))) { \
		(v).a[0] = (v).a[i>>1]; \
		(v).a[i>>1] = (v).a[i]; \
		(v).a[i] = (v).a[0]; \
		i >>= 1; \
	} \
}
#define lmm_kv_hq_pop(lmm, v) ({ \
	debug("push, n(%llu), m(%llu)", (v).n, (v).m); \
	uint64_t i = 1, j = 2; \
	(v).a[0] = (v).a[i]; \
	(v).a[i] = (v).a[--(v).n]; \
	(v).a[(v).n] = (v).a[0]; \
	while(j < (v).n) { \
		uint64_t k; \
		k = (j + 1 < (v).n && lmm_kv_hq_n(v, j + 1) < lmm_kv_hq_n(v, j)) ? (j + 1) : j; \
		k = (lmm_kv_hq_n(v, k) < lmm_kv_hq_n(v, i)) ? k : 0; \
		if(k == 0) { break; } \
		(v).a[0] = (v).a[k]; \
		(v).a[k] = (v).a[i]; \
		(v).a[i] = (v).a[0]; \
		i = k; j = k<<1; \
	} \
	v.a[v.n]; \
})

/**
 * 2-bit packed vectors (lmm_kpv_*)
 * v.m must be multiple of lmm_kpv_elems(v).
 */
#define _BITS						( 2 )

/**
 * `sizeof(*((v).a)) * 8 / _BITS' is a number of packed elements in an array element.
 */
#define lmm_kpv_elems(v)			( sizeof(*((v).a)) * 8 / _BITS )
#define lmm_kpv_base(v, i)			( ((i) % lmm_kpv_elems(v)) * _BITS )
#define lmm_kpv_mask(v, e)			( (e) & ((1<<_BITS) - 1) )

#define lmm_kpvec_t(type)			struct { uint64_t n, m; type *a; }
#define lmm_kpv_init(lmm, v)		( (v).n = 0, (v).m = LMM_KVEC_INIT_SIZE * lmm_kpv_elems(v), (v).a = lmm_malloc(lmm, (v).m * sizeof(*(v).a)) )
#define lmm_kpv_destroy(lmm, v)		{ free(lmm, (v).a); (v).a = NULL; }

// #define lmm_kpv_A(v, i) ( lmm_kpv_mask(v, (v).a[(i) / lmm_kpv_elems(v)]>>lmm_kpv_base(v, i)) )
#define lmm_kpv_pop(lmm, v) 		( (v).n--, lmm_kpv_mask(v, (v).a[(v).n / lmm_kpv_elems(v)]>>lmm_kpv_base(v, (v).n)) )
#define lmm_kpv_size(v)				( (v).n )
#define lmm_kpv_max(v)				( (v).m )

/**
 * the length of the array is ((v).m + lmm_kpv_elems(v) - 1) / lmm_kpv_elems(v)
 */
#define lmm_kpv_amax(v)				( ((v).m + lmm_kpv_elems(v) - 1) / lmm_kpv_elems(v) )
#define lmm_kpv_asize(v)			( ((v).n + lmm_kpv_elems(v) - 1) / lmm_kpv_elems(v) )

#define lmm_kpv_clear(lmm, v)		( (v).n = 0 )
#define lmm_kpv_resize(lmm, v, s) ( \
	(v).m = (s), (v).a = lmm_realloc(lmm, (v).a, sizeof(*(v).a) * lmm_kpv_amax(v)) )

#define lmm_kpv_reserve(lmm, v, s) ( \
	(v).m > (s) ? 0 : ((v).m = (s), (v).a = lmm_realloc(lmm, (v).a, sizeof(*(v).a) * lmm_kpv_amax(v)), 0) )

#define lmm_kpv_copy(lmm, v1, v0) do {							\
		if ((v1).m < (v0).n) lmm_kpv_resize(lmm, v1, (v0).n);	\
		(v1).n = (v0).n;								\
		memcpy((v1).a, (v0).a, lmm_kpv_amax(v));				\
	} while (0)
#define lmm_kpv_push(lmm, v, x) do {								\
		if ((v).n == (v).m) {							\
			(v).a = lmm_realloc(lmm, (v).a, 2 * sizeof(*(v).a) * lmm_kpv_amax(v)); \
			memset((v).a + lmm_kpv_amax(v), 0, lmm_kpv_amax(v));	\
			(v).m = (v).m * 2;							\
		}												\
		(v).a[(v).n / lmm_kpv_elems(v)] = 					\
			  ((v).a[(v).n / lmm_kpv_elems(v)] & ~(lmm_kpv_mask(v, 0xff)<<lmm_kpv_base(v, (v).n))) \
			| (lmm_kpv_mask(v, x)<<lmm_kpv_base(v, (v).n)); \
		(v).n++; \
	} while (0)

#define lmm_kpv_a(lmm, v, i) ( \
	((v).m <= (size_t)(i)? \
	((v).m = (v).n = (i) + 1, lmm_kv_roundup((v).m, 32), \
	 (v).a = lmm_realloc(lmm, (v).a, sizeof(*(v).a) * (v).m), 0) \
	: (v).n <= (size_t)(i)? (v).n = (i) + 1 \
	: 0), lmm_kpv_mask(v, (v).a[(i) / lmm_kpv_elems(v)]>>lmm_kpv_base(v, i)) )

/** bound-unchecked accessor */
#define lmm_kpv_at(v, i)		( lmm_kpv_mask(v, (v).a[(i) / lmm_kpv_elems(v)]>>lmm_kpv_base(v, i)) )
#define lmm_kpv_ptr(v)			( (v).a )

/**
 * ===== end of kvec.h =====
 */

#endif /* _LMM_H_INCLUDED */
/**
 * end of lmm.c
 */
