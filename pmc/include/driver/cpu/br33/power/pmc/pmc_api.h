#ifndef __PMC_API_H__
#define __PMC_API_H__

void sfr_p2m_int_set(u32 index);

void sfr_p2m_int_ie_set(u32 index, u8 enable);

void soft0_int_kick();

#endif
