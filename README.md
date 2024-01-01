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
* Zen3, Zen4 seem to have some optimization to speed this up very
  significantly. Maybe better cache handling or better optimized
  floating point multiplications. It beats TigerLake by a factor of 5.
  (I have not tested Skylake or AlderLake nor Zen < 3 nor ARM.)
* With gcc-12, Zen is as slow as Tigerlake. gcc-7.5 (SUSE), gcc-11,
  gcc-13 and master (pre-14) don't exhibit the slowliness. The reason
  is not obvious from a quick look at the disassembly.
* llvm (clang) 15 and 17 produce a bit faster code than gcc when
  optimizing for the (znver3/4) CPU (-march=native).
* You can play with `-DPREFETCH=0` or even `-D PREFETCH=64`. The latter
  will hurt performance on Zen, whereas the first seem to help a tiny
  bit.
* To switch off an optimization to skip calculating values that will be
  zero anyway, you can pass `-DNO_SPLIT_LOOP -DTEST_NONZERO` or just
  `-DNO_SPLIT_LOOP` and benchmark the results on your CPU. The program
  will run 2--3x slower.

