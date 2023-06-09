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

#include "cc_internal.h"
#include <corecrypto/ccec_priv.h>
#include "ccec_internal.h"
#include <corecrypto/cc_macros.h>
#include "cc_workspaces.h"
#include "cc_debug.h"

size_t ccec_compact_import_pub_size(size_t in_len)
{
    CC_ENSURE_DIT_ENABLED

    switch (in_len) {
        case 24: return 192;
        case 28: return 224;
        case 32: return 256;
        case 48: return 384;
        case 66: return 521;
        default: return 0;
    }
}

int ccec_compact_import_pub_ws(cc_ws_t ws,
                               ccec_const_cp_t cp,
                               size_t in_len,
                               const uint8_t *in,
                               ccec_pub_ctx_t key)
{
    int result = -1;
    cc_size n = ccec_cp_n(cp);

    CC_DECL_BP_WS(ws, bp);
    cc_unit *t = CC_ALLOC_WS(ws, n);

    /* Length must be identical to the size of p */
    cc_require((in_len == ccec_cp_prime_size(cp)), errOut);

    ccec_ctx_init(cp, key);

    // Read x
    cc_require(ccn_read_uint(ccec_cp_n(cp), ccec_ctx_x(key), in_len, in) == CCERR_OK, errOut);

    // Compute y from the given x intented to be on the curve
    cc_require(ccec_affine_point_from_x_ws(ws, cp, (ccec_affine_point_t)ccec_ctx_point(key), ccec_ctx_x(key)) == CCERR_OK, errOut);

    // https://datatracker.ietf.org/doc/draft-jivsov-ecc-compact/
    // Convention for y = min(y',p-y')
    ccn_sub(n, t, cczp_prime(ccec_cp_zp(cp)), ccec_ctx_y(key));
    if (ccn_cmp(n, t, ccec_ctx_y(key)) < 0) {
        ccn_set(n, ccec_ctx_y(key), t);
    }

    // Set z since internal representation use projective coordinates
    ccn_seti(ccec_cp_n(cp), ccec_ctx_z(key), 1);
    result = CCERR_OK;

errOut:
    CC_FREE_BP_WS(ws, bp);
    return result;
}

int ccec_compact_import_pub(ccec_const_cp_t cp, size_t in_len,
                            const uint8_t *in, ccec_pub_ctx_t key)
{
    CC_ENSURE_DIT_ENABLED

    CC_DECL_WORKSPACE_OR_FAIL(ws, CCEC_COMPACT_IMPORT_PUB_WORKSPACE_N(ccec_cp_n(cp)));
    int rv = ccec_compact_import_pub_ws(ws, cp, in_len, in, key);
    CC_FREE_WORKSPACE(ws);
    return rv;
}
