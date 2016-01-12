
/**
 * @file twig_types.h
 *
 * @brief type definitions for the twig implementation
 */
#ifndef _TWIG_TYPES_H_INCLUDED
#define _TWIG_TYPES_H_INCLUDED

/** twig */
#define twig_dpchar_t			uint8_t
#define twig_dpcell_t			int8_t
#define TWIG_DPCELL_MIN			( INT8_MIN )
#define TWIG_DPCELL_MAX			( INT8_MAX )
#define TWIG_BW 				( 16 )
#define twig_linear_bpl()		( sizeof(twig_dpcell_t) * TWIG_BW )
#define twig_linear_bpb()		( sizeof(struct twig_linear_block) )

#include "../arch/b8c16.h"

struct twig_linear_block_dp {
	twig_dpchar_t dp[BLK][TWIG_BW];
};

struct twig_linear_block_trailer {
	int64_t i, j;
	twig_dpcell_t maxv[TWIG_BW];
#if DP == DYNAMIC
	uint8_t dr[dir_size()];
#endif
};

struct twig_linear_joint_vec {
	twig_dpcell_t pv[TWIG_BW];
	twig_dpcell_t cv[TWIG_BW];
	twig_dpchar_t wa[TWIG_BW];
	twig_dpchar_t wb[TWIG_BW];
};

struct twig_linear_block {
	struct twig_linear_block_dp v;
	struct twig_linear_block_trailer t;
};

struct twig_linear_head {
	struct sea_joint_head head;
	twig_dpcell_t pv[TWIG_BW];
	twig_dpcell_t cv[TWIG_BW];
	struct twig_linear_block_trailer t;
};

struct twig_linear_tail {
	struct twig_linear_joint_vec v;
	struct sea_joint_tail tail;
};

#ifndef _TYPES_INCLUDED
#define _TYPES_INCLUDED

#define dpchar_t 				twig_dpchar_t
#define dpcell_t 				twig_dpcell_t
#define DPCELL_MIN 				TWIG_DPCELL_MIN
#define DPCELL_MAX 				TWIG_DPCELL_MAX
#define BW 						TWIG_BW

#endif /* #ifndef _TYPES_INCLUDED */

#endif /* #ifndef _TWIG_TYPES_H_INCLUDED */
/**
 * end of twig_types.h
 */
