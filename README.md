# libgaba

GABA provides linear-gap and affine-gap penalty adaptive semi-global banded alignment on nucleotide ~~string graphs~~ (current implementation only supports trees). It uses fixed-width adaptive banded semi-global (a variant of Smith-Waterman and Gotoh) algorithm combined with difference DP (an acceleration technique similar to the Myers' bit-parallel edit distance algorithm) and X-drop termination.

Older version of the library (libsea) is available at
[https://bitbucket.org/suzukihajime/libsea/](https://bitbucket.org/suzukihajime/libsea/).

## Usage

### Example

```
#include "gaba.h"

/*
 * Initialize a global context. The global context can be
 * shared between threads. gaba_init function actually takes
 * a pointer to a const struct gaba_params_s, and GABA_PARAMS
 * is a macro to create a pointer with given parameters
 * (C99-style struct initializer is used in this example).
 * GABA_SCORE_SIMPLE(M, X, Gi, Ge) creates a substitution
 * matrix with match award = M, mismatch penalty = X,
 * and gap open penalty = Gi and gap extension penalty = Ge
 * where gap penalty function is g(k) = Gi + k * Ge (k is the
 * length of a contiguous gap). Note that current implementation
 * cannot handle scores with M + 2 * (Gi + Ge) >= 32 (the
 * value must fit in a unsigned 5-bit variable).
 */
gaba_t *ctx = gaba_init(GABA_PARAMS(
	.xdrop = 100,
	.score_matrix = GABA_SCORE_SIMPLE(2, 3, 5, 1)));

/*
 * Create section info (gaba_build_section is a macro).
 * Each section consists of a 2-bit or 4-bit encoded
 * nucleotide array (an array of uint8_t) and its length,
 * and an unique ID to identify the section.
 */
char const *a = "\x01\x08\x01\x08\x01\x08";	/* 4-bit encoded "ATATAT" */
char const *b = "\x01\x08\x01\x02\x01\x08";	/* 4-bit encoded "ATACAT" */
struct gaba_section_s asec = gaba_build_section(0, a, strlen(a));
struct gaba_section_s bsec = gaba_build_section(2, b, strlen(b));

/*
 * lim points the end of memory region of forward
 * sequences. If reverse-complemented sequences are not
 * available, lim should be 0x800000000000 (the tail address
 * of the user space on major Unix-like operating systems on
 * x86_64 processors). When concatenated reverse-complemented
 * sequence is available after concatenated forward sequence
 * (when the sequnces are stored like ->->->...-><-<-<-...<-
 * way, or if you are familiar with the implementation
 * of the BWA software it is the unpacked version of the
 * BWA reference sequence object), a pointer to the head
 * of the reverse-complemented section should be passed to
 * lim then the dp functions will properly use the reverse-
 * -complemented sequence to make the sequence fetching faster.
 */
void const *lim = (void const *)0x800000000000;

/*
 * Initialize a DP context (thread-local context)
 */
gaba_dp_t *dp = gaba_dp_init(ctx, lim, lim);

/*
 * Fill root with section A and section B from (0, 0).
 * dp_fill_root function creates a root of a DP matrix
 * and fill it until either sequence pointer reaches the end
 * of the sequence.
 */
struct gaba_section_s *ap = &asec, *bp = &bsec;
struct gaba_fill_s *f = gaba_dp_fill_root(dp, ap, 0, bp, 0);

/*
 * f->status & GABA_STATUS_UPDATE_A indicates section A
 * reached the end. If you want to continue with another
 * section, you should call gaba_dp_fill with f and the
 * section A pointer replaced with the next section (and
 * section B pointer unchanged if f->status indicates
 * that sequence pointer on section B did not reach the
 * end in the first fill).
 */
if(f->status & GABA_STATUS_UPDATE_A) {
	ap = /* pointer to the next section of a */
}
if(f->status & GABA_STATUS_UPDATE_B) {
	bp = /* pointer to the next section of b */
}
f = gaba_dp_fill(dp, f, ap, bp);

/*
 * ...you can fill sequence trees in arbitrary order
 * with gaba_dp_fill function.
 */

/*
 * Each fill object (f) contains the max score of the
 * section (f->max). All the fill objects are allocated
 * in the dp context so you do not have to (must not)
 * free them even though you want to discard them.
 * Alternatively, you can flush the previous results
 * (and garbages stacked in the dp context) with
 * gaba_dp_flush function. It will destroy all the
 * previous results so it should be called when all the
 * tasks on current sequences (or current read) is done.
 */

/*
 * Traces (alignment paths) are calculated from
 * arbitrary sections to the root. Traces are stored
 * in r->path in a 1-bit packed direction array.
 * (0: RIGHT / 1: DOWN). The generated path of the
 * forward section (second argument) and the reversed
 * path generated from the reverse section (third argument)
 * are concatenated at the root.
 */
struct gaba_result_s *r = gaba_dp_trace(dp,
	f,			/* forward section */
	NULL,		/* reverse section */
	NULL);

/*
 * 1-bit packed direction string can be converted to CIGAR
 * string (defined in the SAM format) with dump_cigar
 * or print_cigar function.
 */
gaba_dp_print_cigar(
	(gaba_dp_fprintf_t)fprintf,
	(void *)stdout,
	r->path->array,
	r->path->offset,
	r->path->len);

/* destroy the DP context */
gaba_dp_clean(dp);

/* destroy the global context */
gaba_clean(ctx);
```

### Input sequence formats

Current implementation accept 2-bit encoded (A = 0x00, C = 0x01, G = 0x02, T = 0x03) sequences or 4-bit encoded (A = 0x01, C = 0x02, G = 0x04, T = 0x08) sequences with ambiguity, stored in uint8\_t arrays. The format must be determined at the compile time of gaba.c and can be switched with -DBIT=2 or -DBIT=4 (default) flags.


### Substitution matrix

If you selected the 2-bit format, the fill-in functions will calculate DP cells with a 4 x 4-sized substitution matrix. The matrix is represented in a 4 x 4-sized two-dimensional int8\_t array (`score_sub` member in `struct gaba_score_s`), with element at [0][0] corresponding to a score of ('A', 'A') pair, [0][1] to ('A', 'C'), ... and [3][3] to ('T', 'T'), respectively. The functions cannot handle 4 x 4-sized matrix in the default 4-bit format setting and use match-mismatch model instead, interpreting the maximum value on the diagonal of the substitution matrix as a match award and minimum value among the other 12 cells as a mismatch penalty.


```
struct gaba_score_s {
	int8_t score_sub[4][4];	
	int8_t score_gi_a, score_ge_a;
	int8_t score_gi_b, score_ge_b;
};
```


### Gap penalty functions

In the default setting it uses the affine-gap penalty function represented in g(k) = Gi + k * Ge form where k is the length of a contiguous gap and Gi and Ge are positive (or zero) integer penalties. Different coefficients on two sequences (query and sequences) are allowed, with setting different values to `score_gi_a` and `score_gi_b` pair, or `score_ge_a` and `score_ge_b` pair in `struct gaba_score_s`. When the library is compiled in the linear-gap penalty setting (when gaba.c is compiled with -DMODEL=LINEAR), it uses a gap penalty function of g(k) = k * (Gi + Ge) form. The library cannot handle scoreing schemes with M + 2 * (Gi + Ge) > 31 due to the limitation of the diff algorithm used in the library. Setting gap extension penalty zero (Ge = 0 in the affine-gap setting or Gi + Ge = 0 in the linear-gap setting) may also result in an incorrect alignment.


### Sections

All the input sequences are passed to the functions stored in the `gaba_section_s` object. The section object consists of a unique ID of the sequence fragment, a pointer to the head of the sequece fragment, and its length.

```
struct gaba_section_s {
	uint32_t id;
	uint32_t len;
	uint8_t const *base;
};
```


#### Unique IDs

Unique IDs are 32-bit positive integer. Even and odd numbers (e.g. 0 and 1) are paired to represent forward and reverse-complemented sequences of the same segment. The traceback function uses this correspondence between forward and reverse-complemented sections to express chains of reversed sections in the reverse traceback (see third argument of `gaba_dp_trace`).


#### Sequences

The sequence fragments can be stored adjacently in the memory or separately, since section is only aware of  where the sequence starts and how long it is. That is, concatenated reference sequences structure (used in many major alignment tools) can be used to feed its reference sequences to the fill functions with additional set of `gaba_section_s`. The extension will be terminated with a X-drop test, or continued to the end of the section. So it will be a good choice to hold each choromosome in a single section. You must aware that the functions will not fill the triangular region at the tail of the band (see fig below) even though it is inside the area defined by the two sections (that is because the band is calculated in the anti-diagonal way and the sequences at the head of the next section is needed to fill the tail triangular area). In order to extend alignment to the end of the sequences, dummy section with random or zero 32-bases should be passed after the end of the terminal segment or margin with dummy sequence should be added to the both terminals when building reference sequence structrure.

```
      section A   ...
     +---------+------
     |    \    |
     |\    \   |
     | \    \  |
  B  |  \    \ |
     |   \    \| margin
     |    \   /|<-->
     |     \ /*|
     +---------+------
     |         |
DP cells in the triangular area (*) will not be
calculated in a call of fill(A, B). Margin is needed
after the sections if you want to fill matrix to the 
end of the (terminal) section.
```


#### Pointers to the reverse-complemented sequences

Sections of reverse-complemented sequences are little bit complecated. They must have IDs  with its bit 0 reversed from its corresponding forward section as described above. Pointers to the reverse-complemented sequences must be mirrored with a constant (void *) lim. For example, when a forward sequence is stored at address `0x10000` having length `0x200` and lim is `0x800000000000` (the default value of lim), the sequence pointer in a reverse-complemented section must be `0xfffffffefdff`, or 2 * lim - ptr - len. The fill functions check the pointer and if it points out of valid region (the default is `0` to `0x7fffffffffff`, which is exactly overlaps with the user space of the major unix-like operating systems on x86\_64 processors), they calculate the mirrored pointer and read sequences in the reverse-complemented way. You can tell the fill functions to use actual array of reverse-complemented sequences on memory when concatenated reverse-complement sequences are stored just after the concatenated forward sequences passing the head address of the reverse-complemented section as the lim.


### Margins after sequences

When the fill functions fetch sequences from array, they use vector load instruction to parallelize sequence handling thus may invade tail boundary of the array. To keep the implementation fast, adding invasion checking in the fill functions are not planned so users must add 32-bytes margin after sequence arrays to avoid segfaults in the fill functions.


### Aligning concatenated sequence of multiple sections

The library can align sequences consisting of multiple sections. The root fill function, `gaba_dp_fill_root` creates the root of a matrix and extend alignment until either sequence pointer reaches the end. The normal fill function, `gaba_dp_fill`, takes a previous matrix (or band, the second argument of the function) and continue aligning with the given next two sections. The return object, `gaba_dp_fill_t` has flags (`gaba_dp_fill_t.status`) that indicates what reason the calculation terminated. The update flags, which are extracted with `GABA_STATUS_UPDATE_A` and `GABA_STATUS_UPDATE_B` masks, becomes non-zero when each sequence pointer reached its end. Only the section with update sign must be replaced with the next section and the other (section without update sign) must be left unchanged when calling the fill-in function with the previous return object to continue extending alignment. The X-drop termination flag, extracted with `GABA_STATUS_TERM`, indicates that the extension was terminated with X-drop test failure and cannot be continued.

#### Pairwise alignment on trees

The return object and corresponding matrix are treated as immutable in the library, enabling users to connect the matrix to multiple subsequent matrices. The multiple connection, or division, of the matrix can be applied to align sequences on tree structures. Each matrix keeps a link to the previous matrix (given to the fill-in funciton as second argument) thus the traceback will properly performed from (arbitrary) matrix fragment to the root. The next update of the library will support merging of multiple matrices (bands). This enables the user to align sequences in graphs (nucleotide string graphs).


### Maximum score reporting policy

The return object of the fill functions (`gaba_dp_fill_t`) contains `max` field that keeps the maximum score among all the matrix fragments from the root to the current. Users must aware that the value will not be updated when the scores in the current section are decreasing from the head to the tail. In this case, the value represents the maximum score of the previous matrix fragments, not the maximum score of **the current matrix fragment**.


## Functions

### Init / cleanup global context

#### gaba\_init

Global context holds global configurations of the alignment. It can be shared between multiple threads (thread-safe) and must be created once at first.

```
gaba_t *gaba_init(gaba_params_t const *params);
```

#### gaba\_clean

```
void gaba_clean(gaba_t *ctx);
```

### Init / cleanup DP context

#### gaba\_dp\_init

DP context is a local context holding thread-local configurations and working buffers. It can't be shared between threads.

```
struct gaba_dp_context_s *gaba_dp_init(
	gaba_t const *ctx,
	gaba_seq_pair_t const *p);
```

#### gaba\_dp\_flush

Flush the working buffer in the DP context. Any previous result (generated by `gaba_dp_fill` and `gaba_dp_trace`) will be invalid after call.

```
void gaba_dp_flush(
	gaba_dp_t *this,
	gaba_seq_pair_t const *p);
```

#### gaba\_dp\_clean

```
void gaba_dp_clean(
	gaba_dp_t *this);
```

### Alignment functions

#### gaba\_dp\_fill\_root

Create a root section of the extension alignment.

```
gaba_fill_t *gaba_dp_fill_root(
	gaba_dp_t *this,
	gaba_section_t const *a,
	uint32_t apos,
	gaba_section_t const *b,
	uint32_t bpos);
```

#### gaba\_dp\_fill

Extends a section.

```
gaba_fill_t *gaba_dp_fill(
	gaba_dp_t *this,
	gaba_fill_t const *prev_sec,
	gaba_section_t const *a,
	gaba_section_t const *b);
```

#### gaba\_dp\_merge

Merge multiple alignment sections. (not implemented yet)

```
gaba_fill_t *gaba_dp_merge(
	gaba_dp_t *this,
	gaba_fill_t const *sec_list,
	uint64_t tail_list_len);
```

#### gaba\_dp\_trace

Traceback from the max. score in the given sections and concatenate paths at the root.

```
gaba_result_t *gaba_dp_trace(
	gaba_dp_t *this,
	gaba_fill_t const *fw_tail,
	gaba_fill_t const *rv_tail,
	gaba_clip_params_t const *clip);
```

### Utils

#### gaba\_dp\_print\_cigar

Convert path string and dump to `FILE *`.

```
typedef int (*gaba_dp_fprintf_t)(void *, char const *, ...);
int64_t gaba_dp_print_cigar(
	gaba_dp_fprintf_t fprintf,
	void *fp,
	uint32_t const *path,
	uint32_t offset,
	uint32_t len);
```

#### gaba\_dp\_dump\_cigar

Convert path string and dump to a `buf`. `buf` must have enough room to store the cigar string.

```
int64_t gaba_dp_dump_cigar(
	char *buf,
	uint64_t buf_size,
	uint32_t const *path,
	uint32_t offset,
	uint32_t len);
```

## License

Apache v2.

Copyright (c) 2015-2016 Hajime Suzuki