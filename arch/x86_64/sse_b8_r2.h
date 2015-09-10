
/**
 * @file sse_b8_r2.h
 *
 * @brief a header for macros of packed 8-bit SSE4 instructions.
 *
 * @detail
 * This is a collection of wrapper macros of SSE4.1 SIMD intrinsics.
 * Each macro corresponds to a set of several intrinsics defined in
 * smmintrin.h. The details of the intrinsics are found in the intel's
 * website: https://software.intel.com/sites/landingpage/IntrinsicsGuide/
 * The required set of macros are documented in porting section of 
 * README.md in the top directory of the library.
 *
 * @sa sse.h
 */
#ifndef _SSE_B8_R2_H_INCLUDED
#define _SSE_B8_R2_H_INCLUDED

#ifndef _SIMD_INCLUDED
#define _SIMD_INCLUDED

#include <smmintrin.h>
#include <stdint.h>

/**
 * register declarations. 
 */
#define vec_size()					( 2 * sizeof(__m128i) )
#define _vec_cell(v)				__m128i v##1, v##2
#define _vec_cell_const(v, k)		__m128i const v##1 = _mm_set1_epi8(k), v##2 = _mm_set1_epi8(k)
#define _vec_cell_reg(v)			__m128i register v##1, v##2

/** for score accumulators (int32_t array) */
#define vec_acc_size()				( sizeof(__m128i) )
#define _vec_acc(v)					__m128i v

/** for constants (m - 2g and x - 2g) */
#define _vec_single_const(v, k)		__m128i const v = _mm_set1_epi8(k)

/** for character vectors */
#define _vec_char_reg(v)			__m128i register v##1, v##2

/**
 * substitution to cell vectors
 */
#define vec_assign(a, b) { \
	(a##1) = (b##1); (a##2) = (b##2); \
}

#define vec_set(v, i) { \
	(v##1) = _mm_set1_epi8(i); \
	(v##2) = _mm_set1_epi8(i); \
}

#define vec_setzero(v) { \
	(v##1) = _mm_setzero_si128(); \
	(v##2) = _mm_setzero_si128(); \
}

#define vec_setones(v) { \
	(v##1) = _mm_set1_epi8(0xff); \
	(v##2) = _mm_set1_epi8(0xff); \
}

/**
 * substitution to char vectors
 */
#define vec_char_setzero(v) { \
	vec_setzero(v); \
}

#define vec_char_setones(v) { \
	vec_setones(v); \
}

/**
 * special substitution macros
 */
#define vec_set_lhalf(v, i) { \
	(v##1) = _mm_set1_epi8(i); \
	(v##2) = _mm_setzero_si128(); \
}

#define vec_set_uhalf(v, i) { \
	(v##1) = _mm_setzero_si128(); \
	(v##2) = _mm_set1_epi8(i); \
}

#define vec_setf_msb(v) { \
	vec_setzero(v); vec_insert_msb(v, 0xf0); \
}

#define vec_setf_lsb(v) { \
	vec_setzero(v); vec_insert_lsb(v, 0x0f); \
}

/**
 * insertion and extraction macros
 */
#define vec_insert_msb(v, a) { \
	(v##2) = _mm_insert_epi8((v##2), (a), sizeof(__m128i)-1); \
}

#define vec_insert_lsb(v, a) { \
	(v##1) = _mm_insert_epi8((v##1), (a), 0); \
}

#define vec_msb(v)		( (signed char)_mm_extract_epi8((v##2), sizeof(__m128i)-1) )
#define vec_lsb(v)		( (signed char)_mm_extract_epi8((v##1), 0) )
#define vec_center(v)	( (signed char)_mm_extract_epi8((v##2), 0) )

/**
 * arithmetic and logic operations
 */
#define vec_or(a, b, c) { \
	(a##1) = _mm_or_si128((b##1), (c##1)); \
	(a##2) = _mm_or_si128((b##2), (c##2)); \
}

#define vec_add(a, b, c) { \
	(a##1) = _mm_adds_epi8((b##1), (c##1)); \
	(a##2) = _mm_adds_epi8((b##2), (c##2)); \
}

#define vec_adds(a, b, c) { \
	(a##1) = _mm_adds_epu8((b##1), (c##1)); \
	(a##2) = _mm_adds_epu8((b##2), (c##2)); \
}

#define vec_sub(a, b, c) { \
	(a##1) = _mm_subs_epi8((b##1), (c##1)); \
	(a##2) = _mm_subs_epi8((b##2), (c##2)); \
}

#define vec_subs(a, b, c) { \
	(a##1) = _mm_subs_epu8((b##1), (c##1)); \
	(a##2) = _mm_subs_epu8((b##2), (c##2)); \
}

#define vec_max(a, b, c) { \
	(a##1) = _mm_max_epi8((b##1), (c##1)); \
	(a##2) = _mm_max_epi8((b##2), (c##2)); \
}

#define vec_min(a, b, c) { \
	(a##1) = _mm_min_epi8((b##1), (c##1)); \
	(a##2) = _mm_min_epi8((b##2), (c##2)); \
}

/**
 * shift operations
 */
#define vec_shift_r(a, b) { \
	(a##1) = _mm_or_si128( \
		_mm_srli_si128((b##1), 1), \
		_mm_slli_si128((b##2), sizeof(__m128i)-1)); \
	(a##2) = _mm_srli_si128((b##2), 1); \
}

#define vec_shift_l(a, b) { \
	(a##2) = _mm_or_si128( \
		_mm_slli_si128((b##2), 1), \
		_mm_srli_si128((b##1), sizeof(__m128i)-1)); \
	(a##1) = _mm_slli_si128((b##1), 1); \
}

#if 0
/**
 * compare and select
 */
#define vec_comp(a, b, c) { \
	(a##1) = _mm_cmpeq_epi8((b##1), (c##1)); \
	(a##2) = _mm_cmpeq_epi8((b##2), (c##2)); \
}

#define vec_select(a, b, c, d) { \
	(a##1) = _mm_blendv_epi8((b##1), (c##1), (d##1)); \
	(a##2) = _mm_blendv_epi8((b##2), (c##2), (d##2)); \
}
#endif

/**
 * @macro vec_comp_sel
 * @brief compare two char vectors q1 and q2, select m (single vec) if match, x (single vec) otherwise
 */
#define vec_comp_sel(a, q1, q2, m, x) { \
	__m128i t1, t2; \
	t1 = _mm_cmpeq_epi8((q1##1), (q2##1)); \
	t2 = _mm_cmpeq_epi8((q1##2), (q2##2)); \
	(a##1) = _mm_blendv_epi8((x), (m), t1); \
	(a##2) = _mm_blendv_epi8((x), (m), t2); \
}

/**
 * @macro vec_comp_mask
 * @brief compare two vectors a and b, make int mask
 */
#define vec_comp_mask(mask, a, b) { \
	__m128i t1 = _mm_cmpeq_epi8((a##1), (b##1)); \
	__m128i t2 = _mm_cmpeq_epi8((a##2), (b##2)); \
	(mask) = (uint32_t)_mm_movemask_epi8(t1) \
		| (_mm_movemask_epi8(t2)<<16); \
}

/**
 * @macro vec_acc_set
 */
#define vec_acc_set(acc, p, scl, scc, scu) { \
	acc = _mm_set_epi32(p, scl, scc, scl - scu); \
}

/**
 * @macro vec_acc
 * @brief accessor for single vector
 */
#define vec_acc(acc, i) ( \
	_mm_extract_epi32(acc, i) \
)
#define vec_acc_p(acc)			vec_acc(acc, 3)
#define vec_acc_scl(acc)		vec_acc(acc, 2)
#define vec_acc_scc(acc)		vec_acc(acc, 1)
#define vec_acc_scu(acc)		vec_acc(acc, 2) - vec_acc(acc, 0)
#define vec_acc_diff(acc)		vec_acc(acc, 0)

/**
 * @macro vec_acc_accum_max
 * @brief accumulate (num_call, v[31], v[16], v[31] - v[0])
 */
#define vec_acc_accum_max(acc, max, v, gi) { \
	__m128i const mask = _mm_set_epi8( \
		0, 0, 0, 0, 0, 0, 0, 0, \
		0, 0, 0, 0, 0, 0, 0, 0xff); \
	__m128i const shuf = _mm_set_epi8( \
		0x80, 0x80, 0x80, 0x80, \
		0x80, 0x80, 0x80, 0x0f, \
		0x80, 0x80, 0x80, 0x00, \
		0x80, 0x80, 0x80, 0x0f); \
	__m128i const inc = _mm_set_epi32(1, gi, gi, 0); \
	/** add difference */ \
	acc = _mm_add_epi32(acc, \
		_mm_sub_epi32( \
			_mm_shuffle_epi8(v##2, shuf), \
			_mm_and_si128(v##1, mask))); \
	/** increment call counter and compensate gi */ \
	acc = _mm_add_epi32(acc, inc); \
	/** take max */ \
	max = _mm_blendv_epi8(acc, max, \
		_mm_shuffle_epi32( \
			_mm_cmplt_epi32(acc, max), \
			0x55)); \
}

/**
 * @macro vec_acc_load
 */
#define vec_acc_load(p, acc) { \
	(acc) = _mm_load_si128((__m128i *)(p)); \
}

/**
 * @macro vec_acc_store
 */
#define vec_acc_store(p, acc) { \
	_mm_store_si128((__m128i *)(p), acc); \
}

/**
 * calculate 16bit prefix sum of 8bit vector
 * (hv, lv) <- prefixsum(v)
 * each element must hold -16 <= a <= 15, (sufficient for 4bit diff algorithm)
 */
 #define vec_psum(hv, lv, v) { \
	__m128i tt1 = v##1, tt2 = v##2; \
	/** 8-bit prefix sum */ \
	debug("prefix sum"); \
	/** shift 1 */ \
	tt2 = _mm_adds_epi8(tt2, _mm_or_si128( \
		_mm_slli_si128(tt2, 1), \
		_mm_srli_si128(tt1, 15))); \
	tt1 = _mm_adds_epi8(tt1, _mm_slli_si128(tt1, 1)); \
	/** shift 2 */ \
	tt2 = _mm_adds_epi8(tt2, _mm_or_si128( \
		_mm_slli_si128(tt2, 2), \
		_mm_srli_si128(tt1, 14))); \
	tt1 = _mm_adds_epi8(tt1, _mm_slli_si128(tt1, 2)); \
	/** shift 4 */ \
	tt2 = _mm_adds_epi8(tt2, _mm_or_si128( \
		_mm_slli_si128(tt2, 4), \
		_mm_srli_si128(tt1, 12))); \
	tt1 = _mm_adds_epi8(tt1, _mm_slli_si128(tt1, 4)); \
	/** expand to 16bit */ \
	lv##1 = _mm_cvtepi8_epi16(tt1); \
	lv##2 = _mm_cvtepi8_epi16(_mm_srli_si128(tt1, 8)); \
	hv##1 = _mm_cvtepi8_epi16(tt2); \
	hv##2 = _mm_cvtepi8_epi16(_mm_srli_si128(tt2, 8)); \
	/** shift 8 */ \
	hv##2 = _mm_adds_epi16(hv##2, hv##1); \
	hv##1 = _mm_adds_epi16(hv##1, lv##2); \
	lv##2 = _mm_adds_epi16(lv##2, lv##1); \
	/** shift 16 */ \
	hv##2 = _mm_adds_epi16(hv##2, lv##2); \
	hv##1 = _mm_adds_epi16(hv##1, lv##1); \
}
#if 0
/**
 * -32 <= a <= 31, but slower
 */
#define vec_psum(hv, lv, v) { \
	__m128i t1 = v##1, t2 = v##2; \
	/** 8-bit prefix sum */ \
	t1 = _mm_adds_epi8(t1, _mm_slli_si128(t1, 1)); \
	t2 = _mm_adds_epi8(t2, _mm_slli_si128(t2, 1)); \
	t1 = _mm_adds_epi8(t1, _mm_slli_si128(t1, 2)); \
	t2 = _mm_adds_epi8(t2, _mm_slli_si128(t2, 2)); \
	/** expand to 16bit */ \
	lv##1 = _mm_cvtepi8_epi16(t1); \
	lv##2 = _mm_cvtepi8_epi16(_mm_srli_si128(t1, 8)); \
	hv##1 = _mm_cvtepi8_epi16(t2); \
	hv##2 = _mm_cvtepi8_epi16(_mm_srli_si128(t2, 8)); \
	/** 16bit prefix sum */ \
	hv##2 = _mm_adds_epi16(hv##2, \
		_mm_or_si128( \
			_mm_srli_si128(hv##1, 8), \
			_mm_slli_si128(hv##2, 8))); \
	hv##1 = _mm_adds_epi16(hv##1, \
		_mm_or_si128( \
			_mm_srli_si128(lv##2, 8), \
			_mm_slli_si128(hv##1, 8))); \
	lv##2 = _mm_adds_epi16(lv##2, \
		_mm_or_si128( \
			_mm_srli_si128(lv##1, 8), \
			_mm_slli_si128(lv##2, 8))); \
	lv##1 = _mm_adds_epi16(lv##1, \
		_mm_slli_si128(lv, 8)); \
	hv##2 = _mm_adds_epi16(hv##2, hv##1); \
	hv##1 = _mm_adds_epi16(hv##1, lv##2); \
	lv##2 = _mm_adds_epi16(lv##2, lv##1); \
}
#endif

/**
 * @macro (internal) vec_maxpos16
 */
#define vec_maxpos16(pos, val, v) { \
	int32_t r1, r2; \
	__m128i register t1, t2; \
	__m128i const offset = _mm_set1_epi16(0x7fff); \
	/** negate vector */ \
	t1 = _mm_sub_epi16(offset, (v##1)); \
	t2 = _mm_sub_epi16(offset, (v##2)); \
	/** calculate max position with phminposuw */ \
	r1 = _mm_extract_epi32(_mm_minpos_epu16(t1), 0); \
	r2 = _mm_extract_epi32(_mm_minpos_epu16(t2), 0); \
	/** extract max value and pos */ \
	int32_t val1, val2; \
	val1 = 0x7fff - (int32_t)(r1 & 0xffff); \
	val2 = 0x7fff - (int32_t)(r2 & 0xffff); \
	if(val2 > val1) { \
		(pos) = (r2>>16) + 8; (val) = val2; \
	} else { \
		(pos) = r1>>16; (val) = val1; \
	} \
}

/**
 * @macro vec_maxpos_dvdh
 * @brief convert a pair of diff vectors to abs vector, add offset,
 * calc prefix sum, and returns max pos and value.
 */
#define vec_maxpos_dvdh(pos, val, dv, dh, of) { \
	_vec_cell_reg(h); \
	_vec_cell_reg(l); \
	_vec_cell_reg(t); \
	/** make adjacent difference */ \
	vec_insert_lsb(dv, 0);	/** dv[0] = 0 */ \
	vec_shift_l(t, dh);		/** dh<<1 */ \
	vec_sub(t, dv, t);		/** t2 = dv - dh<<1 */ \
	/** calculate prefix sum */ \
	debug("calculate prefix sum"); \
	vec_psum(h, l, t); \
	vec_print16(l, h); \
	/** add offset */ \
	(l##1) = _mm_adds_epi16(_mm_cvtepi8_epi16(of##1), l##1); \
	(of##1) = _mm_srli_si128(of##1, 8); \
	(l##2) = _mm_adds_epi16(_mm_cvtepi8_epi16(of##1), l##2); \
	(h##1) = _mm_adds_epi16(_mm_cvtepi8_epi16(of##2), h##1); \
	(of##2) = _mm_srli_si128(of##2, 8); \
	(h##2) = _mm_adds_epi16(_mm_cvtepi8_epi16(of##2), h##2); \
	vec_print16(l, h); \
	/** calc maxpos */ \
	int32_t posh, valh; \
	vec_maxpos16((pos), (val), l); \
	vec_maxpos16(posh, valh, h); \
	if(valh > val) { val = valh; pos = posh + 16; } \
}

/**
 * load and store operations
 */
#define vec_store(p, v) { \
	_mm_store_si128((__m128i *)(p),     v##1); \
	_mm_store_si128((__m128i *)(p) + 1, v##2); \
}

#define vec_store_packed(p, dv, dh) { \
	_mm_store_si128( \
		(__m128i *)(p), \
		_mm_or_si128(_mm_slli_epi64((dh##1), 4), (dv##1))); \
	_mm_store_si128( \
		(__m128i *)(p) + 1, \
		_mm_or_si128(_mm_slli_epi64((dh##2), 4), (dv##2))); \
}

#define vec_store_dvdh(p, dv, dh) { \
	vec_store_packed(p, dv, dh); \
}

#define vec_load(p, v) { \
	(v##1) = _mm_load_si128((__m128i *)(p)    ); \
	(v##2) = _mm_load_si128((__m128i *)(p) + 1); \
}

#define vec_load_packed(p, dv, dh) { \
	__m128i const mask = _mm_set1_epi8(0x0f); \
	/** load vectors */ \
	(dv##1) = _mm_load_si128((__m128i *)(p)    ); \
	(dv##2) = _mm_load_si128((__m128i *)(p) + 1); \
	/** unpack vectors */ \
	(dh##1) = _mm_and_si128(_mm_srli_epi64((dv##1), 4), mask); \
	(dv##1) = _mm_and_si128((dv##1), mask); \
	(dh##2) = _mm_and_si128(_mm_srli_epi64((dv##2), 4), mask); \
	(dv##2) = _mm_and_si128((dv##2), mask); \
}

#define vec_load_dvdh(p, dv, dh) { \
	vec_load_packed(p, dv, dh); \
}

#if 0


	for(i = 0; i < 4; i++) { \
		if(i == 2) { (dv##1) = (dv##2); (dh##1) = (dh##2); } \
		/** load 32bit vectors */ \
		debug("load i(%d)", i); \
		p = _mm_load_si128(pv + i); \
		c = _mm_load_si128(cv + i); \
		vec_print8(stdout, p); \
		vec_print8(stdout, c); \
		t1 = _mm_sub_epi16(c, p);	/** cv - pv */ \
		/** shift by one element and make t */ \
		ct = _mm_or_si128( \
			_mm_slli_si128(c, 2), \
			_mm_srli_si128(carry, 14)); /** cv<<1 */ \
		vec_print8(stdout, ct); \
		t2 = _mm_sub_epi16(ct, p);	/** cv<<1 - pv */ \
		vec_print8(stdout, t1); \
		vec_print8(stdout, t2); \
		/** append to destination */ \
		(dh##2) = _mm_or_si128( \
			_mm_srli_si128(dh##2, 8), \
			_mm_packs_epi16(zv, t1)); \
		(dv##2) = _mm_or_si128( \
			_mm_srli_si128(dv##2, 8), \
			_mm_packs_epi16(zv, t2)); \
		vec_print8(stdout, dh##2); \
		vec_print8(stdout, dv##2); \
		/** update carry */ \
		carry = c; \
	} \

	/** adjust direction */ \
	if(dir == TOP) { \
		/** dv <- dh, dh <- dv>>1 */ \
		vec_shift_r(t, dv); \
		vec_assign(dv, dh); \
		vec_assign(dh, t); \
	} else { \
		vec_insert_lsb(dv, 0); \
	} \


#endif

/**
 * load 16bit vector, convert it to a pair of 8bit diff vectors
 */
#define vec_load16_dvdh(ps, pt, dv, dh, gi, dir) { \
	int i; \
	__m128i p1, p2, p3, p4, c1, c2, c3, c4; \
	__m128i ct, carry, t1, t2; \
	__m128i const gv = _mm_set1_epi8(gi); \
	__m128i const zv = _mm_setzero_si128(); \
	__m128i *pv = (__m128i *)(ps); \
	__m128i *cv = (__m128i *)(pt); \
	/** load */ \
	carry = zv;						/** clear carry */ \
	p1 = _mm_load_si128(pv); \
	p2 = _mm_load_si128(pv + 1); \
	p3 = _mm_load_si128(pv + 2); \
	p4 = _mm_load_si128(pv + 3); \
	c1 = _mm_load_si128(cv); \
	c2 = _mm_load_si128(cv + 1); \
	c3 = _mm_load_si128(cv + 2); \
	c4 = _mm_load_si128(cv + 3); \
	if((0x01 & (dir)) == SEA_UE_TOP) { \
		debug("load top"); \
		(dv##1) = _mm_packs_epi16( \
			_mm_sub_epi16(c1, p1), _mm_sub_epi16(c2, p2)); \
		(dv##2) = _mm_packs_epi16( \
			_mm_sub_epi16(c3, p3), _mm_sub_epi16(c4, p4)); \
		(dh##1) = _mm_packs_epi16( \
			_mm_sub_epi16(c1, \
				_mm_or_si128(_mm_srli_si128(p1, 2), _mm_slli_si128(p2, 14))), \
			_mm_sub_epi16(c2, \
				_mm_or_si128(_mm_srli_si128(p2, 2), _mm_slli_si128(p3, 14)))); \
		(dh##2) = _mm_packs_epi16( \
			_mm_sub_epi16(c3, \
				_mm_or_si128(_mm_srli_si128(p3, 2), _mm_slli_si128(p4, 14))), \
			_mm_sub_epi16(c4, \
				_mm_srli_si128(p4, 2))); \
		vec_insert_msb(dh, 0); \
	} else { \
		debug("load left"); \
		(dh##1) = _mm_packs_epi16( \
			_mm_sub_epi16(c1, p1), _mm_sub_epi16(c2, p2)); \
		(dh##2) = _mm_packs_epi16( \
			_mm_sub_epi16(c3, p3), _mm_sub_epi16(c4, p4)); \
		(dv##1) = _mm_packs_epi16( \
			_mm_sub_epi16(c1, \
				_mm_slli_si128(p1, 2)), \
			_mm_sub_epi16(c2, \
				_mm_or_si128(_mm_slli_si128(p2, 2), _mm_srli_si128(p1, 14)))); \
		(dv##2) = _mm_packs_epi16( \
			_mm_sub_epi16(c3, \
				_mm_or_si128(_mm_slli_si128(p3, 2), _mm_srli_si128(p2, 14))), \
			_mm_sub_epi16(c4, \
				_mm_or_si128(_mm_slli_si128(p4, 2), _mm_srli_si128(p3, 14)))); \
		vec_insert_lsb(dv, 0); \
	} \
	/** add offset */ \
	(dv##1) = _mm_subs_epi8(dv##1, gv); \
	(dv##2) = _mm_subs_epi8(dv##2, gv); \
	(dh##1) = _mm_subs_epi8(dh##1, gv); \
	(dh##2) = _mm_subs_epi8(dh##2, gv); \
}

/**
 * store vector to int32_t array
 */
#define vec_store32(p, v) { \
	__m128i t = v##1; \
	vec_store(p,                _mm_cvtepi8_epi32(t)); \
	t = _mm_slli_si128(t, 4); \
	vec_store(p +   vec_size(), _mm_cvtepi8_epi32(t)); \
	t = _mm_slli_si128(t, 4); \
	vec_store(p + 2*vec_size(), _mm_cvtepi8_epi32(t)); \
	t = _mm_slli_si128(t, 4); \
	vec_store(p + 3*vec_size(), _mm_cvtepi8_epi32(t)); \
	t = v##2; \
	vec_store(p + 4*vec_size(), _mm_cvtepi8_epi32(t)); \
	t = _mm_slli_si128(t, 4); \
	vec_store(p + 5*vec_size(), _mm_cvtepi8_epi32(t)); \
	t = _mm_slli_si128(t, 4); \
	vec_store(p + 6*vec_size(), _mm_cvtepi8_epi32(t)); \
	t = _mm_slli_si128(t, 4); \
	vec_store(p + 7*vec_size(), _mm_cvtepi8_epi32(t)); \
}

/**
 * (internal) store
 */
#define vec_store32_dvdh_intl(p, v, t, base) { \
	__m128i tmp; \
	__m128i const bv = _mm_set1_epi32(base); \
	tmp = _mm_cvtepi16_epi32((v##1)); \
	tmp = _mm_add_epi32(tmp, (bv)); \
	tmp = _mm_sub_epi32(tmp, \
		_mm_cvtepi8_epi32((t))); \
	_mm_store_si128((__m128i *)(p), tmp); \
	(v##1) = _mm_srli_si128((v##1), 8); \
	(t) = _mm_srli_si128((t), 4); \
	tmp = _mm_cvtepi16_epi32((v##1)); \
	tmp = _mm_add_epi32(tmp, (bv)); \
	tmp = _mm_sub_epi32(tmp, \
		_mm_cvtepi8_epi32((t))); \
	_mm_store_si128((__m128i *)(p) + 1, tmp); \
	(t) = _mm_srli_si128((t), 4); \
	tmp = _mm_cvtepi16_epi32((v##2)); \
	tmp = _mm_add_epi32(tmp, (bv)); \
	tmp = _mm_sub_epi32(tmp, \
		_mm_cvtepi8_epi32((t))); \
	_mm_store_si128((__m128i *)(p) + 2, tmp); \
	(v##2) = _mm_srli_si128((v##2), 8); \
	(t) = _mm_srli_si128((t), 4); \
	tmp = _mm_cvtepi16_epi32((v##2)); \
	tmp = _mm_add_epi32(tmp, (bv)); \
	tmp = _mm_sub_epi32(tmp, \
		_mm_cvtepi8_epi32((t))); \
	_mm_store_si128((__m128i *)(p) + 3, tmp); \
}

/**
 * convert a pair of diff vectors to abs vector, expand it to 32bit and store it.
 */
#define vec_store32_dvdh(p, dv, dh, base, gi, d) { \
	_vec_cell_const(gv, gi); \
	_vec_cell_reg(t); \
	_vec_cell_reg(h); \
	_vec_cell_reg(l); \
	/** make adjacent difference */ \
	vec_insert_lsb(dv, 0);	/** dv[0] = 0 */ \
	vec_shift_l(t, dh);		/** dh<<1 */ \
	vec_sub(t, dv, t);		/** t2 = dv - dh<<1 */ \
	/** calculate prefix sum */ \
	vec_psum(h, l, t); \
	/** select pv */ \
	if((d) == TOP) { vec_assign(t, dv); } else { vec_assign(t, dh); } \
	vec_subs(t, t, gv); \
	/** expand and store lower 16 elems */ \
	vec_store32_dvdh_intl((__m128i *)(p),     l, t1, base); \
	/** higher 16 elems */ \
	vec_store32_dvdh_intl((__m128i *)(p) + 4, h, t2, base); \
}

/**
 * load vector from int32_t array
 */
#define vec_load32(p, v) { \
	__m128i t1, t2, t3, t4; \
	_mm_load_si128((__m128i *)(p),     t1); \
	_mm_load_si128((__m128i *)(p) + 1, t2); \
	_mm_load_si128((__m128i *)(p) + 2, t3); \
	_mm_load_si128((__m128i *)(p) + 3, t4); \
	v##1 = _mm_packs_epi32( \
		_mm_packs_epi16(t1, t2), \
		_mm_packs_epi16(t3, t4)); \
	_mm_load_si128((__m128i *)(p) + 4, t1); \
	_mm_load_si128((__m128i *)(p) + 5, t2); \
	_mm_load_si128((__m128i *)(p) + 6, t3); \
	_mm_load_si128((__m128i *)(p) + 7, t4); \
	v##2 = _mm_packs_epi32( \
		_mm_packs_epi16(t1, t2), \
		_mm_packs_epi16(t3, t4)); \
}

/**
 * load 32bit vector, convert it to a pair of 8bit diff vectors
 */
#define vec_load32_dvdh(p1, p2, dv, dh, gi, dir) { \
	int i; \
	__m128i p, c, ct, carry, t1, t2; \
	__m128i const gv = _mm_set1_epi8(gi); \
	__m128i const zv = _mm_setzero_si128(); \
	uint8_t *pv = (uint8_t *)(p1); \
	uint8_t *cv = (uint8_t *)(p2); \
	/** load lower half */ \
	carry = zv;						/** clear carry */ \
	for(i = 0; i < 4; i++) { \
		/** load 32bit vectors */ \
		p = _mm_load_si128((__m128i *)(pv)); \
		c = _mm_load_si128((__m128i *)(cv)); \
		t1 = _mm_sub_epi32(c, p);	/** cv - pv */ \
		/** shift by one element and make t */ \
		ct = _mm_or_si128( \
			_mm_slli_si128(c, 4), \
			_mm_srli_si128(carry, 12)); /** cv<<1 */ \
		t2 = _mm_sub_epi32(ct, p);	/** cv<<1 - pv */ \
		/** append to destination */ \
		dh##1 = _mm_or_si128( \
			_mm_srli_si128(dh##1, 4), \
			_mm_packs_epi16(zv, _mm_packs_epi32(zv, t1))); \
		dv##1 = _mm_or_si128( \
			_mm_srli_si128(dv##1, 4), \
			_mm_packs_epi16(zv, _mm_packs_epi32(zv, t2))); \
		/** update carry */ \
		carry = c; \
	} \
	/** load upper half */ \
	for(i = 0; i < 4; i++) { \
		/** load 32bit vectors */ \
		p = _mm_load_si128((__m128i *)(pv) + 1); \
		c = _mm_load_si128((__m128i *)(cv) + 1); \
		t1 = _mm_sub_epi32(c, p);	/** cv - pv */ \
		/** shift by one element and make t */ \
		ct = _mm_or_si128( \
			_mm_slli_si128(c, 4), \
			_mm_srli_si128(carry, 12)); /** cv<<1 */ \
		t2 = _mm_sub_epi32(ct, p);	/** cv<<1 - pv */ \
		/** append to destination */ \
		dh##2 = _mm_or_si128( \
			_mm_srli_si128(dh##2, 4), \
			_mm_packs_epi16(zv, _mm_packs_epi32(zv, t1))); \
		dv##2 = _mm_or_si128( \
			_mm_srli_si128(dv##2, 4), \
			_mm_packs_epi16(zv, _mm_packs_epi32(zv, t2))); \
		/** update carry */ \
		carry = c; \
	} \
	/** add offset */ \
	dv##1 = _mm_subs_epi8(dv##1, gv); \
	dv##2 = _mm_subs_epi8(dv##2, gv); \
	dh##1 = _mm_subs_epi8(dh##1, gv); \
	dh##2 = _mm_subs_epi8(dh##2, gv); \
	/** adjust direction */ \
	if(dir == TOP) { \
		/** dv <- dh, dh <- dv>>1 */ \
		vec_shift_r(t, dv); \
		vec_assign(dv, dh); \
		vec_assign(dh, t); \
	} else { \
		vec_insert_lsb(dv, 0); \
	} \
}

#define DH(c, g)					( (*((pack_t *)(c))>>4) + g )
#define DV(c, g)					( (*((pack_t *)(c)) & 0x0f) + g )
#define DE(c, g)					( (*(((pack_t *)(c) + BW))>>4) + g )
#define DF(c, g)					( (*(((pack_t *)(c) + BW)) & 0x0f) + g )

/*
#define vec_store_dvdh(p, dv, dh) { \
	vec_store(p, dv); \
	vec_store(p, dh); \
}

#define DH(c, g)					( (*((cell_t *)(c) + BW)) + g )
#define DV(c, g)					( (*(cell_t *)(c)) + g )
#define DE(c, g)					( (*((cell_t *)(c) + 3*BW)) + g )
#define DF(c, g)					( (*((cell_t *)(c) + 2*BW)) + g )
*/

/**
 * print vector
 */
#ifdef DEBUG

#define vec_print(v) { \
	int8_t b[32]; \
	void *p = (void *)b; \
	vec_store(p, v); \
	fprintf(stderr, \
		"[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d," \
		"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]\n", \
		b[31], b[30], b[29], b[28], b[27], b[26], b[25], b[24], \
		b[23], b[22], b[21], b[20], b[19], b[18], b[17], b[16], \
		b[15], b[14], b[13], b[12], b[11], b[10], b[9], b[8], \
		b[7], b[6], b[5], b[4], b[3], b[2], b[1], b[0]); \
}

#define vec_print8(v) { \
	int8_t b[16]; \
	_mm_store_si128((__m128i *)b, v); \
	fprintf(stderr, \
/*		"[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]\n",*/ \
		"[%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x]\n", \
		(uint8_t)b[15], (uint8_t)b[14], (uint8_t)b[13], (uint8_t)b[12], (uint8_t)b[11], (uint8_t)b[10], (uint8_t)b[9], (uint8_t)b[8], \
		(uint8_t)b[7], (uint8_t)b[6], (uint8_t)b[5], (uint8_t)b[4], (uint8_t)b[3], (uint8_t)b[2], (uint8_t)b[1], (uint8_t)b[0]); \
}

#define vec_print16(v1, v2) { \
	int16_t b[32]; \
	_mm_store_si128((__m128i *)b, v1##1); \
	_mm_store_si128((__m128i *)b + 1, v1##2); \
	_mm_store_si128((__m128i *)b + 2, v2##1); \
	_mm_store_si128((__m128i *)b + 3, v2##2); \
	fprintf(stderr, \
		"[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d," \
		"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]\n", \
		b[31], b[30], b[29], b[28], b[27], b[26], b[25], b[24], \
		b[23], b[22], b[21], b[20], b[19], b[18], b[17], b[16], \
		b[15], b[14], b[13], b[12], b[11], b[10], b[9], b[8], \
		b[7], b[6], b[5], b[4], b[3], b[2], b[1], b[0]); \
}

#else /* #ifdef DEBUG */

#define vec_print(v)		;
#define vec_print8(v)		;
#define vec_print16(v1, v2)	;

#endif /* #ifdef DEBUG */

/**
 * char vector operations
 */
#define vec_char_shift_r(a, b) { \
	vec_shift_r(a, b); \
}

#define vec_char_shift_l(a, b) { \
	vec_shift_l(a, b); \
}

#define vec_char_insert_msb(x, y) { \
	vec_insert_msb(x, y); \
}

#define vec_char_insert_lsb(x, y) { \
	vec_insert_lsb(x, y); \
}

#define vec_char_print(s, v) { \
	vec_print(s, v); \
}

#endif /* #ifndef _SIMD_INCLUDED */

#endif /* #ifndef _SSE_B8_R2_H_INCLUDED */
/**
 * end of sse_b8_r2.h
 */
