
/**
 * @file branch_types.h
 *
 * @brief type definitoins for branch implementation
 */
#ifndef _BRANCH_TYPES_H_INCLUDED
#define _BRANCH_TYPES_H_INCLUDED

/** branch */
#define branch_dpchar_t			uint8_t
#define branch_dpcell_t			int16_t
#define BRANCH_DPCELL_MIN		( INT16_MIN )
#define BRANCH_DPCELL_MAX		( INT16_MAX )
#define BRANCH_BW 				( 32 )
#define branch_linear_bpl()		( sizeof(branch_dpcell_t) * BRANCH_BW )
#define branch_linear_bpb()		( sizeof(struct branch_linear_block) )

#include "../arch/b16c32.h"		/** 16bit 32cell */

/**
 * @struct branch_linear_block_dp
 */
struct branch_linear_block_dp {
	branch_dpcell_t dp[BLK][BRANCH_BW];
};

/**
 * @struct branch_linear_block_trailer
 */
struct branch_linear_block_trailer {
	int64_t i, j;
	branch_dpcell_t maxv[BRANCH_BW];
#if DP == DYNAMIC
	uint8_t dr[dir_size()];
#endif
};

/**
 * @struct branch_linear_joint_vec
 */
struct branch_linear_joint_vec {
	branch_dpcell_t pv[BRANCH_BW];
	branch_dpcell_t cv[BRANCH_BW];
	branch_dpchar_t wa[BRANCH_BW];
	branch_dpchar_t wb[BRANCH_BW];
};

/**
 * @struct branch_linear_block
 */
struct branch_linear_block {
	struct branch_linear_block_dp v;
	struct branch_linear_block_trailer t;
};

/**
 * @struct branch_linear_head
 */
struct branch_linear_head {
	struct sea_joint_head head;
	branch_dpcell_t pv[BRANCH_BW];
	branch_dpcell_t cv[BRANCH_BW];
	struct branch_linear_block_trailer t;
};

/**
 * @struct branch_linear_tail
 */
struct branch_linear_tail {
	struct branch_linear_joint_vec v;
	struct sea_joint_tail tail;
};

#ifndef _TYPES_INCLUDED
#define _TYPES_INCLUDED

#define dpchar_t 				branch_dpchar_t
#define dpcell_t 				branch_dpcell_t
#define DPCELL_MIN 				BRANCH_DPCELL_MIN
#define DPCELL_MAX 				BRANCH_DPCELL_MAX
#define BW 						BRANCH_BW

#endif /* #ifndef _TYPES_INCLUDED */

#endif /* #ifndef _BRANCH_TYPES_H_INCLUDED */
/**
 * end of branch_types.h
 */
