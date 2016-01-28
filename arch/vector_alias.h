
/**
 * @file vector_alias.h
 *
 * @brief make alias to vector macros
 */
#ifndef _VECTOR_ALIAS_PREFIX
#error "_VECTOR_ALIAS_PREFIX must be defined when alias.h is included."
#else /* _VECTOR_ALIAS_PREFIX */

#define vector_prefix 					_VECTOR_ALIAS_PREFIX
#define vector_alias_join_intl(a,b)		a##b
#define vector_alias_join(a,b)			vector_alias_join_intl(a,b)

#define vec_t 		vector_alias_join(vector_prefix, _t)
// #define vec_s 		vector_prefix##_s

/* address */
#define _pv 		vector_alias_join(_pv_, vector_prefix)

/* load and store */
#define _load 		vector_alias_join(_load_, vector_prefix)
#define _loadu 		vector_alias_join(_loadu_, vector_prefix)
#define _store 		vector_alias_join(_store_, vector_prefix)
#define _storeu		vector_alias_join(_storeu_, vector_prefix)

/* broadcast */
#define _set 		vector_alias_join(_set_, vector_prefix)

/* logics */
#define _not 		vector_alias_join(_not_, vector_prefix)
#define _and 		vector_alias_join(_and_, vector_prefix)
#define _or 		vector_alias_join(_or_, vector_prefix)
#define _xor 		vector_alias_join(_xor_, vector_prefix)
#define _andn 		vector_alias_join(_andn_, vector_prefix)

/* arithmetics */
#define _add 		vector_alias_join(_add_, vector_prefix)
#define _sub 		vector_alias_join(_sub_, vector_prefix)
#define _adds 		vector_alias_join(_adds_, vector_prefix)
#define _subs 		vector_alias_join(_subs_, vector_prefix)
#define _max 		vector_alias_join(_max_, vector_prefix)
#define _min 		vector_alias_join(_min_, vector_prefix)

/* shuffle */
#define _shuf 		vector_alias_join(_shuf_, vector_prefix)

/* blend */
#define _sel 		vector_alias_join(_sel_, vector_prefix)

/* compare */
#define _eq 		vector_alias_join(_eq_, vector_prefix)
#define _lt 		vector_alias_join(_lt_, vector_prefix)
#define _gt 		vector_alias_join(_gt_, vector_prefix)

/* insert and extract */
#define _ins 		vector_alias_join(_ins_, vector_prefix)
#define _ext		vector_alias_join(_ext_, vector_prefix)

/* shift */
#define _shl 		vector_alias_join(_shl_, vector_prefix)
#define _shr 		vector_alias_join(_shr_, vector_prefix)

/* mask */
#define _mask 		vector_alias_join(_mask_, vector_prefix)

/* broadcast */
#define _bc_v16i8	vector_alias_join(_bc_v16i8_, vector_prefix)
#define _bc_v32i8	vector_alias_join(_bc_v32i8_, vector_prefix)

#endif /* _VECTOR_ALIAS_PREFIX */
/**
 * end of vector_alias.h
 */