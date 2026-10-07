/****************************************************************
NAME:     TypeExt.h
ABSTRACT: Define extended type
Copyright (c) 2003 by Actions, All Rights Reserved.


*****************************************************************/
#ifndef _TYPEEXT_H
#define _TYPEEXT_H

#ifdef WIN32

typedef unsigned char    UCHAR, uchar, BYTE, byte;
typedef unsigned char    uint8, uint8_t, u8;
typedef signed char      int8, s8;                 

typedef unsigned short   uint16, uint16_t, u16;
typedef signed short     int16, s16;               

typedef unsigned int    uint32, uint32_t, u32;     
typedef signed int      int32, s32;                

#define BIT(n) (1UL<<(n))

#endif

#endif  /* _TYPEEXT_H*/

