
/**
 * @file branch2.c
 */


#define _is_down(x)		( _ex32(x, 0) > 0 )
#define _is_right(x)	( _ex32(x, 0) <= 0 )
#define _likely(x)		__builtin_expect((x), 1)
#define _unlikely(x)	__builtin_expect((x), 0)

struct sea_chain_status linear_fill(
	struct sea_dp_context *this,
	uint8_t *pdp,					/** tail of the previous section */
	struct sea_section_pair *sec)
{
	/* load context and joint_vec pointers */
	struct sea_dp_context register *k = this;
	struct sea_joint_vec register *s = _tail(pdp, v);

	/* load constant vectors */
	__m128i const sbv = _load(k->sc.score_sub);
	__m128i const gav = _set1(k->sc.score_gi_a);
	__m128i const gbv = _set1(k->sc.score_gi_b);
	__m128i const mask = _set(
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0xff);
	__m128i const shuf = _set(
		0x80, 0x80, 0x80, 0x80,
		0x80, 0x80, 0x80, 0x0f,
		0x80, 0x80, 0x80, 0x00,
		0x80, 0x80, 0x80, 0x0f);
	__m128i const inc = _set32(1, k->sc.score_gi_a, k->sc.score_gi_b, 0);

	/* init working stack */
	_head(k->stack_top, p_tail) = pdp;
	uint8_t *cdp = k->stack_top + sizeof(struct sea_joint_head);

	/* init sequence reader */
	rd_set_section(k->r, k->rr, sec);
	rd_load(k->r, k->rr, s->wa, s->wb);

	/* init vector registers */
	struct sea_joint_vec *v = (struct sea_joint_vec *)k->v;
	__m128i da = _load(_pv(v.da));
	__m128i dv1 = _load(_pv(&v.dv[0]));
	__m128i dv2 = _load(_pv(&v.dv[_vlen]));
	__m128i dh1 = _load(_pv(&v.dh[0]));
	__m128i dh2 = _load(_pv(&v.dh[_vlen]));

	/* store phantom vector */
	_store(_pv(cdp), _pack(dv1, dh1)); cdp += _vlen;
	_store(_pv(cdp), _pack(dv2, dh2)); cdp += _vlen;

	/* store first block tail */
	_store(_pv(cdp), _zero()); cdp += _vlen;
	_store(_pv(cdp), _zero()); cdp += _vlen;
	dir_end_block(dr, cdp);

	/* init loop temporary vectors */
	int64_t p = 0;
	__m128i accv1, accv2;			/** 8bit accumulators */
	__m128i maxv1, maxv2;			/** max vectors */

	/**
	 * @macro _fill_body
	 * @brief diff linear
	 */
	#define _fill_body() { \
		__m128i t1 = _match( \
			_loadu(rd_ptra(k->rr, 0)), \
			_loadu(rd_ptrb(k->rr, 0))); \
		__m128i t2 = _match( \
			_loadu(rd_ptra(k->rr, 1)), \
			_loadu(rd_ptrb(k->rr, 1))); \
		t1 = _max(t1, dv1); \
		t1 = _max(t1, dh1); \
		t2 = _max(t2, dv2); \
		t2 = _max(t2, dh2); \
		__m128i _dv1 = _sub(t1, dh1); \
		__m128i _dv2 = _sub(t2, dh2); \
		dh1 = _sub(t1, dv1); \
		dh2 = _sub(t2, dv2); \
		dv1 = _dv1; dv2 = _dv2; \
		_store(cdp, _pack(dv1, dh1)); cdp += _vlen; \
		_store(cdp, _pack(dv2, dh2)); cdp += _vlen; \
	}

	/**
	 * @macro _fill_right
	 * @brief pointer increment + vector shift + _fill_body
	 */
	#define _fill_right() { \
		rd_go_right(); \
		_shl(dh, dh); \
		_fill_body(); \
	}

	/**
	 * @macro _fill_left
	 * @brief pointer increment + vector shift + _fill_body
	 */
	#define _fill_down() { \
		rd_go_down(); \
		_shr(dv, dv); \
		_fill_body(); \
	}

	/**
	 * @macro _fill_block
	 * @brief an element of unrolled fill-in loop
	 */
	#define _fill_block(direction, label, jump_to) { \
		if(_unlikely(!_is_##direction(da))) { \
			goto _linear_fill_##jump_to; \
		} \
		_linear_fill_##label: _fill_##direction(); \
		if(_unlikely(--i == 0)) { break; } \
	}

	while(1) {
		/* fetch sequence */

		/* fill in */
		int64_t i = BW;
		while(1) {
			_fill_block(down, d1, r1);
			_fill_block(right, r1, d2);
			_fill_block(down, d2, r2);
			_fill_block(right, r2, d1);
		}

		/* update offset */
	}
}