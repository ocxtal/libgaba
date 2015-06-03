
# libsea2 - an ultrafast seed-and-extend alignment library

## Overview

Libsea2 is a fast banded dynamic programming library for a nucleotide
sequence alignment. The library offers banded variants of the Smith-
Waterman (local alignment) algorithm, the Needleman-Wunsch (global
alignment) algorithm, and the seed-and-extend (semi-global alignment)
algorithm, which can be used in the various bioinformatics programs,
such as read mapping, sequence assembly, and database search.

## Features

* the linear-gap and affine-gap scoring schemes
* the local, global, and semi-global alignment algorithms
* fixed / variable width banded dynamic programming
* the dynamic band and the guided band algorithms
* various input / output formats
* parallelized with SIMD instructions
* thread-safe
* simple APIs for C
* class-based wrapper for C++, python, java, and D

## Environments

In general, the library can be built on x86_64-based UNIX / Linux
systems with the gcc / clang / icc compilers. Cygwin / mingw
environments on Windows systems are also supported.

### Supported architectures

* x86_64 processors with SSE4.2 or AVX2 instructions

### Tested operating systems and compilers

* Linux
    * icc   (Intel C Compiler, tested on 12.1.3 and 14.0.1 on SL6.5)
    * gcc   (4.4.4 on SL6.5, 4.6.3 on Ubuntu 12.04)
    * clang (3.0 on Ubuntu 12.04)

* Mac OS X
    * clang (3.5 on OS X 10.9.5)
    * gcc   (4.9.0 on OS X 10.9.5)

* Windows
    * gcc (on cygwin)
    * clang (on cygwin)

### Other dependencies

* Python (2.7 or 3.3)

## Installation

The library can be built and installed with the gnu standard build
steps.

	$ git clone git@bitbucket.org:suzukihajime/libsea2.git
	$ cd libsea2
	$ ./configure && make
	$ sudo make install

### Configuration options

General configuration options, such as --prefix=, CC=, ..., are
available.

* `--enable-java' tells the configuration script to build a jar
archive of the wrapper

## Benchmarks

## C APIs

### Functions

#### sea\_init

Generate a context with a given parameters.

```C
sea_t *sea_init(
	int32_t flags,		/** option flags: see Constants */ 
	int8_t m,			/** match score (m > 0) */
	int8_t x,			/** mismatch penalty (x < m) */
	int8_t gi,			/** gap open penalgy (2*gi < x) */
	int8_t ge,			/** gap extension penalty (ge < 0) */
	int32_t tx,			/** xdrop termination threshold (tx > 0) */
	int32_t tc,			/** chain threshold (tc > 0) */
	int32_t tb);		/** balloon termination threshold (tb > tc) */
```

#### sea\_align

Calculate an alignment with given two sequences. The sea\_align function
invokes the dynamic band algorithm if `guide == NULL`, otherwise the
function invokes the guided band algorithm.

```C
sea_res_t *sea_align(
	sea_t const *ctx,	/** a pointer to a context */
	void const *a,		/** a pointer to the head of a sequence a */
	int64_t asp,		/** alignment start position on the seq a */
	int64_t aep,		/** alignment end position on the seq a */
	void const *b,		/** a pointer to the head of a sequence b */
	int64_t bsp,		/** alignment start position on the seq b */
	int64_t bep,		/** alignment end position on the seq b */
	uint8_t const *guide,/** a pointer to the head of a guide string (can be NULL) */
	int64_t glen);		/** the length of the guide string (ignored if guide == NULL */
```

#### sea\_aln\_free

```C
void sea_aln_free(
	sea_t const *ctx,
	sea_res_t *aln);
```

#### sea\_clean

```C
void sea_clean(
	sea_t *ctx);
```


### Types

#### sea\_t

#### sea\_res\_t

### Constants

## Usage

### C

### C++

### Python

### Java

### D
