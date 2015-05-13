
#include <stdio.h>
#include <string.h>
#include "include/sea.h"

int main(void)
{
	sea_ctx_t *ctx;
	sea_res_t *res;
	char const *a = "AAAAAAAAAAAAATTAAAAAAAAAAAAAAAAAAAAAAAAAAA";
	char const *b = "AAAAAAAAAAAAAAAAAAAAAAAATTAAAAAAAAAAAAAAAA";

	ctx = sea_init(
		SEA_DYNAMIC | SEA_LINEAR_GAP_COST | SEA_XSEA,
		2, -3, -5, -1, 30, 30, 30);

	printf("%d\n", ctx->alg);

	res = sea_align(ctx,
		a, 0, strlen(a),
		b, 0, strlen(b));

	printf("%d, %lld, %s\n", res->score, res->len, res->aln);

	sea_aln_free(res);

	sea_clean(ctx);
	return 0;
}
