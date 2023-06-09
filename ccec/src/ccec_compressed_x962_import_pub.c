/* Copyright (c) (2020,2021) Apple Inc. All rights reserved.
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
#include "cc_debug.h"
#include "cc_macros.h"

static int ccec_compressed_x962_import_pub_ws(cc_ws_t ws, ccec_const_cp_t cp,
                                              size_t in_len, const uint8_t *in,
                                              ccec_pub_ctx_t key)
{
    int result = CCERR_OK;
    uint8_t parity = 0;
    size_t len = ccec_cp_prime_size(cp);

    /* Ensure we don't get the point at infinity (unit point) */
    if (in_len == 1) {
        if (in[0] == 0) {
            return CCEC_KEY_CANNOT_BE_UNIT;
        } else {
            return CCEC_COMPRESSED_POINT_ENCODING_ERROR;
        }
    }

    if (in_len != len + 1) {
        return CCEC_COMPRESSED_POINT_ENCODING_ERROR;
    }
    if (in[0] != 2 && in[0] != 3) {
        return CCEC_COMPRESSED_POINT_ENCODING_ERROR;
    }

    CC_DECL_BP_WS(ws, bp);

    parity = in[0] & 1;

    ccec_ctx_init(cp, key);
    cc_require((result = ccn_read_uint(ccec_cp_n(cp), ccec_ctx_x(key), in_len - 1, in + 1)) == 0, errOut);
    cc_require_or_return(ccn_cmp(ccec_cp_n(cp), ccec_ctx_x(key), ccec_cp_p(cp)) == -1, CCEC_COMPRESSED_POINT_ENCODING_ERROR);
    cc_require((result = ccec_affine_point_from_x_ws(ws, cp, (ccec_affine_point_t)ccec_ctx_point(key), ccec_ctx_x(key))) == 0, errOut);

    if ((ccec_ctx_y(key)[0] & 1) != (cc_unit) parity) {
        cczp_negate(ccec_cp_zp(cp), ccec_ctx_y(key), ccec_ctx_y(key));
    }
    ccn_seti(ccec_cp_n(cp), ccec_ctx_z(key), 1); // Set projective z coordinate to 1.

errOut:
    CC_FREE_BP_WS(ws, bp);
    return result;
}

int ccec_compressed_x962_import_pub(ccec_const_cp_t cp, size_t in_len, const uint8_t *in, ccec_pub_ctx_t key)
{
    CC_ENSURE_DIT_ENABLED

    CC_DECL_WORKSPACE_OR_FAIL(ws, CCEC_COMPRESSED_X962_IMPORT_PUB_WORKSPACE_N(ccec_cp_n(cp)));
    int rv = ccec_compressed_x962_import_pub_ws(ws, cp, in_len, in, key);
    CC_FREE_WORKSPACE(ws);
    return rv;
}
