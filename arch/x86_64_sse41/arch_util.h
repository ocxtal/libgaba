
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
 * @macro store_stream
 */
#define store_stream(_ptr, _a) { \
	_mm_stream_si64((int64_t *)(_ptr), (int64_t)(_a)); \
}

/**
 * @macro _aligned_block_memcpy
 *
 * @brief copy size bytes from src to dst.
 *
 * @detail
 * src and dst must be aligned to 16-byte boundary.
 * copy must be multipe of 16.
 */
#define _xmm_rd_a(src, n) (xmm##n) = _mm_load_si128((__m128i *)(src) + (n))
#define _xmm_rd_u(src, n) (xmm##n) = _mm_loadu_si128((__m128i *)(src) + (n))
#define _xmm_wr_a(dst, n) _mm_store_si128((__m128i *)(dst) + (n), (xmm##n))
#define _xmm_wr_u(dst, n) _mm_storeu_si128((__m128i *)(dst) + (n), (xmm##n))
#define _memcpy_blk_intl(dst, src, size, _wr, _rd) { \
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
				     _src += _nreg * sizeof(__m128i); \
				     _dst += _nreg * sizeof(__m128i); \
				     _jmp = 0; \
			    } while(--_lcnt > 0); \
	} \
}
#define _memcpy_blk_aa(dst, src, len)		_memcpy_blk_intl(dst, src, len, _xmm_wr_a, _xmm_rd_a)
#define _memcpy_blk_au(dst, src, len)		_memcpy_blk_intl(dst, src, len, _xmm_wr_a, _xmm_rd_u)
#define _memcpy_blk_ua(dst, src, len)		_memcpy_blk_intl(dst, src, len, _xmm_wr_u, _xmm_rd_a)
#define _memcpy_blk_uu(dst, src, len)		_memcpy_blk_intl(dst, src, len, _xmm_wr_u, _xmm_rd_u)
#define _memset_blk_intl(dst, a, size, _wr) { \
	void *_dst = (void *)(dst); \
	__m128i const xmm0 = _mm_set1_epi8((int8_t)a); \
	int64_t i; \
	for(i = 0; i < size / sizeof(__m128i); i++) { \
		_wr(_dst, 0); _dst += sizeof(__m128i); \
	} \
}
#define _memset_blk_a(dst, a, size)			_memset_blk_intl(dst, a, size, _xmm_wr_a)
#define _memset_blk_u(dst, a, size)			_memset_blk_intl(dst, a, size, _xmm_wr_u)

/**
 * seqreader prototype implementation
 *
 * @memo
 * buf_len = BLK + BW = 64
 * sizeof(vector) = 16 (xmm)
 */

/**
 * reader function declarations (see io.s)
 */
void _loada_ascii_2bit_fw(uint8_t *dst, uint8_t const *src, uint64_t idx, uint64_t src_len, uint64_t copy_len);
void _loada_ascii_2bit_fr(uint8_t *dst, uint8_t const *src, uint64_t idx, uint64_t src_len, uint64_t copy_len);
void _loadb_ascii_2bit_fw(uint8_t *dst, uint8_t const *src, uint64_t idx, uint64_t src_len, uint64_t copy_len);
void _loadb_ascii_2bit_fr(uint8_t *dst, uint8_t const *src, uint64_t idx, uint64_t src_len, uint64_t copy_len);

/**
 * @fn rd_load, rd_loada, rd_loadb
 * @brief wrapper of loada function
 */
#define rd_load(func, dst, src, pos, lim, len) { \
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
		: "%r8", "%xmm0", "%xmm1", "%xmm2", "%xmm3", "%xmm4", "%xmm5"); \
}

/**
 * alnwriters
 */


/**
 * @macro wr_init
 *
 * sea_dp_malloc must be in scope
 */
#define wr_init(_this, _psum) ({ \
	struct sea_cigar_s *cigar = ; \
})

#if 0
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
#endif

#endif /* #ifndef _ARCH_UTIL_H_INCLUDED */
/**
 * end of arch_util.h
 */
