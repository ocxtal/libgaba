
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
#define _vec_cell(v)				__m128i v##1, v##2
#define _vec_cell_const(v, k)		__m128i const v##1 = _mm_set1_epi8(k), v##2 = _mm_set1_epi8(k)
#define _vec_cell_reg(v)			__m128i register v##1, v##2
#define _vec_acc(v)					__m128i v
#define _vec_single_const(v, k)		__m128i const v = _mm_set1_epi8(k)
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
 * @macro vec_acc_set
 */
#define vec_acc_set(acc, p, scl, scc, scu) { \
	acc = _mm_set_epi32(p, scl, scc, scu); \
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
#define vec_acc_scu(acc)		vec_acc(acc, 0)

/**
 * @macro vec_acc_accum_max
 * @brief accumulate (num_call, v[31], v[16], v[0])
 */
#define vec_acc_accum_max(acc, max, v, gi) { \
	__m128i const mask = _mm_set_epi8( \
		0, 0, 0, 0, 0, 0, 0, 0, \
		0, 0, 0, 0, 0, 0, 0, 0xff); \
	__m128i const shuf = _mm_set_epi8( \
		0x80, 0x80, 0x80, 0x80, \
		0x80, 0x80, 0x80, 0x0f, \
		0x80, 0x80, 0x80, 0x00, \
		0x80, 0x80, 0x80, 0x80); \
	__m128i const inc = _mm_set_epi32(1, gi, gi, gi); \
	/** add difference */ \
	acc = _mm_add_epi32(acc, \
		_mm_or_si128( \
			_mm_and_si128(v##1, mask), \
			_mm_shuffle_epi8(v##2, shuf))); \
	/** increment call counter and compensate gi */ \
	acc = _mm_add_epi32(acc, inc); \
	/** take max */ \
	max = _mm_blendv_epi8(acc, max, \
		_mm_shuffle_epi32( \
			_mm_cmplt_epi32(acc, max), \
			0x55)); \
}

/**
 * calculate 16bit prefix sum of 8bit vector
 * (hv, lv) <- prefixsum(v)
 * each element must hold -16 <= a <= 15, (sufficient for 4bit diff algorithm)
 */
 #define vec_psum(hv, lv, v) { \
	__m128i tt1 = v##1, tt2 = v##2; \
	/** 8-bit prefix sum */ \
	tt1 = _mm_adds_epi8(tt1, _mm_slli_si128(tt1, 1)); \
	tt2 = _mm_adds_epi8(tt2, _mm_slli_si128(tt2, 1)); \
	tt1 = _mm_adds_epi8(tt1, _mm_slli_si128(tt1, 2)); \
	tt2 = _mm_adds_epi8(tt2, _mm_slli_si128(tt2, 2)); \
	tt1 = _mm_adds_epi8(tt1, _mm_slli_si128(tt1, 4)); \
	tt2 = _mm_adds_epi8(tt2, _mm_slli_si128(tt2, 4)); \
	/** expand to 16bit */ \
	lv##1 = _mm_cvtepi8_epi16(tt1); \
	lv##2 = _mm_cvtepi8_epi16(_mm_srli_si128(tt1, 8)); \
	hv##1 = _mm_cvtepi8_epi16(tt2); \
	hv##2 = _mm_cvtepi8_epi16(_mm_srli_si128(tt2, 8)); \
	/** 16bit prefix sum */ \
	hv##2 = _mm_adds_epi16(hv##2, hv##1); \
	hv##1 = _mm_adds_epi16(hv##1, lv##2); \
	lv##2 = _mm_adds_epi16(lv##2, lv##1); \
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
	__m128i t1, t2; \
	__m128i const offset = _mm_set1_epi16(0x7fff); \
	/** negate vector */ \
	t1 = _mm_sub_epu16(offset, t1); \
	t2 = _mm_sub_epu16(offset, t2); \
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
 * @brief convert a pair of diff vectors to abs vector, calc prefix sum, and returns max pos and value.
 */
#define vec_maxpos_dvdh(pos, val, dv, dh) { \
	\
}

/**
 * load and store operations
 */
#define vec_store(p, v) { \
	_mm_store_si128((__m128i *)(p), v##1); p += sizeof(__m128i); \
	_mm_store_si128((__m128i *)(p), v##2); p += sizeof(__m128i); \
}

#define vec_store_packed(p, dv, dh) { \
	_mm_store_si128( \
		(__m128i *)(p), \
		_mm_or_si128(_mm_slli_epi64((dh##1), 4), (dv##1))); \
	p += sizeof(__m128i); \
	_mm_store_si128( \
		(__m128i *)(p), \
		_mm_or_si128(_mm_slli_epi64((dh##2), 4), (dv##2))); \
	p += sizeof(__m128i); \
}

#define vec_store_dvdh(p, dv, dh) { \
	vec_store_packed(p, dv, dh); \
}

#define vec_load(p, v) { \
	(v##1) = _mm_load_si128((__m128i *)(p)); p += sizeof(__m128i); \
	(v##2) = _mm_load_si128((__m128i *)(p)); p += sizeof(__m128i); \
}

#define vec_load_packed(p, dv, dh) { \
	__m128i const mask = _mm_set1_epi8(0x0f); \
	(dv##1) = _mm_load_si128((__m128i *)(p)); \
	(dh##1) = _mm_and_si128(_mm_srli_epi64((dv##1), 4), mask); \
	(dv##1) = _mm_and_si128((dv##1), mask); \
	p += sizeof(__m128i); \
	(dv##2) = _mm_load_si128((__m128i *)(p)); \
	(dh##2) = _mm_and_si128(_mm_srli_epi64((dv##2), 4), mask); \
	(dv##2) = _mm_and_si128((dv##2), mask); \
	p += sizeof(__m128i); \
}

#define vec_load_dvdh(p, dv, dh) { \
	vec_load_packed(p, dv, dh); \
}

/**
 * store vector to int32_t array
 */
#define vec_store32(p, v) { \
	__m128i t = v##1; \
	vec_store(p, _mm_cvtepi8_epi32(t)); \
	t = _mm_slli_si128(t, 4); \
	vec_store(p, _mm_cvtepi8_epi32(t)); \
	t = _mm_slli_si128(t, 4); \
	vec_store(p, _mm_cvtepi8_epi32(t)); \
	t = _mm_slli_si128(t, 4); \
	vec_store(p, _mm_cvtepi8_epi32(t)); \
	t = v##2; \
	vec_store(p, _mm_cvtepi8_epi32(t)); \
	t = _mm_slli_si128(t, 4); \
	vec_store(p, _mm_cvtepi8_epi32(t)); \
	t = _mm_slli_si128(t, 4); \
	vec_store(p, _mm_cvtepi8_epi32(t)); \
	t = _mm_slli_si128(t, 4); \
	vec_store(p, _mm_cvtepi8_epi32(t)); \
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
	_mm_store_si128((__m128i *)p, tmp); p += sizeof(__m128i); \
	(v##1) = _mm_srli_si128((v##1), 8); \
	(t) = _mm_srli_si128((t), 4); \
	tmp = _mm_cvtepi16_epi32((v##1)); \
	tmp = _mm_add_epi32(tmp, (bv)); \
	tmp = _mm_sub_epi32(tmp, \
		_mm_cvtepi8_epi32((t))); \
	_mm_store_si128((__m128i *)p, tmp); p += sizeof(__m128i); \
	(t) = _mm_srli_si128((t), 4); \
	tmp = _mm_cvtepi16_epi32((v##2)); \
	tmp = _mm_add_epi32(tmp, (bv)); \
	tmp = _mm_sub_epi32(tmp, \
		_mm_cvtepi8_epi32((t))); \
	_mm_store_si128((__m128i *)p, tmp); p += sizeof(__m128i); \
	(v##2) = _mm_srli_si128((v##2), 8); \
	(t) = _mm_srli_si128((t), 4); \
	tmp = _mm_cvtepi16_epi32((v##2)); \
	tmp = _mm_add_epi32(tmp, (bv)); \
	tmp = _mm_sub_epi32(tmp, \
		_mm_cvtepi8_epi32((t))); \
	_mm_store_si128((__m128i *)p, tmp); p += sizeof(__m128i); \
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
	vec_store32_dvdh_intl(p, l, t1, base); \
	/** higher 16 elems */ \
	vec_store32_dvdh_intl(p, h, t2, base); \
}

/**
 * load vector from int32_t array
 */
#define vec_load32(p, v) { \
	__m128i t1, t2, t3, t4; \
	vec_load(p, t1); \
	vec_load(p, t2); \
	vec_load(p, t3); \
	vec_load(p, t4); \
	v##1 = _mm_packs_epi32( \
		_mm_packs_epi16(t1, t2), \
		_mm_packs_epi16(t3, t4)); \
	vec_load(p, t1); \
	vec_load(p, t2); \
	vec_load(p, t3); \
	vec_load(p, t4); \
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
		p = _mm_load_si128((__m128i *)pv); pv += sizeof(__m128i); \
		c = _mm_load_si128((__m128i *)cv); cv += sizeof(__m128i); \
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
		p = _mm_load_si128((__m128i *)pv); pv += sizeof(__m128i); \
		c = _mm_load_si128((__m128i *)cv); cv += sizeof(__m128i); \
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
#define vec_print(s, v) { \
	int8_t b[32]; \
	void *p = (void *)b; \
	vec_store(p, v); \
	fprintf(s, \
		"[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d," \
		"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]\n", \
		b[31], b[30], b[29], b[28], b[27], b[26], b[25], b[24], \
		b[23], b[22], b[21], b[20], b[19], b[18], b[17], b[16], \
		b[15], b[14], b[13], b[12], b[11], b[10], b[9], b[8], \
		b[7], b[6], b[5], b[4], b[3], b[2], b[1], b[0]); \
}

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
