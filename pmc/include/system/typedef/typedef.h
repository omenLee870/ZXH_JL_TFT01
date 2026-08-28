//*********************************************************************************//
// Module name : typedef.h                                                         //
// Description : typedef head file                                                 //
// By Designer : zequan_liu                                                        //
// Dat changed :                                                                   //
//*********************************************************************************//

#ifndef __typedef_h__
#define __typedef_h__

//------------------------------------------------------//
// typedef
//------------------------------------------------------//

//typedef bool                _bit, bit;
typedef float               _f32, f32;
typedef double              _d64, d64;

typedef char                _s08, s08, _s8, s8;
typedef signed short        _s16, s16;
typedef signed int          _s32, s32;
typedef signed long long    _s64, s64;

typedef unsigned char       _u08, u08, _u8, u8, bool, BOOL;
typedef unsigned short      _u16, u16;
typedef unsigned int        _u32, u32;
typedef unsigned long long  _u64, u64;


/* portable 8-bit unsigned integer */
typedef unsigned char           uint8_t;
/* portable 8-bit signed integer */
typedef signed char             int8_t;
/* portable 16-bit unsigned integer */
typedef unsigned short int      uint16_t;
/* portable 16-bit signed integer */
typedef signed short int        int16_t;
/* portable 32-bit unsigned integer */
typedef unsigned int            uint32_t;
/* portable 32-bit signed integer */
typedef signed int              int32_t;


#define ARRAY_SIZE(array)  (sizeof(array)/sizeof(array[0]))

//------------------------------------------------------//
// condition
//------------------------------------------------------//


#define SIMEND       asm ("trigger")

#define FALSE        0
#define TRUE         1
#define false        0
#define true         1
#ifndef NULL
#define NULL         (void *)0
#endif

#define AT(x)        	__attribute__ ((section(#x)))
#define SEC_USED(x)     __attribute__((section(#x),used))
#define sec(x)          __attribute__((section(#x),used))
#define sec_used(x)     __attribute__((section(#x),used))
#define _INLINE_     	__attribute__ ((always_inline))
#define _NOINLINE_      __attribute__ ((noinline))
#define __WEAK__	    __attribute__((weak))
#define __NO_INIT__
#define ___interrupt 	__attribute__((interrupt("")))
#define ALIGNED(x)	    __attribute__((aligned(x)))
#define _GNU_PACKED_    __attribute__((packed))


//------------------------------------------------------//
// common define
//------------------------------------------------------//

#define hi(adr)           ( (adr) >> 16 )
#define lo(adr)           ( (adr) & 0xffff )

#define LDR_W(ptr)       *( (volatile _u32*)(ptr) )
#define LDR_H(ptr)       *( (volatile _u16*)(ptr) )
#define LDR_B(ptr)       *( (volatile _u08*)(ptr) )

#define STR_W(ptr,vld)   *( (volatile _u32*)(ptr) ) = (_u32)(vld);
#define STR_H(ptr,vld)   *( (volatile _u16*)(ptr) ) = (_u16)(vld);
#define STR_B(ptr,vld)   *( (volatile _u08*)(ptr) ) = (_u08)(vld);

#define BIT(n)            (        1<<(n)  )
#define BITSET(r,n)       ( r |=  (1<<(n)) )
#define BITCLR(r,n)       ( r &= ~(1<<(n)) )
#define BITXOR(r,n)       ( r ^=  (1<<(n)) )
#define BITTST(r,n)       ( r &   (1<<(n)) )

#define BIT_CHK1(r,n)     ( BITTST((r),(n)) == (1<<(n)) )
#define BIT_CHK0(r,n)     ( BITTST((r),(n)) == (0<<(n)) )

#define MIN(a,b)          ( ((a)<(b)) ? (a) : (b) )
#define MAX(a,b)          ( ((a)>(b)) ? (a) : (b) )

#define SFR(sfr, start, len, dat) (sfr = sfr & ~((~(0xffffffff << (len))) << (start)) | (((dat) & (~(0xffffffff << (len)))) << (start)))

#define NOP()				asm volatile ("nop")


#define readb(addr)   *((volatile unsigned char*)(addr))
#define readw(addr)   *((volatile unsigned short *)(addr))
#define readl(addr)   *((volatile unsigned long*)(addr))

#define writeb(addr, val)  *((volatile unsigned char*)(addr)) = (u8)(val)
#define writew(addr, val)  *((volatile unsigned short *)(addr)) = (u16)(val)
#define writel(addr, val)  *((volatile unsigned long*)(addr)) = (u32)(val)




#endif

//*********************************************************************************//
//                                                                                 //
//                               end of this module                                //
//                                                                                 //
//*********************************************************************************//
