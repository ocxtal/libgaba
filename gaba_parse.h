#define TEMP		0
/**
 * @file gaba_parse.h
 *
 * @brief libgaba utility function implementations
 *
 * @author Hajime Suzuki
 * @date 2018/1/20
 * @license Apache v2
 *
 * NOTE: Including this header is not recommended since this header internally include
 *   arch/arch.h, which floods a lot of SIMD-related defines. The same function will
 *   be found in the libgaba.a. The functions can be overridden by this header by
 *   compiling the file with a proper SIMD-enabling flag, such as `-mavx2 -mbmi -mpopcnt'
 *   (see Makefile for the details), which result in a slightly better performace
 *   with the functions inlined.
 *
 * CIGAR printers:
 *   gaba_dump_cigar_forward:
 *   gaba_dump_cigar_reverse:
 *   gaba_print_cigar_forward:
 *   gaba_print_cigar_reverse:
 *
 * MAF-style gapped alignment printers:
 *   gaba_dump_gapped_forward
 *   gaba_dump_gapped_reverse
 *
 * match and gap counter (also calculates score of subalignment):
 *   gaba_calc_score
 */

#ifndef _GABA_PARSE_H_INCLUDED
#define _GABA_PARSE_H_INCLUDED
#include <stdint.h>				/* uint32_t, uint64_t, ... */
#include <stddef.h>				/* ptrdiff_t */
#include "gaba.h"
#include "arch/arch.h"


/**
 * @type gaba_printer_t
 * @brief callback to CIGAR element printer
 * called with a pair of cigar operation (c) and its length (len).
 * void *fp is an opaque pointer to the context of the printer.
 */
typedef int (*gaba_printer_t)(void *, uint64_t, char);	/* moved to gaba.h */


/* macros */
#ifndef _GABA_PARSE_EXPORT_LEVEL
#  define _GABA_PARSE_EXPORT_LEVEL		static inline	/* hidden */
#endif

#define _gaba_parse_min2(_x, _y)		( (_x) < (_y) ? (_x) : (_y) )
#define _gaba_parse_ptr(_p)				( (uint64_t const *)((uint64_t)(_p) & ~(sizeof(uint64_t) - 1)) )
#define _gaba_parse_ofs(_p)				( ((uint64_t)(_p) & sizeof(uint32_t)) ? 32 : 0 )


#if BIT == 2
/* 2bit encoding */
static uint8_t const gaba_parse_ascii_fw[16] __attribute__(( aligned(16) )) = {
	'A', 'C', 'G', 'T', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N'
};
static uint8_t const gaba_parse_ascii_rv[16] __attribute__(( aligned(16) )) = {
	'T', 'G', 'C', 'A', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N'
};
#else
/* 4bit encoding */
static uint8_t const gaba_parse_ascii_fw[16] __attribute__(( aligned(16) )) = {
	'N', 'A', 'C', 'M', 'G', 'R', 'S', 'V', 'T', 'W', 'Y', 'H', 'K', 'D', 'B', 'N'
};
static uint8_t const gaba_parse_ascii_rv[16] __attribute__(( aligned(16) )) = {
	'N', 'T', 'G', 'K', 'C', 'Y', 'S', 'B', 'A', 'W', 'R', 'D', 'M', 'H', 'V', 'N'
};
#endif


/**
 * @fn gaba_parse_u64
 */
static inline
uint64_t gaba_parse_u64(
	uint64_t const *ptr,
	int64_t pos)
{
	int64_t rem = pos & 63;
	uint64_t a = (ptr[pos>>6]>>rem) | ((ptr[(pos>>6) + 1]<<(63 - rem))<<1);
	return(a);
}

/**
 * @fn gaba_parse_dump_match_string
 */
static inline
uint64_t gaba_parse_dump_match_string(
	char *buf,
	uint64_t len)
{
	if(len < 64) {
		static uint8_t const conv[64] = {
			0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
			0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
			0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29,
			0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
			0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
			0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
			0x60, 0x61, 0x62, 0x63
		};
		char *p = buf;
		*p = (conv[len]>>4) + '0'; p += (conv[len] & 0xf0) != 0;
		*p++ = (conv[len] & 0x0f) + '0';
		*p++ = 'M';
		return(p - buf);
	} else {
		uint64_t adv;
		uint8_t b[16] = { 'M', '0' }, *p = &b[1];
		while(len != 0) { *p++ = (len % 10) + '0'; len /= 10; }
		for(p -= (p != &b[1]), adv = (int64_t)((ptrdiff_t)(p - b)) + 1; p >= b; p--) { *buf++ = *p; }
		return(adv);
	}
}

/**
 * @fn gaba_parse_dump_gap_string
 */
static inline
uint64_t gaba_parse_dump_gap_string(
	char *buf,
	uint64_t len,
	char ch)
{
	if(len < 64) {
		static uint8_t const conv[64] = {
			0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
			0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
			0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29,
			0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
			0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
			0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
			0x60, 0x61, 0x62, 0x63
		};
		char *p = buf;
		*p = (conv[len]>>4) + '0'; p += (conv[len] & 0xf0) != 0;
		*p++ = (conv[len] & 0x0f) + '0';
		*p++ = ch;
		return(p - buf);
	} else {
		uint64_t adv;
		uint8_t b[16] = { (uint8_t)ch, '0' }, *p = &b[1];
		while(len != 0) { *p++ = (len % 10) + '0'; len /= 10; }
		for(p -= (p != &b[1]), adv = (uint64_t)((ptrdiff_t)(p - b)) + 1; p >= b; p--) { *buf++ = *p; }
		return(adv);
	}
}

/**
 * @macro _gaba_parse_count_match, _gaba_parse_count_gap
 * @brief match and gap counters
 */
#define _gaba_diag(_x)			( (_x) ^ 0x5555555555555555 )
#define _gaba_parse_count_match_forward(_arr) ({ \
	tzcnt(_gaba_diag(_arr)); \
})
#define _gaba_parse_count_gap_forward(_arr) ({ \
	uint64_t _a = (_arr); \
	uint64_t mask = 0ULL - (_a & 0x01); \
	uint64_t gc = tzcnt(_a ^ mask) + (uint64_t)mask; \
	gc; \
})
#define _gaba_parse_count_match_reverse(_arr) ({ \
	lzcnt(_gaba_diag(_arr)); \
})
#define _gaba_parse_count_gap_reverse(_arr) ({ \
	uint64_t _a = (_arr); \
	uint64_t mask = (uint64_t)(((int64_t)_a)>>63); \
	uint64_t gc = lzcnt(_a ^ mask) - ((int64_t)mask + 1); \
	gc; \
})

/**
 * @fn gaba_print_cigar_forward
 * @brief parse path string and print cigar to file
 */
_GABA_PARSE_EXPORT_LEVEL
uint64_t gaba_print_cigar_forward(
	gaba_printer_t printer,
	void *fp,
	uint32_t const *path,
	uint64_t offset,
	uint64_t len)
{
	uint64_t clen = 0;

	/* convert path to uint64_t pointer */
	uint64_t const *p = _gaba_parse_ptr(path);
	uint64_t lim = offset + _gaba_parse_ofs(path) + len;
	uint64_t ridx = len;

	while(1) {
		uint64_t rsidx = ridx;
		while(1) {
			ZCNT_RESULT uint64_t m = _gaba_parse_count_match_forward(
				gaba_parse_u64(p, lim - ridx)
			);
			uint64_t a = _gaba_parse_min2(m, ridx) & ~0x01;
			ridx -= a;
			if(a < 64) { break; }
		}
		uint64_t m = (rsidx - ridx)>>1;
		if(m > 0) {
			clen += printer(fp, m, 'M');
		}
		if(ridx == 0) { break; }

		uint64_t arr;
		uint64_t g = _gaba_parse_min2(
			_gaba_parse_count_gap_forward(arr = gaba_parse_u64(p, lim - ridx)),
			ridx
		);
		if(g > 0) {
			clen += printer(fp, g, 'D' + ((char)(0ULL - (arr & 0x01)) & ('I' - 'D')));
		}
		if((ridx -= g) <= 1) { break; }
	}
	return(clen);
}

/**
 * @fn gaba_dump_cigar_forward
 * @brief parse path string and store cigar to buffer
 */
_GABA_PARSE_EXPORT_LEVEL
uint64_t gaba_dump_cigar_forward(
	char *buf,
	uint64_t buf_size,
	uint32_t const *path,
	uint64_t offset,
	uint64_t len)
{
	uint64_t const filled_len_margin = 5;
	char *b = buf, *blim = buf + buf_size - filled_len_margin;

	/* convert path to uint64_t pointer */
	uint64_t const *p = _gaba_parse_ptr(path);
	uint64_t lim = offset + _gaba_parse_ofs(path) + len;
	uint64_t ridx = len;

	while(1) {
		uint64_t rsidx = ridx;
		while(1) {
			ZCNT_RESULT uint64_t m = _gaba_parse_count_match_forward(
				gaba_parse_u64(p, lim - ridx)
			);
			uint64_t a = _gaba_parse_min2(m, ridx) & ~0x01;
			ridx -= a;
			if(a < 64) { break; }
		}
		uint64_t m = (rsidx - ridx)>>1;
		if(m > 0) {
			b += gaba_parse_dump_match_string(b, m);
		}
		if(ridx == 0 || b > blim) { break; }

		uint64_t arr;
		uint64_t g = _gaba_parse_min2(
			_gaba_parse_count_gap_forward(arr = gaba_parse_u64(p, lim - ridx)),
			ridx
		);
		if(g > 0) {
			b += gaba_parse_dump_gap_string(b, g, 'D' + ((char)(0ULL - (arr & 0x01)) & ('I' - 'D')));
		}
		if((ridx -= g) <= 1 || b > blim) { break; }
	}
	*b = '\0';
	return(b - buf);
}

/**
 * @fn gaba_print_cigar_reverse
 * @brief parse path string and print cigar to file
 */
_GABA_PARSE_EXPORT_LEVEL
uint64_t gaba_print_cigar_reverse(
	gaba_printer_t printer,
	void *fp,
	uint32_t const *path,
	uint64_t offset,
	uint64_t len)
{
	int64_t clen = 0;

	/* convert path to uint64_t pointer */
	uint64_t const *p = _gaba_parse_ptr(path);
	uint64_t ofs = (int64_t)offset + _gaba_parse_ofs(path) - 64;
	uint64_t idx = len;

	while(1) {
		uint64_t sidx = idx;
		while(1) {
			ZCNT_RESULT uint64_t m = _gaba_parse_count_match_reverse(
				gaba_parse_u64(p, idx + ofs)
			);
			uint64_t a = _gaba_parse_min2(m, idx) & ~0x01;
			idx -= a;
			if(a < 64) { break; }
		}
		uint64_t m = (sidx - idx)>>1;
		if(m > 0) {
			clen += printer(fp, m, 'M');
		}
		if(idx == 0) { break; }

		uint64_t arr;
		uint64_t g = _gaba_parse_min2(
			_gaba_parse_count_gap_reverse(arr = gaba_parse_u64(p, idx + ofs)),
			idx
		);
		if(g > 0) {
			clen += printer(fp, g, 'D' + ((char)(((int64_t)arr)>>63) & ('I' - 'D')));
		}
		if((idx -= g) <= 1) { break; }
	}
	return(clen);
}

/**
 * @fn gaba_dump_cigar_reverse
 * @brief parse path string and store cigar to buffer
 */
_GABA_PARSE_EXPORT_LEVEL
uint64_t gaba_dump_cigar_reverse(
	char *buf,
	uint64_t buf_size,
	uint32_t const *path,
	uint64_t offset,
	uint64_t len)
{
	uint64_t const filled_len_margin = 5;
	char *b = buf, *blim = buf + buf_size - filled_len_margin;

	/* convert path to int64_t pointer */
	uint64_t const *p = _gaba_parse_ptr(path);
	uint64_t ofs = (int64_t)offset + _gaba_parse_ofs(path) - 64;
	uint64_t idx = len;

	while(1) {
		uint64_t sidx = idx;
		while(1) {
			ZCNT_RESULT uint64_t m = _gaba_parse_count_match_reverse(
				gaba_parse_u64(p, idx + ofs)
			);
			uint64_t a = _gaba_parse_min2(m, idx) & ~0x01;
			idx -= a;
			if(a < 64) { break; }
		}
		uint64_t m = (sidx - idx)>>1;
		if(m > 0) {
			b += gaba_parse_dump_match_string(b, m);
		}
		if(idx == 0 || b > blim) { break; }

		uint64_t arr;
		uint64_t g = _gaba_parse_min2(
			_gaba_parse_count_gap_reverse(arr = gaba_parse_u64(p, idx + ofs)),
			idx
		);
		if(g > 0) {
			b += gaba_parse_dump_gap_string(b, g, 'D' + ((char)(((int64_t)arr)>>63) & ('I' - 'D')));
		}
		if((idx -= g) <= 1 || b > blim) { break; }
	}
	*b = '\0';
	return(b - buf);
}

#if TEMP
/**
 * @fn gaba_dump_seq_forward
 * @brief traverse the path in the forward direction and dump the sequence with appropriate gaps
 */
static inline
uint64_t gaba_dump_seq_forward(
	uint8_t *buf,
	uint64_t buf_size,
	uint32_t conf,				/* { SEQ_A, SEQ_B } x { SEQ_FW, SEQ_RV } */
	uint32_t const *path,
	uint64_t offset,
	uint64_t len,
	uint8_t const *seq,			/* a->seq[s->alen] when SEQ_RV */
	char gap)					/* gap char, '-' */
{
	#define _dump_gap(_r, _q, _c) { \
		for((_r) += (_c); (_c) > 0; (_c) -= MIN2(_c, 16)) { \
			_storeu_v16i8((_r) - (_c), gv); \
		} \
	}
	#define _dump_fw(_r, _q, _c) { \
		for((_q) += (_c), (_r) += (_c); (_c) > 0; (_c) -= MIN2(_c, 16)) { \
			_storeu_v16i8((_r) - (_c), _shuf_v16i8(cv, _loadu_v16i8((_q) - (_c)))); \
		} \
	}
	#define _dump_rv(_r, _q, _c) { \
		for((_r) += (_c); (_c) > 0; (_c) -= MIN2(_c, 16)) { \
			uint64_t _l = MIN2(_c, 16); \
			_storeu_v16i8((_r) - (_c), _shuf_v16i8(cv, \
				_swapn_v16i8(_loadu_v16i8((_q) + (_c) - (_l)), (_l)) \
			)); \
		} \
	}
	#define _loop(_ins_dump, _del_dump, _diag_dump) { \
		while((int64_t)ridx > 0) { \
			uint64_t c; \
			/* insertions */ \
			ridx -= (c = lzcnt(~gaba_parse_u64(p, lim - ridx))); \
			_ins_dump(r, q, c); \
			/* deletions */ \
			ridx -= (c = lzcnt(gaba_parse_u64(p, lim - ridx)) - 1); \
			_del_dump(r, q, c);		/* at most 64bp */ \
			/* diagonals */ \
			do { \
				ridx -= 2 * (c = (lzcnt(_gaba_diag(gaba_parse_u64(p, lim - ridx)))>>1)); \
				_diag_dump(r, q, c); \
			} while(c == 32); \
		} \
	}

	v16i8_t const cv = _load_v16i8((conf & GABA_SEQ_RV) ? gaba_parse_ascii_rv : gaba_parse_ascii_fw);
	v16i8_t const gv = _seta_v16i8(gap);
	uint64_t const *p = _gaba_parse_ptr(path);
	uint64_t lim = offset + _gaba_parse_ofs(path) + len, ridx = len;
	uint8_t const *q = seq;
	uint8_t *r = buf;
	switch(conf) {
		case GABA_SEQ_A | GABA_SEQ_FW: _loop(_dump_gap, _dump_fw, _dump_fw); break;
		case GABA_SEQ_A | GABA_SEQ_RV: _loop(_dump_gap, _dump_rv, _dump_rv); break;
		case GABA_SEQ_B | GABA_SEQ_FW: _loop(_dump_fw, _dump_gap, _dump_fw); break;
		case GABA_SEQ_B | GABA_SEQ_RV: _loop(_dump_rv, _dump_gap, _dump_rv); break;
	}
	*r++ = '\0';
	return(r - buf);

	#undef _dump_gap
	#undef _dump_fw
	#undef _dump_rv
	#undef _loop
}

/**
 * @fn gaba_dump_seq_reverse
 * @brief traverse the path in the reverse direction and dump the sequence with appropriate gaps
 */
static inline
uint64_t gaba_dump_seq_reverse(
	uint8_t *buf,
	uint64_t buf_size,
	uint32_t conf,				/* { SEQ_A, SEQ_B } x { SEQ_FW, SEQ_RV } */
	uint32_t const *path,
	uint32_t offset,
	uint32_t len,
	uint8_t const *seq,			/* a->seq[s->alen] when SEQ_RV */
	char gap)					/* gap char, '-' */
{
	#define _dump_gap(_r, _q, _c) { \
		for((_r) += (_c); (_c) > 0; (_c) -= MIN2(_c, 16)) { \
			_storeu_v16i8((_r) - (_c), gv); \
		} \
	}
	#define _dump_fw(_r, _q, _c) { \
		for((_q) += (_c), (_r) += (_c); (_c) > 0; (_c) -= MIN2(_c, 16)) { \
			_storeu_v16i8((_r) - (_c), _shuf_v16i8(cv, _loadu_v16i8((_q) - (_c)))); \
		} \
	}
	#define _dump_rv(_r, _q, _c) { \
		for((_q) += (_c), (_r) += (_c); (_c) > 0; (_c) -= MIN2(_c, 16)) { \
			_storeu_v16i8((_r) - (_c), _shuf_v16i8(cv, _loadu_v16i8((_q) - (_c)))); \
		} \
	}
	#define _loop(_ins_dump, _del_dump, _diag_dump) { \
		while((int64_t)idx > 0) { \
			uint64_t c; \
			/* insertions */ \
			idx -= (c = lzcnt(~gaba_parse_u64(p, ofs + idx))); \
			_ins_dump(r, q, c); \
			/* deletions */ \
			idx -= (c = lzcnt(gaba_parse_u64(p, ofs + idx)) - 1); \
			_del_dump(r, q, c);		/* at most 64bp */ \
			/* diagonals */ \
			do { \
				idx -= 2 * (c = (lzcnt(_gaba_diag(gaba_parse_u64(p, ofs + idx)))>>1)); \
				_diag_dump(r, q, c); \
			} while(c == 32); \
		} \
	}

	v16i8_t const cv = _load_v16i8((conf & GABA_SEQ_RV) ? gaba_parse_ascii_rv : gaba_parse_ascii_fw);
	v16i8_t const gv = _seta_v16i8(gap);
	uint64_t const *p = _gaba_parse_ptr(path);
	uint64_t ofs = (int64_t)offset + _gaba_parse_ofs(path) - 64, idx = len;
	uint8_t const *q = seq;
	uint8_t *r = buf;
	switch(conf) {
		case GABA_SEQ_A | GABA_SEQ_FW: _loop(_dump_gap, _dump_fw, _dump_fw); break;
		case GABA_SEQ_A | GABA_SEQ_RV: _loop(_dump_gap, _dump_rv, _dump_rv); break;
		case GABA_SEQ_B | GABA_SEQ_FW: _loop(_dump_fw, _dump_gap, _dump_fw); break;
		case GABA_SEQ_B | GABA_SEQ_RV: _loop(_dump_rv, _dump_gap, _dump_rv); break;
	}
	*r++ = '\0';
	return(r - buf);

	#undef _dump_gap
	#undef _dump_fw
	#undef _dump_rv
	#undef _loop
}

/**
 * @fn gaba_dump_seq_ref, gaba_dump_seq_query
 */
static inline
uint64_t gaba_dump_seq_ref(
	uint8_t *buf,
	uint64_t buf_size,
	uint32_t const *path,
	gaba_path_section_t const *s,
	gaba_section_t const *a,
	uint8_t const *alim)
{
	uint8_t const *base = (a->aid & 0x01)
		? gaba_rev(a->base, alim)
		? &a->base[a->apos];

	return(gaba_dump_seq_forward(
		buf, buf_size,
		a->base >= alim ? 
		path, s->ppos, gaba_len(a),
		a->base >= alim ? &a->base[s->apos] : gaba_rev(&a->base[s->apos], alim), '-'
	));
}
static inline
uint64_t gaba_dump_seq_query(
	uint8_t *buf,
	uint64_t buf_size,
	uint32_t const *path,
	gaba_path_section_t const *s,
	gaba_section_t const *b,
	uint8_t const *alim)
{
	return;
}

/**
 * @fn gaba_calc_score
 */
static inline
int64_t gaba_calc_score(
	uint32_t const *path,
	gaba_path_section_t const *s,
	gaba_section_t const *a,
	gaba_section_t const *b)
{
	return;
}

/**
 * @fn gaba_dump_sam_md
 * @brief print sam MD tag
 */
_GABA_PARSE_EXPORT_LEVEL
void gaba_dump_sam_md(
	uint8_t *buf,
	uint64_t buf_size,
	uint32_t const *path,
	gaba_path_section_t const *s,
	gaba_section_t const *a,
	gaba_section_t const *b)
{
	uint64_t dir = (s->bid & 0x01) ? 1 : 0;
	static uint8_t const comp[16] __attribute__(( aligned(16) )) = {
		0x00, 0x08, 0x04, 0x0c, 0x02, 0x0a, 0x06, 0x0e,
		0x01, 0x09, 0x05, 0x0d, 0x03, 0x0b, 0x07, 0x0f
	};
	static uint8_t const id[16] __attribute__(( aligned(16) )) = {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
	};
	v32i8_t const cv = _from_v16i8_v32i8(_load_v16i8(dir ? comp : id));

	uint64_t const *p = _gaba_parse_ptr(path);
	uint64_t lim = s->ppos + _gaba_parse_ofs(path) + gaba_plen(s);
	uint64_t ridx = gaba_plen(s);

	uint32_t as = s->apos, bs = s->bpos;
	uint8_t const *rp = &a->base[as], *rb = rp;
	uint8_t const *qp = &b->base[dir ? (uint64_t)b->len - bs - 32 : bs];
	while(ridx > 0) {
		/* suppose each indel block is shorter than 32 bases */
		uint64_t arr = gaba_parse_u64(p, lim - ridx);
		uint64_t cnt = tzcnt(~arr) - (arr & 0x01);		/* count #ins */
		ridx -= cnt;
		qp += dir ? -cnt : cnt;

		if((arr & 0x01) == 0) {							/* is_del */
			ridx -= cnt = tzcnt(arr);					/* count #del */
			_putn(b, (int32_t)(rp - rb));
			_put(b, '^');
			_putsnt32(b, rp, cnt, decaf);
			rp += cnt;
			rb = rp;
		}

		/* match or mismatch */
		uint64_t acnt = 32;
		while(acnt == 32) {
			/* count diagonal */
			arr = gaba_parse_u64(p, lim - ridx);
			acnt = MIN2(tzcnt(arr ^ 0x5555555555555555), ridx)>>1;

			/* load sequence to detect mismatch */
			v32i8_t rv = _shuf_v32i8(cv, _loadu_v32i8(rp)), qv = _loadu_v32i8(qp);
			if(dir) { qv = _swap_v32i8(qv); }

			/* compare and count matches */
			uint64_t mmask = (uint64_t)((v32_masku_t){ .mask = _mask_v32i8(_eq_v32i8(rv, qv)) }).all;
			uint64_t mcnt = MIN2(acnt, tzcnt(~mmask));

			/* adjust pos */
			ridx -= 2*mcnt;
			rp += mcnt;
			qp += dir ? -mcnt : mcnt;

			if(mcnt >= acnt) { continue; }				/* continues longer than 32bp */
			_putn(b, (int32_t)(rp - rb));				/* print match length */
			ridx -= 2*(cnt = MIN2(tzcnt(mmask>>mcnt), acnt - mcnt));
			qp += dir ? -cnt : cnt;
			for(uint64_t i = 0; i < cnt - 1; i++) {
				_put(b, decaf[*rp]);		/* print mismatch base */
				_put(b, '0');							/* padding */
				rp++;
			}
			_put(b, decaf[*rp]);			/* print mismatch base */
			rp++;
			rb = rp;
		}
	}
	_putn(b, (int32_t)(rp - rb));						/* always print tail even if # == 0 */
	return;
}
#endif

#endif /* _GABA_PARSE_H_INCLUDED */
/**
 * end of gaba_parse.h
 */
