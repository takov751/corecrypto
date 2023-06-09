/* Copyright (c) (2011-2021) Apple Inc. All rights reserved.
 *
 * corecrypto is licensed under Apple Inc.’s Internal Use License Agreement (which
 * is contained in the License.txt file distributed with corecrypto) and only to
 * people who accept that license. IMPORTANT:  Any license rights granted to you by
 * Apple Inc. (if any) are limited to internal use within your organization only on
 * devices and computers you own or control, for the sole purpose of verifying the
 * security characteristics and correct functioning of the Apple Software.  You may
 * not, directly or indirectly, redistribute the Apple Software or any portions thereof.
 */

#include "ccrsa_internal.h"

int ccrsa_generate_key_internal(size_t nbits, ccrsa_full_ctx_t fk,
                                size_t e_nbytes, const uint8_t *e,
                                struct ccrng_state *rng,
                                struct ccrng_state *rng_mr)
{
#if CC_DISABLE_RSAKEYGEN
    (void)nbits;    (void)fk;
    (void)e_nbytes; (void)e;
    (void)rng;      (void)rng_mr;

    return CCRSA_FIPS_KEYGEN_DISABLED;
#else
    // RSA key generation takes a lot of stack space
    // therefore sanity check the key size.
    if (nbits > CCRSA_KEYGEN_MAX_NBITS) {
        return CCRSA_INVALID_INPUT;
    }

    cc_size pbits = (nbits >> 1) + 1, qbits = nbits - pbits;

    // Sanity check for the computations above.
    if (pbits != ((nbits >> 1) + 1) || (qbits + pbits) != nbits) {
        return CCRSA_KEY_ERROR;
    }

    cc_size n = ccn_nof(nbits);

    /* size of pub zp priv zp and zq - ensure p > q */
    ccrsa_ctx_n(fk) = n;
    ccrsa_pub_ctx_t pubk = ccrsa_ctx_public(fk);

    CCZP_N(ccrsa_ctx_private_zp(fk)) = ccn_nof(pbits);
    CCZP_N(ccrsa_ctx_private_zq(fk)) = ccn_nof(qbits);
    if (ccn_read_uint(n, ccrsa_ctx_e(pubk), e_nbytes, e)) {
        return CCRSA_KEY_ERROR;
    }

    /* The public key e must be odd. */
    if ((ccrsa_ctx_e(pubk)[0] & 1) == 0) {
        return CCRSA_KEY_ERROR;
    }

    /* The public key e must be > 1. */
    if (ccn_bitlen(n, ccrsa_ctx_e(pubk)) <= 1) {
        return CCRSA_KEY_ERROR;
    }

    cczp_t zp = ccrsa_ctx_private_zp(fk);
    cczp_t zq = ccrsa_ctx_private_zq(fk);

    /* Generate random n bit primes p and q. */
    do {
        if (ccrsa_generate_prime(pbits, CCZP_PRIME(zp), ccrsa_ctx_e(pubk), rng, rng_mr)) {
            return CCRSA_KEYGEN_PRIME_NOT_FOUND;
        }

        if (ccrsa_generate_prime(qbits, CCZP_PRIME(zq), ccrsa_ctx_e(pubk), rng, rng_mr)) {
            return CCRSA_KEYGEN_PRIME_NOT_FOUND;
        }

        if (cczp_init(zp) || cczp_init(zq)) {
            return CCRSA_KEYGEN_PRIME_NOT_FOUND;
        }

        /* Repeat until we make a valid key from the candidates. */
    } while (ccrsa_crt_makekey(fk));

    /* Final consistency check. */
    if (ccrsa_pairwise_consistency_check(fk, rng)) {
        return CCERR_OK;
    }

    return CCRSA_KEYGEN_KEYGEN_CONSISTENCY_FAIL;
#endif
}

