/* Copyright (c) (2017-2021) Apple Inc. All rights reserved.
 *
 * corecrypto is licensed under Apple Inc.’s Internal Use License Agreement (which
 * is contained in the License.txt file distributed with corecrypto) and only to
 * people who accept that license. IMPORTANT:  Any license rights granted to you by
 * Apple Inc. (if any) are limited to internal use within your organization only on
 * devices and computers you own or control, for the sole purpose of verifying the
 * security characteristics and correct functioning of the Apple Software.  You may
 * not, directly or indirectly, redistribute the Apple Software or any portions thereof.
 */

#ifndef _CORECRYPTO_CCN_INTERNAL_H
#define _CORECRYPTO_CCN_INTERNAL_H

#include <corecrypto/ccn.h>
#include "cc_workspaces.h"
#include "cc_memory.h"

#if CCN_UNIT_SIZE == 8
#define cc_clz_nonzero cc_clz64
#define cc_ctz_nonzero cc_ctz64
#define CC_STORE_UNIT_BE(x, out) cc_store64_be(x, out)
#define CC_LOAD_UNIT_BE(x, out) (x = cc_load64_be(out))
#elif CCN_UNIT_SIZE == 4
#define cc_clz_nonzero cc_clz32
#define cc_ctz_nonzero cc_ctz32
#define CC_STORE_UNIT_BE(x, out) cc_store32_be(x, out)
#define CC_LOAD_UNIT_BE(x, out) (x = cc_load32_be(out))
#else
#error unsupported CCN_UNIT_SIZE
#endif

CC_NONNULL((6, 8))
int ccn_div_euclid(cc_size nq, cc_unit *cc_counted_by(nq) q,
                   cc_size nr, cc_unit *cc_counted_by(nr) r,
                   cc_size na, const cc_unit *cc_counted_by(na) a,
                   cc_size nd, const cc_unit *cc_counted_by(nd) d);

// Same as ccn_div_euclid(), takes a ws.
int ccn_div_euclid_ws(cc_ws_t ws, cc_size nq, cc_unit *cc_counted_by(nq) q, cc_size nr, cc_unit *cc_counted_by(nr) r,
                      cc_size na, const cc_unit *cc_counted_by(na) a, cc_size nd, const cc_unit *cc_counted_by(nd) d);

#define ccn_div(nq, q, na, a, nd, d) ccn_div_euclid(nq, q, 0, NULL, na, a, nd, d)
#define ccn_mod(nr, r, na, a, nd, d) ccn_div_euclid(0 , NULL, nr, r, na, a, nd, d)

/* |s - t| -> r return 1 iff t > s, 0 otherwise */
cc_unit ccn_abs_ws(cc_ws_t ws, cc_size n, cc_unit *cc_counted_by(n) r, const cc_unit *cc_counted_by(n) s, const cc_unit *cc_counted_by(n) t);

/* Returns the number of bits which are zero before the first one bit
   counting from least to most significant bit. */
CC_NONNULL((2))
size_t ccn_trailing_zeros(cc_size n, const cc_unit *s);

/*! @function ccn_shift_right_multi
 @abstract Constant-time, SPA-safe, right shift.

 @param n Length of r and s as number of cc_units.
 @param r Destination, can overlap with s.
 @param s Input that's shifted by k bits.
 @param k Number of bits by which to shift s to the right.
 */
CC_NONNULL_ALL
void ccn_shift_right_multi(cc_size n, cc_unit *cc_counted_by(n) r, const cc_unit *cc_counted_by(n) s, size_t k);

/* s << k -> r return bits shifted out of most significant word in bits [0, n>
 { N bit, scalar -> N bit } N = n * sizeof(cc_unit) * 8
 the _multi version doesn't return the shifted bits, but does support multiple
 word shifts */
CC_NONNULL_ALL
void ccn_shift_left(cc_size n, cc_unit *cc_counted_by(n) r, const cc_unit *cc_counted_by(n) s, size_t k) __asm__("_ccn_shift_left");

CC_NONNULL_ALL
void ccn_shift_left_multi(cc_size n, cc_unit *cc_counted_by(n) r, const cc_unit *cc_counted_by(n) s, size_t k);

// Conditionally swap the content of r0 and r1 buffers in constant time
// r0:r1 <- r1*k1 + s0*(k1-1)
CC_NONNULL_ALL
void ccn_cond_swap(cc_size n, cc_unit ki, cc_unit *cc_counted_by(n) r0, cc_unit *cc_counted_by(n) r1);

/*! @function ccn_cond_shift_right
 @abstract Constant-time, SPA-safe, conditional right shift.

 @param n Length of each of r and a as number of cc_units.
 @param s Selector bit (0 or 1).
 @param r Destination, can overlap with a.
 @param a Input that's shifted by k bits, if s=1.
 @param k Number of bits by which to shift a to the right, if s=1.
          (k must not be larger than CCN_UNIT_BITS.)
 */
CC_NONNULL_ALL
void ccn_cond_shift_right(cc_size n, cc_unit s, cc_unit *cc_counted_by(n) r, const cc_unit *cc_counted_by(n) a, size_t k);

/*! @function ccn_cond_neg
 @abstract Constant-time, SPA-safe, conditional negation.

 @param n Length of each of r and x as number of cc_units.
 @param s Selector bit (0 or 1).
 @param r Destination, can overlap with x.
 @param x Input that's negated, if s=1.
 */
void ccn_cond_neg(cc_size n, cc_unit s, cc_unit *cc_counted_by(n) r, const cc_unit *cc_counted_by(n) x);

/*! @function ccn_cond_shift_right_carry
 @abstract Constant-time, SPA-safe, conditional right shift.

 @param n Length of each of r and a as number of cc_units.
 @param s Selector bit (0 or 1).
 @param r Destination, can overlap with a.
 @param a Input that's shifted by k bits, if s=1.
 @param k Number of bits by which to shift a to the right, if s=1.
          (k must not be larger than CCN_UNIT_BITS.)
 @param c Carry bit(s), the most significant bit(s) after shifting, if s=1.
 */
CC_NONNULL_ALL
void ccn_cond_shift_right_carry(cc_size n, cc_unit s, cc_unit *cc_counted_by(n) r, const cc_unit *cc_counted_by(n) a, size_t k, cc_unit c);

/*! @function ccn_cond_add
 @abstract Constant-time, SPA-safe, conditional addition.

 @param n Length of each of r, x, and y as number of cc_units.
 @param s Selector bit (0 or 1).
 @param r Destination, can overlap with x or y.
 @param x First addend.
 @param y Second addend.

 @return The carry bit, if s=1. 0 otherwise.
 */
CC_NONNULL_ALL
cc_unit ccn_cond_add(cc_size n, cc_unit s, cc_unit *cc_counted_by(n) r, const cc_unit *cc_counted_by(n) x, const cc_unit *cc_counted_by(n) y);

/*! @function ccn_mux
 @abstract Constant-time, SPA-safe multiplexer. Sets r = (s ? a : b).

 @discussion This works like a normal multiplexer (s & a) | (~s & b) but is
             slightly more complicated and expensive. Out of `s` we build
             half-word masks to hide extreme Hamming weights of operands.

 @param n Length of each of r, a, and b as number of cc_units.
 @param s Selector bit (0 or 1).
 @param r Destination, can overlap with a or b.
 @param a Input selected when s=1.
 @param b Input selected when s=0.
 */
CC_NONNULL_ALL
void ccn_mux(cc_size n, cc_unit s, cc_unit *cc_counted_by(n) r, const cc_unit *cc_counted_by(n) a, const cc_unit *cc_counted_by(n) b);

/*!
 @brief ccn_div_use_recip(nq, q, nr, r, na, a, nd, d) computes q=a/d and r=a%d
 @discussion q and r can be NULL. Reads na from a and nd from d. Writes nq in q and nr in r. nq and
 nr must be large enough to accomodate results, otherwise error is returned. Execution time depends
 on the size of a. Computation is perfomed on of fixedsize and the leading zeros of a of q are are
 also used in the computation.
 @param nq length of array q that hold the quotients. The maximum length of quotient is the actual
 length of dividend a
 @param q  returned quotient. If nq is larger than needed, it is filled with leading zeros. If it is
 smaller, error is returned. q can be set to NULL, if not needed.
 @param nr length of array r that hold the remainder. The maximum length of remainder is the actual
 length of divisor d
 @param r  returned remainder. If nr is larger than needed, it is filled with leading zeros. If nr is
 smaller, an error is returned. r can be set to NULL if not required.
 @param na length of dividend. Dividend may have leading zeros.
 @param a  input Dividend
 @param nd length of input divisor. Divisor may have leading zeros.
 @param d  input Divisor
 @param recip_d The reciprocal of d, of length nd+1.

 @return  returns 0 if successful, negative of error.
 */
CC_NONNULL((7, 9, 10))
int ccn_div_use_recip_ws(cc_ws_t ws,
                         cc_size nq,
                         cc_unit *cc_counted_by(nq) q,
                         cc_size nr,
                         cc_unit *cc_counted_by(nr) r,
                         cc_size na,
                         const cc_unit *cc_counted_by(na) a,
                         cc_size nd,
                         const cc_unit *cc_counted_by(nd) d,
                         const cc_unit *cc_unsafe_indexable recip_d);

/*! @function ccn_gcd_ws
 @abstract Computes the greatest common divisor of s and t,
           r = gcd(s,t) / 2^k, and returns k.

 @param ws Workspace.
 @param rn Length of r as a number of cc_units.
 @param r  Resulting GCD.
 @param sn Length of s as a number of cc_units.
 @param s  First number s.
 @param tn Length of t as a number of cc_units.
 @param t  First number t.

 @return The factor of two to shift r by to compute the actual GCD.
 */
CC_NONNULL_ALL
size_t ccn_gcd_ws(cc_ws_t ws, cc_size rn, cc_unit *cc_counted_by(rn) r, cc_size sn, const cc_unit *cc_counted_by(sn) s, cc_size tn, const cc_unit *cc_counted_by(tn) t);

/*! @function ccn_lcm_ws
 @abstract Computes lcm(s,t), the least common multiple of s and t.

 @param ws  Workspace.
 @param n   Length of s,t as a number of cc_units.
 @param r2n Resulting LCM of length 2*n.
 @param s   First number s.
 @param t   First number t.
 */
void ccn_lcm_ws(cc_ws_t ws, cc_size n, cc_unit *cc_unsafe_indexable r2n, const cc_unit *cc_counted_by(n) s, const cc_unit *cc_counted_by(n) t);

/* s * t -> r_2n                   r_2n must not overlap with s nor t
 { n bit, n bit -> 2 * n bit } n = count * sizeof(cc_unit) * 8
 { N bit, N bit -> 2N bit } N = ccn_bitsof(n) */
CC_NONNULL((2, 3, 4))
void ccn_mul(cc_size n, cc_unit *cc_unsafe_indexable r_2n, const cc_unit *cc_counted_by(n) s, const cc_unit *cc_counted_by(n) t) __asm__("_ccn_mul");

/* s[0..n) * v -> r[0..n)+return value
 { N bit, sizeof(cc_unit) * 8 bit -> N + sizeof(cc_unit) * 8 bit } N = n * sizeof(cc_unit) * 8 */
CC_NONNULL((2, 3))
cc_unit ccn_mul1(cc_size n, cc_unit *cc_counted_by(n) r, const cc_unit *cc_counted_by(n) s, const cc_unit v);

/* s[0..n) * v + r[0..n) -> r[0..n)+return value
 { N bit, sizeof(cc_unit) * 8 bit -> N + sizeof(cc_unit) * 8 bit } N = n * sizeof(cc_unit) * 8 */
CC_NONNULL((2, 3))
cc_unit ccn_addmul1(cc_size n, cc_unit *cc_counted_by(n) r, const cc_unit *cc_counted_by(n) s, const cc_unit v);

/* s * t -> r_2n                   r_2n must not overlap with s nor t
 { n bit, n bit -> 2 * n bit } n = count * sizeof(cc_unit) * 8
 { N bit, N bit -> 2N bit } N = ccn_bitsof(n)
 Provide a workspace for potential speedup */
CC_NONNULL_ALL
void ccn_mul_ws(cc_ws_t ws, cc_size count, cc_unit *cc_unsafe_indexable r, const cc_unit *cc_counted_by(count) s, const cc_unit *cc_counted_by(count) t);

/*!
 @brief ccn_make_recip(cc_size nd, cc_unit *recip, const cc_unit *d)
    computes the reciprocal of d: recip = 2^2b/d where b=bitlen(d)
    if d = 0, recip is set to 0.

 @param nd      length of array d
 @param recip   returned reciprocal of size nd+1
 @param d       input number d
*/
CC_NONNULL_ALL
void ccn_make_recip_ws(cc_ws_t ws, cc_size nd, cc_unit *cc_unsafe_indexable recip, const cc_unit *cc_counted_by(nd) d);

/* s^2 -> r
 { n bit -> 2 * n bit } */
CC_NONNULL_ALL
void ccn_sqr_ws(cc_ws_t ws, cc_size n, cc_unit *cc_unsafe_indexable r, const cc_unit *cc_counted_by(n) s);

/*! @function ccn_div_ws
 @abstract Computes q = a / d.

 @discussion Use CCN_DIV_EUCLID_WORKSPACE_N(na, nd) for the workspace.

 @param ws  Workspace
 @param nq  Length of q as a number of cc_units.
 @param q   The resulting quotient.
 @param na  Length of a as a number of cc_units.
 @param a   The dividend a.
 @param nd  Length of d as a number of cc_units.
 @param d   The divisor d.

 @return 0 on success, non-zero on failure. See cc_error.h for more details.
 */
#define ccn_div_ws(ws, nq, q, na, a, nd, d) ccn_div_euclid_ws(ws, nq, q, 0, NULL, na, a, nd, d)

/*! @function ccn_mod_ws
 @abstract Computes r = a % d.

 @discussion Use CCN_DIV_EUCLID_WORKSPACE_N(na, nd) for the workspace.

 @param ws  Workspace
 @param nr  Length of r as a number of cc_units.
 @param r   The resulting remainder.
 @param na  Length of a as a number of cc_units.
 @param a   The dividend a.
 @param nd  Length of d as a number of cc_units.
 @param d   The divisor d.

 @return 0 on success, non-zero on failure. See cc_error.h for more details.
 */
#define ccn_mod_ws(ws, nr, r, na, a, nd, d) ccn_div_euclid_ws(ws, 0, NULL, nr, r, na, a, nd, d)

/*! @function ccn_neg
 @abstract Computes the two's complement of x.

 @param n  Length of r and x
 @param r  Result of the negation
 @param x  Number to negate
 */
CC_NONNULL_ALL
void ccn_neg(cc_size n, cc_unit *cc_counted_by(n) r, const cc_unit *cc_counted_by(n) x);

/*! @function ccn_invert
 @abstract Computes x^-1 (mod 2^w).

 @param x  Number to invert

 @return x^-1 (mod 2^w)
 */
CC_CONST CC_NONNULL_ALL
CC_INLINE cc_unit ccn_invert(cc_unit x)
{
    cc_assert(x & 1);

    // Initial precision is 5 bits.
    cc_unit y = (3 * x) ^ 2;

    // Newton-Raphson iterations.
    // Precision doubles with every step.
    y *= 2 - y * x;
    y *= 2 - y * x;
    y *= 2 - y * x;
#if CCN_UNIT_SIZE > 4
    y *= 2 - y * x;
#endif

    cc_assert(y * x == 1);
    return y;
}

/*! @function ccn_div_exact_ws
 @abstract Computes q = a / d where a = 0 (mod d).

 @param ws  Workspace
 @param n   Length of q,a,d as a number of cc_units.
 @param q   The resulting exact quotient.
 @param a   The dividend a.
 @param d   The divisor d.
 */
CC_NONNULL_ALL
void ccn_div_exact_ws(cc_ws_t ws, cc_size n, cc_unit *cc_counted_by(n) q, const cc_unit *cc_counted_by(n) a, const cc_unit *cc_counted_by(n) d);

/*! @function ccn_divides1
 @abstract Returns whether q divides x.

 @param n  Length of x as a number of cc_units.
 @param x  The dividend x.
 @param q  The divisor q.

 @return True if q divides x without remainder, false otherwise.
 */
CC_NONNULL_ALL
bool ccn_divides1(cc_size n, const cc_unit *cc_counted_by(n) x, cc_unit q);

/*! @function ccn_select
 @abstract Select r[i] in constant-time, not revealing i via cache-timing.

 @param start Start index.
 @param end   End index (length of r).
 @param r     Big int r.
 @param i     Offset into r.

 @return r[i], or zero if start > i or end < i.
 */
CC_INLINE cc_unit ccn_select(cc_size start, cc_size end, const cc_unit *cc_counted_by(end) r, cc_size i)
{
    cc_unit ri = 0;

    for (cc_size j = start; j < end; j++) {
        cc_size i_neq_j; // i≠j?
        CC_HEAVISIDE_STEP(i_neq_j, i ^ j);
        ri |= r[j] & ((cc_unit)i_neq_j - 1);
    }

    return ri;
}

/*! @function ccn_invmod_ws
 @abstract Computes the inverse of x modulo m, r = x^-1 (mod m).
           Returns an error if there's no inverse, i.e. gcd(x,m) ≠ 1.

 @discussion This is a very generic version of the binary XGCD algorithm. You
             don't want to use it when you have an odd modulus.

             This function is meant to be used by RSA key generation, for
             computation of d = e^1 (mod lcm(p-1,q-1)), where m can be even.

             x > m is allowed as long as xn == n, i.e. they occupy the same
             number of cc_units.

 @param ws Workspace.
 @param n  Length of r and m as a number of cc_units.
 @param r  The resulting inverse r.
 @param xn Length of x as a number of cc_units.
 @param x  The number to invert.
 @param m  The modulus.

 @return 0 on success, non-zero on failure. See cc_error.h for more details.
 */
int ccn_invmod_ws(cc_ws_t ws, cc_size n, cc_unit *cc_counted_by(n) r, cc_size xn, const cc_unit *cc_counted_by(xn) x, const cc_unit *cc_counted_by(n) m);

/*! @function ccn_mux_seed_mask
 @abstract Refreshes the internal state of the PRNG used to mask cmov/cswap
           operations with a given seed.

 @discussion The seed should be of good entropy, i.e. generated by our default
             RNG. This function should be called before running algorithms that
             defend against side-channel attacks by using cmov/cswap. Examples
             are blinded modular exponentation (for RSA, DH, or MR) and EC
             scalar multiplication.

 @param seed A seed value.
 */
void ccn_mux_seed_mask(cc_unit seed);

#endif // _CORECRYPTO_CCN_INTERNAL_H
