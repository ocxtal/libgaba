
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

#include "../arch/b8c32.h"

struct trunk_linear_block {
	trunk_dpchar_t dp[BLK][TRUNK_BW];
	int64_t i, j;
	trunk_dpcell_t maxv[TRUNK_BW];
#if DP == DYNAMIC
	_dir_vec(dr);
#endif
};

struct trunk_linear_joint_vec {
	trunk_dpchar_t wt[TRUNK_BW];
	trunk_dpchar_t wq[TRUNK_BW];
	trunk_dpcell_t pv[TRUNK_BW];
	trunk_dpcell_t cv[TRUNK_BW];
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
