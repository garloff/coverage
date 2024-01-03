# coverage collection

This collection of programs represent a variety of approaches I used
to investigate a mathematical problem. On my way, I found a few more
interesting observations.

## The math problem

I was wondering about how much I could improve upon cryptographic
attacks (non-brute-force!) by stretching a key. Say that I have e.g.
a `N`=128bit key and want to do another round of encryption using a derived
`N`=128bit key. Derivation could e.g. be done by hashing the primary key.
A good hash function would result in a distribution of secondary keys
indistinguishable from random numbers. The question I was wondering:
How many bits do I really gain? Deriving a secondary key from all
possible primary keys will likely not exhaust all possible seondary
values, as there will be different primary keys that results in the
same secondary key ("hash collisions"). Assuming a perfect random
number generator: How many different values `S` do I see when I generate
`2^N` random values? How large is the coverage percentage `P` = `S/2^N`?

## The answer

I did not find a nice analytic formula for this.

But I designed several alogrithms to calculate the value.

The answer seems to be `P = 63.21%` for large `N`.
(Convergence is good, you get good approximation with `N ~ 14`,
 the percentages are slightly higher smaller `N`.)

## The algorithms

### `dist`

This program uses libc's pseudo random number generator (alternatively
frandom with option `-f`) to generate `2^N` random numbers in the range
`0 .. 2^N-1`. I creates a bitfield to track which numbers have already
been seen.

###  `comb`

Calculate all options and count.
This needs to generate `2^N * 2^N` numbers, only works for small `N`
(~10).

### `chains`

Recursively walk the chain: In step `s`, we draw a random number `R` and
calculate the combinatorial probability that it hits an already existing
number (keeping the coverage the same) or a new one (increasing the coverage
by one) and do this `2^N` times. Do this for all steps ...

Works only well for small `N` (~10).

### `chains2`

Same idea as `chains`, but do it stepwise and highly optimized: Calculate
probabilities for all coverage possibilities `1..s` in step `s` and store
the result. Do some scaling every step to avoid over/underflow with double
precision floats.
Then go for step `s+1`.

Optimized to use as little memory as possible, so we overwrite the results
from step `s` in step `s+1`.
This gives us precise results and works well until `N ~ 20`.

The result for `N = 22`: `63.21060%`.

An interesting observation:
* intel Haswell up to and including TigerLake is really slow on this.
  A factor 10 ... 100 slower than Zen 2, 3, 4. (I have not been able
  to test Zen 1 nore intel AlderLake or MeteorLake.) ARM Cortex-A76
  (Orange Pi 5) and A715 are fast.
  Something is strange here on intel. Problems with cache handling?
  Confused hw prefetcher? Slowliness on FP multiplication or FMA?
* With gcc-12, Zen is as 5x slower than with gcc-7.5 (SUSE), gcc-11,
  gcc-13 and master (pre-14). Analyzing the assembly, we see that
  gcc-13 writes back the two results (in `freq_one_step()`) with
  two individual `movsd` instructions. gcc-12 uses `unpcklpd`
  to gather both values in one xmm register and writes both back
  with a single `movups` instruction. This seems to be much slower,
  maybe because we create a depencency between the two calculations
  or maybe because the unaligned write instruction is slow or both.
* llvm (clang) 15 and 17 produce a bit faster code than gcc when
  optimizing for the (znver3/4) CPU (-march=native).
  Both clang and gcc produce FMA (`vfmadd231sd`) instructions, which
  seem to help. gcc-13 still uses the `unpacklpd, movups` insns to
  write back the results in some places. Hand editing the assembly
  closes half of the gap to clang (see chains2.S.diff).
  The remaining difference seems to be that clang does some more
  unrolling that allows it to process 4 values at a time at some
  places (using AVX2 ymm 256bit registers).
* You can play with `-DPREFETCH=0` or even `-D PREFETCH=64`. The latter
  will hurt performance on Zen, whereas the first seem to help a tiny
  bit.
* To switch off an optimization to skip calculating values that will be
  zero anyway, you can pass `-DNO_SPLIT_LOOP -DTEST_NONZERO` or just
  `-DNO_SPLIT_LOOP` and benchmark the results on your CPU. The program
  will run 2--3x slower.

### `chains3`

Further(!) optimized version from chains.
Rather than touching two values per iteration, do the complete
math for one. This saves a write to a write buffer.
It also produces cleaner code, making the compiler's job easier,
e.g. when it tries to unroll the loop to extract further parallelism.
chains3 is indeed faster than chains2. The `_p0` variant with
preteching seems slower this time than the one without.

Compiling with `-DFORCE_EVEN` forces start and end offsets of
the split loops to be even, thus allowing for better unrolling
or proving to the compiler that we can use aligned insns.

gcc-12 is slower than gcc-13, but this time not by as much and it's
not the unfortunate `unpcklpd` with `movups` this time.
clang-17 is beating gcc-13 by a larger margin, not yet analyzed.

