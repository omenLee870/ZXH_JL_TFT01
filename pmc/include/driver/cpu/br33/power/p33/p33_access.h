#ifndef __P33_ACCESS_H__
#define __P33_ACCESS_H__

//
//
//					for p33 access
//
//
//
/**************************************************************/

//ROM
u8 p33_buf(u8 buf);


#define p33_xor_1word(addr, data0)      (*((volatile u32 *)&addr + 0xB000/4)    = data0);   asm volatile ("csync")
#define p33_or_1word(addr, data0)       (*((volatile u32 *)&addr + 0xA000/4)    = data0);   asm volatile ("csync")
#define p33_and_1word(addr, data0)      (*((volatile u32 *)&addr + 0x9000/4)    = data0);   asm volatile ("csync")
#define p33_tx_1word(addr, data0)       (*((volatile u32 *)&addr + 0x8000/4)    = data0);   asm volatile ("csync")
#define p33_rx_1word(addr)               *((volatile u32 *)&addr + 0x8000/4)
#define p33_xor_1byte(addr, data0)      (*((volatile u8 *)&addr  + 0x3000)      = data0);   asm volatile ("csync")
#define p33_or_1byte(addr, data0)       (*((volatile u8 *)&addr  + 0x2000)      = data0);   asm volatile ("csync")
#define p33_and_1byte(addr, data0)      (*((volatile u8 *)&addr  + 0x1000)      = data0);   asm volatile ("csync")
#define p33_tx_1byte(addr, data0)       (*((volatile u8 *)&addr  )              = data0);   asm volatile ("csync")
#define p33_rx_1byte(addr)               *((volatile u8 *)&addr  )
#define P33_CON_SET(sfr, start, len, data)  (sfr = (sfr & ~((~(0xffffffff << (len))) << (start))) | \
	 (((data) & (~(0xffffffff << (len)))) << (start)));\
     asm volatile ("csync")
#define P33_CON_GET(sfr)    (sfr)

#if 1

#define p33_fast_access(reg, data, en)           \
{ 												 \
    if (en) {                                    \
		p33_or_1byte(reg, (data));               \
    } else {                                     \
		p33_and_1byte(reg, (u8)~(data));             \
    }                                            \
}

#else

#define p33_fast_access(reg, data, en)         \
{                                              \
	if (en) {                                  \
       	reg |= (data);                         \
	} else {                                   \
		reg &= ~(data);                        \
    }                                          \
}

#endif









#endif
