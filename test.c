
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "sea.h"

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
	sea_t *d = NULL, *c = NULL, *r;
	sea_res_t *dres = NULL, *cres = NULL, *rres = NULL, *res = NULL;
	char *a, *b, *at, *bt;
	struct timeval tv;
	// char const *a = "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890";
	// char const *b = "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890";
	int len = 300;

	d = sea_init(
		SEA_LINEAR_GAP_COST | SEA_XSEA | SEA_ALN_DIR,
		2, -3, -5, -1, 100);

	c = sea_init(
		SEA_LINEAR_GAP_COST | SEA_XSEA | SEA_ALN_CIGAR,
		2, -3, -5, -1, 100);

	r = sea_init(
		SEA_LINEAR_GAP_COST | SEA_XSEA | SEA_ALN_ASCII,
		2, -3, -5, -1, 100);


//	printf("%x\n", ctx->flags);

	gettimeofday(&tv, NULL);
	unsigned long s = (argc == 2) ? atoi(argv[1]) : tv.tv_usec;
	srand(s);
	printf("%lu\n", s);

	a = rseq(len * 9 / 10);
	b = mseq(a, 10, 40, 40);
	at = rseq(len / 10);
	bt = rseq(len / 10);
	a = realloc(a, 2*len); strcat(a, at); free(at);
	b = realloc(b, 2*len); strcat(b, bt); free(bt);

	printf("%s\n%s\n", a, b);

	int lm = 5, rm = 5;
/*
	dres = sea_align(d,
		a, lm, strlen(a)-rm,
		b, lm, strlen(b)-rm,
		NULL, 0);

	cres = sea_align(c,
		a, lm, strlen(a)-rm,
		b, lm, strlen(b)-rm,
		dres->aln, dres->slen);

	rres = sea_align(r,
		a, lm, strlen(a)-rm,
		b, lm, strlen(b)-rm,
		dres->aln, dres->slen);
*/
	res = sea_align(r,
		a, lm, strlen(a)-rm,
		b, lm, strlen(b)-rm,
		NULL, 0);

	sea_add_clips(c, res, lm, rm, SEA_CLIP_HARD);

//	printf("%d, %lld\n", dres->score, dres->plen);
//	printf("%d, %lld, %s\n", cres->score, cres->plen, cres->aln);
//	printf("%d, %lld, %s\n", rres->score, rres->plen, rres->aln);
	printf("%d, %lld, %s\n", res->score, res->plen, res->aln);

	free(a);
	free(b);

//	sea_aln_free(d, dres);
//	sea_aln_free(c, cres);
//	sea_aln_free(r, rres);
	sea_aln_free(c, res);

	sea_close(d);
	sea_close(c);
	return 0;
}
