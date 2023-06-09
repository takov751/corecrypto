/* Copyright (c) (2016-2021) Apple Inc. All rights reserved.
 *
 * corecrypto is licensed under Apple Inc.’s Internal Use License Agreement (which
 * is contained in the License.txt file distributed with corecrypto) and only to
 * people who accept that license. IMPORTANT:  Any license rights granted to you by
 * Apple Inc. (if any) are limited to internal use within your organization only on
 * devices and computers you own or control, for the sole purpose of verifying the
 * security characteristics and correct functioning of the Apple Software.  You may
 * not, directly or indirectly, redistribute the Apple Software or any portions thereof.
 */

#include "crypto_test_dh.h"
#include <corecrypto/ccdh.h>
#include <corecrypto/ccdh_gp.h>
#include <corecrypto/ccrng_sequence.h>
#include <corecrypto/cc_config.h>
#include "testmore.h"
#include <stdlib.h>
#include "ccdh_internal.h"

#define F false
#define P true

static const struct ccdh_compute_vector dh_compute_vectors[]=
{
    #include "../test_vectors/DH.inc"
};

#define N_COMPUTE_VECTORS (sizeof(dh_compute_vectors)/sizeof(dh_compute_vectors[0]))

static int testDHCompute (void) {
    int rc = 1;
    for(unsigned int i = 0; i < N_COMPUTE_VECTORS; i++) {
        rc &= is(ccdh_test_compute_vector(&dh_compute_vectors[i]), 0, "testDHCompute Vector %d", i);
    }
    return rc;
}

#include <corecrypto/ccdh_gp.h>

/*
 This test generates 2 random key pairs for a given group and do the key exchange both way,
 Test fail if the generated secrets do not match
 */

static int testDHexchange(ccdh_const_gp_t gp) {
    int rc = 1;
    struct ccrng_sequence_state seq_rng;
    struct ccrng_state *rng_dummy = (struct ccrng_state *)&seq_rng;
    struct ccrng_state *rng = global_test_rng;

    /* Key exchange with l */
    const cc_size n = ccdh_gp_n(gp);
    const size_t s = ccn_sizeof_n(n);
    uint8_t key_seed[s];
    ccdh_full_ctx_decl(s, a);
    ccdh_full_ctx_decl(s, b);
    uint8_t z1[s], z2[s];
    size_t z1_len = s,z2_len = s;
    size_t private_key_length;

    
    rc &= is(ccdh_gp_prime_bitlen(gp), ccn_bitsof_n(n), "Bitlength");

    rc &= is(ccdh_generate_key(gp, rng, a), 0, "Computing first key");

    private_key_length = ccn_bitlen(n, ccdh_ctx_x(a));
    if (ccdh_gp_order_bitlen(gp)) {
        // Probabilistic test. Fails with prob < 2^-64
        rc &= ok((private_key_length<=ccdh_gp_order_bitlen(gp))
                      && (private_key_length>ccdh_gp_order_bitlen(gp)-64),
                      "Checking private key length is exactly l");
    }
    else if (ccdh_gp_l(gp)) {
        rc &= ok(private_key_length == ccdh_gp_l(gp),
                      "Checking private key length is exactly l");
    }

    rc &= is(ccdh_generate_key(gp, rng, b),0, "Computing second key");
    private_key_length = ccn_bitlen(n, ccdh_ctx_x(a));
    if (ccdh_gp_order_bitlen(gp)) {
        // Probabilistic test. Fails with prob < 2^-64
        rc &= ok((private_key_length <= ccdh_gp_order_bitlen(gp))
                      && (private_key_length > ccdh_gp_order_bitlen(gp) - 64),
                      "Checking private key length is exactly l");
    }
    else if (ccdh_gp_l(gp)) {
        rc &= ok(private_key_length == ccdh_gp_l(gp),
                      "Checking private key length is exactly l");
    }

    memset(z1,'a', z1_len);
    memset(z2,'b', z2_len);
    rc&=is(ccdh_compute_shared_secret(a, ccdh_ctx_public(b), &z1_len, z1, rng), 0, "Computing first secret");
    rc&=is(ccdh_compute_shared_secret(b, ccdh_ctx_public(a), &z2_len, z2, rng), 0, "Computing second secret");
    rc&=is(z1_len, z2_len, "Shared key have same size");
    rc&=ok_memcmp(z1, z2, z2_len, "Computed secrets dont match");

    /* Key exchange without l, 4 steps. */
    ccdh_gp_decl(ccn_sizeof_n(n), gp2);
    ccdh_gp_t gp_local = (ccdh_gp_t)gp2;
    CCDH_GP_N(gp_local) = n;

    // a) encode / decode in gp_local
    size_t encSize = ccder_encode_dhparams_size(gp);
    uint8_t *encder = malloc(encSize);
    uint8_t *encder_end = encder + encSize;
    is(ccder_encode_dhparams(gp, encder, encder_end),encder,"Encode failed");
    isnt(ccder_decode_dhparams(gp_local, encder, encder_end),NULL,"Decode failed");
    free(encder);

    // b) Force l to 0
    CCDH_GP_L(gp_local) = 0;

    // c) re-generate the key a
    rc&=is(ccdh_generate_key(gp_local, rng, a), 0, "Computing first key with l=0");
    rc&=ok((ccn_bitlen(n, ccdh_ctx_x(a)) <= ccn_bitlen(n,ccdh_ctx_prime(a)))
                  && (ccn_bitlen(n,ccdh_ctx_x(a)) >= ccn_bitlen(n,ccdh_ctx_prime(a))) - 64,
                  "Checking private key length when l==0");

    // d) Key exchange
    z1_len = s;
    z2_len = s;
    memset(z1, 'c', z1_len);
    memset(z2, 'd', z2_len);
    
    rc &= is(ccdh_compute_shared_secret(a, ccdh_ctx_public(b), &z1_len, z1, rng), 0, "Computing first secret");
    rc &= is(ccdh_compute_shared_secret(b, ccdh_ctx_public(a), &z2_len, z2, rng), 0, "Computing second secret");
    rc &= is(z1_len, z2_len, "Shared key have same size");
    rc &= ok_memcmp(z1, z2, z2_len,"Computed secrets dont match");

    // In the following tests, the regeneration of edge cases will fail if ccder_decode_dhaparams
    // returns the group order q in gp_local, as it changes how the random dummy keys are created.
    // To circumvent this, and get good tests, we zero out q in gp_local
    ccn_zero(CCDH_GP_N(gp_local), CCDH_GP_Q(gp_local));
    
    // e) re-generate the key a = p-2
    cc_unit p_minus_2[n];
    ccn_sub1(n, p_minus_2, ccdh_ctx_prime(a), 2);
    memcpy(key_seed, p_minus_2, s);
    ccrng_sequence_init(&seq_rng, sizeof(key_seed), key_seed);
    rc &= is(ccdh_generate_key(gp_local, rng_dummy, a), 0, "Private key with random = p-2");
    rc &= ok_memcmp(ccdh_ctx_x(a), p_minus_2, s, "Private key is p-2");

    // f) re-generate the key a = 1
    memset(key_seed, 0x00, s);
    key_seed[0] = 1;
    ccrng_sequence_init(&seq_rng, sizeof(key_seed), key_seed);
    rc &= is(ccdh_generate_key(gp_local, rng_dummy, a), 0, "Private key with random = 1");
    rc &= ok_memcmp(ccdh_ctx_x(a), key_seed, s, "Private key is 1");

    /* Negative testing */

    // 1) Bad random
    ccrng_sequence_init(&seq_rng,0,NULL);
    rc &= is(ccdh_generate_key(gp, rng_dummy, a),
                   CCERR_CRYPTO_CONFIG,
                   "Error random");

    // 2) Random too big
    uint8_t c=0xff;
    ccrng_sequence_init(&seq_rng,1,&c);
    rc &= is(ccdh_generate_key(gp_local, rng_dummy, a),
                   CCDH_GENERATE_KEY_TOO_MANY_TRIES,
                   "Value consistently too big (all FF)");

    // 3) Random too big p-1
    memcpy(key_seed, ccdh_ctx_prime(a), s);
    key_seed[0] ^= 1;
    ccrng_sequence_init(&seq_rng, 1, &c);
    rc &= is(ccdh_generate_key(gp_local, rng_dummy, a),
                   CCDH_GENERATE_KEY_TOO_MANY_TRIES,
                "Value consistently too big (p-1)");

    // 4) Verify that ccdh_valid_shared_secret is catching errors */
    cc_unit shared_secret_placebo[n];
    ccn_sub1(n, shared_secret_placebo, ccdh_gp_prime(gp), 1);
    rc &= is(ccdh_valid_shared_secret(n, shared_secret_placebo, gp), false,
            "Failure to catch shared secret that is p-1");
    ccn_seti(n, shared_secret_placebo, 0);
    rc &= is(ccdh_valid_shared_secret(n, shared_secret_placebo, gp), false,
            "Failure to catch shared secret that is 0");
    ccn_seti(n, shared_secret_placebo, 1);
    rc &= is(ccdh_valid_shared_secret(n, shared_secret_placebo, gp), false,
            "Failure to catch shared secret that is 1");
    
        // 5) Random zero
    c = 0;
    ccrng_sequence_init(&seq_rng, 1, &c);
    rc&=is(ccdh_generate_key(gp_local, rng_dummy, a),
                   CCDH_GENERATE_KEY_TOO_MANY_TRIES,
                   "Value consistently zero");

    return rc;
}

struct {
    const char *name;
    char *data;
    size_t length;
    int pass;
    int actualL;
    int retrievedL;
} dhparams[] = {
    {
        .name = "no l",
        .data = "\x30\x06\x02\x01\x03\x02\x01\x04",
        .length = 8,
        .pass = 1,
        .actualL = 0,
        .retrievedL = CCDH_MAX_GROUP_EXPONENT_BIT_LENGTH,
    },
    {
        .name = "with l smaller than 160",
        .data = "\x30\x09\x02\x01\x03\x02\x01\x04\x02\x01\x05",
        .length = 11,
        .pass = 1,
        .actualL = 5,
        .retrievedL = CCDH_MIN_GROUP_EXPONENT_BIT_LENGTH,
    },
    {
        .name = "with l at 160",
        .data = "\x30\x0A\x02\x01\x03\x02\x01\x04\x02\x02\x00\xA0",
        .length = 12,
        .pass = 1,
        .actualL = 160,
        .retrievedL = 160,
    },
    {
        .name = "with l at 256",
        .data = "\x30\x0A\x02\x01\x03\x02\x01\x04\x02\x02\x01\x00",
        .length = 12,
        .pass = 1,
        .actualL = 256,
        .retrievedL = 256,

    },
    {
        .name = "missing g",
        .data = "\x30\x03\x02\x01\x03",
        .length = 5,
        .pass = 0,
        .actualL = 0,
        .retrievedL = 0,
    }
};

static int testDHParameter(void) {
    const uint8_t *der, *der_end;
    const size_t size = 2048;
    ccdh_gp_decl(size, gp);
    size_t n;
    int rc=1;
    ccdh_gp_t gpfoo = (ccdh_gp_t)gp;

    CCDH_GP_N(gpfoo) = ccn_nof_size(size);

    for (n = 0; n < sizeof(dhparams) / sizeof(dhparams[0]); n++) {
        der = (const uint8_t *)dhparams[n].data;
        der_end = (const uint8_t *)dhparams[n].data + dhparams[n].length;

        size_t nNew = ccder_decode_dhparam_n(der, der_end);
        rc &= is(nNew, (size_t)1, "cc_unit is small? these have really small integers tests");

        der = ccder_decode_dhparams(gp, der, der_end);
        if (der == NULL) {
            rc &= ok(!dhparams[n].pass, "not passing test is supposed to pass");
            break;
        }
        rc &= ok(dhparams[n].pass, "passing test is not supposed to pass");

        size_t encSize = ccder_encode_dhparams_size(gp);
        if (dhparams[n].actualL == dhparams[n].retrievedL){
            rc &= is(encSize, dhparams[n].length, "length wrong");
        } else {
            rc &= isnt(encSize, dhparams[n].length, "length wrong");
        }
        
        uint8_t *encder = malloc(encSize);
        uint8_t *encder2, *encder_end;

        encder_end = encder + encSize;
        encder2 = ccder_encode_dhparams(gp, encder, encder_end);
        if (encder2 == NULL) {
            rc &= ok(false, "log foo");
            free(encder);
            break;
        }
        rc &= is(encder2, encder, "didn't encode the full length");

        // Only test for proper re-encoding if we didn't change the exponent length in read.
        if (dhparams[n].actualL == dhparams[n].retrievedL) {
             rc &= ok_memcmp(encder, dhparams[n].data, dhparams[n].length, "encoding length wrong on test %d" , n);
         }

        free(encder);
    }
    return rc;
}

// Tests to ensure that ccdh_copy_gp works. Copies an arbitrary group, and then ensures that memcmp matches the two groups.
void ccdh_copy_gp_test(void)
{
    int error;
    ccdh_const_gp_t test_group = ccdh_gp_apple768(); // Need a group to compare to, apple768 is arbitray
    ccdh_gp_decl(ccn_sizeof_n(test_group->n), gp1);
    CCDH_GP_N(gp1) = test_group->n; // Set the destination to be of the same length as the source group.
    ccdh_copy_gp(gp1, test_group);
    is(memcmp(gp1, test_group, ccdh_gp_n(test_group) * sizeof(cc_unit) ), 0, "ccdh_copy_gp_test failed memcmp, group didn't copy");
    
    // Create another group which should fail because group sizes are different
    ccdh_gp_decl (ccn_sizeof_n(test_group->n), gp2);
    CCDH_GP_N(gp2) = test_group->n + 1;
    error = ccdh_copy_gp(gp2, gp1);
    is (error, CCDH_DOMAIN_PARAMETER_MISMATCH, "ccdh_copy_gp_test failed size comparison");
    
    return;
}

// Tests to ensure that the ramp function properly increases the exponent bit-length in the group function
// in a monitonically increasing way, with a minimum bit length.
void ccdh_gp_ramp_exponent_test(void)
{
    // Need a writeable group to apply ramp function to, apple768 is arbitray
    ccdh_const_gp_t test_group = ccdh_gp_apple768();
    ccdh_gp_decl(ccn_sizeof_n(test_group->n), gp1);
    CCDH_GP_N(gp1) = test_group->n; // Set the destination to be of the same length as the source group.
    ccdh_copy_gp(gp1, test_group);

    // !!!! We use CCDH_GP_L macro below for comparison as opposed to the funciton cal ccdh_gp_l,
    // because the compiler was erroneously optimizing these calls by calling the first time, and storing the result
    // leading to erroneous reults when compiled in release mode.
    
    // Test to ensure exponents that enter lower than MIN ramp to MIN.
    CCDH_GP_L(gp1) = CCDH_MIN_GROUP_EXPONENT_BIT_LENGTH;
    ccdh_ramp_gp_exponent(CCDH_MIN_GROUP_EXPONENT_BIT_LENGTH - 10, gp1);
    is(CCDH_GP_L(gp1), CCDH_MIN_GROUP_EXPONENT_BIT_LENGTH, "ccdh_gp_ramp_exponent_test: Not Min Length");
    
    // Test to ensure exponents that enter lower than MIN ramp to MIN.
    CCDH_GP_L(gp1) = CCDH_MIN_GROUP_EXPONENT_BIT_LENGTH - 10;
    ccdh_ramp_gp_exponent(CCDH_MIN_GROUP_EXPONENT_BIT_LENGTH - 10, gp1);
    is(CCDH_GP_L(gp1), CCDH_MIN_GROUP_EXPONENT_BIT_LENGTH, "ccdh_gp_ramp_exponent_test: Not Min Length");

    // Test to ensure exponents that enter higher than MIN, but MAX is already set maintain max.
    CCDH_GP_L(gp1) = CCDH_MAX_GROUP_EXPONENT_BIT_LENGTH;
    ccdh_ramp_gp_exponent(CCDH_MIN_GROUP_EXPONENT_BIT_LENGTH + 10, gp1);
    is(CCDH_GP_L(gp1), CCDH_MAX_GROUP_EXPONENT_BIT_LENGTH, "ccdh_gp_ramp_exponent_test: Not Max Length");
    
    // Test to ensure exponents that enter higher than MIN, but MAX is already set maintain max.
    CCDH_GP_L(gp1) = CCDH_MIN_GROUP_EXPONENT_BIT_LENGTH;
    ccdh_ramp_gp_exponent(CCDH_MAX_GROUP_EXPONENT_BIT_LENGTH, gp1);
    is(CCDH_GP_L(gp1), CCDH_MAX_GROUP_EXPONENT_BIT_LENGTH,"ccdh_gp_ramp_exponent_test: Not Max Length");
 
    return;
}

static void ccdh_test_invalid_gp()
{
    ccdh_const_gp_t orig = ccdh_gp_rfc5114_MODP_1024_160();

    ccdh_gp_decl(ccn_sizeof(1024), gp);
    CCDH_GP_N(gp) = ccdh_gp_n(orig);

    int rv = ccdh_copy_gp(gp, orig);
    is(rv, CCERR_OK, "ccdh_copy_gp() failed");

    // Set a generator that's not in the large prime subgroup.
    ccn_seti(ccdh_gp_n(gp), CCDH_GP_G(gp), 2);

    // Generating a DH key should fail.
    ccdh_full_ctx_decl_gp(gp, full);
    rv = ccdh_generate_key(gp, global_test_rng, full);
    is(rv, CCDH_SAFETY_CHECK, "ccdh_generate_key() should fail");

    // Generating a DH key in the original group should work.
    ccdh_full_ctx_decl_gp(orig, full2);
    rv = ccdh_generate_key(orig, global_test_rng, full2);
    is(rv, CCERR_OK, "ccdh_generate_key() failed");
}

static void ccdh_test_p_with_leading_zeros()
{
    const uint8_t p[] = {
        0x00,
        0xac, 0x77, 0xe0, 0x9e, 0x8c, 0x8f, 0xc2, 0x0e,
        0x90, 0x85, 0xf0, 0xf7, 0x1f, 0xba, 0xce, 0x1e,
        0x54, 0xcd, 0x94, 0xcb, 0xc1, 0x00, 0x36, 0xcf,
        0x82, 0x70, 0x54, 0x93, 0xdf, 0xfc, 0x0f, 0xe2,
        0x4c, 0x5c, 0x53, 0x72, 0x48, 0x0d, 0xec, 0xd0,
        0xdd, 0xf1, 0xbe, 0x29, 0x05, 0xba, 0x0d, 0x38,
        0xbf, 0x00, 0x14, 0xf0, 0xc3, 0xbc, 0xbf, 0xde,
        0xf1, 0x68, 0x5e, 0xf2, 0x1b, 0x2a, 0xc2, 0x50,
        0x5a, 0xa6, 0x30, 0xf0, 0xd6, 0x2a, 0x5e, 0x7e,
        0x82, 0x5c, 0x5f, 0x18, 0xe2, 0x75, 0x38, 0xb6,
        0x60, 0x60, 0x7a, 0x41, 0x9f, 0x81, 0xd2, 0x16,
        0xf3, 0x76, 0xb9, 0x51, 0x95, 0x0a, 0x08, 0xa4,
        0xcc, 0xdd, 0x23, 0xa1, 0x94, 0xdd, 0x7d, 0xc4,
        0x54, 0x19, 0x39, 0x29, 0x1a, 0xc5, 0x6c, 0xcc,
        0x31, 0x2f, 0xee, 0x0f, 0x6e, 0xaf, 0x05, 0xf1,
        0x82, 0x55, 0xf6, 0x77, 0x90, 0x88, 0xa2, 0x53
    };

    cc_size n = ccn_nof_sizeof(p);
    ccdh_gp_decl(ccn_sizeof_n(n), gp);

    int warning;
    const uint8_t g = 0x02;
    int rv = ccdh_init_safe_gp_from_bytes(gp, n, sizeof(p), p, 1, &g, 0, NULL, 0, &warning);
    is(rv, CCERR_OK, "ccdh_init_safe_gp_from_bytes failed");
    is(warning, CCERR_OK, "no warning should be set");
}

static void ccdh_test_p_non_safe()
{
    const uint8_t p[] = {
        0xdb, 0xe6, 0x6e, 0x78, 0x53, 0x4b, 0xbc, 0xc9,
        0x73, 0x31, 0x65, 0x09, 0x2d, 0x88, 0xcd, 0xf5,
        0x03, 0xdc, 0xc5, 0x32, 0x0a, 0x7a, 0x1b, 0x11,
        0xf5, 0x8b, 0xe3, 0x54, 0x5d, 0x75, 0x25, 0x61,
        0x6a, 0x74, 0xce, 0xd1, 0x4e, 0x85, 0x93, 0xbf,
        0x69, 0x56, 0xcc, 0xae, 0x3b, 0x61, 0x25, 0x9b,
        0xd1, 0x77, 0x1f, 0xe4, 0x52, 0xb6, 0x4b, 0xb4,
        0x90, 0x4e, 0x6a, 0x67, 0x29, 0xd0, 0xa7, 0x98,
        0x95, 0x62, 0x43, 0xed, 0x72, 0xea, 0x7b, 0x51,
        0x58, 0xdd, 0xe5, 0xfa, 0x94, 0x42, 0xfe, 0x84,
        0x5d, 0xdc, 0x85, 0xc5, 0xc8, 0xc7, 0x02, 0x74,
        0xcc, 0xb3, 0x5a, 0x96, 0x26, 0x8b, 0xe6, 0x8e,
        0xc9, 0xfa, 0x9c, 0x10, 0xfc, 0x9c, 0xdd, 0x07,
        0x7b, 0xdd, 0x4b, 0xec, 0x3e, 0x88, 0xb5, 0x79,
        0xdd, 0x95, 0x02, 0xfc, 0xc2, 0x90, 0x57, 0xc2,
        0x0e, 0x30, 0x4f, 0x94, 0x51, 0x63, 0xf8, 0x85
    };

    cc_size n = ccn_nof_sizeof(p);
    ccdh_gp_decl(ccn_sizeof_n(n), gp);

    int warning;
    const uint8_t g = 0x02;
    int rv = ccdh_init_safe_gp_from_bytes(gp, n, sizeof(p), p, 1, &g, 0, NULL, 0, &warning);
    is(rv, CCDH_SAFETY_CHECK, "ccdh_init_safe_gp_from_bytes should fail");
    is(warning, CCDH_GP_NONSAFE_PRIME, "non-safe prime should be detected");
}

static void ccdh_test_p_composite()
{
    const uint8_t p[] = {
        0xe5, 0x20, 0x0d, 0xa0, 0x44, 0x2f, 0x76, 0xce,
        0xdf, 0xcf, 0x90, 0xad, 0xd3, 0x38, 0x10, 0xd4,
        0xc1, 0x96, 0xe1, 0x71, 0x84, 0x47, 0x29, 0xe5,
        0xc3, 0x51, 0xed, 0x20, 0xf4, 0x4b, 0x76, 0xe1,
        0xa1, 0x93, 0x88, 0x99, 0xb1, 0xdc, 0x5a, 0x58,
        0xad, 0x15, 0xe1, 0x81, 0x4b, 0xea, 0x72, 0x92,
        0x27, 0x20, 0xe8, 0xd5, 0xd2, 0xf2, 0x21, 0x3e,
        0x16, 0x68, 0xe1, 0x55, 0xf8, 0x24, 0x2f, 0x10,
        0xe5, 0xfa, 0x55, 0x9b, 0x79, 0xdb, 0xdc, 0xa6,
        0x30, 0xca, 0x2c, 0x4c, 0x73, 0x47, 0x86, 0xcf,
        0x6b, 0x30, 0x68, 0xd4, 0x22, 0xab, 0x61, 0xf1,
        0x88, 0x05, 0xac, 0x2f, 0xf8, 0x13, 0x0a, 0x60,
        0xb1, 0xc3, 0xc4, 0x65, 0x23, 0x08, 0xc2, 0x93,
        0x4d, 0x28, 0x79, 0xf8, 0x07, 0x9e, 0x24, 0xb5,
        0xe8, 0x63, 0x5b, 0x49, 0xd8, 0x21, 0x6b, 0x64,
        0x9d, 0x3d, 0x1b, 0x9c, 0xb6, 0x2f, 0x7e, 0x65
    };

    cc_size n = ccn_nof_sizeof(p);
    ccdh_gp_decl(ccn_sizeof_n(n), gp);

    const uint8_t g = 0x02;
    int rv = ccdh_init_gp_from_bytes(gp, n, sizeof(p), p, 1, &g, 0, NULL, 0);
    is(rv, CCDH_GP_P_NOTPRIME, "ccdh_init_gp_from_bytes should fail");
}

static void ccdh_test_q_composite()
{
    const uint8_t p[] = {
        0xdb, 0xe6, 0x6e, 0x78, 0x53, 0x4b, 0xbc, 0xc9,
        0x73, 0x31, 0x65, 0x09, 0x2d, 0x88, 0xcd, 0xf5,
        0x03, 0xdc, 0xc5, 0x32, 0x0a, 0x7a, 0x1b, 0x11,
        0xf5, 0x8b, 0xe3, 0x54, 0x5d, 0x75, 0x25, 0x61,
        0x6a, 0x74, 0xce, 0xd1, 0x4e, 0x85, 0x93, 0xbf,
        0x69, 0x56, 0xcc, 0xae, 0x3b, 0x61, 0x25, 0x9b,
        0xd1, 0x77, 0x1f, 0xe4, 0x52, 0xb6, 0x4b, 0xb4,
        0x90, 0x4e, 0x6a, 0x67, 0x29, 0xd0, 0xa7, 0x98,
        0x95, 0x62, 0x43, 0xed, 0x72, 0xea, 0x7b, 0x51,
        0x58, 0xdd, 0xe5, 0xfa, 0x94, 0x42, 0xfe, 0x84,
        0x5d, 0xdc, 0x85, 0xc5, 0xc8, 0xc7, 0x02, 0x74,
        0xcc, 0xb3, 0x5a, 0x96, 0x26, 0x8b, 0xe6, 0x8e,
        0xc9, 0xfa, 0x9c, 0x10, 0xfc, 0x9c, 0xdd, 0x07,
        0x7b, 0xdd, 0x4b, 0xec, 0x3e, 0x88, 0xb5, 0x79,
        0xdd, 0x95, 0x02, 0xfc, 0xc2, 0x90, 0x57, 0xc2,
        0x0e, 0x30, 0x4f, 0x94, 0x51, 0x63, 0xf8, 0x85
    };

    const uint8_t q[] = {
        0xdb, 0xe6, 0x6e, 0x78, 0x53, 0x4b, 0xbc, 0xc9,
        0x73, 0x31, 0x65, 0x09, 0x2d, 0x88, 0xcd, 0xf5,
        0x03, 0xdc, 0xc5, 0x32, 0x0a, 0x7a, 0x1b, 0x11,
        0xf5, 0x8b, 0xe3, 0x54, 0x5d, 0x75, 0x25, 0x61,
        0x6a, 0x74, 0xce, 0xd1, 0x4e, 0x85, 0x93, 0xbf,
        0x69, 0x56, 0xcc, 0xae, 0x3b, 0x61, 0x25, 0x9b,
        0xd1, 0x77, 0x1f, 0xe4, 0x52, 0xb6, 0x4b, 0xb4,
        0x90, 0x4e, 0x6a, 0x67, 0x29, 0xd0, 0xa7, 0x98,
        0x95, 0x62, 0x43, 0xed, 0x72, 0xea, 0x7b, 0x51,
        0x58, 0xdd, 0xe5, 0xfa, 0x94, 0x42, 0xfe, 0x84,
        0x5d, 0xdc, 0x85, 0xc5, 0xc8, 0xc7, 0x02, 0x74,
        0xcc, 0xb3, 0x5a, 0x96, 0x26, 0x8b, 0xe6, 0x8e,
        0xc9, 0xfa, 0x9c, 0x10, 0xfc, 0x9c, 0xdd, 0x07,
        0x7b, 0xdd, 0x4b, 0xec, 0x3e, 0x88, 0xb5, 0x79,
        0xdd, 0x95, 0x02, 0xfc, 0xc2, 0x90, 0x57, 0xc2,
        0x0e, 0x30, 0x4f, 0x94, 0x51, 0x63, 0xf8, 0x84
    };

    cc_size n = ccn_nof_sizeof(p);
    ccdh_gp_decl(ccn_sizeof_n(n), gp);

    const uint8_t g = 0x02;
    int rv = ccdh_init_gp_from_bytes(gp, n, sizeof(p), p, 1, &g, sizeof(q), q, 0);
    is(rv, CCDH_GP_Q_NOTPRIME, "ccdh_init_gp_from_bytes should fail");
}

#define TEST_GP(_name_)     diag("Test " #_name_); ok(testDHexchange(ccdh_gp_##_name_()), #_name_);
#define TEST_GP_SRP(_name_) diag("Test " #_name_); ok(testDHexchange(ccsrp_gp_##_name_()), #_name_);

int ccdh_tests(TM_UNUSED int argc, TM_UNUSED char *const *argv)
{
    plan_tests(573);

    diag("testDHCompute");
    ok(testDHCompute(), "testDHCompute");

    diag("testDHParameter");
    ok(testDHParameter(), "testDHParameter");

#if CORECRYPTO_HACK_FOR_WINDOWS_DEVELOPMENT
    TEST_GP(rfc5114_MODP_1024_160)
    TEST_GP(rfc5114_MODP_2048_224)
    TEST_GP(rfc2409group02)
    TEST_GP(rfc3526group05)
    TEST_GP(rfc3526group14)
    TEST_GP(rfc3526group15)
    TEST_GP(rfc3526group16)
    TEST_GP_SRP(rfc5054_1024)
    TEST_GP_SRP(rfc5054_2048)
#else
    TEST_GP(apple768)
    TEST_GP(rfc5114_MODP_1024_160)
    TEST_GP(rfc5114_MODP_2048_224)
    TEST_GP(rfc5114_MODP_2048_256)
    TEST_GP(rfc2409group02)
    TEST_GP(rfc3526group05)
    TEST_GP(rfc3526group14)
    TEST_GP(rfc3526group15)
    TEST_GP(rfc3526group16)
    TEST_GP(rfc3526group17)
    TEST_GP(rfc3526group18)
    TEST_GP_SRP(rfc5054_1024)
    TEST_GP_SRP(rfc5054_2048)
    TEST_GP_SRP(rfc5054_3072)
    TEST_GP_SRP(rfc5054_4096)
    TEST_GP_SRP(rfc5054_8192)
#endif
    ccdh_copy_gp_test();
    ccdh_gp_ramp_exponent_test();
    ccdh_test_gp_lookup();
    ccdh_test_invalid_gp();
    ccdh_test_p_with_leading_zeros();
    ccdh_test_p_non_safe();
    ccdh_test_p_composite();
    ccdh_test_q_composite();
    return 0;
}
