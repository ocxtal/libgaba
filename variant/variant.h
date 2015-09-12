
/**
 * @file variant.h
 *
 * @brief variant selector
 *
 * select one of the variant from
 * {naive, branch, twig, ...} x {linear, affine} x {dynamic, guided}
 * according to the macros (defines) and include a set of proper headers.
 */
#ifndef _VARIANT_H_INCLUDED
#define _VARIANT_H_INCLUDED

#include "../sea.h"
#include "../util/util.h"
#include <stdint.h>

/**
 * constants for selector
 */
/** base variant */
#define NAIVE 		( 2 )
#define BRANCH 		( 3 )
#define TWIG 		( 4 )
#define TRUNK 		( 5 )
#define CAP 		( 6 )
#define BULGE 		( 7 )
#define BALOON 		( 8 )

/** cost variant */
#define LINEAR 		( 10 )
#define AFFINE 		( 11 )

/** dp variant */
#define DYNAMIC		( 20 )		// avoid 1
#define GUIDED 		( 21 )

/**
 * defaults
 */
// #define BASE 			NAIVE
// #define DP 				GUIDED
// #define COST 			LINEAR

/**
 * macro aliasing
 */

/** base variant */
#ifndef BASE
	#error "BASE undefined"
#endif
#if BASE == NAIVE
	#define BASE_LABEL 			naive
#elif BASE == BRANCH
	#define BASE_LABEL 			branch
#elif BASE == TWIG
	#define BASE_LABEL 			twig
#elif BASE == TRUNK
	#define BASE_LABEL 			trunk
#elif BASE == CAP
	#define BASE_LABEL 			cap
#elif BASE == BULGE
	#define BASE_LABEL 			bulge
#elif BASE == BALOON
	#define BASE_LABEL 			baloon
#else
	#error "invalid BASE"
#endif /* #if BASE == NAIVE */

/** cost variant */
#ifndef COST
	#error "COST undefined"
#endif
#if COST == LINEAR
	#define COST_LABEL 			linear
	#define COST_SUFFIX 		_linear
#elif COST == AFFINE
	#define COST_LABEL	 		affine
	#define COST_SUFFIX 		_affine
#else /* #if COST == LINEAR */
	#error "COST must be LINEAR or AFFINE."
#endif /* #if COST == LINEAR */

/** dp variant */
#ifndef DP
	#warning "DP undefined"
#endif
#if DP == DYNAMIC
	#define DP_LABEL 			dynamic
	#define DP_SUFFIX 			_dynamic
#elif DP == GUIDED
	#define DP_LABEL 			guided
	#define DP_SUFFIX 			_guided
#else
	#error "invalid DP"
#endif /* #if DP == DYNAMIC */

/** variant label */
#define VARIANT_LABEL			label3(BASE_LABEL, COST_SUFFIX, DP_SUFFIX)

/**
 * variant selector
 */
#define header(base)			QUOTE(label2(base, _impl.h))

/** dp variants */
#include header(DP_LABEL)

/** address calculation */
#define dr_size					label2(DP_LABEL, _dr_size)
#define blk_num					label2(DP_LABEL, _blk_num)
#define blk_addr				label2(DP_LABEL, _blk_addr)
#define addr 					label2(DP_LABEL, _addr)

/** direction accessors */
#define dir 					label2(DP_LABEL, _dir)
#define dir2 					label2(DP_LABEL, _dir2)
#define dir_ue 					label2(DP_LABEL, _dir_ue)
#define dir2_ue 				label2(DP_LABEL, _dir2_ue)
#define dir_le 					label2(DP_LABEL, _dir_le)
#define dir2_le 				label2(DP_LABEL, _dir2_le)
#define dir_raw 				label2(DP_LABEL, _dir_raw)

/** direction determiners */
#define dir_init 				label2(DP_LABEL, _dir_init)
#define dir_start_block 		label2(DP_LABEL, _dir_start_block)
#define dir_det_next 			label2(DP_LABEL, _dir_det_next)
#define dir_empty				label2(DP_LABEL, _dir_empty)
#define dir_end_block 			label2(DP_LABEL, _dir_end_block)
#define dir_test_bound			label2(DP_LABEL, _dir_test_bound)
#define dir_test_bound_cap		label2(DP_LABEL, _dir_test_bound_cap)

#define dir_set_pdr 			label2(DP_LABEL, _dir_set_pdr)
#define dir_load_forward		label2(DP_LABEL, _dir_load_forward)
#define dir_go_forward			label2(DP_LABEL, _dir_go_forward)
#define dir_load_backward 		label2(DP_LABEL, _dir_load_backward)
#define dir_go_backward			label2(DP_LABEL, _dir_go_backward)
#define dir_sum_i_blk			label2(DP_LABEL, _dir_sum_i_blk)
#define dir_set_pdr_fast 		label2(DP_LABEL, _dir_set_pdr_fast)
#define dir_load_backward_fast 	label2(DP_LABEL, _dir_load_backward_fast)

/** base and cost variants */
#include header(BASE_LABEL)

/** address calculation */
#define bpl						label3(BASE_LABEL, COST_SUFFIX, _bpl)
#define dp_size					label3(BASE_LABEL, COST_SUFFIX, _dp_size)
#define co_size					label3(BASE_LABEL, COST_SUFFIX, _co_size)
#define jam_size				label3(BASE_LABEL, COST_SUFFIX, _jam_size)
#define head_size				label3(BASE_LABEL, COST_SUFFIX, _head_size)
#define tail_size				label3(BASE_LABEL, COST_SUFFIX, _tail_size)
#define bpb						label3(BASE_LABEL, COST_SUFFIX, _bpb)
#define topq					label3(BASE_LABEL, COST_SUFFIX, _topq)
#define leftq					label3(BASE_LABEL, COST_SUFFIX, _leftq)
#define topleftq				label3(BASE_LABEL, COST_SUFFIX, _topleftq)
#define dir_exp_top 			label3(BASE_LABEL, COST_SUFFIX, _dir_exp_top)
#define dir_exp_bottom 			label3(BASE_LABEL, COST_SUFFIX, _dir_exp_bottom)

/** fill-in */
#define fill_decl				label3(BASE_LABEL, COST_SUFFIX, _fill_decl)
#define fill_init				label3(BASE_LABEL, COST_SUFFIX, _fill_init)
#define fill_start 				label3(BASE_LABEL, COST_SUFFIX, _fill_start)
#define fill_former_body		label3(BASE_LABEL, COST_SUFFIX, _fill_former_body)
#define fill_former_body_cap	label3(BASE_LABEL, COST_SUFFIX, _fill_former_body_cap)
#define fill_go_down			label3(BASE_LABEL, COST_SUFFIX, _fill_go_down)
#define fill_go_down_cap		label3(BASE_LABEL, COST_SUFFIX, _fill_go_down_cap)
#define fill_go_right			label3(BASE_LABEL, COST_SUFFIX, _fill_go_right)
#define fill_go_right_cap		label3(BASE_LABEL, COST_SUFFIX, _fill_go_right_cap)
#define fill_latter_body		label3(BASE_LABEL, COST_SUFFIX, _fill_latter_body)
#define fill_latter_body_cap	label3(BASE_LABEL, COST_SUFFIX, _fill_latter_body_cap)
#define fill_empty_body			label3(BASE_LABEL, COST_SUFFIX, _fill_empty_body)
#define fill_test_xdrop			label3(BASE_LABEL, COST_SUFFIX, _fill_test_xdrop)
#define fill_test_xdrop_cap		label3(BASE_LABEL, COST_SUFFIX, _fill_test_xdrop_cap)
#define fill_test_bound 		label3(BASE_LABEL, COST_SUFFIX, _fill_test_bound)
#define fill_test_bound_cap		label3(BASE_LABEL, COST_SUFFIX, _fill_test_bound_cap)
#define fill_test_mem			label3(BASE_LABEL, COST_SUFFIX, _fill_test_mem)
#define fill_test_mem_cap		label3(BASE_LABEL, COST_SUFFIX, _fill_test_mem_cap)
#define fill_test_chain			label3(BASE_LABEL, COST_SUFFIX, _fill_test_chain)
#define fill_test_chain_cap		label3(BASE_LABEL, COST_SUFFIX, _fill_test_chain_cap)
#define fill_check_term			label3(BASE_LABEL, COST_SUFFIX, _fill_check_term)
#define fill_check_term_cap		label3(BASE_LABEL, COST_SUFFIX, _fill_check_term_cap)
#define fill_end 				label3(BASE_LABEL, COST_SUFFIX, _fill_end)
#define fill_finish				label3(BASE_LABEL, COST_SUFFIX, _fill_finish)

/** search terminal (NW) */
#define set_terminal			label3(BASE_LABEL, COST_SUFFIX, _set_terminal)

/** traceback */
#define trace_decl				label3(BASE_LABEL, COST_SUFFIX, _trace_decl)
#define trace_init				label3(BASE_LABEL, COST_SUFFIX, _trace_init)
#define trace_body				label3(BASE_LABEL, COST_SUFFIX, _trace_body)
#define trace_check_term 		label3(BASE_LABEL, COST_SUFFIX, _trace_check_term)
#define trace_finish			label3(BASE_LABEL, COST_SUFFIX, _trace_finish)

#endif /* #ifndef _VARIANT_H_INCLUDED */

/**
 * end of variant.h
 */
