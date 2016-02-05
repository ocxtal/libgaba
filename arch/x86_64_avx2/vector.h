
/**
 * @file vector.h
 *
 * @brief header for various vector (SIMD) macros
 */
#ifndef _VECTOR_H_INCLUDED
#define _VECTOR_H_INCLUDED

/**
 * abstract vector types
 *
 * v2i32_t, v2i64_t for pair of 32-bit, 64-bit signed integers. Mainly for
 * a pair of coordinates. Conversion between the two types are provided.
 *
 * v16i8_t is a unit vector for substitution matrices and gap vectors.
 * Broadcast to v16i8_t and v32i8_t are provided.
 *
 * v32i8_t is a unit vector for small differences in banded alignment. v16i8_t
 * vector can be broadcasted to high and low 16 elements of v32i8_t. It can
 * also expanded to v32i16_t.
 *
 * v32i16_t is for middle differences in banded alignment. It can be converted
 * from v32i8_t
 */
#include "v2i32.h"
#include "v2i64.h"
#include "v16i8.h"
#include "v32i8.h"
#include "v32i16.h"

/* conversion and cast between vector types */
#define _bc_v16i8_v32i8(x)		(v32i8_t){ _mm256_broadcastsi128_si256((x).v1) }
#define _bc_v32i8_v32i8(x)		(v32i8_t){ (x).v1 }

#define _bc_v16i8_v16i8(x)		(v16i8_t){ (x).v1 }
#define _bc_v32i8_v16i8(x)		(v16i8_t){ _mm256_castsi256_si128((x).v1) }

#endif /* _VECTOR_H_INCLUDED */
/**
 * end of vector.h
 */