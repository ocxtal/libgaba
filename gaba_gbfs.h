
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
 * @struct gaba_gbfs_wfill_s
 */
struct gaba_gbfs_wfill_s {
	struct gaba_fill_s f;
	/* with plim */
};
#define _wfill(x)				( (struct gaba_gbfs_wfill_s *)(x) )

/**
 * @struct gaba_gbfs_qfill_s
 */
struct gaba_gbfs_qfill_s {
	int64_t ppos;
	struct gaba_joint_tail_s const *tail;				/* user-defined metadata after this */
};


void hq_push(hq_t *hq, qfill_t e) { return; }
qfill_t hq_pop(hq_t *hq) { return; }


#define _is_p_break(_f)			( (_f)->f.stat == CONT )/* not (UPDATE_A or UPDATE_B) and CONT */
#define _is_a_break(_f)			( (_f)->f.stat & GABA_UPDATE_A )
#define _is_b_break(_f)			( (_f)->f.stat & GABA_UPDATE_B )

/**
 * @fn gaba_gbfs_merge_tails
 */
void gaba_gbfs_merge_tails(
	gdp_t *gdp,
	struct gaba_joint_tail_s *tail)
{
	struct gaba_joint_tail_s **warr = get_meta(tail->f);	/* what is the difference between coordinates? */
	return;
}

/**
 * @fn gaba_gbfs_set_break_a, gaba_gbfs_set_break_b
 * @brief set p-break
 */
void gaba_gbfs_set_break_a(
	gdp_t *gdp,
	struct gaba_joint_tail_s *tail)
{
	tail->abrk |= 0x01ULL<<(BW - 1);

	/* */

	return;
}
void gaba_gbfs_set_break_b(
	gdp_t *gdp,
	struct gaba_joint_tail_s *tail)
{
	tail->bbrk |= 0x01ULL<<(BW - 1);
	return;
}


/**
 * @fn gaba_gbfs_extend
 * @brief traverse graph in a depth-first manner
 */
static
struct gaba_fill_s *gaba_gbfs_extend(
	gdp_t *gdp,
	gaba_section_t const *asec,
	uint32_t apos,
	gaba_section_t const *bsec,
	uint32_t bpos)
{
	/* fill root */ {
		struct gaba_joint_tail_s *tail = _tail(gaba_dp_fill_root(gdp->dp, asec, apos, bsec, bpos));
		kv_hq_push(gdp->q, (struct gaba_gbfs_qfill_s){
			.ppos = tail->ppos,							/* sorted by ppos */
			.tail = tail
		});
	}

	while(!hq_is_empty(gdp->q)) {
		struct gaba_joint_tail_s *tail = kv_hq_pop(gdp->q).tail;/* pop a tail with the shortest path */
		if(_is_p_break(tail)) { gaba_gbfs_merge_tails(gdp, tail); }
		if(_is_a_break(tail)) { gaba_gbfs_set_break_a(gdp, tail); }
		if(_is_b_break(tail)) { gaba_gbfs_set_break_b(gdp, tail); }

		struct gaba_gbfs_link_s alink = ((tail->f.stat & UPDATE_A)
			? gdp->get_link(gdp->g, tail->s.aid)
			: (struct gaba_gbfs_link_s){ .cnt = 1, .sec = &tail->a }
		);
		struct gaba_gbfs_link_s blink = ((tail->stat & UPDATE_B)
			? gdp->get_link(gdp->g, tail->s.bid)
			: (struct gaba_gbfs_link_s){ .cnt = 1, .sec = &tail->b }
		);
		for(uint64_t i = 0; i < alink.cnt; i++) {
			for(uint64_t j = 0; j < blink.cnt; j++) {
				gdp->q.push(gaba_dp_fill(gdp->dp, f, alink.sec[i], blink.sec[j]));
			}
		}
	}


	return(fill);
}


#endif _GABA_GBFS_H_INCLUDED	/* #ifdef */

/**
 * end of gaba_gbfs.h
 */

