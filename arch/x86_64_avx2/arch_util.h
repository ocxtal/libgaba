
/**
 * @file arch_util.h
 *
 * @brief architecture-dependent utilities devided from util.h
 */
#ifndef _ARCH_UTIL_H_INCLUDED
#define _ARCH_UTIL_H_INCLUDED

#include "../../sea.h"
#include <smmintrin.h>
#include <immintrin.h>
#include <stdint.h>

/**
 * misc bit operations (popcnt, tzcnt, and lzcnt)
 */

/**
 * @macro popcnt
 */
#ifdef __POPCNT__
	#define popcnt(x)		_mm_popcnt_u64(x)
#else
	static inline
	int popcnt(uint64_t n)
	{
		uint64_t c = 0;
		c = (n & 0x5555555555555555) + ((n>>1) & 0x5555555555555555);
		c = (c & 0x3333333333333333) + ((c>>2) & 0x3333333333333333);
		c = (c & 0x0f0f0f0f0f0f0f0f) + ((c>>4) & 0x0f0f0f0f0f0f0f0f);
		c = (c & 0x00ff00ff00ff00ff) + ((c>>8) & 0x00ff00ff00ff00ff);
		c = (c & 0x0000ffff0000ffff) + ((c>>16) & 0x0000ffff0000ffff);
		c = (c & 0x00000000ffffffff) + ((c>>32) & 0x00000000ffffffff);
		return(c);
	}
#endif

/**
 * @macro tzcnt
 * @brief trailing zero count (count #continuous zeros from LSb)
 */
#ifdef __BMI__
	/** immintrin.h is already included */
	#define tzcnt(x)		_tzcnt_u64(x)
#else
	static inline
	int tzcnt(uint64_t n)
	{
		n |= n<<1;
		n |= n<<2;
		n |= n<<4;
		n |= n<<8;
		n |= n<<16;
		n |= n<<32;
		return(64-popcnt(n));
	}
#endif

/**
 * @macro lzcnt
 * @brief leading zero count (count #continuous zeros from MSb)
 */
#ifdef __LZCNT__
	#define lzcnt(x)		_lzcnt_u64(x)
#else
	static inline
	int lzcnt(uint64_t n)
	{
		n |= n>>1;
		n |= n>>2;
		n |= n>>4;
		n |= n>>8;
		n |= n>>16;
		n |= n>>32;
		return(64-popcnt(n));
	}
#endif

/**
 * @macro _aligned_block_memcpy
 *
 * @brief copy size bytes from src to dst.
 *
 * @detail
 * src and dst must be aligned to 16-byte boundary.
 * copy must be multipe of 16.
 */
#define _ymm_rd_a(src, n) (ymm##n) = _mm256_load_si256((__m256i *)(src) + (n))
#define _ymm_rd_u(src, n) (ymm##n) = _mm256_loadu_si256((__m256i *)(src) + (n))
#define _ymm_wr_a(dst, n) _mm256_store_si256((__m256i *)(dst) + (n), (ymm##n))
#define _ymm_wr_u(dst, n) _mm256_storeu_si256((__m256i *)(dst) + (n), (ymm##n))
#define _memcpy_blk_intl(dst, src, size, _wr, _rd) { \
	/** duff's device */ \
	void *_src = (void *)(src), *_dst = (void *)(dst); \
	int64_t const _nreg = 16;		/** #ymm registers == 16 */ \
	int64_t const _tcnt = (size) / sizeof(__m256i); \
	int64_t const _offset = ((_tcnt - 1) & (_nreg - 1)) - (_nreg - 1); \
	int64_t _jmp = _tcnt & (_nreg - 1); \
	int64_t _lcnt = (_tcnt + _nreg - 1) / _nreg; \
	__m256i register ymm0, ymm1, ymm2, ymm3, ymm4, ymm5, ymm6, ymm7; \
	__m256i register ymm8, ymm9, ymm10, ymm11, ymm12, ymm13, ymm14, ymm15; \
	_src += _offset * sizeof(__m256i); \
	_dst += _offset * sizeof(__m256i); \
	switch(_jmp) { \
		case 0: do { _rd(_src, 0); \
		case 15:     _rd(_src, 1); \
		case 14:     _rd(_src, 2); \
		case 13:     _rd(_src, 3); \
		case 12:     _rd(_src, 4); \
		case 11:     _rd(_src, 5); \
		case 10:     _rd(_src, 6); \
		case 9:      _rd(_src, 7); \
		case 8:      _rd(_src, 8); \
		case 7:      _rd(_src, 9); \
		case 6:      _rd(_src, 10); \
		case 5:      _rd(_src, 11); \
		case 4:      _rd(_src, 12); \
		case 3:      _rd(_src, 13); \
		case 2:      _rd(_src, 14); \
		case 1:      _rd(_src, 15); \
		switch(_jmp) { \
			case 0:  _wr(_dst, 0); \
			case 15: _wr(_dst, 1); \
			case 14: _wr(_dst, 2); \
			case 13: _wr(_dst, 3); \
			case 12: _wr(_dst, 4); \
			case 11: _wr(_dst, 5); \
			case 10: _wr(_dst, 6); \
			case 9:  _wr(_dst, 7); \
			case 8:  _wr(_dst, 8); \
			case 7:  _wr(_dst, 9); \
			case 6:  _wr(_dst, 10); \
			case 5:  _wr(_dst, 11); \
			case 4:  _wr(_dst, 12); \
			case 3:  _wr(_dst, 13); \
			case 2:  _wr(_dst, 14); \
			case 1:  _wr(_dst, 15); \
		} \
				     _src += _nreg * sizeof(__m256i); \
				     _dst += _nreg * sizeof(__m256i); \
				     _jmp = 0; \
			    } while(--_lcnt > 0); \
	} \
}
#define _memcpy_blk_aa(dst, src, len)		_memcpy_blk_intl(dst, src, len, _ymm_wr_a, _ymm_rd_a)
#define _memcpy_blk_au(dst, src, len)		_memcpy_blk_intl(dst, src, len, _ymm_wr_a, _ymm_rd_u)
#define _memcpy_blk_ua(dst, src, len)		_memcpy_blk_intl(dst, src, len, _ymm_wr_u, _ymm_rd_a)
#define _memcpy_blk_uu(dst, src, len)		_memcpy_blk_intl(dst, src, len, _ymm_wr_u, _ymm_rd_u)
#define _memset_blk_intl(dst, a, size, _wr) { \
	void *_dst = (void *)(dst); \
	__m128i const ymm0 = _mm_set1_epi8((int8_t)a); \
	int64_t i; \
	for(i = 0; i < size / sizeof(__m128i); i++) { \
		_wr(_dst, 0); _dst += sizeof(__m128i); \
	} \
}
#define _memset_blk_a(dst, a, size)			_memset_blk_intl(dst, a, size, _ymm_wr_a)
#define _memset_blk_u(dst, a, size)			_memset_blk_intl(dst, a, size, _ymm_wr_u)

/**
 * seqreader prototype implementation
 *
 * @memo
 * buf_len = BLK + BW = 64
 * sizeof(vector) = 16 (ymm)
 */

/**
 * reader function declarations (see io.s)
 */
#if 0
uint8_t _pop_ascii(uint8_t const *p, int64_t pos);
uint8_t _pop_4bit(uint8_t const *p, int64_t pos);
uint8_t _pop_2bit(uint8_t const *p, int64_t pos);
uint8_t _pop_4bit8packed(uint8_t const *p, int64_t pos);
uint8_t _pop_2bit8packed(uint8_t const *p, int64_t pos);
#endif

void _loada_ascii_2bit_fw(uint8_t *dst, uint8_t const *src, uint64_t idx, uint64_t src_len, uint64_t copy_len);
void _loada_ascii_2bit_fr(uint8_t *dst, uint8_t const *src, uint64_t idx, uint64_t src_len, uint64_t copy_len);
void _loadb_ascii_2bit_fw(uint8_t *dst, uint8_t const *src, uint64_t idx, uint64_t src_len, uint64_t copy_len);
void _loadb_ascii_2bit_fr(uint8_t *dst, uint8_t const *src, uint64_t idx, uint64_t src_len, uint64_t copy_len);

/**
 * @fn rd_load, rd_loada, rd_loadb
 * @brief wrapper of loada function
 */
#define _rd_load(func, dst, src, pos, lim, len) { \
	uint64_t register u0, u1, u2, u3, u4; \
	__asm__ __volatile__ ( \
		"movq %%rbx, %%r8\n\t" \
		"call *%%rax\n\t" \
		: "=a"(u0), \
		  "=D"(u1), \
		  "=S"(u2), \
		  "=d"(u3), \
		  "=c"(u4) \
		: "a"(func), \
		  "D"(dst), \
		  "S"(src), \
		  "d"(pos), \
		  "c"(lim), \
		  "b"(len) \
		: "%r8", "%ymm0", "%ymm1", "%ymm2", "%ymm3", "%ymm4", "%ymm5"); \
}

#if 0
/**
 * @macro _rd_vec_char_reg
 */
#define _rd_vec_char_reg_16(v)		__m128i v##1;
#define _rd_vec_char_reg_32(v)		__m128i v##1, v##2;

/**
 * @macro rd_set_base_ptr
 * @brief set base pointers
 */
#define rd_set_base_ptr(r, rr, ptra, lena, ptrb, lenb) { \
	(rr).p.pa = (ptra); \
	(rr).p.pb = (ptrb); \
	(rr).p.alen = (lena); \
	(rr).p.blen = (lenb); \
}

/**
 * @macro rd_set_section
 */
#define rd_set_section(r, rr, ptr_sec) { \
	_aligned_block_memcpy(&(rr).s, ptr_sec, sizeof(struct sea_section_pair)); \
}

/**
 * @macro rd_load_32_32
 * @brief load 32cell-wide vector to 32cell-wide vector
 */
#define rd_load_32_32(r, rr, ptra, ptrb) { \
	__m128i a1, a2, b1, b2; \
	/** load vectors */ \
	a1 = _mm_load_si128((__m128i *)(ptra)); \
	a2 = _mm_load_si128((__m128i *)(ptra) + 1); \
	b1 = _mm_load_si128((__m128i *)(ptrb) + 2); \
	b2 = _mm_load_si128((__m128i *)(ptrb) + 3); \
	_mm_store_si128((__m128i *)((rr).bufa + BLK), a1); \
	_mm_store_si128((__m128i *)((rr).bufa + BLK + sizeof(__m128i)), a2); \
	_mm_store_si128((__m128i *)(rr).bufb, b1); \
	_mm_store_si128((__m128i *)((rr).bufb + sizeof(__m128i)), b2); \
	/** clear cnt */ \
	debug("cnta(%llu), cntb(%llu)", (rr).cnta, (rr).cntb); \
	_mm_store_si128((__m128i *)&(rr).cnta, _mm_setzero_si128()); \
}

/**
 * @macro rd_load_16_32
 * @brief load 16cell-wide vector to 32cell-wide vector
 */
#define rd_load_16_32(r, rr, ptra, ptrb) { \
	__m128i a1, b1; \
	debug("wa(%p), wb(%p)", ptra, ptrb); \
	/** load vector a */ \
	(r).loada((rr).bufa + BLK, (rr).p.pa, (rr).s.body.asp, (rr).p.alen, 8); \
	(rr).s.body.asp += 8; \
	debug("load"); \
	a1 = _mm_load_si128((__m128i *)(ptra)); \
	debug("store"); \
	_mm_storeu_si128((__m128i *)((rr).bufa + BLK + 8), a1); \
	debug("pad"); \
	*((uint64_t *)((rr).bufa + BLK + 24)) = 0x8080808080808080; \
	/** load vector b */ \
	*((uint64_t *)((rr).bufb)) = 0xffffffffffffffff; \
	b1 = _mm_load_si128((__m128i *)(ptrb)); \
	_mm_storeu_si128((__m128i *)((rr).bufb + 8), b1); \
	(r).loadb((rr).bufb + 24, (rr).p.pb, (rr).s.body.bsp, (rr).p.blen, 8); \
	(rr).s.body.bsp += 8; \
	/** clear cnt */ \
	debug("cnta(%llu), cntb(%llu)", (rr).cnta, (rr).cntb); \
	_mm_store_si128((__m128i *)&(rr).cnta, _mm_setzero_si128()); \
}

/**
 * @macro rd_load_16_16
 * @brief load 16cell-wide vector to 16cell-wide vector
 */
#define rd_load_16_16(r, rr, ptra, ptrb) { \
	__m128i a1, b1; \
	/** load vectors */ \
	a1 = _mm_load_si128((__m128i *)(ptra)); \
	b1 = _mm_load_si128((__m128i *)(ptrb) + 1); \
	_mm_store_si128((__m128i *)(rr).bufa + 1, a1); \
	_mm_store_si128((__m128i *)(rr).bufb + 2, b1); \
	/** clear cnt */ \
	debug("cnta(%llu), cntb(%llu)", (rr).cnta, (rr).cntb); \
	_mm_store_si128((__m128i *)&(rr).cnta, _mm_setzero_si128()); \
}

/**
 * @macro rd_load_init_16
 * @brief initialize 16cell-wide vector
 */
#define rd_load_init_16(r, rr) { \
	/** calc len */ \
	__m128i const zv = _mm_setzero_si128(); \
	__m128i const lv = _mm_set1_epi64x(8); \
	__m128i pos = _mm_load_si128((__m128i *)&(rr).s.body.asp); \
	__m128i lim = _mm_load_si128((__m128i *)&(rr).s.body.aep); \
	__m128i len = _mm_sub_epi64(lim, pos); \
	len = _mm_min_epi32(lv, len); \
	len = _mm_max_epi32(zv, len); \
	pos = _mm_add_epi64(pos, len); \
	/** load seq a */ \
	(r).loada((rr).bufa + 8, \
		(rr).p.pa, \
		rev((rr).s.body.asp, (rr).p.alen), \
		(rr).p.alen, 32); \
	*((uint64_t *)((rr).bufa + BLK + 8)) = 0x8080808080808080; \
	/** load seq b */ \
	*((uint64_t *)((rr).bufb)) = 0xffffffffffffffff; \
	(r).loadb((rr).bufb + 8, \
		(rr).p.pb, \
		(rr).s.body.bsp, \
		(rr).p.blen, 32); \
	/** update pos */ \
	_mm_store_si128((__m128i *)&(rr).s.body.asp, pos); \
	debug("pos1(%llx)", _lo64(pos)); \
	debug("lim1(%llx)", _lo64(lim)); \
	debug("len1(%llx)", _lo64(len)); \
	/*for(int a = 0; a < 64; a++) { printf("%01x", (rr).bufa[a] & 0x0f); }*/ \
	/*printf("\n");*/ \
	/*for(int a = 0; a < 64; a++) { printf("%01x", (rr).bufb[a] & 0x0f); }*/ \
	/*printf("\n");*/ \
}

/**
 * prefetch internals
 */
#define _lo64(x)	 _mm_cvtsi128_si64(x)
#define rd_prefetch_load_vec_32(ptr, t) { \
	(t##1) = _mm_loadu_si128((__m128i *)(ptr)); \
	(t##2) = _mm_loadu_si128((__m128i *)(ptr) + 1); \
}
#define rd_prefetch_store_vec_32(ptr, t) { \
	_mm_store_si128((__m128i *)(ptr), (t##1)); \
	_mm_store_si128((__m128i *)(ptr) + 1, (t##2)); \
}
#define rd_prefetch_load_vec_16(ptr, t) { \
	(t##1) = _mm_loadu_si128((__m128i *)(ptr)); \
}
#define rd_prefetch_store_vec_16(ptr, t) { \
	_mm_store_si128((__m128i *)(ptr), (t##1)); \
}

/**
 * @macro rd_prefetch
 * int _load(uint8_t *dst, uint8_t *src, uint64_t idx, uint64_t src_len, uint64_t copy_len);
 */
#define rd_prefetch_impl(r, rr, t, load_vec, store_vec) { \
	/** load pos */ \
	__m128i pos = _mm_load_si128((__m128i *)&(rr).s.body.asp); \
	__m128i cnt = _mm_load_si128((__m128i *)&(rr).cnta); \
	pos = _mm_add_epi64(pos, cnt); \
	/** write back pos */ \
	_mm_store_si128((__m128i *)&(rr).s.body.asp, pos); \
	/** seq a */ \
	load_vec((rr).bufa + BLK - _lo64(cnt), t); \
	(r).loada((rr).bufa, \
		(rr).p.pa, \
		rev(_lo64(pos), (rr).p.alen), \
		(rr).p.alen, BLK); \
	store_vec((rr).bufa + BLK, t); \
	/** extract upper 64bit */ \
	pos = _mm_srli_si128(pos, sizeof(uint64_t)); \
	cnt = _mm_srli_si128(cnt, sizeof(uint64_t)); \
	/** seq b */ \
	load_vec((rr).bufb + _lo64(cnt), t); \
	store_vec((rr).bufb, t); \
	(r).loadb((rr).bufb + BW, \
		(rr).p.pb, \
		_lo64(pos), \
		(rr).p.blen, BLK); \
	/** clear counter */ \
	_mm_store_si128((__m128i *)&(rr).cnta, _mm_setzero_si128()); \
	/*for(int a = 0; a < 64; a++) { printf("%01x", (rr).bufa[a] & 0x0f); }*/ \
	/*printf("\n");*/ \
	/*for(int a = 0; a < 64; a++) { printf("%01x", (rr).bufb[a] & 0x0f); }*/ \
	/*printf("\n");*/ \
}
#define rd_prefetch_32(r, rr) { \
	__m128i t1, t2; \
	rd_prefetch_impl(r, rr, t, \
		rd_prefetch_load_vec_32, rd_prefetch_store_vec_32); \
}
#define rd_prefetch_16(r, rr) { \
	__m128i t1; \
	rd_prefetch_impl(r, rr, t, \
		rd_prefetch_load_vec_16, rd_prefetch_store_vec_16); \
}

/**
 * @macro rd_prefetch_cap
 */
#define rd_prefetch_cap_impl(r, rr, t, load_vec, store_vec) { \
	/** constants */ \
	__m128i const tot = _mm_set1_epi64x(BLK); \
	__m128i const zv = _mm_setzero_si128(); \
	/*__m128i const mask32 = _mm_set1_epi32(0, 0xffffffff, 0, 0xffffffff); */ \
	/** load previous (posa1, posb1) and (posa2, posb2) */ \
	__m128i pos1 = _mm_load_si128((__m128i *)&(rr).s.body.asp); \
	__m128i pos2 = _mm_load_si128((__m128i *)&(rr).s.tail.asp); \
	/** load lim */ \
	__m128i lim1 = _mm_load_si128((__m128i *)&(rr).s.body.aep); \
	__m128i lim2 = _mm_load_si128((__m128i *)&(rr).s.tail.aep); \
	/** calc (lena1, lenb1) and (lena1, lenb2) */ \
	__m128i len1 = _mm_sub_epi64(lim1, pos1); \
	__m128i len2 = _mm_sub_epi64(lim2, pos2); \
	/** load cnt */ \
	__m128i cnt = _mm_load_si128((__m128i *)&(rr).cnta); \
	__m128i cnt2 = _mm_max_epi32(_mm_sub_epi64(cnt, len1), zv); \
	__m128i cnt1 = _mm_sub_epi64(cnt, cnt2); \
	/** update section 1 */ \
	len1 = _mm_sub_epi64(len1, cnt1);	/** 0 <= len1 */ \
	pos1 = _mm_add_epi64(pos1, cnt1);	/** pos1 <= lim1 */ \
	len1 = _mm_min_epi32(len1, tot);	/** 0 <= len1 <= BW */ \
	/** update section 2 */ \
	len2 = _mm_sub_epi32(len2, cnt2); \
	pos2 = _mm_add_epi64(pos2, cnt2); \
	len2 = _mm_min_epi32(len2, _mm_sub_epi32(tot, len1)); \
	debug("pos1(%llx), pos2(%llx)", _lo64(pos1), _lo64(pos2)); \
	debug("lim1(%llx), lim2(%llx)", _lo64(lim1), _lo64(lim2)); \
	debug("len1(%llx), len2(%llx)", _lo64(len1), _lo64(len2)); \
	debug("cnt(%llx), cnt1(%llx), cnt2(%llx)", _lo64(cnt), _lo64(cnt1), _lo64(cnt2)); \
	/** write back pos */ \
	_mm_store_si128((__m128i *)&(rr).s.body.asp, pos1); \
	_mm_store_si128((__m128i *)&(rr).s.tail.asp, pos2); \
	/** seq a */ \
	load_vec((rr).bufa + BLK - _lo64(cnt), t); \
	__m128i t1 = _mm_loadu_si128((__m128i *)((rr).bufa + BLK - _lo64(cnt))); \
	__m128i t2 = _mm_loadu_si128((__m128i *)((rr).bufa + BLK + sizeof(__m128i) - _lo64(cnt))); \
	(r).loada((rr).bufa + BLK - (_lo64(len1) + _lo64(len2)), \
		(rr).p.pa, \
		rev(_lo64(pos2), (rr).p.alen), \
		(rr).p.alen, _lo64(len2)); \
	(r).loada((rr).bufa + BLK - _lo64(len1), \
		(rr).p.pa, \
		rev(_lo64(pos1), (rr).p.alen), \
		(rr).p.alen, _lo64(len1)); \
	store_vec((rr).bufa + BLK, t); \
	/** extract upper 64bit */ \
	cnt = _mm_srli_si128(cnt, sizeof(uint64_t)); \
	pos1 = _mm_srli_si128(pos1, sizeof(uint64_t)); \
	len1 = _mm_srli_si128(len1, sizeof(uint64_t)); \
	pos2 = _mm_srli_si128(pos2, sizeof(uint64_t)); \
	len2 = _mm_srli_si128(len2, sizeof(uint64_t)); \
	/** seq b */ \
	load_vec((rr).bufb + _lo64(cnt), t); \
	store_vec((rr).bufb, t); \
	(r).loadb((rr).bufb + BW, \
		(rr).p.pb, \
		_lo64(pos1), \
		(rr).p.blen, _lo64(len1)); \
	(r).loadb((rr).bufb + BW + _lo64(len1), \
		(rr).p.pb, \
		_lo64(pos2), \
		(rr).p.blen, _lo64(len2)); \
	/** clear counter */ \
	_mm_store_si128((__m128i *)&(rr).cnta, zv); \
	/*for(int a = 0; a < 64; a++) { printf("%01x", (rr).bufa[a] & 0x0f); }*/ \
	/*printf("\n");*/ \
	/*for(int a = 0; a < 64; a++) { printf("%01x", (rr).bufb[a] & 0x0f); }*/ \
	/*printf("\n");*/ \
}
#define rd_prefetch_cap_32(r, rr) { \
	__m128i t1, t2; \
	rd_prefetch_cap_impl(r, rr, t, \
		rd_prefetch_load_vec_32, rd_prefetch_store_vec_32); \
}
#define rd_prefetch_cap_16(r, rr) { \
	__m128i t1; \
	rd_prefetch_cap_impl(r, rr, t, \
		rd_prefetch_load_vec_16, rd_prefetch_store_vec_16); \
}

/**
 * @macro rd_go_right, rd_go_down
 */
#define rd_go_right(r, rr) { \
	(rr).cnta++; \
}
#define rd_go_down(r, rr) { \
	(rr).cntb++; \
}

/**
 * @macro rd_cmp
 */
#define rd_cmp(r, rr, offseta, offsetb) ( \
	(rr).bufa[BLK + (offseta)] == (rr).bufb[(offsetb)] \
)

/**
 * @macro rd_cmp_vec
 */
#define rd_cmp_vec_32(r, rr, vec32) { \
	(vec32##1) = _mm_cmpeq_epi8( \
		_mm_loadu_si128((__m128i *)((rr).bufa + BLK - (rr).cnta)), \
		_mm_loadu_si128((__m128i *)((rr).bufb + (rr).cntb))); \
	(vec32##2) = _mm_cmpeq_epi8( \
		_mm_loadu_si128((__m128i *)((rr).bufa + BLK + sizeof(__m128i) - (rr).cnta)), \
		_mm_loadu_si128((__m128i *)((rr).bufb + sizeof(__m128i) + (rr).cntb))); \
}
#define rd_cmp_vec_16(r, rr, vec16) { \
	(vec16##1) = _mm_cmpeq_epi8( \
		_mm_loadu_si128((__m128i *)((rr).bufa + BLK - (rr).cnta)), \
		_mm_loadu_si128((__m128i *)((rr).bufb + (rr).cntb))); \
}

/**
 * @macro rd_test_fast_prefetch
 */
#define rd_test_fast_prefetch(r, rr, p) ( \
	  ((int64_t)(rr).s.body.aep - BW - (rr).s.body.asp - (rr).cnta) \
	| ((int64_t)(rr).s.body.bep - BW - (rr).s.body.bsp - (rr).cntb) \
)

/**
 * @macro rd_test_break
 */
#define rd_test_break(r, rr, p) ( \
	  ((int64_t)(rr).s.tail.aep - BW - (rr).s.tail.asp) \
	| ((int64_t)(rr).s.tail.bep - BW - (rr).s.tail.bsp) \
	| ((int64_t)(rr).s.limp - p) \
)

/**
 * @macro rd_store
 */
#define rd_store_impl(r, rr, ptra, ptrb, t, load_vec, store_vec) { \
	load_vec((rr).bufa + BLK - (rr).cnta, t); \
	store_vec(ptra, t); \
	load_vec((rr).bufb + (rr).cntb, t); \
	store_vec(ptrb, t); \
}
#define rd_store_32(r, rr, ptra, ptrb) { \
	__m128i t1, t2; \
	rd_store_impl(r, rr, ptra, ptrb, t, \
		rd_prefetch_load_vec_32, rd_prefetch_store_vec_32); \
}
#define rd_store_16(r, rr, ptra, ptrb) { \
	__m128i t1; \
	rd_store_impl(r, rr, ptra, ptrb, t, \
		rd_prefetch_load_vec_16, rd_prefetch_store_vec_16); \
}

/**
 * @macro rd_get_section
 */
#define rd_get_section(r, rr, ptr_sec) { \
	_aligned_block_memcpy(ptr_sec, &(rr).s, sizeof(struct sea_section_pair)); \
}

#if 0
/**
 * seqreaders
 */

/**
 * @macro rd_init
 * @brief initialize a sequence reader instance.
 */
#define rd_init(r, fp, base) { \
	(r).p = (uint8_t *)base; \
	(r).b = 0; \
	(r).pop = fp; \
}

/**
 * @macro rd_fetch, rd_fetch_fast, rd_fetch_safe
 * @brief fetch a decoded base into r.b.
 */
#define rd_fetch(r, pos) { \
	uint8_t register b; \
	uint64_t register unused; \
	__asm__ ( \
		"call *%[fp]" \
		: "=a"(b),				/** output list */ \
		  "=S"(unused) \
		: [fp]"r"((r).pop),		/** input list */ \
		  "D"((r).p), \
		  "S"(pos) \
		: /*rsi", */"rcx"); \
	(r).b = b; \
	/*(r).b = (r).pop((r).p, pos);*/ \
}
#define rd_fetch_fast(r, pos, sp, ep, dummy) { \
	rd_fetch(r, pos); \
}
#define rd_fetch_safe(r, pos, sp, ep, dummy) { \
	if((uint64_t)((pos) - (sp)) < (uint64_t)((ep) - (sp))) { \
		rd_fetch(r, pos); \
	} else { \
		(r).b = (dummy); \
	} \
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
#endif
#endif

/**
 * alnwriters
 */
#if 0
/**
 * @macro SEA_CLIP_LEN
 * @brief length of the reserved buffer for clip sequence
 */
#define SEA_CLIP_LEN 		( 8 )

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
#define wr_init(w, ww, f) { \
	(w).p = NULL; \
	(w).init = (f).init; \
	(w).push = (f).push; \
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
#define wr_alloc_size(s) ( \
	  sizeof(struct sea_result) \
	+ sizeof(uint8_t) * ( \
	    (s) \
	  + 16 - SEA_CLIP_LEN /* margin for ymm bulk write, sizeof(__m128i) == 16. (see pushm_cigar_r in io.s) */ \
	  + SEA_CLIP_LEN /* margin for clip seqs at the head */ \
	  + SEA_CLIP_LEN /* margin for clip seqs at the tail */ ) \
)
#define wr_alloc(w, ww, s) { \
	debug("wr_alloc called"); \
	if((w).p == NULL || (w).size < wr_alloc_size(s)) { \
		if((w).p == NULL) { \
			free((w).p); (w).p = NULL; \
		} \
		(w).size = wr_alloc_size(s); \
		(w).p = malloc((w).size); \
	} \
	/*(w).pos = (w).init((w).p,*/ \
		/*(w).size - SEA_CLIP_LEN,*/ /** margin: must be consistent to wr_finish */ \
		/*sizeof(struct sea_result) + SEA_CLIP_LEN);*/ /** margin: must be consistent to wr_finish */ \
	(w).len = 0; \
}

/**
 * @macro wr_finish
 * @brief finish the instance
 */
#define wr_finish(w, ww) { \
	/*int64_t p = (w).finish((w).p, (w).pos);*/ \
	uint64_t register unused; \
	int64_t p; \
	__asm__ ( \
		"call *%[fp]" \
		: "=a"(p),				/** output list */ \
		  "=S"(unused) \
		: [fp]"r"((w).finish),	/** input list */ \
		  "D"((w).p), \
		  "S"((w).pos) \
		: /*"rsi", "rdx", */"rdx", "rcx", "r8", "ymm0"); \
	if(p <= (w).pos) { \
		(w).size = ((w).size - SEA_CLIP_LEN) - p - 1; \
		(w).pos = p; \
	} else { \
		(w).size = p - (sizeof(struct sea_result) + SEA_CLIP_LEN) - 1; \
		(w).pos = sizeof(struct sea_result) + SEA_CLIP_LEN; \
	} \
}

/**
 * @macro wr_clean
 * @brief free malloc'd memory and fill the instance with zero.
 */
#define wr_clean(w) { \
	if((w).p != NULL) { \
		free((w).p); (w).p = NULL; \
	} \
	(w).size = 0; \
	(w).pos = 0; \
	(w).len = 0; \
	(w).pushm = (w).pushx = (w).pushi = (w).pushd = NULL; \
}
#endif

/**
 * @macro WR_TAIL_MARGIN, WR_HEAD_MARGIN
 * @brief length of the reserved buffer for clip sequence
 */
#define WR_TAIL_MARGIN 		( 8 )
#define WR_HEAD_MARGIN		( 8 + 4 + sizeof(struct sea_result) )

/**
 * @enum _wr_type, _wr_fr
 * @brief writer tag, writer direction
 */
enum _wr_type {
	WR_ASCII	= 1,
	WR_CIGAR	= 2,
	WR_DIR		= 3
};
enum _wr_fr {
	WR_FW		= 1,
	WR_RV		= 2
};

/**
 * @enum _wr_char
 * @brief alignment character, used in ascii output format.
 */
enum _wr_char {
	M_CHAR = 0,
	X_CHAR = 1,
	I_CHAR = 2,
	D_CHAR = 3
};

/**
 * writer function declarations (see io.s)
 */
uint64_t _push_ascii_r(uint8_t *p, uint64_t dst, uint64_t src, uint8_t c);
uint64_t _push_ascii_f(uint8_t *p, uint64_t dst, uint64_t src, uint8_t c);
uint64_t _push_cigar_r(uint8_t *p, uint64_t dst, uint64_t src, uint8_t c);
uint64_t _push_cigar_f(uint8_t *p, uint64_t dst, uint64_t src, uint8_t c);
uint64_t _push_dir_r(uint8_t *p, uint64_t dst, uint64_t src, uint8_t c);
uint64_t _push_dir_f(uint8_t *p, uint64_t dst, uint64_t src, uint8_t c);

/**
 * @struct wr_cigar_pair
 */
struct wr_cigar_pair {
	uint32_t len 	: 29;
	uint32_t c 		: 3;
};
#define _cig(ptr, member)		((struct wr_cigar_pair *)(ptr) - 1)->member
#define _cig_all(ptr)			*((uint32_t *)(ptr) - 1)

/**
 * @macro wr_init
 * @brief initialize alnwriter context
 */
#define wr_init_intl_ascii(w, ww) { \
	/** nothing to do */ \
}
#define wr_init_intl_cigar(w, ww) { \
	/** forward writer */ \
	_cig_all((ww).p + WR_HEAD_MARGIN) = 0; \
	/** reverse writer */ \
	_cig_all((ww).p + (ww).rpos) = 0; \
}
#define wr_init_intl_dir(w, ww) { \
	/** nothing to do */ \
}
#define wr_init(w, ww, psum) { \
	debug("wr_init"); \
	(ww).size = wr_alloc_size(psum); \
	(ww).p = malloc((ww).size); \
	(ww).rpos = (ww).size - WR_TAIL_MARGIN; \
	(ww).pos = (ww).rpos; \
	(ww).len = 0; \
	(ww).p[(ww).rpos] = '\0'; \
	/** reverse writer must be set */ \
	if((w).type == WR_ASCII) { \
		wr_init_intl_ascii(w, ww); \
	} else if((w).type == WR_CIGAR) { \
		wr_init_intl_cigar(w, ww); \
	} else if((w).type == WR_DIR) { \
		wr_init_intl_dir(w, ww); \
	} else { \
		/** error */ \
		log_error("invalid writer tag"); \
	} \
}

/**
 * @macro wr_start
 * @brief initialize pos
 */
#define wr_start(w, ww) { \
	(ww).pos = ((w).fr == WR_RV) ? (ww).rpos : WR_HEAD_MARGIN; \
}

/**
 * @macro wr_push_intl, wr_push
 * @brief push an alignment character
 */
#define wr_push_intl(w, ww, ret, dst, src, c) { \
	uint64_t register unused1, unused2, unused3; \
	__asm__ ( \
		"call *%[fp]" \
		: "=a"(ret),			/** output list */ \
		  "=S"(unused1), \
		  "=d"(unused2), \
		  "=c"(unused3) \
		: [fp]"r"((w).push),	/** input list */ \
		  "D"((ww).p), \
		  "S"((uint64_t)dst),	/** dst pointer */ \
		  "d"((uint64_t)src),	/** src (unused) pointer */ \
		  "c"(c) \
		: "r8");				/** clobber */ \
}
#define wr_push(w, ww, c) { \
	wr_push_intl(w, ww, (ww).pos, (ww).pos, (ww).pos, c); \
	(ww).len++; \
	/*debug("pushm: %c, pos(%lld)", (w).p[(w).pos], (w).pos);*/ \
}

/**
 * @macro wr_end
 * @brief update rpos
 */
#define wr_end(w, ww) { \
	(ww).pos = ((w).fr == WR_RV) ? (ww).pos : (ww).rpos; \
}

/**
 * @macro wr_finish
 */
#define wr_finish_reverse_memcpy(dst, src, len) { \
	uint8_t _src = (src), _dst = (dst); \
	int64_t _len = (len); \
	while(_len--) { *--_dst = *--_src; } \
}
#define wr_finish_intl_ascii(w, ww) { \
	/** copy forward string from pos to rpos */ \
	wr_finish_reverse_memcpy( \
		(ww).p + (ww).rpos, \
		(ww).p + (ww).pos, \
		(ww).pos - WR_HEAD_MARGIN); \
}
#define wr_finish_intl_cigar(w, ww) { \
	/** copy forward string from pos to rpos, converting to cigar string */ \
	if(_cig((ww).p + (ww).rpos, c) == _cig((ww).p + (ww).pos, c)) { \
		_cig((ww).p + (ww).pos, len) += _cig((ww).p + (ww).rpos, len); \
	} else { \
		(ww).pos += sizeof(uint32_t); \
		_cig_all((ww).p + (ww).pos) = _cig_all((ww).p + (ww).rpos); \
	} \
	while((ww).pos >= WR_HEAD_MARGIN) { \
		wr_push_intl(w, ww, (ww).rpos, (ww).rpos, (ww).pos, 0); \
		(ww).pos -= sizeof(uint32_t); \
	} \
}
#define wr_finish_intl_dir(w, ww) { \
	/** copy forward string from pos to rpos */ \
	wr_finish_reverse_memcpy( \
		(ww).p + (ww).rpos, \
		(ww).p + (ww).pos, \
		(ww).pos - WR_HEAD_MARGIN); \
}
#define wr_finish(w, ww) { \
	if((ww).tag == WR_ASCII) { \
		wr_finish_intl_ascii(w, ww); \
	} else if((ww).tag == WR_CIGAR) { \
		wr_finish_intl_cigar(w, ww); \
	} else if((ww).tag == WR_DIR) { \
		wr_finish_intl_dir(w, ww); \
	} else { \
		log_error("invalid writer tag"); \
	} \
}

#endif /* #ifndef _ARCH_UTIL_H_INCLUDED */
/**
 * end of arch_util.h
 */
