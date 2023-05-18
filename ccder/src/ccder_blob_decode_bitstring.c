/* Copyright (c) (2012,2015,2019,2021) Apple Inc. All rights reserved.
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
#include <corecrypto/ccder.h>
#include <stddef.h>

bool
ccder_blob_decode_bitstring(ccder_read_blob *from, ccder_read_blob *bit_range, size_t *bit_count) {
    CC_ENSURE_DIT_ENABLED

    if (!ccder_blob_decode_range(from, CCDER_BIT_STRING, bit_range)) {
        return false;
    }
    *bit_count = 0;
    size_t len = ccder_blob_size(*bit_range);
    
    if (len == 0) {
        return true;
    }
    
    if (((len - 1) * 8) >= *bit_range->der) {
        *bit_count = ((len - 1) * 8) - *bit_range->der;
    } else {
        /*
        It's technically an error but to match the previous version of
        corecrypto we'll ignore this. The *bit_count will still be zero.
        */
        // return false;
    }
    
    bit_range->der = bit_range->der + 1;
    bit_range->der_end = bit_range->der_end;
    return true;
}
