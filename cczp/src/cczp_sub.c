/* Copyright (c) (2010-2012,2015-2021) Apple Inc. All rights reserved.
 *
 * corecrypto is licensed under Apple Inc.’s Internal Use License Agreement (which
 * is contained in the License.txt file distributed with corecrypto) and only to
 * people who accept that license. IMPORTANT:  Any license rights granted to you by
 * Apple Inc. (if any) are limited to internal use within your organization only on
 * devices and computers you own or control, for the sole purpose of verifying the
 * security characteristics and correct functioning of the Apple Software.  You may
 * not, directly or indirectly, redistribute the Apple Software or any portions thereof.
 */

#include <corecrypto/cczp.h>
#include "cczp_internal.h"
#include "ccn_internal.h"
#include "cc_workspaces.h"

void cczp_sub_ws(cc_ws_t ws, cczp_const_t zp, cc_unit *r, const cc_unit *x, const cc_unit *y)
{
    cc_size n = cczp_n(zp);

    CC_DECL_BP_WS(ws, bp);
    cc_unit *t = CC_ALLOC_WS(ws, n);

    cc_unit borrow = ccn_sub(n, r, x, y);
    ccn_add(n, t, cczp_prime(zp), r);
    ccn_mux(n, borrow, r, t, r);

    cc_assert(ccn_cmp(n, r, cczp_prime(zp)) < 0);
    CC_FREE_BP_WS(ws, bp);
}

int cczp_sub(cczp_const_t zp, cc_unit *r, const cc_unit *x, const cc_unit *y)
{
    CC_DECL_WORKSPACE_OR_FAIL(ws, CCZP_SUB_WORKSPACE_N(cczp_n(zp)));
    cczp_sub_ws(ws, zp, r, x, y);
    CC_FREE_WORKSPACE(ws);
    return CCERR_OK;
}
