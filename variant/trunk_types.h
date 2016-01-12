
/**
 * @file trunk_types.h
 *
 * @brief type definitions for the trunk implementation
 */
#ifndef _TRUNK_TYPES_H_INCLUDED
#define _TRUNK_TYPES_H_INCLUDED

#define trunk_dpchar_t			uint8_t
#define trunk_dpcell_t			uint8_t
#define TRUNK_DPCELL_MIN		( UINT8_MIN )
#define TRUNK_DPCELL_MAX		( UINT8_MAX )
#define TRUNK_BW 				( 32 )
#define trunk_linear_bpl()		( sizeof(trunk_dpcell_t) * TRUNK_BW )
#define trunk_linear_bpb()		( sizeof(struct trunk_linear_block) )

#include "../arch/b8c32.h"		/** 8bit 32cell */

/**
 * @struct trunk_linear_block_dp
 */
struct trunk_linear_block_dp {
	trunk_dpcell_t dp[BLK][TRUNK_BW];
};

/**
 * @struct trunk_linear_block_trailer
 */
struct trunk_linear_block_trailer {
	int64_t i, j;
	trunk_dpcell_t maxv[TRUNK_BW];
#if DP == DYNAMIC
	uint8_t dr[dir_size()];
#endif
};

/**
 * @struct trunk_linear_joint_vec
 */
struct trunk_linear_joint_vec {
	trunk_dpcell_t dv[TRUNK_BW];
	trunk_dpcell_t dh[TRUNK_BW];
	trunk_dpcell_t acc[TRUNK_BW];
	trunk_dpchar_t wa[TRUNK_BW];
	trunk_dpchar_t wb[TRUNK_BW];
};

/**
 * @struct trunk_linear_block
 */
struct trunk_linear_block {
	struct trunk_linear_block_dp v;
	struct trunk_linear_block_trailer t;
};

/**
 * @struct trunk_linear_head
 */
struct trunk_linear_head {
	struct sea_joint_head head;
	trunk_dpcell_t pv[TRUNK_BW];
	trunk_dpcell_t cv[TRUNK_BW];
	struct trunk_linear_block_trailer t;
};

/**
 * @struct trunk_linear_tail
 */
struct trunk_linear_tail {
	struct trunk_linear_joint_vec v;
	struct sea_joint_tail tail;
};

#ifndef _TYPES_INCLUDED
#define _TYPES_INCLUDED

#define dpchar_t 				trunk_dpchar_t
#define dpcell_t 				trunk_dpcell_t
#define DPCELL_MIN 				TRUNK_DPCELL_MIN
#define DPCELL_MAX 				TRUNK_DPCELL_MAX
#define BW 						TRUNK_BW

#endif /* #ifndef _TYPES_INCLUDED */

#endif /* #ifndef _TRUNK_TYPES_H_INCLUDED */
/**
 * end of trunk_types.h
 */
