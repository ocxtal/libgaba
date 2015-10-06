
/**
 * @file naive_types.h
 *
 * @brief type definitions for the naive implementation
 */
#ifndef _NAIVE_TYPES_H_INCLUDED
#define _NAIVE_TYPES_H_INCLUDED

#define naive_dpchar_t			uint8_t
#define naive_dpcell_t			int32_t
#define NAIVE_DPCELL_MIN		( INT32_MIN )
#define NAIVE_DPCELL_MAX		( INT32_MAX )
#define NAIVE_BW 				( 32 )


struct naive_linear_block {
	naive_dpchar_t dp[BLK][NAIVE_BW];
	int64_t i, j;
#if DP == DYNAMIC
	_dir_vec(dr);
#endif
};

struct naive_linear_joint_vec {
	naive_dpchar_t wt[NAIVE_BW];
	naive_dpchar_t wq[NAIVE_BW];
	naive_dpcell_t pv[NAIVE_BW];
	naive_dpcell_t cv[NAIVE_BW];
};

#ifndef _TYPES_INCLUDED
#define _TYPES_INCLUDED

#define dpchar_t 				naive_dpchar_t
#define dpcell_t 				naive_dpcell_t
#define DPCELL_MIN 				NAIVE_DPCELL_MIN
#define DPCELL_MAX 				NAIVE_DPCELL_MAX
#define BW 						NAIVE_BW

#endif /* #ifndef _TYPES_INCLUDED */

#endif /* #ifndef _NAIVE_TYPES_H_INCLUDED */
/**
 * end of naive_types.h
 */
