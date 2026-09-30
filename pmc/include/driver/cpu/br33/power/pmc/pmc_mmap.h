#ifndef __PMC_MMAP_H__
#define __PMC_MMAP_H__

/////////////////////////////////////////////////////////////////////////////////
#define P11_RAM_BASE 				(0x7e40000)
#define P11_RAM_BEGIN 				(P11_RAM_BASE)
#define P11_RAM_END					(0x7e60000)
#define P11_RAM_SIZE				(P11_RAM_END - P11_RAM_BEGIN)

/////////////////////////////////////////////////////////////////////////////////
#define MSYS_POFF_RAM_END   		P11_RAM_END
#define MSYS_POFF_RAM_SIZE  	    0x20
#define MSYS_POFF_RAM_BEGIN 		(MSYS_POFF_RAM_END - MSYS_POFF_RAM_SIZE)

/////////////////////////////////////////////////////////////////////////////////
#define MSYS_ROM_EXPORT_RAM_END   	MSYS_POFF_RAM_BEGIN
#define MSYS_ROM_EXPORT_RAM_SIZE  	0x120
#define MSYS_ROM_EXPORT_RAM_BEGIN 	(MSYS_ROM_EXPORT_RAM_END - MSYS_ROM_EXPORT_RAM_SIZE)

/////////////////////////////////////////////////////////////////////////////////
#define M2P_MESSAGE_END 			MSYS_ROM_EXPORT_RAM_BEGIN
#define M2P_MESSAGE_SIZE 			0xe0
#define M2P_MESSAGE_RAM_BEGIN 		(M2P_MESSAGE_END - M2P_MESSAGE_SIZE)

/////////////////////////////////////////////////////////////////////////////////
#define P2M_MESSAGE_END				M2P_MESSAGE_RAM_BEGIN
#define P2M_MESSAGE_SIZE 			0x40
#define P2M_MESSAGE_RAM_BEGIN 		(P2M_MESSAGE_END - P2M_MESSAGE_SIZE)

/////////////////////////////////////////////////////////////////////////////////
#define P2P_MESSAGE_END				P2M_MESSAGE_RAM_BEGIN
#define P2P_MESSAGE_SIZE 			0x40
#define P2P_MESSAGE_RAM_BEGIN 		(P2P_MESSAGE_END - P2P_MESSAGE_SIZE)

/////////////////////////////////////////////////////////////////////////////////
#define P11_RAM1_END				P2P_MESSAGE_RAM_BEGIN
#define P11_RAM1_SIZE				(66*1024)
#define P11_RAM1_BEGIN				(P11_RAM1_END - P11_RAM1_SIZE)

//////////////////////////////////////////////////////////////////////////////
#define P11_ISR_BEGIN				P11_RAM_BASE
#define P11_ISR_SIZE				0x200
#define P11_ISR_END					(P11_ISR_BEGIN+P11_ISR_SIZE)

//////////////////////////////////////////////////////////////////////////////
#define PMC1_RESET_BEGIN			(P11_ISR_END)
#define PMC1_RESET_SIZE				(0x4)
#define PMC1_RESET_END				(PMC1_RESET_BEGIN+PMC1_RESET_SIZE)


//////////////////////////////////////////////////////////////////////////////
#define P11_RAM0_END				P11_RAM1_BEGIN
#define P11_RAM0_BEGIN 				(PMC1_RESET_END)
#define P11_RAM0_SIZE				(P11_RAM0_END - P11_RAM0_BEGIN)
#define PMC0_ROM_EXPORT_RAM_SIZE  	(0x200)
#define PMC0_RESET_ADDR             (P11_RAM0_BEGIN + PMC0_ROM_EXPORT_RAM_SIZE)


//////////////////////////////////////////////////////////////////////////////////
#define P11_RAM_ACCESS(x)   							(*(volatile u8 *)(x))
#define M2P_MESSAGE_ACCESS(x)      						P11_RAM_ACCESS(M2P_MESSAGE_RAM_BEGIN + x)
#define P2M_MESSAGE_ACCESS(x)      						P11_RAM_ACCESS(P2M_MESSAGE_RAM_BEGIN + x)
#define P2P_MESSAGE_ACCESS(x)							P11_RAM_ACCESS(P2P_MESSAGE_RAM_BEGIN + x)




#endif
