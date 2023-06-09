# Copyright (c) (2015,2016,2018,2019,2021) Apple Inc. All rights reserved.
#
# corecrypto is licensed under Apple Inc.’s Internal Use License Agreement (which
# is contained in the License.txt file distributed with corecrypto) and only to
# people who accept that license. IMPORTANT:  Any license rights granted to you by
# Apple Inc. (if any) are limited to internal use within your organization only on
# devices and computers you own or control, for the sole purpose of verifying the
# security characteristics and correct functioning of the Apple Software.  You may
# not, directly or indirectly, redistribute the Apple Software or any portions thereof.

#include <corecrypto/cc_config.h>


#if CCAES_ARM_ASM
#if !defined(__arm64__) && defined(__ARM_NEON__)

    .text

/*

    1. to compute GF(2^8) X*H, we can use the equality
    
        reflected (X)*reflected (H<<1 mod g(x)) = reflected (X*H) mod g(x)

    2. Karatsuba algorithm is used to reduce from 4 to 3 multiply operatoins

        AH:AL * BH:BL

        z2 = AH*BH
        z0 = AL*BL
        z1 = (AH+AL)*(BH+BL) - z2 - z0;

*/

/*
    gcm_init(ctx->Htable, ctx->H);

    this function pre-computes Htable = reflected((H << 1) mod g(x));

*/
    .globl	_gcm_init
    .align	4
_gcm_init:
#if CC_KERNEL
    vstmdb  sp!, {q0-q3}
#endif
    vld1.64     {q3},[r1,:64]
	vmov.i8		q0,#0xe1
    vext.8      q3, q3, q3, #8
	vshl.i64	d1,#57
	vrev64.8	q3,q3
	vshr.u64	d0,#63
	vdup.8		q1,d7[7]
	vshr.u64	d4,d6,#63
	vshr.s8		q1,#7
	vshl.i64	q3,q3,#1
	vand		q0,q0,q1
	vorr		d7,d4
	veor		q3,q3,q0
    vst1.64     {q3}, [r0,:64]
#if CC_KERNEL
    vldmia  sp!, {q0-q3}
#endif
	bx	lr



/*
    ARM NEON 64-bit binary multiplication C = A·B with 128-bit result from
    http://conradoplg.cryptoland.net/files/2010/12/mocrysen13.pdf
*/
    #define k48 d29
    #define k32 d24
    #define k16 d25
    #define BL  d26
    #define BH  d27
    #define BHL d28

    #define t0Q q8  
    #define t0L d16
    #define t0H d17
    #define t1Q q9  
    #define t1L d18
    #define t1H d19
    #define t2Q q10  
    #define t2L d20
    #define t2H d21
    #define t3Q q11  
    #define t3L d22
    #define t3H d23

    .macro  arm_neon_64bits_multiply
	vext.8		t0L, $3, $3, #1
	vext.8		$1, $2, $2, #1
	vmull.p8	t0Q, t0L, $2
	vmull.p8	$0, $3, $1
	vext.8		t1L, $3, $3, #2
	vmull.p8	t1Q, t1L, $2
	vext.8		t3L, $2, $2, #2
	vext.8		t2L, $3, $3, #3
	vmull.p8	t3Q, $3, t3L
	veor		t0Q, t0Q, $0
	vmull.p8	t2Q, t2L, $2
	vext.8		$1, $2, $2, #3
	veor		t1Q, t1Q, t3Q
	vmull.p8	$0, $3, $1
	veor		t0L, t0L, t0H
	vand		t0H, t0H, k48
	vext.8		t3L, $2, $2, #4
	veor		t1L, t1L, t1H
	vand		t1H, t1H, k32
	vmull.p8	t3Q, $3, t3L
	veor		t2Q, t2Q, $0
	veor		t0L, t0L, t0H
	veor		t1L, t1L, t1H
	veor		t2L, t2L, t2H
	vand		t2H, t2H, k16
	vext.8		t0Q, t0Q, t0Q, #15
	veor		t3L, t3L, t3H
	vmov.i64	t3H, #0
	vext.8		t1Q, t1Q, t1Q, #14
	veor		t2L, t2L, t2H
	vmull.p8	$0, $3, $2
	vext.8		t3Q, t3Q, t3Q, #12
	vext.8		t2Q, t2Q, t2Q, #13
	veor		t0Q, t0Q, t1Q
	veor		t2Q, t2Q, t3Q
	veor		$0, $0, t0Q
	veor		$0, $0, t2Q
    .endm 

    .macro  load_X_setup_constants
    vld1.64     {$2}, [r0, :64]!
	vmov.i64	k48,#0x0000ffffffffffff
    vld1.64     {$1}, [r0, :64]
	vldmia		r1,{d26-d27}
    vshr.u64    k32, k48, #16
	vrev64.8	$0,$0                   // reflected(X);
    vshr.u64    k16, k32, #16
	veor		BHL,BL,BH		        // Karatsuba pre-processing BH+BL
    .endm

    .macro  save_neon_registers
#if CC_KERNEL
    vstmdb  sp!, {q12-q14}
    vstmdb  sp!, {q8-q11}
    vstmdb  sp!, {q0-q3}
#endif
    .endm   

    .macro  restore_neon_registers
#if CC_KERNEL
    vldmia  sp!, {q0-q3}
    vldmia  sp!, {q8-q11}
    vldmia  sp!, {q12-q14}
#endif
    .endm   



/*
    gcm_gmult((const void *) CCMODE_GCM_KEY_X(key), (const void *) CCMODE_GCM_VNG_KEY_Htable(key), (void *) out);

        X = (X*H) modulo g(x);
        
        using reflected(X*H) mod g(x) = reflected(X) * reflected(H<<1 mod g(x))

        reflected(H<<1 mod g(x)) is already precomputed and stored.

        1. q3 = reflected/byte swapped X
        2. q3 = q3 * reflected(H<<1 mod g(x));  
        3. X = reflected(q3);
*/

    .globl	_gcm_gmult
    .align	4
_gcm_gmult:
    save_neon_registers
    load_X_setup_constants  q3, d6, d7

    // First argument is the out parameter for _gcm_ghash.
    add r0, r2, #8

    mov         r3, #16                     // r3=16 to reuse same code in _gcm_ghash only 1 iteration
    b           1f

/*
    gcm_ghash(CCMODE_GCM_KEY_X(key), (void *) CCMODE_GCM_VNG_KEY_Htable(key), (void*) bytes, j);

    q0 = reflected(X);

    for (i=0;i<nsize;i+=16) {
        q3 = q0 ^ reflected(input++);
        q0 = q3 * reflected(H<<1 mod g(x)); 
    }

    X = reflected(q3);
*/
    .globl	_gcm_ghash
    .align	4
_gcm_ghash:
    save_neon_registers
    load_X_setup_constants  q0, d0, d1

0:      // gcm_ghash main loop
	vld1.64		{d7}, [r2]!		// load input lo
	vld1.64		{d6}, [r2]!		// load input hi
	vrev64.8	q3,q3           // reflected(input);
	veor		q3,q0			// q3 = q3 ^ reflected(input++);


/*
        The following performs q0 = q3 * q13; where q13 = reflected(H<<1 mod g(x));
        The same code is used for both _gcm_ghash and _gcm_gmult (r3 = 16 so it only runs once)
*/

1:
    arm_neon_64bits_multiply    q0, d0, d6, BL      // q0 = AL*BL 
	veor		d6,d6,d7	                        // Karatsuba pre-processing (AH+AL)
    arm_neon_64bits_multiply    q1, d2, d6, BHL     // q1 = (AH+AL)*(BH+BL)
    arm_neon_64bits_multiply    q2, d4, d7, BH      // q2 = AH*BH

	veor		q1,q1,q0		                // q1 = (AH+AL)*BH+AH*BL;
	veor		q1,q1,q2                        // q1 = AH*BL + AL*BH;
	veor		d1,d1,d2
	veor		d4,d4,d3	                    // q2:q0 is now 256-bit result

    /* reduce to 128bit */

	vshl.i64	q9,q0,#57		@ 1st phase
	vshl.i64	q10,q0,#62
	veor		q10,q10,q9		@
	vshl.i64	q9,q0,#63
	veor		q10, q10, q9		@
 	veor		d1,d1,d20	@
	veor		d4,d4,d21
	vshr.u64	q10,q0,#1		@ 2nd phase
	veor		q2,q2,q0
	veor		q0,q0,q10		@
	vshr.u64	q10,q10,#6
	vshr.u64	q0,q0,#1		@
	veor		q0,q0,q2		@
	veor		q0,q0,q10		@

	subs		r3,#16
	bne		    0b

    // write X = reflected(q0);
	vrev64.8	q0,q0
    sub         r0, r0, #8
    vst1.64     {d1}, [r0,:64]!  // write out X
    vst1.64     {d0}, [r0,:64]   // write out X

    restore_neon_registers

	bx	lr					@ bx lr

#endif

#endif  // CCAES_ARM_ASM

