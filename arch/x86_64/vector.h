
/**
 * @file vector.h
 *
 * @brief header for various vector (SIMD) macros
 */
#ifndef _VECTOR_H_INCLUDED
#define _VECTOR_H_INCLUDED

#include "v2i64.h"
#include "v16i8.h"
#include "v32i8.h"
#include "v32i16.h"

/* conversion and cast between vector types */
#define _bc_v16i8_v32i8(x)		(v32i8_t){ (x).v1, (x).v1 }
#define _bc_v32i8_v32i8(x)		(v32i8_t){ (x).v1, (x).v2 }

#define _bc_v16i8_v16i8(x)		(v16i8_t){ (x).v1 }
#define _bc_v32i8_v16i8(x)		(v16i8_t){ (x).v1 }

#endif /* _VECTOR_H_INCLUDED */
/**
 * end of vector.h
 */
