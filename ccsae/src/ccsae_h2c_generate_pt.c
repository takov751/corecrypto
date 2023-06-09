/* Copyright (c) (2020-2021) Apple Inc. All rights reserved.
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
#include <corecrypto/ccsha2.h>
#include "cc_macros.h"
#include "ccsae.h"
#include "ccsae_priv.h"
#include "cch2c.h"
#include "cch2c_internal.h"
#include "cc_priv.h"

int ccsae_generate_h2c_pt(const struct cch2c_info *info,
                          const uint8_t *ssid,
                          size_t ssid_nbytes,
                          const uint8_t *password,
                          size_t password_nbytes,
                          const uint8_t *identifier,
                          size_t identifier_nbytes,
                          uint8_t *pt)
{
    CC_ENSURE_DIT_ENABLED

    
    int status = CCERR_PARAMETER;
    ccec_const_cp_t cp = info->curve_params();
    
    ccec_pub_ctx_decl_cp(cp, P);
    uint8_t data[2 * CCSAE_MAX_PASSWORD_IDENTIFIER_SIZE] = {0};
    
    cc_require(password_nbytes <= CCSAE_MAX_PASSWORD_IDENTIFIER_SIZE &&
               identifier_nbytes <= CCSAE_MAX_PASSWORD_IDENTIFIER_SIZE,
               out);
    
    cc_memcpy(data, password, password_nbytes);
    cc_memcpy(data + password_nbytes, identifier, identifier_nbytes);
    status = cch2c(info, ssid_nbytes, ssid, password_nbytes + identifier_nbytes, data, P);
    cc_require(status == CCERR_OK, out);
    
    ccec_export_pub(P, pt);
    
    status = CCERR_OK;
out:
    return status;
}

