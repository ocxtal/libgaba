
/**
 * @file arch_util.h
 *
 * @brief architecture-dependent utilities devided from util.h
 */
#ifndef _ARCH_UTIL_H_INCLUDED
#define _ARCH_UTIL_H_INCLUDED

#include <smmintrin.h>
#include <immintrin.h>
#include <stdint.h>


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
	__asm__ ( \
		"call *%1" \
		: "=a"(b)			/** output list */ \
		: "r"((r).pop),		/** input list */ \
		  "D"((r).p), \
		  "S"(pos) \
		: "rcx"); \
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


/**
 * alnwriters
 */

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
#define wr_init(w, f) { \
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
	  + 16 - SEA_CLIP_LEN /* margin for xmm bulk write, sizeof(__m128i) == 16. (see pushm_cigar_r in io.s) */ \
	  + SEA_CLIP_LEN /* margin for clip seqs at the head */ \
	  + SEA_CLIP_LEN /* margin for clip seqs at the tail */ ) \
)
#define wr_alloc(w, s) { \
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
	__asm__ ( \
		"call *%1" \
		: "=a"((w).pos)		/** output list */ \
		: "r"((w).init),	/** input list */ \
		  "D"((w).p), \
		  "S"((w).size - SEA_CLIP_LEN), \
		  "d"(sizeof(struct sea_result) + SEA_CLIP_LEN) \
		: ); \
	(w).len = 0; \
}

/**
 * @macro wr_pushm, wr_pushx, wr_pushi, wr_pushd
 * @brief push an alignment character
 */
#define wr_push(w, c) { \
	__asm__ ( \
		"call *%1" \
		: "=a"((w).pos)		/** output list */ \
		: "r"((w).push),	/** input list */ \
		  "D"((w).p), \
		  "S"((w).pos), \
		  "d"(c) \
		: "rcx", "r8", "xmm0"); \
	(w).len++; \
	/*debug("pushm: %c, pos(%lld)", (w).p[(w).pos], (w).pos);*/ \
}

#if 0
#define wr_pushm(w) { \
	/*(w).pos = (w).pushm((w).p, (w).pos); */ \
	__asm__ ( \
		"call *%1" \
		: "=a"((w).pos)		/** output list */ \
		: "r"((w).pushm),	/** input list */ \
		  "D"((w).p), \
		  "S"((w).pos) \
		: "rdx", "rcx", "r8", "xmm0"); \
	(w).len++; \
	/*debug("pushm: %c, pos(%lld)", (w).p[(w).pos], (w).pos);*/ \
}
#define wr_pushx(w) { \
	/*(w).pos = (w).pushx((w).p, (w).pos); */ \
	__asm__ ( \
		"call *%1" \
		: "=a"((w).pos)		/** output list */ \
		: "r"((w).pushx),	/** input list */ \
		  "D"((w).p), \
		  "S"((w).pos) \
		: "rdx", "rcx", "r8", "xmm0"); \
	(w).len++; \
	/*debug("pushx: %c, pos(%lld)", (w).p[(w).pos], (w).pos);*/ \
}
#define wr_pushi(w) { \
	/*(w).pos = (w).pushi((w).p, (w).pos);*/ \
	__asm__ ( \
		"call *%1" \
		: "=a"((w).pos)		/** output list */ \
		: "r"((w).pushi),	/** input list */ \
		  "D"((w).p), \
		  "S"((w).pos) \
		: "rdx", "rcx", "r8", "xmm0"); \
	(w).len++; \
	/*debug("pushi: %c, pos(%lld)", (w).p[(w).pos], (w).pos);*/ \
}
#define wr_pushd(w) { \
	/*(w).pos = (w).pushd((w).p, (w).pos);*/ \
	__asm__ ( \
		"call *%1" \
		: "=a"((w).pos)		/** output list */ \
		: "r"((w).pushd),	/** input list */ \
		  "D"((w).p), \
		  "S"((w).pos) \
		: "rdx", "rcx", "r8", "xmm0"); \
	(w).len++; \
	/*debug("pushd: %c, pos(%lld)", (w).p[(w).pos], (w).pos);*/ \
}
#endif

/**
 * @macro wr_finish
 * @brief finish the instance
 */
#define wr_finish(w) { \
	/*int64_t p = (w).finish((w).p, (w).pos);*/ \
	int64_t p; \
	__asm__ ( \
		"call *%1" \
		: "=a"(p)			/** output list */ \
		: "r"((w).finish),	/** input list */ \
		  "D"((w).p), \
		  "S"((w).pos) \
		: "rdx", "rcx", "r8", "xmm0"); \
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
