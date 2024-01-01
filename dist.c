/* dist.c
 * Draw N integer random numbers < N
 * and measure how much coverage of the space till N we have
 *
 * (c) Kurt Garloff <kurt@garloff.de>, 12/2023
 * SPDX-License-Identifier: GPL-v2-or-later
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "frandom.h"
#include <assert.h>

typedef unsigned char u8;

void usage()
{
	fprintf(stderr, "Usage: dist [-f] [-r rep] MAXNO\n");
	fprintf(stderr, "MAXNO must be smaller than or equal to %i\n", RAND_MAX);
	exit(1);
}


#define SZLONG (sizeof(unsigned long))
#define LONGBITS (8*sizeof(unsigned long))

void set_bit(unsigned long* fld, unsigned int off)
{
	unsigned int arroff = off/LONGBITS;
	unsigned int bit = off%LONGBITS;
	//printf("%i,%i,%08lx,%08lx ", arroff, bit, fld[arroff], fld[arroff] | (1<<bit));
	fld[arroff] |= (1ULL<<bit);
}

unsigned long count_bits(unsigned long *fld, unsigned long sz)
{
	unsigned long bits = 0;
	for (unsigned int i = 0; i < (sz+LONGBITS-1)/LONGBITS; ++i)
		for (unsigned j = 0; j < LONGBITS; ++j)
			if (fld[i] & (1ULL<<j))
				++bits;
	return bits;
}

int highest_bit(unsigned long val)
{
	for (int i = 63; i >= 0; --i)
		if (val & (1ULL<<i))
			return i;
	return 0;
}


int main(int argc, char *argv[])
{
	if (argc < 2)
		usage();
	u8 rndbuf[8]; memset(rndbuf, 0, 8);
	int rep = 1;
	void *rndstate = NULL;
	if (!strcmp(argv[1], "-f")) {
		argv++; argc--;
		rndstate = frandom_init_lrand(time(0));
	} else
		srand(time(0));
	if (argc < 2)
		usage();
	if (!strcmp(argv[1], "-r")) {
		rep = atol(argv[2]);
		argv++; argv++; argc--; argc--;
	}
	if (argc < 2)
		usage();
	//unsigned long maxno = 1ULL << atoi(argv[1]);
	unsigned long maxno = atol(argv[1]);
	if (!rndstate && maxno > RAND_MAX)
		usage();
	unsigned long *cov = calloc((maxno+LONGBITS-1)/LONGBITS, SZLONG);
	double cum = 0.0;
	for (int i = 0; i < rep; ++i) {
		if (rep == 1) {
			printf("Generating %srandom numbers: ", (rndstate? "f": ""));
			fflush(stdout);
		}
		const int rndln = (highest_bit(maxno)+7)/8;
		for (unsigned int i = 0; i < maxno; ++i) {
			unsigned int rnd;
			if (rndstate) {
				frandom_bytes(rndstate, rndbuf, rndln);
				rnd = *(unsigned long*)rndbuf % maxno;
			} else
				rnd = rand() % maxno;
			assert(rnd < maxno);
			set_bit(cov, rnd);
			if (rep == 1 && maxno > 16 && !(i%(maxno/16))) {
				printf(".");
				fflush(stdout);
			}
		}
		if (rep == 1) {
			printf("\nCounting: ...");
			fflush(stdout);
		}
		unsigned long res = count_bits(cov, maxno);
		if (rep == 1) {
			printf("\nCalculated %li random numbers with %li unique results\n",
				maxno, res);
			printf("Coverage: %.2f%%\n", 100.0*res/maxno);
		} else
			printf("Coverage: %.2f%%\r", 100.0*res/maxno);
		cum += (double)res/maxno;
		memset(cov, 0, SZLONG*((maxno+LONGBITS-1)/LONGBITS));
	}
	printf("\nAverage: %f%%\n", 100.0*cum/rep);
	if (rndstate)
		frandom_release(rndstate);
	free(cov);
}

