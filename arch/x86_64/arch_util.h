
/**
 * @file arch_util.h
 *
 * @brief architecture-dependent utilities
 */
#ifndef _ARCH_UTIL_H_INCLUDED
#define _ARCH_UTIL_H_INCLUDED

#include <smmintrin.h>
#include <immintrin.h>
#include <stdint.h>

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
#define _xmm_rd(src, n) (xmm##n) = _mm_load_si128((__m128i *)(src) + (n))
#define _xmm_wr(dst, n) _mm_store_si128((__m128i *)(dst) + (n), (xmm##n))
#define _aligned_block_memcpy(dst, src, size) { \
	/** duff's device */ \
	void *_src = (void *)(src), *_dst = (void *)(dst); \
	int64_t const _nreg = 16;		/** #xmm registers == 16 */ \
	int64_t const _tcnt = (size) / sizeof(__m128i); \
	int64_t const _offset = ((_tcnt - 1) & (_nreg - 1)) - (_nreg - 1); \
	int64_t _jmp = _tcnt & (_nreg - 1); \
	int64_t _lcnt = (_tcnt + _nreg - 1) / _nreg; \
	__m128i register xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7; \
	__m128i register xmm8, xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15; \
	_src += _offset * sizeof(__m128i); \
	_dst += _offset * sizeof(__m128i); \
	switch(_jmp) { \
		case 0: do { _xmm_rd(_src, 0); \
		case 15:     _xmm_rd(_src, 1); \
		case 14:     _xmm_rd(_src, 2); \
		case 13:     _xmm_rd(_src, 3); \
		case 12:     _xmm_rd(_src, 4); \
		case 11:     _xmm_rd(_src, 5); \
		case 10:     _xmm_rd(_src, 6); \
		case 9:      _xmm_rd(_src, 7); \
		case 8:      _xmm_rd(_src, 8); \
		case 7:      _xmm_rd(_src, 9); \
		case 6:      _xmm_rd(_src, 10); \
		case 5:      _xmm_rd(_src, 11); \
		case 4:      _xmm_rd(_src, 12); \
		case 3:      _xmm_rd(_src, 13); \
		case 2:      _xmm_rd(_src, 14); \
		case 1:      _xmm_rd(_src, 15); \
		switch(_jmp) { \
			case 0:  _xmm_wr(_dst, 0); \
			case 15: _xmm_wr(_dst, 1); \
			case 14: _xmm_wr(_dst, 2); \
			case 13: _xmm_wr(_dst, 3); \
			case 12: _xmm_wr(_dst, 4); \
			case 11: _xmm_wr(_dst, 5); \
			case 10: _xmm_wr(_dst, 6); \
			case 9:  _xmm_wr(_dst, 7); \
			case 8:  _xmm_wr(_dst, 8); \
			case 7:  _xmm_wr(_dst, 9); \
			case 6:  _xmm_wr(_dst, 10); \
			case 5:  _xmm_wr(_dst, 11); \
			case 4:  _xmm_wr(_dst, 12); \
			case 3:  _xmm_wr(_dst, 13); \
			case 2:  _xmm_wr(_dst, 14); \
			case 1:  _xmm_wr(_dst, 15); \
		} \
				     _src += _nreg * sizeof(__m128i); \
				     _dst += _nreg * sizeof(__m128i); \
				     _jmp = 0; \
			    } while(--_lcnt > 0); \
	} \
}

#endif /* #ifndef _ARCH_UTIL_H_INCLUDED */
/**
 * end of arch_util.h
 */
