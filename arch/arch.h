
/**
 * @file arch.h
 */
#ifndef _ARCH_H_INCLUDED
#define _ARCH_H_INCLUDED


#ifdef __x86_64__
#  if defined(__AVX2__)
#    include "x86_64_avx2/arch_util.h"
#    include "x86_64_avx2/vector.h"
#  elif defined(__SSE4_1__)
#    include "x86_64_sse41/arch_util.h"
#    include "x86_64_sse41/vector.h"
#  else
#    error "No SIMD instruction set enabled. Check if SSE4.1 or AVX2 instructions are available and add `-msse4.1' or `-mavx2' to CFLAGS."
#  endif
#endif

#ifdef AARCH64
#endif

#ifdef PPC64
#endif

#if !defined(_ARCH_UTIL_H_INCLUDED) || !defined(_VECTOR_H_INCLUDED)
#  error "No SIMD environment detected. Check CFLAGS."
#endif

#endif /* #ifndef _ARCH_H_INCLUDED */
/**
 * end of arch.h
 */
