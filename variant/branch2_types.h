
/**
 * @file types.h
 *
 * @brief type definitoins for branch implementation
 */
#ifndef _TYPES_H_INCLUDED
#define _TYPES_H_INCLUDED

/** branch */
#define BW 						( 32 )
#define linear_bpl()			( sizeof(uint8_t) * BW )
#define linear_bpb()			( sizeof(struct linear_block) )

// #include "../arch/b16c32.h"			/** 16bit 32cell */

/**
 * @struct linear_block_dp
 */
struct linear_block_dp {
	uint8_t dp[BLK][BW];		/** 4bit packed */
};

/**
 * @struct linear_block_trailer
 */
struct linear_block_trailer {
	int64_t i, j;
	int64_t abs;
	int64_t _pad;
	uint8_t absv[BW];			/** 8bit abs offset */
	uint8_t maxv[BW];			/** 8bit max offset */
#if DP == DYNAMIC
	uint8_t dr[dir_size()];
#endif
};

/**
 * @struct linear_joint_vec
 */
struct linear_joint_vec {
	int64_t abs;
	int64_t _pad;
	uint8_t absv[BW];
	uint8_t dv[BW];
	uint8_t dh[BW];
	uint8_t wa[BW];
	uint8_t wb[BW];
};

/**
 * @struct linear_block
 */
struct linear_block {
	struct linear_block_dp v;
	struct linear_block_trailer t;
};

/**
 * @struct linear_head
 */
struct linear_head {
	struct sea_joint_head head;
	uint8_t dp[BW];
	struct linear_block_trailer t;
};

/**
 * @struct linear_tail
 */
struct linear_tail {
	struct linear_joint_vec v;
	struct sea_joint_tail tail;
};

#endif /* #ifndef _TYPES_H_INCLUDED */
/**
 * end of types.h
 */
