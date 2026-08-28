#ifndef  __PMC_MEMORY_APP_H__
#define  __PMC_MEMORY_APP_H__

//*********************************************************************************//
//                                 CODE区域 配置                                   //
//*********************************************************************************//
#define TCFG_PMC_CODE_SIZE							0x20000 //PMC跑flash的代码空间分配。需要主系统和PMC工程中的配置保持一致

//*********************************************************************************//
//                                 SRAM 配置                                       //
//*********************************************************************************//
#define TCFG_MSYS_SRAM_SHARE_WITH_PMC_NUM			0 //主系统分享多少块SRAM给PMC。需要主系统和PMC工程中的配置保持一致

#if (TCFG_MSYS_SRAM_SHARE_WITH_PMC_NUM > 4)
#error"config sram share error"
#elif (TCFG_MSYS_SRAM_SHARE_WITH_PMC_NUM == 4)
#define TCFG_MSYS_SRAM_SHARE_WITH_PMC_SIZE			(64*1024 + 64*1024 + 128*1024 + 128*1024)
#elif (TCFG_MSYS_SRAM_SHARE_WITH_PMC_NUM == 3)
#define TCFG_MSYS_SRAM_SHARE_WITH_PMC_SIZE			(64*1024 + 64*1024 + 128*1024)
#elif (TCFG_MSYS_SRAM_SHARE_WITH_PMC_NUM == 2)
#define TCFG_MSYS_SRAM_SHARE_WITH_PMC_SIZE			(64*1024 + 64*1024)
#elif (TCFG_MSYS_SRAM_SHARE_WITH_PMC_NUM == 1)
#define TCFG_MSYS_SRAM_SHARE_WITH_PMC_SIZE			(64*1024)
#else
#define TCFG_MSYS_SRAM_SHARE_WITH_PMC_SIZE			0
#endif

#endif  /*MASKROM_H*/
