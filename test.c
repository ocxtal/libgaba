
#include <stdio.h>
#include <string.h>
#include "include/sea.h"

int main(void)
{
	sea_ctx_t *ctx;
	sea_res_t *res;
	char const *a = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
	char const *b = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";

	ctx = sea_init(
		SEA_DYNAMIC | SEA_LINEAR_GAP_COST | SEA_XSEA,
		2, -3, -5, -1, 10, 10, 10);

	printf("%d\n", ctx->alg);

	res = sea_align(ctx,
		a, 0, strlen(a),
		b, 0, strlen(b));

	sea_clean(ctx);
	return 0;
}
