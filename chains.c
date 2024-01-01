/* chains.c
 *
 * Calculate probabilities/frequencies by waling all chains recursively
 *
 * (c) Kurt Garloff <kurt@garloff.de>, 12/2023
 * SPDX-License-Identifier: GPL-v2-or-later
 */

/* Idea: We draw a new digit in each step, counting how
 * many options we have to hit an existing one and how
 * many we have to find a new one.
 * We walk the decision trees recursively, counting the possibilities.
 * On each step we have only two options:
 * - Draw an existing digit
 * - Draw a new one
 */


#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef unsigned char uchar;
typedef double datatp;
#define FMT "%f"
#define IFMT "%.0f"

/* Output: A frequency distribution in dist
 * Inputs: infact: Probability factor (freq)
 *         variety: How many different digits do we have already?
 *         step: Which step of the process are we in (recursion depth)
 *         opts: How many different digits exist overall
 */
void freq_one_step(datatp* dist, datatp infact, uchar variety, uchar step, uchar opts)
{
	/* Our options to hit the same digit again and to hit another */
	//assert(infact < (1ULL<<61));
	uchar same = variety;
	uchar othr = opts-variety;
	if (step < opts-1) {
		freq_one_step(dist, infact*same, variety  , step+1, opts);
		freq_one_step(dist, infact*othr, variety+1, step+1, opts); 
	} else {
		dist[variety-1] += infact*same;
		dist[variety  ] += infact*othr;
	}
}

void usage()
{
	fprintf(stderr, "Usage: chain N\n");
	exit(1);
}

datatp ipow(unsigned char base, unsigned char pwr)
{
	datatp res = 1;
	for (unsigned char i = 0; i < pwr; ++i)
		res *= base;
	return res;
}


int main(int argc, char *argv[])
{
	if (argc < 2)
		usage();
	uchar maxln = atoi(argv[1]);
	datatp *dist = calloc(maxln, sizeof(datatp));
	freq_one_step(dist, 1, 1, 1, maxln);
	//ulong total = 0;
	datatp total = 0;
	datatp exp = 0;
	for (uchar ix = 0; ix < maxln; ++ix) {
		exp += (ix+1)*dist[ix];
		total += dist[ix];
		printf(IFMT " ", dist[ix]);
	}
	printf("\n%f\n", 100.0*exp/total/maxln);
	//printf("DEBUG: Opts counted " IFMT ", calculated " IFMT "\n", total, ipow(maxln, maxln-1));
	assert(fabs(total-ipow(maxln, maxln-1))/total < 0.001);
	free(dist);
	return 0;
}

