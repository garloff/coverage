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
#define EFMT "%e"
#define IFMT "%.0f"

//uchar scale;
datatp scale;


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
#define LASTVAR lastvar
#else
#define SPLIT_LOOP_BREAK do {} while(0)
#define LASTVAR step
#endif

#ifdef FORCE_EVEN
#define STARTOFFS (var>1? var%2: 0)
#define ENDOFFS (var<step? var%2: 0)
#else
#define STARTOFFS 0
#define ENDOFFS 0
#endif

 /* Note: SAVE_SCALE results in more cutoff as the handling of underflows
  * seems to be inconsistent. We can deal with this by testing
  * for subnormals and always cut off once we are in the subnormals
  * range ... then SAVE_SCALE does keep smaller numbers ...
  */
//#define ISZERO(v) v == (datatp)0
//#define ISZERO(v) v < 2e-307
#define ISZERO(v) (fpclassify(v) == FP_ZERO || fpclassify(v) == FP_SUBNORMAL)

/* Output: A probability distribution dist
 * 	   (normalized to (maxln*scale)^(maxln-1)
 * Inputs: Number of options opts
 *
 * We draw opts random numbers in the range 0 ... (opts-1)
 * and caculate the probability distribution of observing
 * N different numbers in dist[N-1].
 */
ctrtp calcnet(datatp* dist, const ctrtp opts)
{
	PREFETCH1(dist, 1, 3);
	dist[0] = 1;
	ctrtp start = 1;
#ifndef NO_SPLIT_LOOP
	ctrtp lastvar = 1;
#endif
#ifdef SAVE_SCALE
	/* Only do scaling every 8th iteration to save mult ops */
	const datatp scale8 = pow(scale, 8.0);
#define MULTSCALE(val) (((step)%8)? val: val*scale8)
#else
#define MULTSCALE(val) (val*scale)
#endif
	for (ctrtp step = 1; step < opts; ++step) {
		ctrtp var = start;
		PREFETCH8(dist+start%2, 0, 2);
		datatp nextinfact = dist[var-1];
		if (!(step%1024)) {
			printf("Layer %i (%i .. %i) \r", step, start, LASTVAR);
			fflush(stdout);
		}
#ifndef NO_SPLIT_LOOP
		for (; var <= step && ISZERO(nextinfact); ++var) {
			nextinfact = dist[var];
			dist[var] = 0;
		}
		dist[var-1] = MULTSCALE(nextinfact*var);
		start = var - STARTOFFS;
		/* No testing for zero until lastvar */
		for (; var <= lastvar; ++var) {
			const datatp infact = nextinfact;
			nextinfact = dist[var];
			//dist[var] = 0;
#if PREFETCH != 0
			//if (!(var%8))
			PREFETCH1(dist+var+PREFETCH, 0, 2);
#endif
			dist[var] = MULTSCALE((infact*(opts-var) + nextinfact*(var+1)));
		}
#else
		dist[var-1] = MULTSCALE(nextinfact*var);
#endif
		for (; var <= step; ++var) {
			const datatp infact = nextinfact;
			nextinfact = dist[var];
			//dist[var] = 0;
#if PREFETCH != 0
			//if (!(var%8))
			PREFETCH1(dist+(var+PREFETCH)%step, 0, 2);
#endif
#if !defined(TEST_NONZERO) && defined(NO_SPLIT_LOOP)
			dist[var] = MULTSCALE((infact*(opts-var) + nextinfact*(var+1)));
#else
			if (!(ISZERO(infact)))
				dist[var] = MULTSCALE((infact*(opts-var) + nextinfact*(var+1)));
			else {
				dist[var] = 0;
				SPLIT_LOOP_BREAK;
			}
#endif
		}
#ifndef NO_SPLIT_LOOP
		/* How long should we run branchless b/c we won't hit 0 */
		lastvar = var-1 + ENDOFFS;
		//printf("DEBUG: %i: %i,%i\n", step, start, lastvar);
#endif
	}
	return start;
}


void usage()
{
	fprintf(stderr, "Usage: [-v] chains3 N\n");
	exit(1);
}

int main(int argc, char *argv[])
{
	int verbose = 0;
	if (argc < 2)
		usage();
	if (!strcmp(argv[1], "-v")) {
		++verbose;
		--argc;
		++argv;
	}
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
	ctrtp first = calcnet(dist, maxln);
	//ulong total = 0;
	datatp total = 0;
	datatp exp = 0;
#ifdef SAVE_SCALE
	const datatp norm = pow((double)maxln*scale, (maxln-1)-(maxln-1)%8) * \
			    pow((double)maxln, (maxln-1)%8);
#else
	const datatp norm = pow((double)maxln*scale, maxln-1);
#endif
	if (verbose)
		printf("(%i): ", first);
	for (ctrtp ix = first-1; ix < maxln; ++ix) {
		exp += (ix+1)*dist[ix];
		total += dist[ix];
		if (verbose)
			printf(EFMT " ", dist[ix]*(1.0/norm));
		if (first > 1 && ix > first+10 && dist[ix] == (datatp)0) {
			if (verbose)
				printf(":(%i) ", ix);
			break;
		}
	}
	printf("\n%f%%\n", 100.0*exp/total/maxln);
	if (verbose) {
		printf("DEBUG: Opts counted " FMT ", calculated " FMT ", scale = 1/" FMT "\n",
			total, norm, 1.0/scale);
		printf("DEBUG: prob(%i) = %e\n", maxln*16/25, dist[maxln*16/25-1]/norm);
	}
	assert(fabs(total-norm)/total < 0.001);
	free(dist);
	return 0;
}

