
/**
 * @file arch.h
 */
#ifndef _ARCH_H_INCLUDED
#define _ARCH_H_INCLUDED


#ifdef __x86_64__
#include "x86_64/arch_util.h"
#include "x86_64/vector.h"
#endif

#ifdef AARCH64
#endif

#ifdef PPC64
#endif

#if !defined(_ARCH_UTIL_H_INCLUDED) || !defined(_VECTOR_H_INCLUDED)
#error "Unsupported platform"
#endif

#endif /* #ifndef _ARCH_H_INCLUDED */
/**
 * end of arch.h
 */
