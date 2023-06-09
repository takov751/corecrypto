/* Copyright (c) (2014-2021) Apple Inc. All rights reserved.
 *
 * corecrypto is licensed under Apple Inc.’s Internal Use License Agreement (which
 * is contained in the License.txt file distributed with corecrypto) and only to
 * people who accept that license. IMPORTANT:  Any license rights granted to you by
 * Apple Inc. (if any) are limited to internal use within your organization only on
 * devices and computers you own or control, for the sole purpose of verifying the
 * security characteristics and correct functioning of the Apple Software.  You may
 * not, directly or indirectly, redistribute the Apple Software or any portions thereof.
 */

#include <corecrypto/cc_macros.h>
#include "cczp_internal.h"
#include "ccec_internal.h"
#include "cc_log.h"

#define CCEC_SIGN_MAX_RETRIES 10

int ccec_sign_internal_inner_ws(cc_ws_t ws,
                                ccec_const_cp_t cp,
                                const cc_unit *e,
                                const cc_unit *x,
                                const cc_unit *k,
                                ccec_const_projective_point_t G,
                                const cc_unit *m,
                                cc_unit *r,
                                cc_unit *s,
                                struct ccrng_state *rng)
{
    cczp_const_t zq = ccec_cp_zq(cp);
    cc_size n = ccec_cp_n(cp);

    CC_DECL_BP_WS(ws, bp);
    cc_unit *t0 = CC_ALLOC_WS(ws, n);
    ccec_projective_point *Q = CCEC_ALLOC_POINT_WS(ws, n);

    // Compute Q = k * G.
    int rv = ccec_mult_blinded_ws(ws, cp, Q, k, G, rng);
    cc_require(rv == CCERR_OK, errOut);

    rv = ccec_affinify_x_only_ws(ws, cp, ccec_point_x(Q, cp), Q);
    cc_require(rv == CCERR_OK, errOut);

    // Compute r = pubx (mod q).
    if (ccn_sub(n, r, ccec_const_point_x(Q, cp), cczp_prime(zq))) {
        ccn_set(n, r, ccec_const_point_x(Q, cp));
    }

    // Ensure that r ≠ 0.
    cc_require_action(!ccn_is_zero(n, r), errOut, rv = CCERR_RETRY);

    // Use Q for temp variables.
    cc_unit *t1 = ccec_point_x(Q, cp);
    cc_unit *t2 = ccec_point_y(Q, cp);

    // Mask each intermediary variable.
    cczp_mul_ws(ws, zq, t0, k, m);  // (k*m)
    cczp_mul_ws(ws, zq, t1, e, m);  // (e*m)
    cczp_mul_ws(ws, zq, t2, x, m);  // (x*m)

    // Instead of computing (e + xr) / k (mod q)
    // the masked variant computes ((e.m) + (x.m).r) / (k.m).

    // Find s = (e + xr) / k (mod q).
    cczp_mul_ws(ws, zq, s, t2, r);     // xr (mod q)
    cczp_add_ws(ws, zq, s, t1, s);     // e + xr (mod q)
    rv = cczp_inv_ws(ws, zq, t0, t0);  // k^(-1) (mod q)
    cc_require(rv == CCERR_OK, errOut);
    cczp_mul_ws(ws, zq, s, t0, s);     // (e + xr)k^(-1) (mod q)

    // Ensure that s ≠ 0.
    if (ccn_is_zero(n, s)) {
        rv = CCERR_RETRY;
    }

errOut:
    CC_FREE_BP_WS(ws, bp);
    return rv;
}

int ccec_sign_internal_ws(cc_ws_t ws,
                          ccec_full_ctx_t key,
                          size_t digest_len,
                          const uint8_t *digest,
                          cc_unit *r,
                          cc_unit *s,
                          struct ccrng_state *rng)
{
    ccec_const_cp_t cp = ccec_ctx_cp(key);
    cczp_const_t zq = ccec_cp_zq(cp);
    cc_size n = ccec_cp_n(cp);
    cc_assert(cczp_n(zq) == n);
    int result, i;

    // Simulate a crash when the digest length is < 128 bits.
    cc_log_fault_if(digest_len < 16, "Digest should be at least 128 bits long: argument digest_len = %lu", digest_len);

    CC_DECL_BP_WS(ws, bp);

    cc_unit *e = CC_ALLOC_WS(ws, n);
    cc_unit *k = CC_ALLOC_WS(ws, n);
    cc_unit *m = CC_ALLOC_WS(ws, n);

    ccec_projective_point *G = CCEC_ALLOC_POINT_WS(ws, n);

    result = ccec_projectify_ws(ws, cp, G, ccec_cp_g(cp), rng);
    cc_require(result == CCERR_OK, errOut);

    const cc_unit *x = ccec_ctx_k(key);
    size_t qbitlen = ccec_cp_order_bitlen(cp);

    // Convert digest to a field element
    size_t d_nbytes = CC_MIN_EVAL(digest_len, CC_BITLEN_TO_BYTELEN(qbitlen));
    cc_require(((result = ccn_read_uint(n, e, d_nbytes, digest)) >= 0), errOut);

    // Shift away low-order bits
    if (digest_len * 8 > qbitlen) {
        ccn_shift_right(n, e, e, -qbitlen % 8);
    }

    // e (mod q)
    cc_unit b = ccn_sub(n, r, e, cczp_prime(zq));
    ccn_mux(n, b, e, e, r);

    // Sanity check for private key
    cc_require((result = ccec_validate_scalar(cp, x)) == CCERR_OK, errOut);

    // ECDSA signing core
    for (i = 0; i < CCEC_SIGN_MAX_RETRIES; i += 1) {
        // Generate ephemeral k.
        result = ccec_generate_scalar_fips_retry_ws(ws, cp, rng, k);
        cc_require(result == CCERR_OK, errOut);

        // Generate mask m.
        result = ccec_generate_scalar_fips_retry_ws(ws, cp, rng, m);
        cc_require(result == CCERR_OK, errOut);

        // Compute (r,s). Retry if either r=0 or s=0.
        result = ccec_sign_internal_inner_ws(ws, cp, e, x, k, G, m, r, s, rng);
        if (result == CCERR_OK) {
            break;
        }

        cc_require(result == CCERR_RETRY, errOut);
    }

    if (CC_UNLIKELY(i == CCEC_SIGN_MAX_RETRIES)) {
        cc_try_abort("Failed to generate a valid signature");
        result = CCERR_INTERNAL;
    }

errOut:
    if (result) {
        ccn_clear(n, r);
        ccn_clear(n, s);
    }

    CC_FREE_BP_WS(ws, bp);
    return result;
}
