
/**
 * @file bench.c
 * @brief speed benchmark of libsea
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <getopt.h>
#include "util/kvec.h"
#include "util/util.h"
#include "sea.h"

extern bench_t fill, search, trace;	/** global benchmark variables */

/**
 * @fn print_usage
 */
void print_usage(void)
{
	fprintf(stderr, "usage: bench -l <len> -c <cnt> -x <mismatch rate> -d <indel rate>\n");
}

/**
 * @fn random_base
 *
 * @brief return a character randomly from {'A', 'C', 'G', 'T'}.
 */
char random_base(void)
{
	return(rand() % 4);
}

/**
 * @fn generate_random_sequence
 *
 * @brief malloc memory of size <len>, then fill it randomely with {'A', 'C', 'G', 'T'}.
 *
 * @param[in] len : the length of sequence to generate.
 * @return a pointer to the generated sequence.
 */
char *generate_random_sequence(int len)
{
	int i;
	char *seq;		/** a pointer to sequence */
	seq = (char *)malloc(sizeof(char) * (len + 1));
	if(seq == NULL) { return NULL; }
	for(i = 0; i < len; i++) {
		seq[i] = random_base();
	}
	seq[len] = '\0';
	return seq;
}

/**
 * @fn generate_mutated_sequence
 *
 * @brief take a DNA sequence represented in ASCII, mutate the sequence at the given probability.
 *
 * @param[in] seq : a pointer to the string of {'A', 'C', 'G', 'T'}.
 * @param[in] x : mismatch rate
 * @param[in] d : insertion / deletion rate
 * @param[in] bw : bandwidth (the alignment path of the input sequence and the result does not go out of the band)
 *
 * @return a pointer to mutated sequence.
 */
char *generate_mutated_sequence(char *seq, int len, double x, double d, int bw)
{
	int i, j, wave = 0;			/** wave is q-coordinate of the alignment path */
	char *mutated_seq;

	if(seq == NULL) { return NULL; }
	mutated_seq = (char *)malloc(sizeof(char) * (len + 1));
	if(mutated_seq == NULL) { return NULL; }
	for(i = 0, j = 0; i < len; i++) {
		if(((double)rand() / (double)RAND_MAX) < x) {
			mutated_seq[i] = random_base();	j++;	/** mismatch */
		} else if(((double)rand() / (double)RAND_MAX) < d) {
			if(rand() & 0x01 && wave > -bw+1) {
				mutated_seq[i] = (j < len) ? seq[j++] : random_base();
				j++; wave--;						/** deletion */
			} else if(wave < bw-2) {
				mutated_seq[i] = random_base();
				wave++;								/** insertion */
			} else {
				mutated_seq[i] = (j < len) ? seq[j++] : random_base();
			}
		} else {
			mutated_seq[i] = (j < len) ? seq[j++] : random_base();
		}
	}
	mutated_seq[len] = '\0';
	return mutated_seq;
}

/**
 * @struct params
 */
struct params {
	int64_t len;
	int64_t cnt;
	double x;
	double d;
	char **pa;
	char **pb;
};

/**
 * @fn parse_args
 */
int parse_args(struct params *p, int c, char *arg)
{
	switch(c) {
		/**
		 * sequence generation parameters.
		 */
		case 'l': p->len = atoi((char *)arg); return 0;
		case 'x': p->x = atof((char *)arg); return 0;
		case 'd': p->d = atof((char *)arg); return 0;
		/**
		 * benchmarking options
		 */
		case 'c': p->cnt = atoi((char *)arg); return 0;
		case 'a': printf("%s\n", arg); return 0;
		/**
		 * the others: print help message
		 */
		case 'h':
		default: print_usage(); return -1;
	}
}

/**
 * @fn main
 */
int main(int argc, char *argv[])
{
	int64_t i;
	kvec_t(char *) pa;
	kvec_t(char *) pb;
	struct params p;
	sea_t *d, *g;
	bench_t total;

	/** set defaults */
	p.len = 10000;
	p.cnt = 10000;
	p.x = 0.1;
	p.d = 0.1;
	p.pa = p.pb = NULL;

	/** parse args */
	while((i = getopt(argc, argv, "q:t:o:l:x:d:c:a:seb:h")) != -1) {
		if(parse_args(&p, i, optarg) != 0) { exit(1); }
	}

	/** init sequence */
	kv_init(pa);
	kv_init(pb);
	for(i = 0; i < p.cnt; i++) {
		kv_push(pa, generate_random_sequence(p.len));
		kv_push(pb, generate_mutated_sequence(kv_at(pa, i), p.len, p.x, p.d, 8));
	}
	kv_push(pa, NULL);
	kv_push(pb, NULL);

	/** init context */
	d = sea_init(
		  SEA_SEA | SEA_LINEAR_GAP_COST | SEA_DYNAMIC
		| SEA_SEQ_A_2BIT | SEA_SEQ_B_2BIT | SEA_ALN_ASCII,
		2, -3, -5, -1, 60);
	g = sea_init(
		  SEA_SEA | SEA_LINEAR_GAP_COST | SEA_GUIDED
		| SEA_SEQ_A_2BIT | SEA_SEQ_B_2BIT | SEA_ALN_ASCII,
		2, -3, -5, -1, 60);
	bench_init(fill);
	bench_init(search);
	bench_init(trace);
	bench_init(total);

	/**
	 * run benchmark.
	 */
	for(i = 0; i < p.cnt; i++) {
		bench_start(total);
		sea_res_t *rd = sea_align(d,
			kv_at(pa, i), 0, p.len,
			kv_at(pb, i), 0, p.len,
			NULL, 0);
		bench_end(total);
		sea_aln_free(d, rd);
	}
	
	/**
	 * print results.
	 */
	printf("%lld, %lld, %lld, %lld, %lld\n",
		bench_get(fill),
		bench_get(search),
		bench_get(trace),
		bench_get(fill) + bench_get(search) + bench_get(trace),
		bench_get(total));

	/**
	 * clean malloc'd memories
	 */
	for(i = 0; i < p.cnt; i++) {
		free(kv_at(pa, i));
		free(kv_at(pb, i));
	}
	kv_destroy(pa);
	kv_destroy(pb);

	sea_close(d);
	return 0;
}

/**
 * end of bench.c
 */
