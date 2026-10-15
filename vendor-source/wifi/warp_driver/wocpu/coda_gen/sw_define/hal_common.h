#ifndef ___HAL_COMMON_H__
#define ___HAL_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

#define PACKING
//typedef unsigned int FIELD;
#define FIELD unsigned int


// ---------------------------------------------------------------------------
//  Register Manipulations
// ---------------------------------------------------------------------------

#define IO_R_32(reg) \
    (*(volatile UINT_32 * const)(reg))

#define IO_W_32(reg, val) \
    (*(volatile UINT_32 * const)(reg)) = (val)

#define IO_R_16(reg) \
    (*(volatile UINT_16 * const)(reg))

#define IO_W_16(reg, val) \
    (*(volatile UINT_16 * const)(reg)) = (val)

#define IO_R_8(reg) \
    (*(volatile UINT_8 * const)(reg))

#define IO_W_8(reg, val) \
    (*(volatile UINT_8 * const)(reg)) = (val)

#define IO_CLR_BIT_32(reg, val) IO_W_32(reg, (IO_R_32(reg) & (~(val))))
#define IO_SET_BIT_32(reg, val) IO_W_32(reg, (IO_R_32(reg) | (val)))

#define IO_SET_FIELD(reg, mask, shft, val) IO_W_32((reg), ((IO_R_32(reg) & (~(mask))) | (((val) << (shft)) & (mask))))


#define BIT(n)                          ((UINT_32) 1 << (n))

#define BITS2(m,n)                      (BIT(m) | BIT(n) )
#define BITS3(m,n,o)                    (BIT(m) | BIT (n) | BIT (o))
#define BITS4(m,n,o,p)                  (BIT(m) | BIT (n) | BIT (o) | BIT(p))

/* bits range: for example BITS(16,23) = 0xFF0000
 *   ==>  (BIT(m)-1)   = 0x0000FFFF     ~(BIT(m)-1)   => 0xFFFF0000
 *   ==>  (BIT(n+1)-1) = 0x00FFFFFF
 */
#define BITS(m,n)                       (~(BIT(m)-1) & ((BIT(n) - 1) | BIT(n)))


#define MASK(Field)        \
       ((Field##_SHFT) < 16 ? (Field##_MASK) : ((Field##_MASK) >> 16) )
#define SHFT(Field)        \
       ((Field##_SHFT) < 16 ? (Field##_SHFT) : ((Field##_SHFT) - 16 ) )

#ifdef __cplusplus
}
#endif


#endif // ___HAL_COMMON_H__
