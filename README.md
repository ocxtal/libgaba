
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
* parallelized with SIMD instructions
* thread-safe
* simple APIs for C
* class-based wrapper for C++, python, java, and D

## Environments

In general, the library can be built on x86_64-based UNIX / Linux
systems with the gcc / clang / icc compilers. The cygwin / mingw
environment on the Windows systems are also supported.

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

#### sea\_align

#### sea\_aln\_free

#### sea\_clean

### Types

#### sea\_ctx\_t

#### sea\_res\_t

## Usage

### C

### C++

### Python

### Java

### D
