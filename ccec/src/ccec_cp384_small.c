/* Copyright (c) (2021) Apple Inc. All rights reserved.
 *
 * corecrypto is licensed under Apple Inc.’s Internal Use License Agreement (which
 * is contained in the License.txt file distributed with corecrypto) and only to
 * people who accept that license. IMPORTANT:  Any license rights granted to you by
 * Apple Inc. (if any) are limited to internal use within your organization only on
 * devices and computers you own or control, for the sole purpose of verifying the
 * security characteristics and correct functioning of the Apple Software.  You may
 * not, directly or indirectly, redistribute the Apple Software or any portions thereof.
 */

#include "ccec_internal.h"

static cczp_funcs_decl_inv(cczp_p384_funcs_small, cczp_inv_field_ws);

static const ccec_cp_decl(384) ccec_cp384_small =
{
    .hp = {
        .n = CCN384_N,
        .bitlen = 384,
        .funcs = &cczp_p384_funcs_small
    },
    .p = {
        CCN384_C(ff,ff,ff,ff,ff,ff,ff,ff,ff,ff,ff,ff,ff,ff,ff,ff,ff,ff,ff,ff,ff,ff,ff,ff,ff,ff,ff,ff,ff,ff,ff,fe,ff,ff,ff,ff,00,00,00,00,00,00,00,00,ff,ff,ff,ff)
    },
    .pr = {
        CCN384_C(00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,01,00,00,00,00,ff,ff,ff,ff,ff,ff,ff,ff,00,00,00,01),1
    },
    .b = {
        CCN384_C(b3,31,2f,a7,e2,3e,e7,e4,98,8e,05,6b,e3,f8,2d,19,18,1d,9c,6e,fe,81,41,12,03,14,08,8f,50,13,87,5a,c6,56,39,8d,8a,2e,d1,9d,2a,85,c8,ed,d3,ec,2a,ef)
    },
    .gx = {
        CCN384_C(aa,87,ca,22,be,8b,05,37,8e,b1,c7,1e,f3,20,ad,74,6e,1d,3b,62,8b,a7,9b,98,59,f7,41,e0,82,54,2a,38,55,02,f2,5d,bf,55,29,6c,3a,54,5e,38,72,76,0a,b7)
    },
    .gy = {
        CCN384_C(36,17,de,4a,96,26,2c,6f,5d,9e,98,bf,92,92,dc,29,f8,f4,1d,bd,28,9a,14,7c,e9,da,31,13,b5,f0,b8,c0,0a,60,b1,ce,1d,7e,81,9d,7a,43,1d,7c,90,ea,0e,5f)
    },
    .hq = {
        .n = CCN384_N,
        .bitlen = 384,
        .funcs = &cczp_p384_funcs_small
    },
    .q = {
        CCN384_C(ff,ff,ff,ff,ff,ff,ff,ff,ff,ff,ff,ff,ff,ff,ff,ff,ff,ff,ff,ff,ff,ff,ff,ff,c7,63,4d,81,f4,37,2d,df,58,1a,0d,b2,48,b0,a7,7a,ec,ec,19,6a,cc,c5,29,73)
    },
    .qr = {
        CCN384_C(00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,38,9c,b2,7e,0b,c8,d2,20,a7,e5,f2,4d,b7,4f,58,85,13,13,e6,95,33,3a,d6,8d),1
    }
};

ccec_const_cp_t ccec_cp_384_small(void)
{
    return (ccec_const_cp_t)&ccec_cp384_small;
}
