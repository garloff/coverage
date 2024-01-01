/* comb.c
 * Combinatoric solution to the probability problem
 * (c) Kurt Garloff <kurt@garloff.de>, 12/2023
 * SPDX-License-Idnetifier: GPL-v2-or-later
 */

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* We translate ...
 * 000000 <- 1
 * 000002 <- 2
 * 000003 <- 2
 * 000020 <- 2
 * 000022 <- 2
 * 000023 <- 3
 * So we have a number system modulo N
 * We produce a number with N digits (the first one always being 0)
 * And we count how many different digits are in there.
 * Se we produce all numbers (N-1)^N and count the number of different digits
 */

#ifdef BITFIELD
#define SZLONG (sizeof(unsigned long))
#define LONGBITS (8*sizeof(unsigned long))

inline void set_bit(unsigned long* fld, unsigned char off)
{
	unsigned char arroff = off/LONGBITS;
	unsigned char bit = off%LONGBITS;
	//printf("%i,%i,%08lx,%08lx ", arroff, bit, fld[arroff], fld[arroff] | (1<<bit));
	fld[arroff] |= (1ULL<<bit);
}

inline unsigned long get_bit(unsigned long* fld, unsigned char off)
{
	unsigned char arroff = off/LONGBITS;
	unsigned char bit = off%LONGBITS;
	return fld[arroff] & (1ULL<<bit);
}

inline unsigned long get_set_bit(unsigned long* fld, unsigned char off)
{
	unsigned char arroff = off/LONGBITS;
	unsigned char bit = off%LONGBITS;
	unsigned long mask = 1ULL<<bit;
	unsigned long val = fld[arroff] & mask;
	fld[arroff] |= mask;
	return val;
}
#define clear_bits(arr, ln) memset(arr, 0, (ln+LONGBITS-1)/SZLONG)
#define alloc_bits(ln) calloc((ln+LONGBITS-1)/LONGBITS, SZLONG)
#define BITTYPE unsigned long*
#else
#define set_bit(arr, off) do { arr[off] |= 1; } while (0)
#define get_bit(arr, off) (arr[off])
inline unsigned char get_set_bit(unsigned char *arr, unsigned char off)
{
	unsigned char tmp = arr[off];
	arr[off] |= 1;
	return tmp;
}
#define clear_bits(arr, ln) memset(arr, 0, ln)
#define alloc_bits(ln) calloc(ln, 1)
#define BITTYPE unsigned char*
#endif

unsigned char count(BITTYPE bits, unsigned char* digits, unsigned char ln)
{
	unsigned char cnt = ln;	/* Assume all digits are different */
	clear_bits(bits, ln);
	set_bit(bits, ln-1);
	/* Last digit is fixed to be zero */
	for (unsigned char i = 0; i < ln-1; ++i)
		if (get_set_bit(bits, digits[i]))
			--cnt;
	return cnt;
}

/* little endian */
inline void inc(unsigned char *digits, unsigned char ln)
{
	for (unsigned char i = 0; i < ln; ++i) {
		if (digits[i] == ln-1)
			digits[i] = 0;
		else {
			digits[i] += 1;
			break;
		}
	}
}

void usage()
{
	fprintf(stderr, "Usage: comb N\n");
	exit(1);
}

unsigned long ipow(unsigned char base, unsigned char pwr)
{
	unsigned long res = 1;
	for (unsigned char i = 0; i < pwr; ++i)
		res *= base;
	return res;
}

void dbgout(unsigned char *digits, unsigned char ln, unsigned char cnt)
{
	printf("\r");
	for (unsigned char i = 0; i < ln; ++i)
		printf("%i ", digits[i]);
	printf("=> %i ", cnt);
	fflush(stdout);
}

int main(int argc, char *argv[])
{
	if (argc < 2)
		usage();
	unsigned int maxln = atoi(argv[1]);
	if (maxln > 10)
		printf("This may take a long time ...\n");
	unsigned char *digits = malloc(sizeof(char)*maxln);
	memset(digits, 0, sizeof(char)*maxln);
	BITTYPE cov = alloc_bits(maxln);
	unsigned long exp = 0;
	for (unsigned long i = 0; i < ipow(maxln, maxln-1); ++i) {
		unsigned char cnt = count(cov, digits, maxln);
		exp += cnt;
		//dbgout(digits, maxln, cnt);
		inc(digits, maxln);
	}
	printf("%ld, expect ~ %ld\n", exp, (maxln*ipow(maxln, maxln-1)*13+10)/20);
	//dbgout(digits, maxln, 0);
	printf("\n%f\n",(100.0*exp)/(maxln*ipow(maxln, maxln-1)));
	free(cov);
	free(digits);
	return 0;
}
