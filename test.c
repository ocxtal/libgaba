
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "include/sea.h"

/**
 * random sequence generator, modifier.
 * rseq generates random nucleotide sequence in ascii,
 * mseq takes ascii sequence, modifies the sequence in given rate.
 */
static char rbase(void)
{
	switch(rand() % 4) {
		case 0: return 'A';
		case 1: return 'C';
		case 2: return 'G';
		case 3: return 'T';
		default: return 'A';
	}
}

char *rseq(int len)
{
	int i;
	char *seq;
	seq = (char *)malloc(sizeof(char) * (len + 1));

	for(i = 0; i < len; i++) {
		seq[i] = rbase();
	}
	seq[len] = '\0';
	return(seq);
}

char *mseq(char const *seq, int x, int ins, int del)
{
	int i;
	int len = strlen(seq);
	char *mod, *ptr;
	mod = (char *)malloc(sizeof(char) * 2 * (len + 1));

	ptr = mod;
	for(i = 0; i < len; i++) {
		if(rand() % x == 0) { *ptr++ = rbase(); }
		else if(rand() % ins == 0) { *ptr++ = rbase(); i--; }
		else if(rand() % del == 0) { /* skip a base */ }
		else { *ptr++ = seq[i]; }
	}
	*ptr = '\0';
	return(mod);
}

int main(int argc, char *argv[])
{
	sea_t *ctx;
	sea_res_t *fres, *rres;
	char *a, *b;
	// char const *a = "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890";
	// char const *b = "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890";
	int len = 30;

	ctx = sea_init(
		SEA_DYNAMIC | SEA_LINEAR_GAP_COST | SEA_XSEA | SEA_ALN_CIGAR,
		2, -3, -5, -1, 100, 20, 30);

//	printf("%x\n", ctx->flags);

	unsigned long s = (argc == 2) ? atoi(argv[1]) : time(NULL);
	srand(s);
	printf("%lu\n", s);

	a = rseq(len);
	b = mseq(a, 10, 40, 40);

	printf("%s\n%s\n", a, b);

	int lm = 0, rm = 0;

	fres = sea_align_f(ctx,
		a, lm, strlen(a)-rm,
		b, lm, strlen(b)-rm,
		NULL, 0);

	rres = sea_align_r(ctx,
		a, lm, strlen(a)-rm,
		b, lm, strlen(b)-rm,
		NULL, 0);

	printf("%d, %lld, %s\n", fres->score, fres->len, fres->aln);
	printf("%d, %lld, %s\n", rres->score, rres->len, rres->aln);

	free(a);
	free(b);

	sea_aln_free(ctx, fres);
	sea_aln_free(ctx, rres);

	sea_clean(ctx);
	return 0;
}
