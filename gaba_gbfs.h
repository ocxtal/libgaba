
/**
 * @file gaba_gbfs.h
 *
 * @brief breadth-first search on sequence graphs, with X-drop pruning and matrix (band) merging
 *
 * @author Hajime Suzuki
 * @data 2017/11/30
 * @license Apache v2
 */

#ifndef _GABA_GBFS_H_INCLUDED
#define _GABA_GBFS_H_INCLUDED

/* combined gap penalty is not implemented yet */
#define DISABLE_COMBINED


#include <stdint.h>							/* uint32_t, uint64_t, ... */
#include "gaba.h"
#include "sassert.h"


/* external (callback) functions */
typedef link_set_t (*gaba_get_link_t)(void *opaque, uint32_t sid);
typedef gaba_section_t const *(*gaba_get_segment_t)(void *opaque, uint32_t sid);
typedef void *(*gaba_get_meta_t)(void *opaque, uint32_t sid);
typedef void (*gaba_set_meta_t)(void *opaque, uint32_t sid, void *meta);



struct sec_meta_s {
	struct path_s *path;
};

struct tree_path_s {
	struct gaba_fill_s const *fill[];


	/* a list of (plim, sec_meta_t) (is it heap-queue?) */
};

/**
 * @struct gaba_gbfs_link_s
 */
struct gaba_gbfs_link_s {
	uint32_t cnt;
	struct gaba_section_s const *sec;
};

/**
 * @struct gaba_gbfs_fill_s
 */
struct gaba_gbfs_fill_s {
	uint32_t ascnt, bscnt;		/** (8) aligned section counts */
	uint64_t apos, bpos;		/** (16) #fetched bases from the head (ppos = apos + bpos) */
	int64_t max;				/** (8) max score in the entire band */
	uint32_t stat;				/** (4) status (section update flags) */
	// int32_t ppos;				/** (8) #vectors from the head (FIXME: should be 64bit int) */
	uint32_t reserved[5];
};
_static_assert(sizeof(struct gaba_gbfs_fill_s) == sizeof(struct gaba_fill_s));
_static_assert(offsetof(struct gaba_gbfs_fill_s, ascnt) == offsetof(struct gaba_fill_s, ascnt));
_static_assert(offsetof(struct gaba_gbfs_fill_s, bscnt) == offsetof(struct gaba_fill_s, bscnt));
_static_assert(offsetof(struct gaba_gbfs_fill_s, apos) == offsetof(struct gaba_fill_s, apos));
_static_assert(offsetof(struct gaba_gbfs_fill_s, bpos) == offsetof(struct gaba_fill_s, bpos));
_static_assert(offsetof(struct gaba_gbfs_fill_s, max) == offsetof(struct gaba_fill_s, max));
_static_assert(offsetof(struct gaba_gbfs_fill_s, stat) == offsetof(struct gaba_fill_s, stat));
#define _gfill(x)				( (struct gaba_gbfs_fill_s *)(x) )

/**
 * @struct gaba_gbfs_qfill_s
 */
struct gaba_gbfs_qfill_s {
	int64_t ppos;
	struct gaba_gbfs_fill_s const *f;				/* user-defined metadata after this */
};


void hq_push(hq_t *hq, qfill_t e) { return; }
qfill_t hq_pop(hq_t *hq) { return; }


#define _is_merge(_f)			( 0 )
#define _is_p_break(_f)			( (_f)->f.stat == CONT )/* not (UPDATE_A or UPDATE_B) and CONT */
#define _is_a_break(_f)			( (_f)->f.stat & GABA_UPDATE_A )
#define _is_b_break(_f)			( (_f)->f.stat & GABA_UPDATE_B )

/**
 * @fn gaba_gbfs_set_break
 * @brief set p-break
 */
static inline
void gaba_gbfs_set_break(
	gdp_t *gdp,
	struct gaba_gbfs_fill_s *f,
	uint64_t i)
{
	#define _r(_x, _idx)		( (&(_x))[(_idx)] )

	/* mark breakpoint */
	_r(f->abrk, i) |= 0x01ULL<<(BW - 1);

	/*
	 * here the tail is aligned to the others at the same coordinate
	 * so the off-diagonal distance (qofs) is equals to the distance on the other side.
	 */
	int64_t q = (i == 0 ? -1 : 1) * _r(f->apos, 1 - i);

	struct gaba_gbfs_merge_s *mg = gdp->get_meta(f->s.aid);
	mg->ppos = MAX2(mg->ppos, f->apos + f->bpos);
	mg->farr[mg->fcnt++] = f;
	return((struct gaba_gbfs_qnode_s){
		.ppos = mg->ppos,
		.f = mg
	});

	#undef _r
}

/**
 * @fn gaba_gbfs_merge_tails
 */
static inline
struct gaba_fill_s *gaba_gbfs_merge_tails(
	gdp_t *gdp,
	struct gaba_gbfs_merge_s *mg)
{
	if(mg->fcnt == 1) { return(mg->farr[0]); }

	/* sort tails by ppos */
	radix_sort_128(mg->farr, mg->fcnt);

	/* cluster qpos */

	if(mcnt == mg->fcnt) {
		for(uint64_t i = 1; i < mg->fcnt; i++) {
			;
		}
		return(mg->farr[0]);
	}
	/* extend */

	/* create qofs array */

	return;
}

/**
 * @fn gaba_gbfs_extend
 * @brief traverse graph in a depth-first manner
 */
static
struct gaba_fill_s *gaba_gbfs_extend(
	gdp_t *gdp,
	struct gaba_section_s const *asec,
	uint32_t apos,
	struct gaba_section_s const *bsec,
	uint32_t bpos)
{
	/* fill root */ {
		struct gaba_gbfs_fill_s *f = _gfill(gaba_dp_fill_root(gdp->dp, asec, apos, bsec, bpos, 0));
		struct gaba_gbfs_qnode_s n = { .ppos = f->apos + f->bpos, .f = f };
		if(_is_a_break(f)) { n = gaba_gbfs_set_break(gdp, n.f, 0); }
		if(_is_b_break(f)) { n = gaba_gbfs_set_break(gdp, n.f, 1); }
		kv_hq_push(gdp->q, n);
	}

	while(!hq_is_empty(gdp->q)) {
		struct gaba_gbfs_qnode_s n = kv_hq_pop(gdp->q);
		if(_is_merge(n)) { n = gaba_gbfs_merge_tails(gdp, n.f, n.ppos); }

		struct gaba_gbfs_link_s alink = ((f->stat & UPDATE_A)
			? gdp->get_link(gdp->g, f->s.aid)
			: (struct gaba_gbfs_link_s){ .cnt = 1, .sec = &f->a }
		);
		struct gaba_gbfs_link_s blink = ((f->stat & UPDATE_B)
			? gdp->get_link(gdp->g, f->s.bid)
			: (struct gaba_gbfs_link_s){ .cnt = 1, .sec = &f->b }
		);
		for(uint64_t i = 0; i < alink.cnt; i++) {
			for(uint64_t j = 0; j < blink.cnt; j++) {
				gdp->q.push(gaba_dp_fill(gdp->dp, f, alink.sec[i], blink.sec[j], UINT32_MAX));
			}
		}
	}
	return(fill);
}


#endif _GABA_GBFS_H_INCLUDED	/* #ifdef */

/**
 * end of gaba_gbfs.h
 */

