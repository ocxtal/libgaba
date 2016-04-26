
/**
 * @file bench.c
 * @brief speed benchmark of libsea
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <getopt.h>
#include "kvec.h"
#include "bench.h"
#include "gaba.h"

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
	char const table[4] = {'A', 'C', 'G', 'T'};
	return(table[rand() % 4]);
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
	char *a;
	char *b;
	struct params p;
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

	fprintf(stderr, "len\t%lld\ncnt\t%lld\nx\t%f\nd\t%f\n", p.len, p.cnt, p.x, p.d);

	/** init sequence */
	a = generate_random_sequence(p.len);
	b = generate_mutated_sequence(a, p.len, p.x, p.d, 8);

	/** init context */
	gaba_t *ctx = gaba_init(GABA_PARAMS(
		.seq_a_direction = GABA_FW_ONLY,
		.seq_b_direction = GABA_FW_ONLY,
		.xdrop = 100,
		.score_matrix = GABA_SCORE_SIMPLE(2, 3, 5, 1)));
	gaba_seq_pair_t seq = gaba_build_seq_pair(a, strlen(a), b, strlen(b));
	struct gaba_section_s asec = gaba_build_section(1, 0, strlen(a));
	struct gaba_section_s bsec = gaba_build_section(2, 0, strlen(b));

	bench_init(total);

	/**
	 * run benchmark.
	 */
	int64_t score = 0;
	for(i = 0; i < p.cnt; i++) {
		gaba_dp_t *dp = gaba_dp_init(ctx, &seq);

		bench_start(total);

		struct gaba_fill_s *f = gaba_dp_fill_root(dp, &asec, 0, &bsec, 0);
		// struct gaba_result_s *r = gaba_dp_trace(dp, f, NULL, NULL);
		score += f->max;

		bench_end(total);

		gaba_dp_clean(dp);
	}
	
	/**
	 * print results.
	 */
	printf("%lld\t%lld\n", bench_get(total), score);

	/**
	 * clean malloc'd memories
	 */
	free(a);
	free(b);

	gaba_clean(ctx);
	return 0;
}

/**
 * end of bench.c
 */
