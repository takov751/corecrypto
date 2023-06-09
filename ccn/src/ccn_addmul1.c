/* Copyright (c) (2012-2016,2019-2021) Apple Inc. All rights reserved.
 *
 * corecrypto is licensed under Apple Inc.’s Internal Use License Agreement (which
 * is contained in the License.txt file distributed with corecrypto) and only to
 * people who accept that license. IMPORTANT:  Any license rights granted to you by
 * Apple Inc. (if any) are limited to internal use within your organization only on
 * devices and computers you own or control, for the sole purpose of verifying the
 * security characteristics and correct functioning of the Apple Software.  You may
 * not, directly or indirectly, redistribute the Apple Software or any portions thereof.
 */

#include <corecrypto/cc_runtime_config.h>
#include "ccn_internal.h"

#define CCMULW_BITS (32)
#define CCMULW_MASK ((uint32_t)~0)

#if CCN_ADDMUL1_ASM
cc_unit ccn_addmul1_asm(cc_size n, cc_unit *r, const cc_unit *s, cc_unit v);

CC_INLINE bool ccn_use_addmul1_assembly()
{
#if defined(__x86_64__)
  return CC_HAS_BMI2() && CC_HAS_ADX();
#else
  return true;
#endif
}
#endif

CC_NONNULL_ALL
#if !CCN_UINT128_SUPPORT_FOR_64BIT_ARCH && (CCN_UNIT_SIZE == 8)
static cc_unit ccn_addmul1_(cc_size n, cc_unit *r, const cc_unit *s, cc_unit v)
{
    uint32_t *rout = (uint32_t *)r;
    const uint32_t *sin = (const uint32_t *)s;
    uint32_t v_low = v & CCMULW_MASK;
    uint32_t v_high = (v >> CCMULW_BITS) & CCMULW_MASK;

    uint32_t prevmul = 0;
    uint64_t prod_low, prod_high, carry_low = 0, carry_high = 0;
    uint64_t final_carry;

    for (cc_size j = 0; j < 2 * n; ++j) {
        uint32_t curmul = *sin++;
        uint64_t curDigit = *rout;

        curDigit += carry_low + carry_high;

        prod_low = (uint64_t)curmul * v_low;
        prod_high = (uint64_t)prevmul * v_high;

        carry_low = prod_low >> CCMULW_BITS;
        carry_high = prod_high >> CCMULW_BITS;

        prod_low &= CCMULW_MASK;
        prod_high &= CCMULW_MASK;

        uint64_t prodsum = prod_low + prod_high + curDigit;
        carry_low += prodsum >> CCMULW_BITS;
        *(rout++) = (uint32_t)prodsum;
        prevmul = curmul;
    }

    final_carry = (uint64_t)prevmul * v_high;
    final_carry += carry_low;
    final_carry += carry_high;
    return final_carry;
}
#else // if uint128_t is supported or it is a 32 bit machine
static cc_unit ccn_addmul1_(cc_size n, cc_unit *r, const cc_unit *s, cc_unit v)
{
    cc_unit carry = 0;
    cc_dunit prod = 0;
    for (cc_size j = 0; j < n; j++) {
        prod = (cc_dunit)(*s++);
        prod *= v;
        prod += *r;
        prod += carry;
        *(r++) = (cc_unit)prod;
        carry = prod >> CCN_UNIT_BITS;
    }
    return carry;
}
#endif /* !CCN_UINT128_SUPPORT_FOR_64BIT_ARCH && (CCN_UNIT_SIZE == 8) */

cc_unit ccn_addmul1(cc_size n, cc_unit *r, const cc_unit *s, cc_unit v)
{
#if CCN_ADDMUL1_ASM
    if (ccn_use_addmul1_assembly()) {
        return ccn_addmul1_asm(n, r, s, v);
    }
#endif

    return ccn_addmul1_(n, r, s, v);
}
