
/**
 * @file branch2.c
 *
 * @brief libsea3 API implementations
 *
 * @author Hajime Suzuki
 * @date 2016/1/11
 * @license Apache v2
 */
#include <stdint.h>
#include "sea.h"
#include "arch/arch.h"

/* aliasing vector macros */
#define _VECTOR_ALIAS_PREFIX	v32i8
#include "arch/vector_alias.h"

/* include utils */
#include "util/util.h"

#define BW 							( 32 )
#define BLK 						( 32 )
#define MIN_BULK_BLOCKS				( 32 )
#define SEA_MEM_INIT_SIZE			( (uint64_t)32 * 1024 * 1024 )
#define SEA_MEM_MARGIN_SIZE			( 2048 )

/* forward declarations */
static int32_t sea_dp_add_stack(struct sea_dp_context_s *this);
static void *sea_dp_malloc(struct sea_dp_context_s *this, uint64_t size);
static void *sea_dp_smalloc(struct sea_dp_context_s *this, uint64_t size);
static void sea_dp_free(struct sea_dp_context_s *this, void *ptr);


/**
 * @macro _match
 * @brief alias to sequence matcher macro
 */
#ifndef _match
// #define _match			_eq 		/* for ASCII encoded sequence */
#define _match		_or 		/* for 2bit encoded */
// #define _match		_and		/* for 4bit encoded */
#endif /* _match */

/**
 * @macro _likely, _unlikely
 * @brief branch prediction hint for gcc-compatible compilers
 */
#define _likely(x)		__builtin_expect((x), 1)
#define _unlikely(x)	__builtin_expect((x), 0)

/**
 * @macro _force_inline
 * @brief inline directive for gcc-compatible compilers
 */
#define _force_inline	inline

/**
 * @macro _load_sc
 * @brief load constant vectors from sea_dp_context_s
 */
#define _load_sc(this, name)	( _bc_v16i8(_load_v16i8((this)->scv.name)) )

/* direction macros */
#define DYNAMIC 		( 1 )
#if DYNAMIC
#define direction_prefix 		dynamic_

/* direction determiners for the dynamic band algorithms */
/**
 * @macro _dir_fetch
 */
#define _dir_fetch(_dir) { \
	(_dir).dynamic.array <<= 1; \
	(_dir).dynamic.array |= (uint32_t)((_dir).dynamic.acc < 0); \
}
/**
 * @macro _dir_update
 * @brief update direction determiner for the next band
 */
#define _dir_update(_dir, _vector, _sign) { \
	(_dir).dynamic.acc += (_sign) * (_ext(_vector, 0) - _ext(_vector, BW-1)); \
	/*debug("acc(%d), (%d, %d)", (_dir).dynamic.acc, _ext(_vector, 0), _ext(_vector, BW-1));*/ \
}
/**
 * @macro _dir_adjust_reminder
 * @brief adjust direction array when termination is detected in the middle of the block
 */
#define _dir_adjust_reminder(_dir, i) { \
	(_dir).dynamic.array <<= (BLK - (i) - 1); \
}
/**
 * @macro _dir_test_bound, _dir_test_bound_cap
 * @brief test if the bound of the direction array is invaded
 */
#define _dir_test_bound(_dir, k, p) 		( /* nothing to do */ )
#define _dir_test_bound_cap(_dir, k, p)		( /* nothing to do */ )
/**
 * @macro _dir_is_down, _dir_is_right
 * @brief direction indicator (_dir_is_down returns true if dir == down)
 */
#define _dir_is_down(_dir)					( (_dir).dynamic.array & 0x01 )
#define _dir_is_right(_dir)					( !_dir_is_down(_dir) )
/**
 * @macro _dir_load
 */
#define _dir_load(_blk, _local_idx) ({ \
	union sea_dir_u _dir = (_blk)->dir; \
	_dir.dynamic.array >>= (BLK - (_local_idx) - 1); \
	_dir; \
})
/**
 * @macro _dir_windback
 */
#define _dir_windback(_dir) { \
	(_dir).dynamic.array >>= 1; \
}

#else

#define direction_prefix 		guided_

/* direction determiners for the guided band algorithms */
#define _dir_fetch(_dir)					{ /* nothing to do */ }
#define _dir_update(_dir, _vector) { \
	(_dir).guided.ptr++; \
}
#define _dir_adjust_reminder(_dir, i)		{ /* nothing to do */ }
#define _dir_test_bound(_dir, k, p) ( \
	((k)->tdr - (_dir).guided.ptr + 2) - (p) - BLK \
)
#define _dir_test_bound_cap(_dir, k, p) ( \
	((k)->tdr - (_dir).guided.ptr + 2) - (p) \
)
#define _dir_is_down(_dir)					( *(_dir).guided.ptr != 0 )
#define _dir_is_right(_dir)					( *(_dir).guided.ptr == 0 )
#define _dir_load(_blk, _local_idx) ({ \
	union sea_dir_u _dir = (_blk)->dir; \
	_dir.guided.ptr -= (BLK - (_local_idx)); \
	_dir; \
})
#define _dir_windback(_dir) { \
	(_dir).guided.ptr--; \
}

#endif

/**
 * seqreader macros
 */
#define _rd_pos1(k)		_pv_v2i64(&(k)->rr.body.apos)
#define _rd_pos2(k)		_pv_v2i64(&(k)->rr.tail.apos)
#define _rd_len1(k)		_pv_v2i32(&(k)->rr.body.alen)
#define _rd_len2(k)		_pv_v2i32(&(k)->rr.tail.alen)
#define _rd_cnt(k)		_pv_v2i32(&(k)->rr.acnt)
#define _rd_rem2(k)		_pv_v2i32(&(k)->rr.arem)

#define _rd_curr(k)		_pv_v2i64(&(k)->rr.curr.apos)
#define _rd_next(k)		_pv_v2i64(&(k)->rr.next.apos)
#define _rd_len(k)		_pv_v2i32(&(k)->rr.alen)
#define _rd_lim(k)		_pv_v2i32(&(k)->rr.alim)
#define _rd_rem(k)		_pv_v2i32(&(k)->rr.arem)

// #define _rd_len(k)		_pv_v2i64(&(k)->work.lena)
#define _rd_bufa_base(k)		( (k)->rr.bufa + BLK + BW )
#define _rd_bufb_base(k)		( (k)->rr.bufb )
#define _rd_bufa(k, pos, len)	( _rd_bufa_base(k) - (pos) - (len) )
#define _rd_bufb(k, pos, len)	( _rd_bufb_base(k) + (pos) )
#define _lo64(v)		_ext_v2i64(v, 0)
#define _hi64(v)		_ext_v2i64(v, 1)
#define _lo32(v)		_ext_v2i32(v, 0)
#define _hi32(v)		_ext_v2i32(v, 1)

/*
 * @fn fill_load_section
 */
static _force_inline
void fill_load_section(
	struct sea_dp_context_s *this,
	struct sea_section_s const *curr,
	struct sea_section_s const *next,
	uint64_t plim)
{
	v2i32_t const z = _zero_v2i32();
	v2i32_t const bw = _set_v2i32(BW);	

	/* load current section lengths */
	v2i64_t curr_pos = _loadu_v2i64(&curr->apos);
	v2i32_t len = _loadu_v2i32(&curr->alen);

	/* convert current section to (pos of the end, len) */
	curr_pos = _add_v2i64(curr_pos, _cvt_v2i32_v2i64(len));

	_print_v2i64(curr_pos);
	_print_v2i32(len);

	/* load the next section lengths */
	v2i64_t next_pos;
	v2i32_t lim;
	v2i32_t rem;
	if(next == NULL) {
		/* next section not given */
		next_pos = _set_v2i64(-1);
		lim = bw;
		rem = z;
	} else {
		/* calculate remaining lengths */
		next_pos = _loadu_v2i64(&next->apos);
		v2i32_t tlim = _loadu_v2i32(&next->alen);
		lim = _min_v2i32(tlim, bw);
		rem = _sub_v2i32(tlim, lim);
	}
	/* negate lim */
	lim = _sub_v2i32(z, lim);

	_print_v2i64(next_pos);
	_print_v2i32(lim);
	_print_v2i32(rem);

	/* store sections */
	this->rr.plim = plim;
	_store_v2i64(_rd_curr(this), curr_pos);
	_store_v2i64(_rd_next(this), next_pos);
	_store_v2i32(_rd_len(this), len);
	_store_v2i32(_rd_lim(this), lim);
	_store_v2i32(_rd_rem(this), rem);

	return;
}

/**
 * @fn fill_store_section
 */
static _force_inline
void fill_store_section(
	struct sea_dp_context_s *this,
	struct sea_joint_tail_s *tail)
{
	v2i32_t const z = _zero_v2i32();

	/* load section informations */
	v2i64_t curr_pos = _load_v2i64(_rd_curr(this));
	v2i64_t next_pos = _load_v2i64(_rd_next(this));
	v2i32_t len = _load_v2i32(_rd_len(this));
	v2i32_t lim = _load_v2i32(_rd_lim(this));
	v2i32_t rem = _load_v2i32(_rd_rem(this));

	/* adjust the current section pos */
	v2i32_t curr_len = _max_v2i32(len, z);
	curr_pos = _sub_v2i64(curr_pos, _cvt_v2i32_v2i64(curr_len));

	/* adjust the next section lengths */
	v2i32_t next_len_adj = _sub_v2i32(_sub_v2i32(len, curr_len), lim);
	v2i32_t next_len = _add_v2i32(rem, next_len_adj);

	/* make mask */
	v2i32_t mask32 = _eq_v2i32(curr_len, z);
	v2i64_t mask64 = _cvt_v2i32_v2i64(mask32);

	/* store rem section */
	_store_v2i64(&tail->rem.apos, _sel_v2i64(mask64, next_pos, curr_pos));
	_store_v2i32(&tail->rem.alen, _sel_v2i32(mask32, next_len, curr_len));

	_print_v2i64(_sel_v2i64(mask64, curr_pos, next_pos));
	_print_v2i32(_sel_v2i32(mask32, curr_len, next_len));

	return;
}

/**
 * @fn fill_create_head
 * @brief create joint_head on the stack to start block extension
 */
static _force_inline
struct sea_block_s *fill_create_head(
	struct sea_dp_context_s *this,
	struct sea_joint_tail_s const *prev_tail)
{
	/* init working stack */
	struct sea_joint_head_s *head = (struct sea_joint_head_s *)this->stack_top;
	head->head = NULL;
	head->cigar = NULL;
	head->tail = prev_tail;

	/* copy phantom vectors */
	struct sea_block_s *blk = _phantom_block(head + 1);
	_memcpy_blk_aa(
		(uint8_t *)blk + SEA_BLOCK_PHANTOM_OFFSET,
		(uint8_t *)_last_block(prev_tail) + SEA_BLOCK_PHANTOM_OFFSET,
		SEA_BLOCK_PHANTOM_SIZE);
	debug("blk(%p)", blk);

	/* copy section lengths */
	v2i32_t len = _load_v2i32(_rd_len(this));
	_store_v2i32(&blk->alen, len);

	_print_v2i32(len);

	return(blk + 1);
}

/**
 * @fn fill_create_tail
 * @brief create joint_tail at the end of the blocks
 */
static _force_inline
struct sea_joint_tail_s *fill_create_tail(
	struct sea_dp_context_s *this,
	struct sea_joint_tail_s const *prev_tail,
	struct sea_block_s *blk,
	int64_t p)
{
	debug("create tail");

	/* write back lengths */
	v2i32_t len = _load_v2i32(&(blk - 1)->alen);
	_store_v2i32(_rd_len(this), len);
	_print_v2i32(len);

	/* create joint_tail */
	struct sea_joint_tail_s *tail = (struct sea_joint_tail_s *)blk;
	this->stack_top = (void *)(tail + 1);	/* write back stack_top */
	tail->v = prev_tail->v;					/* copy middle deltas */

	/* save misc to joint_tail */
	tail->psum = p + prev_tail->psum;
	tail->p = p;
	debug("psum(%lld), p(%d), v(%p)", tail->psum, tail->p, prev_tail->v);

	/* search max section */
	v32i16_t sd = _cvt_v32i8_v32i16(_load(&(blk - 1)->sd.max));
	_print_v32i16(sd);
	v32i16_t md = _load_v32i16(prev_tail->v);
	_print_v32i16(md);

	/* extract max */
	md = _add_v32i16(md, sd);
	int16_t max = _hmax_v32i16(md);
	_print_v32i16(md);

	/* determine q-coordinate */
	vec_mask_t mask_max = _mask_v32i16(_eq_v32i16(_set_v32i16(max), md));

	/* store */
	tail->mask_max.mask = mask_max;
	tail->max = max + (blk - 1)->offset;

	debug("offset(%lld)", (blk - 1)->offset);
	debug("max(%d)", _hmax_v32i16(_add_v32i16(md, sd)));
	debug("mask_max(%u)", tail->mask_max.all);

	/* misc */
	tail->internal = 0;

	fill_store_section(this, tail);
	return(tail);
}

/**
 * @fn fill_bulk_fetch
 *
 * @brief fetch 32bases from current section
 */
static _force_inline
v2i32_t fill_bulk_fetch(
	struct sea_dp_context_s *this,
	struct sea_block_s *blk)
{
	/* load sequence from previous block */
	vec_t const mask = _set(0x0f);
	vec_t w = _load(&(blk - 1)->ch.w);
	vec_t a = _and(mask, w);
	vec_t b = _and(mask, _shr(w, 4));

	_print(w);
	_print(a);
	_print(b);

	debug("apos(%lld), alen(%d)", this->rr.curr.apos, (blk-1)->alen);
	debug("bpos(%lld), blen(%d)", this->rr.curr.bpos, (blk-1)->blen);

	/* fetch seq a */
	#define _ra(x)		( rev((x), this->rr.p.alen) )
	rd_load(this->r.loada,
		_rd_bufa(this, BW, BLK),			/* dst */
		this->rr.p.pa,						/* src */
		_ra(this->rr.curr.apos - (blk - 1)->alen),/* pos */
		this->rr.p.alen,					/* lim */
		BLK);								/* len */
	#undef _ra
	_store(_rd_bufa(this, 0, BW), a);

	/* fetch seq b */
	_store(_rd_bufb(this, 0, BW), b);
	rd_load(this->r.loadb,
		_rd_bufb(this, BW, BLK),			/* dst */
		this->rr.p.pb,						/* src */
		this->rr.curr.bpos - (blk - 1)->blen,/* pos */
		this->rr.p.blen,					/* lim */
		BLK);								/* len */

	return(_seta_v2i32(BLK, BLK));
}

/**
 * @fn fill_cap_fetch
 *
 * @return the length of the sequence fetched.
 */
static _force_inline
v2i32_t fill_cap_fetch(
	struct sea_dp_context_s *this,
	struct sea_block_s *blk)
{
	/* const */
	v2i32_t const z = _zero_v2i32();
	v2i32_t const tot = _set_v2i32(BLK);

	/* load lengths */
	v2i32_t len = _load_v2i32(&(blk - 1)->alen);
	v2i32_t lim = _load_v2i32(_rd_lim(this));

	/* clip len1 with max load length */
	v2i32_t ofs1 = _max_v2i32(len, z);
	v2i32_t len1 = _min_v2i32(ofs1, tot);

	/* clip len2 with max - len1 */
	v2i32_t ofs2 = _sub_v2i32(len, ofs1);
	v2i32_t len2 = _min_v2i32(_sub_v2i32(tot, len1), _sub_v2i32(ofs2, lim));

	/* load sequence from previous block */
	vec_t const mask = _set(0x0f);
	vec_t w = _load(&(blk - 1)->ch.w);
	vec_t a = _and(mask, w);
	vec_t b = _and(mask, _shr(w, 4));

	_print(w);
	_print(a);
	_print(b);

	/* fetch seq a */
	#define _ra(x)		( rev((x), this->rr.p.alen) )
	rd_load(this->r.loada,
		_rd_bufa(this, BW + _lo32(len1), _lo32(len2)),	/* dst */
		this->rr.p.pa,						/* src */
		_ra(this->rr.next.apos - _lo32(ofs2)),/* pos */
		this->rr.p.alen,					/* lim */
		_lo32(len2));						/* len */
	rd_load(this->r.loada,
		_rd_bufa(this, BW, _lo32(len1)),	/* dst */
		this->rr.p.pa,						/* src */
		_ra(this->rr.curr.apos - _lo32(ofs1)),/* pos */
		this->rr.p.alen,					/* lim */
		_lo32(len1));						/* len */
	#undef _ra
	_store(_rd_bufa(this, 0, BW), a);

	/* fetch seq b */
	_store(_rd_bufb(this, 0, BW), b);
	rd_load(this->r.loadb,
		_rd_bufb(this, BW, _hi32(len1)),	/* dst */
		this->rr.p.pb,						/* src */
		this->rr.curr.bpos - _hi32(ofs1),	/* pos */
		this->rr.p.blen,					/* lim */
		_hi32(len1));						/* len */
	rd_load(this->r.loadb,
		_rd_bufb(this, BW + _hi32(len1), _hi32(len2)),
		this->rr.p.pb,						/* src */
		this->rr.next.bpos - _hi32(ofs2),	/* pos */
		this->rr.p.blen,					/* lim */
		_hi32(len2));						/* len */

	return(_add_v2i32(len1, len2));
}

/**
 * @macro fill_restore_fetch
 * @brief fetch sequence from existing block
 */
static _force_inline
void fill_restore_fetch(
	struct sea_dp_context_s *this,
	struct sea_block_s *blk)
{
	vec_t const mask = _set(0x0f);

	/* calc cnt */
	v2i32_t curr_len = _load_v2i32(&blk->alen);
	v2i32_t prev_len = _load_v2i32(&(blk - 1)->alen);
	v2i32_t cnt = _sub_v2i32(prev_len, curr_len);

	/* from current block */
	vec_t cw = _load(&blk->ch.w);
	vec_t ca = _and(mask, cw);
	vec_t cb = _and(mask, _shr(cw, 4));
	_storeu(_rd_bufa(this, _lo32(cnt), BW), ca);
	_storeu(_rd_bufb(this, _hi32(cnt), BW), cb);

	/* from previous block */
	vec_t pw = _load(&(blk - 1)->ch.w);
	vec_t pa = _and(mask, pw);
	vec_t pb = _and(mask, _shr(pw, 4));
	_store(_rd_bufa(this, 0, BW), pa);
	_store(_rd_bufb(this, 0, BW), pb);

	return;
}

/**
 * @macro fill_bulk_update_section
 */
static _force_inline
void fill_bulk_update_section(
	struct sea_dp_context_s *this,
	struct sea_block_s *blk,
	v2i32_t cnt)
{
	/* update pos and len */
	v2i32_t len = _load_v2i32(&(blk - 1)->alen);
	len = _sub_v2i32(len, cnt);
	_print_v2i32(len);

	/* store len to block */
	_store_v2i32(&blk->alen, len);

	vec_t a = _loadu(_rd_bufa(this, _lo32(cnt), BW));
	vec_t b = _loadu(_rd_bufb(this, _hi32(cnt), BW));
	_store(&blk->ch.w, _or(a, _shl(b, 4)));

	_print(a);
	_print(b);
	return;
}

/**
 * @macro fill_cap_update_section
 */
static _force_inline
void fill_cap_update_section_intl(
	struct sea_dp_context_s *this,
	struct sea_block_s *blk,
	v2i32_t cnt,
	v2i32_t len)
{
	/* load lim */
	v2i32_t lim = _load_v2i32(_rd_lim(this));

	/* update lengths */	
	len = _max_v2i32(_sub_v2i32(len, cnt), lim);
	_print_v2i32(len);
	_print_v2i32(lim);

	/* store len to block */
	_store_v2i32(&blk->alen, len);

	vec_t a = _loadu(_rd_bufa(this, _lo32(cnt), BW));
	vec_t b = _loadu(_rd_bufb(this, _hi32(cnt), BW));
	_store(&blk->ch.w, _or(a, _shl(b, 4)));

	_print(a);
	_print(b);
	return;
}
static _force_inline
void fill_cap_update_section(
	struct sea_dp_context_s *this,
	struct sea_block_s *blk,
	v2i32_t cnt)
{
	fill_cap_update_section_intl(
		this, blk, cnt, _load_v2i32(&(blk - 1)->alen));
	return;
}

/**
 * @fn fill_init_seq
 */
static _force_inline
struct sea_chain_status_s fill_init_seq(
	struct sea_dp_context_s *this,
	struct sea_joint_tail_s const *prev_tail,
	struct sea_section_s const *curr,
	struct sea_section_s const *next,
	uint64_t plim)
{
	struct sea_block_s *blk = _block(prev_tail);

	/* load section */
	fill_load_section(this, curr, next, plim);

	/* load last sequence buffer */
	vec_t const mask = _set(0x0f);
	vec_t w = _load(&(blk - 1)->ch.w);
	vec_t a = _and(mask, w);
	vec_t b = _and(mask, _shr(w, 4));
	_store(_rd_bufa(this, 0, BW), a);
	_store(_rd_bufb(this, 0, BW), b);

	/* check if additional fetch is needed */
	if(prev_tail->psum < 2) {
		v2i32_t const tot = _set_v2i32(BW/2);

		/* calc fetched sequence length */
		v2i32_t plen = _load_v2i32(&prev_tail->init_alen);
		_print_v2i32(plen);

		/* cap fetch */
		v2i32_t flen = fill_cap_fetch(this, blk);
		_print_v2i32(flen);

		/* update section */
		v2i32_t alen = _add_v2i32(flen, plen);
		v2i32_t blen = _add_v2i32(_swap_v2i32(alen), _seta_v2i32(0, 1));
		v2i32_t clen = _min_v2i32(_min_v2i32(alen, blen), tot);
		v2i32_t cnt = _sub_v2i32(clen, plen);

		_print_v2i32(alen);
		_print_v2i32(blen);
		_print_v2i32(clen);
		_print_v2i32(cnt);

		blk = fill_create_head(this, prev_tail);
		fill_cap_update_section_intl(
			this, blk - 1, cnt, _load_v2i32(_rd_len(this)));
	
		dump(blk - 1, sizeof(struct sea_block_s));

		prev_tail = fill_create_tail(
			this, prev_tail, blk, _lo32(cnt) + _hi32(cnt));
		_store_v2i32(&prev_tail->init_alen, clen);
	}
	return((struct sea_chain_status_s){
		.sec = _fill(prev_tail),
		.rem = &prev_tail->rem
	});
}

/**
 * @macro _fill_load_contexts
 * @brief load vectors onto registers
 */
#define _fill_load_contexts(_blk) \
	debug("blk(%p)", (_blk)); \
	/* load direction determiner */ \
	union sea_dir_u dir = ((_blk) - 1)->dir; \
	/* load large offset */ \
	int64_t offset = ((_blk) - 1)->offset; \
	/*debug("offset(%lld)", offset);*/ \
	/* load sequence buffer offset */ \
	uint8_t *aptr = _rd_bufa(this, 0, BW); \
	uint8_t *bptr = _rd_bufb(this, 0, BW); \
	/* load mask pointer */ \
	union sea_mask_pair_u *mask_ptr = (_blk)->mask; \
	/* load vector registers */ \
	vec_t const mask = _set(0x07); \
	vec_t register dh = _load(_pv(((_blk) - 1)->diff.dh)); \
	vec_t register dv = _load(_pv(((_blk) - 1)->diff.dv)); \
	vec_t register de = _and(mask, dh); \
	vec_t register df = _and(mask, dv); \
	dh = _sar(_or(mask, dh), 3); \
	dv = _shr(_andn(mask, dv), 3); \
	df = _sub(df, dh); \
	de = _add(de, dv); \
	/*_print(dh);*/ \
	/*_print(dv);*/ \
	/*_print(de);*/ \
	/*_print(df);*/ \
	/* load delta vectors */ \
	vec_t register delta = _load(_pv(((_blk) - 1)->sd.delta)); \
	vec_t register max = _load(_pv(((_blk) - 1)->sd.max)); \
	/*_print_v32i16(_load_v32i16(this->tail->v));*/ \
	/*_print(delta);*/ \
	_print(max); \
	_print_v32i16(_add_v32i16(_cvt_v32i8_v32i16(delta), _load_v32i16(this->tail->v)));

/**
 * @macro _fill_body
 * @brief update vectors
 */
#define _fill_body() { \
	vec_t register t = _match(_loadu(aptr), _loadu(bptr)); \
	/*_print(_loadu(aptr));*/ \
	/*_print(_loadu(bptr));*/ \
	/*_print(t);*/ \
	t = _shuf(_load_sc(this, sb), t); \
	/*_print(_load_sc(this, sb));*/ \
	t = _max(de, t); \
	mask_ptr->pair.h.mask = _mask(_eq(t, de)); \
	t = _max(df, t); \
	mask_ptr->pair.v.mask = _mask(_eq(t, df)); \
	mask_ptr++; \
	df = _sub(_max(_add(df, _load_sc(this, adjv)), t), dv); \
	dv = _sub(dv, t); \
	de = _add(_max(_add(de, _load_sc(this, adjh)), t), dh); \
	t = _add(dh, t); \
	dh = dv; dv = t; \
	/*_print(dh);*/ \
	/*_print(dv);*/ \
	/*_print(de);*/ \
	/*_print(df);*/ \
}

/**
 * @macro _fill_update_delta
 * @brief update small delta vector and max vector
 */
#define _fill_update_delta(_op, _vector, _offset, _sign) { \
	delta = _op(delta, _add(_vector, _offset)); \
	/*_print(delta);*/ \
	max = _max(max, delta); \
	/*_print(max);*/ \
	_dir_update(dir, _vector, _sign); \
	/*_print_v32i16(_add_v32i16(_cvt_v32i8_v32i16(delta), _load_v32i16(this->tail->v)));*/ \
}

/**
 * @macro _fill_right, _fill_down
 * @brief wrapper of _fill_body and _fill_update_delta
 */
#define _fill_right() { \
	debug("go right"); \
	aptr--;				/* increment sequence buffer pointer */ \
	dh = _bsl(dh, 1);	/* shift left dh */ \
	df = _bsl(df, 1);	/* shift left df */ \
	_fill_body();		/* update vectors */ \
	_fill_update_delta(_sub, dh, _load_sc(this, ofsh), -1); \
}
#define _fill_down() { \
	debug("go down"); \
	bptr++;				/* increment sequence buffer pointer */ \
	dv = _bsr(dv, 1);	/* shift right dv */ \
	de = _bsr(de, 1);	/* shift right de */ \
	_fill_body();		/* update vectors */ \
	_fill_update_delta(_add, dv, _load_sc(this, ofsv), 1); \
}

/**
 * @macro _fill_update_offset
 * @brief update offset and max vector, reset the small delta
 */
#define _fill_update_offset() { \
	int8_t _cd = _ext(delta, BW/2); \
	offset += _cd; \
	delta = _sub(delta, _set(_cd)); \
	max = _sub(max, _set(_cd)); \
	/*debug("_cd(%d), offset(%lld)", _cd, offset);*/ \
}

/**
 * @macro _fill_store_vectors
 * @brief store vectors at the end of the block
 */
#define _fill_store_vectors(_blk) ({ \
	/* store direction array */ \
	(_blk)->dir = dir; \
	/* store large offset */ \
	(_blk)->offset = offset; \
	/* store diff vectors */ \
	vec_t const mask = _set(0x07); \
	de = _sub(de, dv); \
	df = _add(df, dh); \
	dh = _shl(dh, 3); \
	dv = _shl(dv, 3); \
	_store(_pv((_blk)->diff.dh), _or(_andn(mask, dh), de)); \
	_store(_pv((_blk)->diff.dv), _or(dv, df)); \
	/* store delta vectors */ \
	_store(_pv((_blk)->sd.delta), delta); \
	_store(_pv((_blk)->sd.max), max); \
	/* calc cnt */ \
	uint64_t acnt = bptr - _rd_bufb(this, 0, BW); \
	uint64_t bcnt = _rd_bufa(this, 0, BW) - aptr; \
	/*_print_v2i32(_seta_v2i32(bcnt, acnt));*/ \
	_seta_v2i32(bcnt, acnt); \
})

/**
 * @fn fill_test_xdrop
 * @brief returns negative if terminate-condition detected
 */
static _force_inline
int64_t fill_test_xdrop(
	struct sea_dp_context_s const *this,
	struct sea_block_s const *blk)
{
	return(this->tx - blk->sd.max[BW/2]);
}

/**
 * @fn fill_bulk_test_ij_bound
 * @brief returns negative if ij-bound (for the bulk fill) is invaded
 */
static _force_inline
int64_t fill_bulk_test_ij_bound(
	struct sea_dp_context_s const *this,
	struct sea_block_s const *blk)
{
	debug("test(%lld, %lld), len(%d, %d)",
		(int64_t)blk->alen - BW,
		(int64_t)blk->blen - BW,
		blk->alen, blk->blen);
	return(((int64_t)blk->alen - BW)
		 | ((int64_t)blk->blen - BW));
}

/**
 * @fn fill_cap_test_ij_bound
 * @brief returns negative if ij-bound (for the cap fill) is invaded
 */
#define _fill_cap_test_ij_bound_init(_blk) \
	uint8_t *alim = _rd_bufa(this, (_blk)->alen - this->rr.alim, BW); \
	uint8_t *blim = _rd_bufb(this, (_blk)->blen - this->rr.blim, BW);
#define _fill_cap_test_ij_bound() ({ \
	/*debug("a(%lld), b(%lld)", ((int64_t)aptr - (int64_t)alim), ((int64_t)blim - (int64_t)bptr));*/ \
	((int64_t)aptr - (int64_t)alim) | ((int64_t)blim - (int64_t)bptr); \
})
#if 0
static _force_inline
int64_t fill_cap_test_ij_bound(
	struct sea_dp_context_s const *this,
	struct sea_block_s const *blk)
{
	debug("test(%lld, %lld), len(%d, %d), cnt(%d, %d)",
		(int64_t)this->rr.body.alen - this->rr.acnt,
		(int64_t)this->rr.body.blen - this->rr.bcnt,
		this->rr.body.alen, this->rr.body.blen,
		this->rr.acnt, this->rr.bcnt);
	return(((int64_t)this->rr.body.alen - this->rr.acnt)
		 | ((int64_t)this->rr.body.blen - this->rr.bcnt));
}
#endif

/**
 * @fn fill_bulk_test_p_bound
 * @brief returns negative if p-bound (for the bulk fill) is invaded
 */
static _force_inline
int64_t fill_bulk_test_p_bound(
	struct sea_dp_context_s const *this,
	int64_t p)
{
	return(this->rr.plim - p);
}

/**
 * @fn fill_bulk_block
 * @brief fill a block
 */
static _force_inline
void fill_bulk_block(
	struct sea_dp_context_s *this,
	struct sea_block_s *blk)
{
	/* fetch sequence */
	fill_bulk_fetch(this, blk);

	/* load vectors onto registers */
	debug("blk(%p)", blk);
	_fill_load_contexts(blk);
	/**
	 * @macro _fill_block
	 * @brief an element of unrolled fill-in loop
	 */
	#define _fill_block(_direction, _label, _jump_to) { \
		_dir_fetch(dir); \
		if(_unlikely(!_dir_is_##_direction(dir))) { \
			goto _linear_fill_##_jump_to; \
		} \
		_linear_fill_##_label: _fill_##_direction(); \
		if(--i == 0) { break; } \
	}

	/* update diff vectors */
	int64_t i = BLK;
	while(1) {					/* 4x unrolled loop */
		_fill_block(down, d1, r1);
		_fill_block(right, r1, d2);
		_fill_block(down, d2, r2);
		_fill_block(right, r2, d1);
	}

	/* update seq offset */
	_fill_update_offset();

	/* store vectors */
	v2i32_t cnt = _fill_store_vectors(blk);

	/* update section */
	fill_bulk_update_section(this, blk, cnt);

	return;
}

/**
 * @struct sea_fill_status_s
 * @brief result container for block fill functions
 */
struct sea_fill_status_s {
	struct sea_block_s *blk;
	int64_t p;
	int32_t stat;
};

/**
 * @fn fill_bulk_predetd_blocks
 * @brief fill <blk_cnt> contiguous blocks without ij-bound test
 */
static _force_inline
struct sea_fill_status_s fill_bulk_predetd_blocks(
	struct sea_dp_context_s *this,
	struct sea_block_s *blk,
	uint64_t blk_cnt)
{
	int32_t stat = CONT;
	uint64_t bc = 0;
	for(bc = 0; bc < blk_cnt; bc++) {
		/* check xdrop termination */
		if(fill_test_xdrop(this, blk - 1) < 0) {
			stat = TERM; break;
		}

		/* bulk fill */
		debug("blk(%p)", blk);
		fill_bulk_block(this, blk++);
	}
	return((struct sea_fill_status_s){
		.blk = blk,
		.p = (int64_t)bc * BLK,
		.stat = stat
	});
}

/**
 * @fn fill_bulk_seq_bounded
 * @brief fill blocks with ij-bound test
 */
static _force_inline
struct sea_fill_status_s fill_bulk_seq_bounded(
	struct sea_dp_context_s *this,
	struct sea_block_s *blk)
{
	int32_t stat = CONT;

	/* init local coordinate */
	int64_t p = 0;

	/* bulk fill loop */
	while(1) {
		/* check termination */
		if((fill_test_xdrop(this, blk - 1)
		  | fill_bulk_test_ij_bound(this, blk - 1)
		  | fill_bulk_test_p_bound(this, p)) < 0) {
			break;
		}
		
		/* bulk fill */
		debug("blk(%p)", blk);
		fill_bulk_block(this, blk++);
		
		/* update p-coordinate */
		p += BLK;
	}
	if(fill_test_xdrop(this, blk) < 0) { stat = TERM; }
	return((struct sea_fill_status_s){
		.blk = blk,
		.p = p,
		.stat = stat 		/* < 0 if stat == TERM */
	});
}

/**
 * @fn fill_cap_seq_bounded
 * @brief fill blocks with cap test
 */
static _force_inline
struct sea_fill_status_s fill_cap_seq_bounded(
	struct sea_dp_context_s *this,
	struct sea_block_s *blk)
{
	int32_t stat = CONT;
	uint64_t i = 0;
	int64_t p = 0;

	do {
		/* check xdrop termination */
		if(fill_test_xdrop(this, blk - 1) < 0) {
			stat = TERM; goto _fill_cap_seq_bounded_finish;
		}
		/* fetch sequence */
		fill_cap_fetch(this, blk);

		/**
		 * @macro _fill_block_cap
		 * @brief an element of unrolled fill-in loop
		 */
		#define _fill_block_cap() { \
			_dir_fetch(dir); \
			if(_dir_is_right(dir)) { \
				_fill_right(); \
			} else { \
				_fill_down(); \
			} \
		}

		/* vectors on registers inside this block */ {
			_fill_cap_test_ij_bound_init(blk - 1);
			_fill_load_contexts(blk);

			/* update diff vectors */
			for(i = 0; i < BLK; i++) {
				_fill_block_cap();
				if(_fill_cap_test_ij_bound() < 0) {
					/* adjust remainders */
					debug("adjust remainder");

					blk->mask[BLK-1] = blk->mask[i];
					_dir_adjust_reminder(dir, i);
					p -= BLK - i - 1;
					blk++;
					break;
				}
			}
			
			/* update seq offset */
			_fill_update_offset();
			
			/* store mask and vectors */
			v2i32_t cnt = _fill_store_vectors(blk);

			/* update section */
			fill_cap_update_section(this, blk, cnt);
		}
		
		/* update block pointer and p-coordinate */
		blk++; p += BLK;

	} while(i == BLK);

	debug("blk(%p), p(%lld), stat(%d)", blk, p, stat);

_fill_cap_seq_bounded_finish:;
	return((struct sea_fill_status_s){
		.blk = blk,
		.p = p,
		.stat = stat
	});
}

/**
 * @fn calc_max_bulk_blocks_mem
 * @brief calculate maximum number of blocks (limited by stack size)
 */
static _force_inline
uint64_t calc_max_bulk_blocks_mem(
	struct sea_dp_context_s const *this)
{
	uint64_t const rem = sizeof(struct sea_joint_head_s)
					   + sizeof(struct sea_joint_tail_s)
					   + 3 * sizeof(struct sea_block_s);
	uint64_t mem_size = this->stack_end - this->stack_top;
	return((mem_size - rem) / sizeof(struct sea_block_s) / BLK);
}

/**
 * @fn calc_max_bulk_blocks_seq
 * @brief calculate maximum number of blocks (limited by seq bounds)
 */
static _force_inline
uint64_t calc_max_bulk_blocks_seq(
	struct sea_dp_context_s const *this)
{
	uint64_t max_bulk_p = MIN3(
		this->rr.alen,
		this->rr.blen,
		this->rr.plim);
	return(max_bulk_p / BLK);
}

/**
 * @fn fill_mem_bounded
 * @brief fill <blk_cnt> contiguous blocks without seq bound tests, adding head and tail
 */
static _force_inline
struct sea_chain_status_s fill_mem_bounded(
	struct sea_dp_context_s *this,
	struct sea_joint_tail_s const *prev_tail,
	uint64_t blk_cnt)
{
	struct sea_fill_status_s stat = fill_bulk_predetd_blocks(
		this,
		fill_create_head(this, prev_tail),
		blk_cnt);
	return((struct sea_chain_status_s){
		.sec = _fill(fill_create_tail(this, prev_tail, stat.blk, stat.p)),
		.rem = (stat.stat == TERM) ? NULL : (void *)1
	});
}

/**
 * @fn fill_seq_bounded
 * @brief fill blocks with seq bound tests, adding head and tail
 */
static _force_inline
struct sea_chain_status_s fill_seq_bounded(
	struct sea_dp_context_s *this,
	struct sea_joint_tail_s const *prev_tail)
{
	int64_t psum = 0;
	struct sea_fill_status_s stat = {
		.blk = fill_create_head(this, prev_tail)
	};

	/* calculate block size */
	uint64_t seq_bulk_blocks = calc_max_bulk_blocks_seq(this);
	while(seq_bulk_blocks > MIN_BULK_BLOCKS) {
		/* bulk fill without ij-bound test */
		psum += (stat = fill_bulk_predetd_blocks(this, stat.blk, seq_bulk_blocks)).p;
		if(stat.stat == TERM) {
			goto _fill_seq_bounded_finish;	/* skip cap */
		}
		seq_bulk_blocks = calc_max_bulk_blocks_seq(this);
	}

	/* bulk fill with ij-bound test */
	psum += (stat = fill_bulk_seq_bounded(this, stat.blk)).p;

	if(stat.stat == TERM) {
		goto _fill_seq_bounded_finish;	/* skip cap */
	}

	/* cap fill (without p-bound test) */
	psum += (stat = fill_cap_seq_bounded(this, stat.blk)).p;

_fill_seq_bounded_finish:;
	return((struct sea_chain_status_s){
		.sec = _fill(fill_create_tail(this, prev_tail, stat.blk, psum)),
		.rem = (stat.stat == TERM) ? NULL : (void *)1
	});
}

/**
 * @fn sea_dp_fill
 * @brief fill dp matrix inside section pairs
 */
struct sea_chain_status_s sea_dp_fill(
	struct sea_dp_context_s *this,
	struct sea_fill_s const *prev_tail,
	struct sea_section_s const *curr,
	struct sea_section_s const *next,
	int64_t plim)
{
	/* init section and restore sequence reader buffer */
	struct sea_chain_status_s s = fill_init_seq(this, _tail(prev_tail), curr, next, plim);
	if(s.sec->psum < 0) {
		return(s);
	}

	/* calculate block sizes */
	uint64_t mem_bulk_blocks = calc_max_bulk_blocks_mem(this);
	uint64_t seq_bulk_blocks = calc_max_bulk_blocks_seq(this);

	/* extra large bulk fill (with stack allocation) */
	while(_unlikely(mem_bulk_blocks < seq_bulk_blocks)) {
		if(mem_bulk_blocks > MIN_BULK_BLOCKS) {
			if((s = fill_mem_bounded(this, _tail(s.sec), mem_bulk_blocks)).rem == NULL) {
				return(s);
			}

			/* mark tail as internal section */
			_tail(s.sec)->internal = 1;

			/* fill-in area has changed */
			seq_bulk_blocks = calc_max_bulk_blocks_seq(this);
		}

		/* malloc the next stack and set pointers */
		sea_dp_add_stack(this);

		/* stack size has changed */
		mem_bulk_blocks = calc_max_bulk_blocks_mem(this);
	}

	debug("v(%p), psum(%lld), p(%d)",
		_tail(s.sec)->v,
		_tail(s.sec)->psum,
		_tail(s.sec)->p);

	/* bulk fill with seq bound check */
	return(fill_seq_bounded(this, _tail(s.sec)));
}

/**
 * @fn sea_dp_merge
 */
struct sea_chain_status_s sea_dp_merge(
	struct sea_dp_context_s *this,
	struct sea_fill_s const *tail_list,
	uint64_t tail_list_len)
{
	return((struct sea_chain_status_s){ 0, 0 });
}

/**
 * @struct trace_coord_s
 */
struct trace_coord_s {
	int32_t p;
	int32_t q;
};

/**
 * @struct trace_block_s
 */
struct trace_block_s {
	struct sea_block_s *blk;
	int32_t len;
	int32_t p;
};

/**
 * @fn trace_detect_max_block
 */
static _force_inline
struct trace_block_s trace_detect_max_block(
	struct sea_dp_context_s *this,
	struct sea_joint_tail_s const *tail)
{
	struct sea_block_s *blk = _last_block(tail);

	v32i16_t md = _load_v32i16(tail->v);
	debug("p(%d)", tail->p);

	/* load mask */
	uint32_t mask_max = _tail(tail)->mask_max.all;
	debug("mask_max(%x)", mask_max);

	/* load the last max vector */
	vec_t max = _load(&blk->sd.max);
	int64_t offset = blk->offset;
	_print_v32i16(_add_v32i16(_add_v32i16(md, _cvt_v32i8_v32i16(max)), _set_v32i16(offset)));

	/* iterate towards the head */
	for(int32_t b = (tail->p - 1) / BLK; b > 0; b--, blk--) {
		debug("b(%d), blk(%p)", b, blk);

		/* load the previous max vector and offset */
		vec_t prev_max = _load(&(blk - 1)->sd.max);
		int64_t prev_offset = (blk - 1)->offset;
		_print_v32i16(_add_v32i16(_add_v32i16(md, _cvt_v32i8_v32i16(prev_max)), _set_v32i16(prev_offset)));

		/* adjust offset */
		prev_max = _add(prev_max, _set(prev_offset - offset));
		_print(max);
		_print(prev_max);

		/* take mask */
		uint32_t mask_update = mask_max & ~((union sea_mask_u){
			.mask = _mask(_eq(prev_max, max))
		}).all;
		debug("mask_update(%x)", mask_update);
		if(mask_update != 0) {
			/* max update occured at current block */
			debug("blk(%p), len(%d), p(%d)", blk, MIN2(tail->p - b * BLK, BLK), b * BLK);
			_print_v32i16(_add_v32i16(_add_v32i16(md, _cvt_v32i8_v32i16(prev_max)), _set_v32i16(offset)));
			return((struct trace_block_s){
				.blk = blk,
				.len = MIN2(tail->p - b * BLK, BLK),
				.p = b * BLK
			});
		}

		/* windback a block */
		max = prev_max;
		offset = prev_offset;
	}
	return((struct trace_block_s){
		.blk = blk,
		.len = 0,
		.p = 0,
	});
}

/**
 * @fn trace_detect_max_pos
 */
static _force_inline
struct trace_coord_s trace_detect_max_pos(
	struct sea_dp_context_s *this,
	struct sea_joint_tail_s const *tail,
	struct trace_block_s b)
{
	/* load mask */
	uint32_t mask_max = _tail(tail)->mask_max.all;
	debug("mask_max(%x)", mask_max);

	/* recalculate block */
	union sea_mask_u mask_max_arr[BLK];
	union sea_mask_u *mask_max_ptr = mask_max_arr;
	{
		fill_restore_fetch(this, b.blk);

		#define _fill_block_leaf(_mask_ptr) { \
			_dir_fetch(dir); \
			if(_dir_is_right(dir)) { \
				_fill_right(); \
			} else { \
				_fill_down(); \
			} \
			(_mask_ptr)++->mask = _mask(_eq(max, delta)); \
			debug("mask(%x)", ((union sea_mask_u){ .mask = _mask(_eq(max, delta)) }).all); \
		}

		_fill_load_contexts(b.blk);
		(void)offset;			/* to avoid warning */
		for(int64_t a = 0; a < b.len; a++) {
			_fill_block_leaf(mask_max_ptr);
		}
	}

	/* determine max pos */
	for(int64_t a = b.len-1; a >= 0; a--) {
		uint32_t mask_update = mask_max_arr[a].all & mask_max;
		if(mask_update != 0) {
			debug("p(%lld), q(%d)", b.p + a, tzcnt(mask_update) - BW/2);
			return((struct trace_coord_s){
				.p = b.p + a,
				.q = tzcnt(mask_update) - BW/2
			});
		}
	}

	/* not found in the block */
	v32i16_t md = _load_v32i16(tail->v);
	v32i16_t max = _cvt_v32i8_v32i16(_load(&(b.blk - 1)->sd.max));
	md = _add_v32i16(md, max);
	uint32_t mask = ((union sea_mask_u){
		.mask = _mask_v32i16(_eq_v32i16(md, _set_v32i16(_hmax_v32i16(md))))
	}).all;
	debug("p(%d), q(%d)", b.p - 1, tzcnt(mask & mask_max) - BW/2);
	return((struct trace_coord_s){
		.p = b.p - 1,
		.q = tzcnt(mask & mask_max) - BW / 2
	});
}

/**
 * @fn sea_dp_build_leaf
 */
struct sea_trace_s *sea_dp_build_leaf(
	struct sea_dp_context_s *this,
	struct sea_fill_s const *tail)
{
	/* create joint_head */
	struct sea_joint_head_s *head = sea_dp_smalloc(
		this,
		sizeof(struct sea_joint_head_s));

	/* search max block */
	struct trace_block_s b = trace_detect_max_block(this, _tail(tail));

	/* determine pos in the block */
	struct trace_coord_s c = trace_detect_max_pos(this, _tail(tail), b);

	/* store coordinates to joint_head */
	head->p = c.p - _tail(tail)->p;
	head->q = c.q;
	head->head = NULL;
	head->cigar = wr_init(this, _tail(tail)->psum);
	head->tail = _tail(tail);

	return((struct sea_trace_s *)head);
}

/**
 * @fn sea_dp_trace
 */
struct sea_trace_s *sea_dp_trace(
	struct sea_dp_context_s *this,
	struct sea_trace_s *prev_head,
	struct sea_clip_params_s const *clip)
{
	/* state transition arrays */
	uint64_t const diag_flag_array = 0x00010001<<3;
	uint64_t const q_diff_array = 0x0fff5000;
	uint64_t const q_term_array = 0x00c00000;

	struct sea_joint_head_s *head = _head(prev_head);
	struct sea_joint_tail_s const *tail = head->tail;

	do {
		/* calc index */
		int64_t p = tail->p + head->p;
		int64_t q = head->q + BW/2;		/* offsetted by BW/2 */
		int64_t a = p % BLK;			/* index in the block */

		/* load direction array */
		struct sea_block_s *blk = _last_block(tail) - ((tail->p - 1) / BLK) + (p / BLK);
		union sea_dir_u dir = _dir_load(blk, a);

		/* init diagonal flag */
		uint32_t diag_flag = 0;

		/* traceback loop */
		while(p >= 0) {
			/* load mask */
			uint64_t mask = blk->mask[a].all;

			/* 3-bit trace direction pointer flag in trace_flag[3..1] */
			uint32_t trace_flag = (0x01 & (mask>>q)) | (0x02 & (mask>>(q + BW - 1)));
			trace_flag = (trace_flag + trace_flag) | diag_flag;

			/* update alignment writer */
			// wr_push(this->w, this->ww, trace_flag);

			/* state transition */
			trace_flag |= _dir_is_down(dir)<<4;
			diag_flag = 0x08 & (diag_flag_array>>trace_flag);
			p--;
			q += (int32_t)(q_diff_array<<trace_flag)>>30;

			debug("trans_idx(%x), dir_flag(%x), diag_flag(%x), p(%lld), q(%lld), dq(%d)",
				trace_flag, _dir_is_down(dir)<<4, diag_flag, p, q, (int32_t)(q_diff_array<<trace_flag)>>30);

			_dir_windback(dir);
			if(a-- == 0) {
				/* load the previous block */
				blk--;
				a = BLK - 1;
				dir = _dir_load(blk, a);
				debug("p(%lld), arr(%x)", p, dir.dynamic.array);
			}
		}

		/* adjust coordinates */
		p -= diag_flag>>3;
		q += (int32_t)(q_term_array<<(diag_flag | (_dir_is_down(dir)<<4)))>>30;

		debug("p(%lld), q(%lld)", p, q);

		/* update joint_head */
		head = _phantom_head(blk);
		head->p = p;
		head->q = q - BW/2;
		tail = head->tail;
	} while(tail->internal == 1);
	return(_trace(head));
}

/**
 * @fn extract_max
 * @brief extract max value from 8-bit 16-cell vector
 */
static _force_inline
int8_t extract_max(int8_t const vector[][4])
{
	int8_t *v = (int8_t *)vector;
	int8_t max = -128;
	for(int i = 0; i < 16; i++) {
		max = (v[i] > max) ? v[i] : max;
	}
	return(max);
}

/**
 * @fn sea_init_restore_default_params
 */
static _force_inline
void sea_init_restore_default_params(
	struct sea_params_s *params)
{
	#define restore(_name, _default) { \
		params->_name = ((uint64_t)(params->_name) == 0) ? (_default) : (params->_name); \
	}
	// restore(band_width, 		SEA_WIDE);
	// restore(band_type, 			SEA_DYNAMIC);
	restore(seq_a_format, 		SEA_ASCII);
	restore(seq_a_direction, 	SEA_FW_ONLY);
	restore(seq_b_format, 		SEA_ASCII);
	restore(seq_b_direction, 	SEA_FW_ONLY);
	restore(aln_format, 		SEA_ASCII);
	restore(head_margin, 		0);
	restore(tail_margin, 		0);
	restore(xdrop, 				100);
	restore(score_matrix, 		SEA_SCORE_SIMPLE(1, 1, 1, 1));
	return;
}

/**
 * @fn sea_init_create_score_vector
 */
static _force_inline
struct sea_score_vec_s sea_init_create_score_vector(
	struct sea_score_s const *score_matrix)
{
	int8_t *v = (int8_t *)score_matrix->score_sub;
	int8_t geh = -score_matrix->score_ge_a;
	int8_t gev = -score_matrix->score_ge_b;
	int8_t gih = -score_matrix->score_gi_a;
	int8_t giv = -score_matrix->score_gi_b;
	struct sea_score_vec_s sc;
	for(int i = 0; i < 16; i++) {
		sc.sb[i] = v[i] - (geh + gih + gev + giv);
		sc.adjh[i] = -gih;
		sc.adjv[i] = -giv;
		sc.ofsh[i] = -(geh + gih);
		sc.ofsv[i] = gev + giv;
	}
	return(sc);
}

/**
 * @fn sea_init_create_dir_dynamic
 */
static _force_inline
union sea_dir_u sea_init_create_dir_dynamic(
	struct sea_score_s const *score_matrix)
{
	return((union sea_dir_u) {
		.dynamic = {
			.acc = 0,			/* zero independent of scoreing schemes */
			.array = 0x01		/* (0, 0) -> (0, 1) */
		}
	});
}

/**
 * @fn sea_init_create_small_delta
 */
static _force_inline
struct sea_small_delta_s sea_init_create_small_delta(
	struct sea_score_s const *score_matrix)
{
	int8_t max = extract_max(score_matrix->score_sub);
	int8_t geh = -score_matrix->score_ge_a;
	int8_t gev = -score_matrix->score_ge_b;
	// int8_t gih = -score_matrix->score_gi_a;
	int8_t giv = -score_matrix->score_gi_b;

	int8_t diff_a = max - geh;
	int8_t diff_b = gev;

	struct sea_small_delta_s sd;
	for(int i = 0; i < BW/2; i++) {
		sd.delta[i] = diff_a;
		sd.delta[BW/2 + i] = diff_b;
		sd.max[i] = diff_a;
		sd.max[BW/2 + i] = 0;
	}
	sd.delta[BW/2] += giv;
	return(sd);
}

/**
 * @fn sea_init_create_middle_delta
 */
static _force_inline
struct sea_middle_delta_s sea_init_create_middle_delta(
	struct sea_score_s const *score_matrix)
{
	int8_t max = extract_max(score_matrix->score_sub);
	int8_t geh = -score_matrix->score_ge_a;
	int8_t gev = -score_matrix->score_ge_b;
	int8_t gih = -score_matrix->score_gi_a;
	int8_t giv = -score_matrix->score_gi_b;

	int16_t coef_a = -max + 2*geh;
	int16_t coef_b = -max + 2*gev;
	int16_t ofs_a = gih;
	int16_t ofs_b = giv;

	struct sea_middle_delta_s md;
	for(int i = 0; i < BW/2; i++) {
		md.delta[i] = ofs_a + coef_a * (BW/2 - i);
		md.delta[BW/2 + i] = ofs_b + coef_b * i;
	}
	md.delta[BW/2] = 0;
	return(md);
}

/**
 * @fn sea_init_create_diff_vectors
 *
 * @detail
 * dH[i, j] = S[i, j] - S[i - 1, j]
 * dV[i, j] = S[i, j] - S[i, j - 1]
 * dE[i, j] = E[i, j] - S[i, j]
 * dF[i, j] = F[i, j] - S[i, j]
 */
static _force_inline
struct sea_diff_vec_s sea_init_create_diff_vectors(
	struct sea_score_s const *score_matrix)
{
	int8_t max = extract_max(score_matrix->score_sub);
	int8_t geh = -score_matrix->score_ge_a;
	int8_t gev = -score_matrix->score_ge_b;
	int8_t gih = -score_matrix->score_gi_a;
	int8_t giv = -score_matrix->score_gi_b;

	int8_t ofs_a = -(geh + gih);
	int8_t ofs_b = -(gev + giv);
	
	int8_t drop_dh = geh + ofs_a;
	int8_t raise_dh = max - gev + ofs_a;
	int8_t drop_dv = gev + ofs_b;
	int8_t raise_dv = max - geh + ofs_b;
	int8_t drop_de = 0 - giv;
	int8_t drop_df = 0 - gih;

	int8_t dh[BW] __attribute__(( aligned(16) ));
	int8_t dv[BW] __attribute__(( aligned(16) ));
	int8_t de[BW] __attribute__(( aligned(16) ));
	int8_t df[BW] __attribute__(( aligned(16) ));

	struct sea_diff_vec_s diff __attribute__(( aligned(16) ));
	/**
	 * dh: dH[i, j] - geh
	 * dv: dV[i, j] - gev
	 * de: dE[i, j] + gih + dV[i, j] - gev
	 * df: dF[i, j] + giv + dH[i, j] - geh
	 */
	/* calc dh and dv */
	for(int i = 0; i < BW/2; i++) {
		dh[i] = drop_dh;
		dh[BW/2 + i] = raise_dh;
		dv[i] = raise_dv;
		dv[BW/2 + i] = drop_dv;
	}
 	dh[BW/2-1] += gih;
 	dv[BW/2] += giv;

	/* calc de and df */
	for(int i = 0; i < BW; i++) {
		de[i] = drop_de;
		df[i] = drop_df;
 	}

 	/* negate dh */
 	for(int i = 0; i < BW; i++) {
 		dh[i] = -dh[i];
 	}

	vec_t const mask = _set(0x07);
 	vec_t _dh = _load(dh);
 	vec_t _dv = _load(dv);
 	vec_t _de = _load(de);
 	vec_t _df = _load(df);
	_dh = _shl(_dh, 3);
	_dv = _shl(_dv, 3);
	_store(&diff.dh, _or(_andn(mask, _dh), _de));
	_store(&diff.dv, _or(_dv, _df));

	return(diff);
}

/**
 * @fn sea_init_create_char_vector
 */
static _force_inline
struct sea_char_vec_s sea_init_create_char_vector(
	void)
{
	struct sea_char_vec_s ch;

	for(int i = 0; i < BW; i++) {
		ch.w[i] = 0x80;
	}
	return(ch);
}

/**
 * @fn sea_init
 */
sea_t *sea_init(
	struct sea_params_s const *params)
{
	/* sequence reader table */
	void (*const rd_seq_a_table[3][7])(
		uint8_t *dst,
		uint8_t const *src,
		uint64_t idx,
		uint64_t src_len,
		uint64_t copy_len) = {
		[SEA_FW_ONLY] = {
			[SEA_ASCII] = _loada_ascii_2bit_fw
		},
		[SEA_FW_RV] = {
			[SEA_ASCII] = _loada_ascii_2bit_fr
		}
	};
	void (*const rd_seq_b_table[3][7])(
		uint8_t *dst,
		uint8_t const *src,
		uint64_t idx,
		uint64_t src_len,
		uint64_t copy_len) = {
		[SEA_FW_ONLY] = {
			[SEA_ASCII] = _loadb_ascii_2bit_fw
		},
		[SEA_FW_RV] = {
			[SEA_ASCII] = _loadb_ascii_2bit_fr
		}
	};
	/* alignment writer table */
	struct sea_writer_s wr_fw_table[4] = {
		[SEA_STR] = {
			.push = _push_ascii_f,
			.type = WR_ASCII,
			.fr = WR_FW
		},
		[SEA_CIGAR] = {
			.push = _push_cigar_f,
			.type = WR_CIGAR,
			.fr = WR_FW
		},
		[SEA_DIR] = {
			.push = _push_dir_f,
			.type = WR_DIR,
			.fr = WR_FW
		}
	};
	struct sea_writer_s wr_rv_table[4] = {
		[SEA_STR] = {
			.push = _push_ascii_r,
			.type = WR_ASCII,
			.fr = WR_RV
		},
		[SEA_CIGAR] = {
			.push = _push_cigar_r,
			.type = WR_CIGAR,
			.fr = WR_RV
		},
		[SEA_DIR] = {
			.push = _push_dir_r,
			.type = WR_DIR,
			.fr = WR_RV
		}
	};

	if(params == NULL) {
		debug("params must not be NULL");
		return(NULL);
	}

	/* copy params to local stack */
	struct sea_params_s params_intl = *params;

	/* restore defaults */
	sea_init_restore_default_params(&params_intl);

	/* malloc sea_context */
	struct sea_context_s *ctx = (struct sea_context_s *)sea_aligned_malloc(
		sizeof(struct sea_context_s),
		SEA_MEM_ALIGN_SIZE);
	if(ctx == NULL) {
		return(NULL);
	}

	/* fill context */
	*ctx = (struct sea_context_s) {
		/* template */
		.k = (struct sea_dp_context_s) {
			.stack_top = NULL,						/* filled on dp init */
			.stack_end = NULL,						/* filled on dp init */
			.pdr = NULL,							/* filled on dp init */
			.tdr = NULL,							/* filled on dp init */
			.tail = &ctx->tail,

			.ll = (struct sea_writer_work_s) { 0 },	/* work: no need to init */
			.rr = (struct sea_reader_work_s) { {0} },	/* work: no need to init */
			.l = (struct sea_writer_s)(ctx->rv),	/* reverse writer (default on init) */
			.r = (struct sea_reader_s) {
				.loada = rd_seq_a_table
					[params_intl.seq_a_direction]
					[params_intl.seq_a_format],		/* seq a reader */
				.loadb = rd_seq_b_table
					[params_intl.seq_b_direction]
					[params_intl.seq_b_format]		/* seq b reader */
			},

			.scv = sea_init_create_score_vector(params_intl.score_matrix),
			.tx = params_intl.xdrop,

			.mem_cnt = 0,
			.mem_size = SEA_MEM_INIT_SIZE,
			.mem_array = { 0 },						/* NULL */
		},
		.md = sea_init_create_middle_delta(params_intl.score_matrix),
		.blk = (struct sea_phantom_block_s) {
			.mask = {
				[0].pair = {
					.h.all = 0x00000000,
					.v.all = 0x00000000
				},
				[1].pair = {
					.h.all = 0x0000ffff,
					.v.all = 0xffff0000
				}
			},
			.dir = sea_init_create_dir_dynamic(params_intl.score_matrix),
			.offset = 0,
			.diff = sea_init_create_diff_vectors(params_intl.score_matrix),
			.sd = sea_init_create_small_delta(params_intl.score_matrix),
			.alen = 0,
			.blen = 0,
			.ch = sea_init_create_char_vector()
		},
		.tail = (struct sea_joint_tail_s) {
			.v = &ctx->md,
			.max = 0,
			.psum = 2 - BW,
			.p = 0,
			.curr = { 0 },
			.rem = { 0 },
			.init_alen = 0,
			.init_blen = 0,
			.internal = 0
		},
		.rv = wr_rv_table[params_intl.aln_format],
		.fw = wr_fw_table[params_intl.aln_format],
		.params = params_intl
	};

	return((sea_t *)ctx);
}

/**
 * @fn sea_clean
 */
void sea_clean(
	struct sea_context_s *ctx)
{
	if(ctx != NULL) { sea_aligned_free(ctx); }
	return;
}

/**
 * @fn sea_dp_init
 */
struct sea_dp_context_s *sea_dp_init(
	struct sea_context_s const *ctx,
	struct sea_seq_pair_s const *p,
	uint8_t const *guide,
	uint64_t glen)
{
	/* malloc stack memory */
	struct sea_dp_context_s *this = (struct sea_dp_context_s *)sea_aligned_malloc(
		ctx->k.mem_size,
		SEA_MEM_ALIGN_SIZE);
	if(this == NULL) {
		debug("failed to malloc memory");
		return(NULL);
	}

	/* init seq pointers */
	_memcpy_blk_au(
		&this->rr.p,
		p,
		sizeof(struct sea_seq_pair_s));

	/* load template */
	uint64_t const dp_sz = sizeof(struct sea_dp_context_s);
	uint64_t const ph_sz = sizeof(struct sea_phantom_block_s);
	uint64_t const tl_sz = sizeof(struct sea_joint_tail_s);

	_memcpy_blk_aa(
		(uint8_t *)this + SEA_DP_CONTEXT_LOAD_OFFSET,
		(uint8_t *)&ctx->k + SEA_DP_CONTEXT_LOAD_OFFSET,
		SEA_DP_CONTEXT_LOAD_SIZE);

	_memcpy_blk_aa(
		(uint8_t *)this + dp_sz,
		(uint8_t *)&ctx->blk,
		SEA_DP_ROOT_LOAD_SIZE);

	/* init stack pointers */
	this->stack_top = (uint8_t *)this + (dp_sz + ph_sz + tl_sz);
	this->stack_end = (uint8_t *)this + this->mem_size - SEA_MEM_MARGIN_SIZE;
	this->pdr = guide;
	this->tdr = guide + glen;
	this->tail = (struct sea_joint_tail_s *)((uint8_t *)this + (dp_sz + ph_sz));

	return(this);
}

/**
 * @fn sea_dp_build_root
 */
struct sea_chain_status_s sea_dp_build_root(
	struct sea_dp_context_s *this,
	struct sea_section_s const *curr)
{
	return((struct sea_chain_status_s){ _fill(this->tail), curr });
}

/**
 * @fn sea_dp_add_stack
 */
static _force_inline
int32_t sea_dp_add_stack(
	struct sea_dp_context_s *this)
{
	uint8_t *ptr = this->mem_array[this->mem_cnt];
	uint64_t size = SEA_MEM_INIT_SIZE<<(this->mem_cnt + 1);
	if(ptr == NULL) {
		ptr = (uint8_t *)sea_aligned_malloc(size, SEA_MEM_ALIGN_SIZE);
	}
	if(ptr == NULL) {
		return(SEA_ERROR_OUT_OF_MEM);
	}
	this->mem_array[this->mem_cnt++] = this->stack_top = ptr;
	this->stack_end = this->stack_top + size - SEA_MEM_MARGIN_SIZE;
	return(SEA_SUCCESS);
}

/**
 * @fn sea_dp_flush
 */
void sea_dp_flush(
	struct sea_dp_context_s *this,
	struct sea_seq_pair_s const *p,
	uint8_t const *guide,
	uint64_t glen)
{
	uint64_t const dp_sz = sizeof(struct sea_dp_context_s);
	uint64_t const ph_sz = sizeof(struct sea_phantom_block_s);
	uint64_t const tl_sz = sizeof(struct sea_joint_tail_s);

	_memcpy_blk_au(
		&this->rr.p,
		p,
		sizeof(struct sea_seq_pair_s));

	this->stack_top = (uint8_t *)this + (dp_sz + ph_sz + tl_sz);
	this->stack_end = (uint8_t *)this + this->mem_size - SEA_MEM_MARGIN_SIZE;
	this->pdr = guide;
	this->tdr = guide + glen;

	/* clear memory */
	this->mem_cnt = 0;
	return;
}

/**
 * @fn sea_dp_malloc
 */
static _force_inline
void *sea_dp_malloc(
	struct sea_dp_context_s *this,
	uint64_t size)
{
	/* roundup */
	uint64_t const align_size = 16;
	size += (align_size - 1);
	size &= ~(align_size - 1);

	/* malloc */
	if((this->stack_end - this->stack_top) < size) {
		if(this->mem_size < size) { this->mem_size = size; }
		if(sea_dp_add_stack(this) != SEA_SUCCESS) {
			return(NULL);
		}
	}
	this->stack_top += size;
	return((void *)(this->stack_top - size));
}

/**
 * @fn sea_dp_smalloc
 * @brief small malloc without boundary check, for joint_head, joint_tail, merge_tail, and delta vectors.
 */
static _force_inline
void *sea_dp_smalloc(
	struct sea_dp_context_s *this,
	uint64_t size)
{
	// assert(size < 256);

	this->stack_top += size;
	return((void *)(this->stack_top - size));
}

/**
 * @fn sea_dp_free
 */
static _force_inline
void sea_dp_free(
	struct sea_dp_context_s *this,
	void *ptr)
{
	/* nothing to do */
	return;
}

/**
 * @fn sea_dp_clean
 */
void sea_dp_clean(
	struct sea_dp_context_s *this)
{
	if(this == NULL) {
		return;
	}

	for(uint64_t i = 0; i < SEA_MEM_ARRAY_SIZE; i++) {
		sea_aligned_free(this->mem_array[i]);
	}
	sea_aligned_free(this);
	return;
}

#if 0
/**
 * @fn sea_align_dynamic
 */
struct sea_result *sea_align_dynamic(
	struct sea_context_s const *ctx,
	struct sea_seq_pair_s const *seq,
	struct sea_checkpoint_s const *cp,
	uint64_t cplen)
{
	return(NULL);
}

/**
 * @fn sea_align_guided
 */
struct sea_result *sea_align_guided(
	struct sea_context_s const *ctx,
	struct sea_seq_pair_s const *seq,
	struct sea_checkpoint_s const *cp,
	uint64_t cplen,
	uint8_t const *guide,
	uint64_t glen)
{
	return(NULL);
}
#endif

/* unittests */
#ifdef TEST
#include <assert.h>
void unittest(void)
{
	char const *a = "ACGTACGTACGTACGTCCCCGGGCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC";
	char const *b = "ACGTACGTACGTACGTGCCCGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG";
	// char const *b = "GGGTTAGGGTTAGGGTTAGCGCGCATATATATAAAATTTT";

	/* sea_init */
	sea_t *ctx = sea_init(SEA_PARAMS(
		.seq_a_format = SEA_ASCII,
		.seq_a_direction = SEA_FW_ONLY,
		.seq_b_format = SEA_ASCII,
		.seq_b_direction = SEA_FW_ONLY,
		.aln_format = SEA_ASCII,
		.xdrop = 10,
		.score_matrix = SEA_SCORE_SIMPLE(2, 3, 5, 1)));
	assert(ctx != NULL);

	/* build dp context */
	sea_seq_pair_t seq = sea_build_seq_pair(a, strlen(a), b, strlen(b));

	sea_dp_t *dp = sea_dp_init(ctx, &seq, NULL, 0);
	assert(dp != NULL);

	// dump(dp, 1024);
	// int phantom_size = sizeof(struct sea_phantom_block_s) + sizeof(struct sea_joint_head_s);
	// dump(stat.ptr - phantom_size, sizeof(struct sea_joint_tail_s) + phantom_size);

	struct sea_section_s curr = sea_build_section(0, strlen(a), 0, strlen(b));
	struct sea_section_s next = sea_build_section(0, strlen(a), 0, strlen(b));

	struct sea_chain_status_s stat = sea_dp_build_root(dp, &curr);
	stat = sea_dp_fill(dp, stat.sec, &curr, &next, 32);
	debug("stat.tail(%p), stat.rem(%p)", stat.sec, stat.rem);
	log("max(%lld), psum(%lld), p(%d)", stat.sec->max, stat.sec->psum, stat.sec->p);
	// dump(dp, (void *)stat.sec - (void *)dp + sizeof(struct sea_joint_tail_s));

	struct sea_trace_s *trace = sea_dp_build_leaf(dp, stat.sec);
	debug("p(%d), q(%d)", _head(trace)->p, _head(trace)->q);

	trace = sea_dp_trace(dp, trace, NULL);

	sea_dp_clean(dp);

	/* sea_clean */
	sea_clean(ctx);

	return;
}

#endif

#ifdef MAIN
#include <stdio.h>
int main(void)
{
	unittest();
	return 0;
}
#endif

/**
 * end of branch2.c
 */
