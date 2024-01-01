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

typedef unsigned int ctrtp;
typedef double datatp;
#define FMT "%f"
#define IFMT "%.0f"

//uchar scale;
datatp scale;

/* Output: A probability distribution at layer N
 * Inputs: infact: Probability factor (freq)
 *         variety: How many different digits do we have already?
 *         opts: How many different digits exist overall
 */
inline void freq_one_step(datatp* dist, const datatp infact, 
			  const ctrtp variety, const ctrtp opts)
{
	const ctrtp same = variety;
	const ctrtp othr = opts-variety;
	dist[variety-1] += infact*same;
	dist[variety  ]  = infact*othr;
}

#ifdef PREFETCH
#define PREFETCH1(addr, rw, loc) __builtin_prefetch(addr, rw, loc)
#define PREFETCH8(addr, rw, loc) \
	__builtin_prefetch(addr, rw, loc); \
	__builtin_prefetch(addr+8, rw, loc); \
	__builtin_prefetch(addr+16, rw, loc); \
	__builtin_prefetch(addr+24, rw, loc); \
	__builtin_prefetch(addr+32, rw, loc); \
	__builtin_prefetch(addr+40, rw, loc); \
	__builtin_prefetch(addr+48, rw, loc); \
	__builtin_prefetch(addr+56, rw, loc)
#else
#define PREFETCH1(addr, rw, loc) do {} while(0)
#define PREFETCH8(addr, rw, loc) do {} while(0)
#endif

#ifndef NO_SPLIT_LOOP
#define SPLIT_LOOP_BREAK break
#else
#define SPLIT_LOOP_BREAK do {} while(0)
#endif

void calcnet(datatp* dist, const ctrtp opts)
{
	PREFETCH1(dist, 1, 3);
	dist[0] = 1;
	ctrtp start = 1;
#ifndef NO_SPLIT_LOOP
	ctrtp lastvar = 1;
#endif
	for (ctrtp step = 1; step < opts; ++step) {
		ctrtp var = start;
		PREFETCH8(dist+start-1, 0, 2);
		double nextinfact = dist[var-1]*scale;
		dist[var-1] = 0;
		if (!(step%1024)) {
			printf("Layer %i \r", step);
			fflush(stdout);
		}
#ifndef NO_SPLIT_LOOP
		for (; var <= step && nextinfact == (datatp)0; ++var) {
			nextinfact = dist[var]*scale;
			/* It could be that we only get to 0 b/c scale mult, so clear */
			dist[var] = 0;
		}
		start = var;
		/* No testing for zero until lastvar */
		for (; var <= lastvar; ++var) {
			const double infact = nextinfact;
			nextinfact = dist[var]*scale;
			//dist[var] = 0;
#if PREFETCH != 0
			//if (!(var%8))
			PREFETCH1(dist+(var+PREFETCH)%step, 0, 2);
#endif
			freq_one_step(dist, infact, var, opts);
		}
#endif
		for (; var <= step; ++var) {
			const double infact = nextinfact;
			nextinfact = dist[var]*scale;
			//dist[var] = 0;
#if PREFETCH != 0
			//if (!(var%8))
			PREFETCH1(dist+(var+PREFETCH)%step, 0, 2);
#endif
#if !defined(TEST_NONZERO) && defined(NO_SPLIT_LOOP)
			freq_one_step(dist, infact, var, opts);
#else
			if (infact != (datatp)0)
				freq_one_step(dist, infact, var, opts);
			else {
				dist[var] = 0;
				SPLIT_LOOP_BREAK;
			}
#endif
		}
#ifndef NO_SPLIT_LOOP
		/* Not needed, we have written a final 0 already if we have not reached the end
		if (var < step)
			dist[var+1] = 0;
		*/
		/* How long should we run branchless b/c we won't hit 0 */
		lastvar = var-1;
		//printf("DEBUG: %i: %i,%i\n", step, start, lastvar);
#endif
	}
}


void usage()
{
	fprintf(stderr, "Usage: chain N\n");
	exit(1);
}

/*
datatp ipow(unsigned char base, unsigned char pwr)
{
	datatp res = 1;
	for (unsigned char i = 0; i < pwr; ++i)
		res *= base;
	return res;
}
*/

int main(int argc, char *argv[])
{
	if (argc < 2)
		usage();
	ctrtp maxln = atoi(argv[1]);
#ifdef USE_VALLOC
	datatp *dist = valloc(maxln * sizeof(datatp));
#ifndef NO_SPLIT_LOOP
	memset(dist, 0, maxln * sizeof(datatp));
#endif
#else
	datatp *dist = calloc(maxln, sizeof(datatp));
#endif
	//printf("%p\n", dist);
	scale = pow(maxln,(maxln-3.0)/(1.0-maxln));
	calcnet(dist, maxln);
	//ulong total = 0;
	datatp total = 0;
	datatp exp = 0;
	for (ctrtp ix = 0; ix < maxln; ++ix) {
		exp += (ix+1)*dist[ix];
		total += dist[ix];
		//printf(FMT " ", dist[(maxln-1)*maxln+ix]);
	}
	printf("\n%f\n", 100.0*exp/total/maxln);
	printf("DEBUG: Opts counted " FMT ", calculated " FMT ", scale = 1/" FMT "\n",
		total, pow((double)maxln*scale,maxln-1), 1.0/scale);
	//assert(fabs(total-ipow(maxln, maxln-1))/total < 0.001);
	assert(fabs(total-pow((double)maxln*scale,maxln-1))/total < 0.001);
	free(dist);
	return 0;
}

