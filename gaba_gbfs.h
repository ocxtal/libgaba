
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
typedef gaba_section_t const *(*gaba_get_segment_t)(uint32_t sid);
typedef void *(*gaba_get_meta_t)(uint32_t sid);
typedef void (*gaba_set_meta_t)(uint32_t sid, void *meta);



struct sec_meta_s {
	struct path_s *path;
};

struct tree_path_s {
	gaba_fill_t const *fill[];


	/* a list of (plim, sec_meta_t) (is it heap-queue?) */
};

struct link_set_s {
	uint32_t cnt;
	gaba_section_t const *next;
};


struct wfill_s {
	gaba_fill_t f;
	/* with plim */
};

struct qfill_s {
	int64_t ppos;
	wfill_t const *f;											/* user-defined metadata after this */
};


void hq_push(hq_t *hq, qfill_t e) { return; }
qfill_t hq_pop(hq_t *hq) { return; }


#define _is_p_break(_f)			( (_f)->f.stat == CONT )		/* not (UPDATE_A or UPDATE_B) and CONT */
#define _is_a_break(_f)			( (_f)->f.stat & UPDATE_A )
#define _is_b_break(_f)			( (_f)->f.stat & UPDATE_B )


void merge_tails(gdp_t *gdp, wfill_t *f)
{
	wfill_t **warr = get_meta(f->f);							/* what is the difference between coordinates? */
	return;
}

void set_p_break(gdp_t *gdp, wfill_t *f)
{
	return;
}

void set_p_break(gdp_t *gdp, wfill_t *f)
{
	return;
}


/* API */
gaba_fill_t *fill_graph(gdp_t *gdp, uint64_t aid_pos, uint64_t bid_pos)
{
	gaba_section_t const *asec = get_segment(aid_pos>>32);
	gaba_section_t const *bsec = get_segment(bid_pos>>32);

	/* fill root */ {
		gaba_fill_t *f = gaba_dp_fill_root(gdp->dp, asec, aid_pos, bsec, bid_pos);
		hq_push(gdp->q, (qfill_t){ .ppos = f->ppos, .f = (wfill_t *)f });
	}

	while(!hq_is_empty(gdp->q)) {
		wfill_t *f = hq_pop(gdp->q).f;
		if(_is_p_break(f)) { merge_tails(gdp, f); }
		if(_is_a_break(f)) { set_p_break(gdp, f); }
		if(_is_b_break(f)) { set_p_break(gdp, f); }
		asec = (f->stat & UPDATE_A) ? get_link(f->a) : f->a;
		bsec = (f->stat & UPDATE_B) ? get_link(f->b) : f->b;
		for(asec x bsec) { gdp->q.push(gaba_dp_fill(f, a, b)); }
	}


	return(fill);
}


#endif _GABA_GBFS_H_INCLUDED	/* #ifdef */

/**
 * end of gaba_gbfs.h
 */

