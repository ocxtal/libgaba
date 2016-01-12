
/**
 * @file vector_alias.h
 *
 * @brief make alias to vector macros
 */
#ifndef _VECTOR_ALIAS_PREFIX
#error "_VECTOR_ALIAS_PREFIX must be defined when alias.h is included."
#else /* _VECTOR_ALIAS_PREFIX */

#define prefix 		_VECTOR_ALIAS_PREFIX
#define vector_alias_join_intl(a,b)		a##b
#define vector_alias_join(a,b)			vector_alias_join_intl(a,b)

#define vec_t 		vector_alias_join(prefix, _t)
// #define vec_s 		prefix##_s

/* address */
#define _pv 		vector_alias_join(_pv_, prefix)

/* load and store */
#define _load 		vector_alias_join(_load_, prefix)
#define _loadu 		vector_alias_join(_loadu_, prefix)
#define _store 		vector_alias_join(_store_, prefix)
#define _storeu		vector_alias_join(_storeu_, prefix)

/* broadcast */
#define _set 		vector_alias_join(_set_, prefix)

/* logics */
#define _not 		vector_alias_join(_not_, prefix)
#define _and 		vector_alias_join(_and_, prefix)
#define _or 		vector_alias_join(_or_, prefix)
#define _xor 		vector_alias_join(_xor_, prefix)
#define _andn 		vector_alias_join(_andn_, prefix)

/* arithmetics */
#define _add 		vector_alias_join(_add_, prefix)
#define _sub 		vector_alias_join(_sub_, prefix)
#define _adds 		vector_alias_join(_adds_, prefix)
#define _subs 		vector_alias_join(_subs_, prefix)
#define _max 		vector_alias_join(_max_, prefix)
#define _min 		vector_alias_join(_min_, prefix)

/* shuffle */
#define _shuf 		vector_alias_join(_shuf_, prefix)

/* blend */
#define _sel 		vector_alias_join(_sel_, prefix)

/* compare */
#define _eq 		vector_alias_join(_eq_, prefix)
#define _lt 		vector_alias_join(_lt_, prefix)
#define _gt 		vector_alias_join(_gt_, prefix)

/* insert and extract */
#define _ins 		vector_alias_join(_ins_, prefix)
#define _ext		vector_alias_join(_ext_, prefix)

/* shift */
#define _shl 		vector_alias_join(_shl_, prefix)
#define _shr 		vector_alias_join(_shr_, prefix)

/* mask */
#define _mask 		vector_alias_join(_mask_, prefix)

/* broadcast */
#define _bc_v16i8	vector_alias_join(_bc_v16i8_, prefix)

#endif /* _VECTOR_ALIAS_PREFIX */
/**
 * end of vector_alias.h
 */