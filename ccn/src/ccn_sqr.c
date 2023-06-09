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

#include <corecrypto/ccn.h>
#include "ccn_internal.h"

#if CCN_DEDICATED_SQR

typedef cc_unit cc_sqrw;
typedef cc_dunit cc_sqrd;
#define CCSQRW_BITS  CCN_UNIT_BITS

#if !CCN_UINT128_SUPPORT_FOR_64BIT_ARCH
#error "ccn_sqr requires support of 128-bit integers on 64bit architecture"
#endif

/* Do r = s^2, r is 2 * n cc_units in size, s is n * cc_units in size. */
void ccn_sqr_ws(cc_ws_t ws, cc_size n, cc_unit *r, const cc_unit *s)
{
    cc_sqrd prod,sum;
    cc_sqrw si, c,cadd;
    CC_DECL_BP_WS(ws,bp);
    cc_unit *t=CC_ALLOC_WS(ws, 2 * n);

    // First iteration separately since t[i]==0;
    t[0]=0;
    c=0;

    // Set of s0*sj
    si=s[0];
    for (cc_size j = 1; j < n; j++) {
        prod= ((cc_sqrd) si * s[j]) + c;  // Implicit C integer promotion
        t[j]=(cc_sqrw)prod;
        c = prod >> CCSQRW_BITS;
    }
    t[n]=c;

    // Set of s0^2
    prod= (cc_sqrd) si * si;
    r[0]= (cc_sqrw)prod;
    sum=(cc_sqrd)2*t[1]+(prod >> CCSQRW_BITS);
    r[1]=(cc_sqrw)sum;
    cadd=(sum >> CCN_UNIT_BITS);

    // Main loop
    for (cc_size i = 1; i < n; i++) {
        // Set of si^2
        c=0;
        si=s[i];

        // Set of si*sj
        for (cc_size j = (i+1); j < n; j++) {
            prod= ((cc_sqrd) si * s[j] + t[i+j]) + c;  // Implicit C integer promotion
            t[i+j]=(cc_sqrw)prod;
            c = prod >> CCSQRW_BITS;
        }
        t[i+n]=c;

        // 2t + r
        prod= (cc_sqrd) si * si + cadd;
        sum=(cc_sqrd)2*t[2*i]
                + (cc_sqrw)prod;
        r[2*i]= (cc_sqrw)sum;
        sum=(cc_sqrd)2*t[2*i+1]
                + (prod >> CCSQRW_BITS)
                + (sum >> CCSQRW_BITS);
        r[2*i+1]= (cc_sqrw)sum;
        cadd=(sum >> CCSQRW_BITS);
    }

    CC_FREE_BP_WS(ws,bp);
}

#else

void ccn_sqr_ws(cc_ws_t ws, cc_size n, cc_unit *r, const cc_unit *s)
{
    ccn_mul_ws(ws, n, r, s, s);
}

#endif // CCN_DEDICATED_SQR
