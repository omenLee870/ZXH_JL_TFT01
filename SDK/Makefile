
# make 编译并下载
# make VERBOSE=1 显示编译详细过程
# make clean 清除编译临时文件
#
# 注意： Linux 下编译方式：
#     1. 从 http://pkgman.jieliapp.com/doc/all 处找到下载链接
#     2. 下载后，解压到 /opt/jieli 目录下，保证
#       /opt/jieli/common/bin/clang 存在（注意目录层次）
#     3. 确认 ulimit -n 的结果足够大（建议大于8096），否则链接可能会因为打开文件太多而失败
#       可以通过 ulimit -n 8096 来设置一个较大的值
#

# 工具路径设置
ifeq ($(OS), Windows_NT)
# Windows 下工具链位置
TOOL_DIR := C:/JL/pi32/bin
CC    := clang.exe
CXX   := clang.exe
LD    := pi32v2-lto-wrapper.exe
AR    := llvm-ar.exe
export MKDIR := mkdir_win -p
RM    := rm -rf

export SYS_LIB_DIR := C:/JL/pi32/pi32v2-lib/r3-large
export SYS_INC_DIR := C:/JL/pi32/pi32v2-include
export EXT_CFLAGS  := # Windows 下不需要 -D__SHELL__
export PATH:=$(TOOL_DIR);$(PATH)

## 后处理脚本
export FIXBAT          := tools/utils/fixbat.exe # 用于处理 utf8->gbk 编码问题
export POST_SCRIPT     := cpu/br33/tools/download.bat
export RUN_POST_SCRIPT := cpu\br33\tools\download.bat
else
# Linux 下工具链位置
TOOL_DIR := /opt/jieli/pi32v2/bin
CC    := clang
CXX   := clang
LD    := lto-wrapper
AR    := lto-ar
export MKDIR := mkdir -p
RM    := rm -rf
export OBJDUMP := $(TOOL_DIR)/objdump
export OBJCOPY := $(TOOL_DIR)/objcopy
export OBJSIZEDUMP := $(TOOL_DIR)/objsizedump

export SYS_LIB_DIR := $(TOOL_DIR)/../lib/r3-large
export SYS_INC_DIR := $(TOOL_DIR)/../include
export EXT_CFLAGS  := -D__SHELL__ # Linux 下需要这个保证正确处理 download.c
export PATH:=$(TOOL_DIR):$(PATH)

## 后处理脚本
export FIXBAT          := touch # Linux下不需要处理 bat 编码问题
export POST_SCRIPT     := cpu/br33/tools/download.sh
export RUN_POST_SCRIPT := bash $(POST_SCRIPT)
endif

export CC  := $(TOOL_DIR)/$(CC)
export CXX := $(TOOL_DIR)/$(CXX)
export LD  := $(TOOL_DIR)/$(LD)
export AR  := $(TOOL_DIR)/$(AR)
# 输出文件设置
export OUT_ELF   := cpu/br33/tools/sdk.elf
export OBJ_FILE  := $(OUT_ELF).objs.txt
# 编译路径设置
export BUILD_DIR := objs

# 编译参数设置
export CFLAGS := \
	-target pi32v2 \
	-mcpu=r3 \
	-integrated-as \
	-flto \
	-Wuninitialized \
	-Wno-invalid-noreturn \
	-fno-common \
	-integrated-as \
	-Oz \
	-g \
	-flto \
	-fallow-pointer-null \
	-fprefer-gnu-section \
	-Wno-shift-negative-value \
	-Wincompatible-pointer-types \
	-Wreturn-type \
	-Wimplicit-function-declaration \
	-mllvm -pi32v2-large-program=true \
	-fms-extensions \
	-fdiscrete-bitfield-abi \
	-fno-unwind-tables \
	-ffunction-sections \
	-fdata-sections \
	-fmessage-length=0 \


# C++额外的编译参数
export CXXFLAGS := \
	-std=gnu++14 \
	-fno-exceptions \
	-fno-rtti \


# 宏定义
export DEFINES := \
	-DSUPPORT_MS_EXTENSIONS \
	-DCONFIG_RELEASE_ENABLE \
	-DH264E_MAX_THREADS=1 \
	-DCONFIG_CPU_BR33 \
	-DCONFIG_IIC_VERSION2 \
	-DCONFIG_PRINT_FLOAT_ENABLE \
	-DNEW_JLFS \
	-DROM_SECURE_BOOT \
	-DUSB_HW_20 \
	-DCPU_CORE_NUM=2 \
	-DPMC_CPU_CORE_NUM=2 \
	-DCONFIG_PHYSIC_RAM_SIZE=0x100000 \
	-DCONFIG_IPU_SID_SEN=1 \
	-DLWIP_USE_BT \
	-DCONFIG_NET_ENABLE \
	-DCONFIG_JIELI_BR33 \
	-DCONFIG_SDIO_SUPPORT \
	-DAIC_BUS_SDIO \
	-DCONFIG_AIC8800D80 \
	-DCONFIG_AIC8800D80_H \
	-DCONFIG_LWIP \
	-DCONFIG_LOAD_FW=2 \
	-DWPA_TYPES_DEFINED \
	-DCONFIG_SOCK_RTOS_AL \
	-DMAX_RTOS_SOCKETS=12 \
	-DPLATFORM_NO_FILE \
	-DAIC_FEATURE_P2P \
	-DAIC_JIELI_STA_ROOT_CAUSE_TEST=0 \
	-DAIC_JIELI_P2P_GO_AUTO_RECYCLE=0 \
	-DAIC_JIELI_DEFER_STA_REUSE=1 \
	-DCONFIG_SUPPORT_AES_CCM_FOR_EDR \
	-DCONFIG_AES_CCM_RXMAXBUF_EXT_MICLEN \
	-DCONFIG_SOFT_BASEBAND_ENABLE \
	-DCONFIG_BLE_SOFT_BASEBAND_ENABLE \
	-DCONFIG_CMAC_SCH_INCMAC \
	-DCONFIG_CMAC_LOW_POWER_INCMAC \
	-DBREDR_APP_CODE \
	-DBREDR_SDK_CODE \
	-DCONFIG_TICK_TIMER_INT_UNMASK_ENABLE \
	-DBR33_FPGA_CODE_VERIFY \
	-DCONFIG_NEW_BREDR_ENABLE \
	-DCONFIG_NEW_MODEM_ENABLE \
	-DCONFIG_NEW_TWS_FORWARD_ENABLE \
	-DCONFIG_UCOS_ENABLE \
	-DCONFIG_EQ_SUPPORT_ASYNC \
	-DEQ_CORE_V1 \
	-DCONFIG_SIN_TONE_V2 \
	-DCONFIG_DNS_ENABLE \
	-DCONFIG_DMS_MALLOC \
	-DCONFIG_MMU_ENABLE \
	-DCONFIG_AEC_M=256 \
	-DCONFIG_AUDIO_ONCHIP \
	-DCONFIG_MEDIA_NEW_ENABLE \
	-DCONFIG_SUPPORT_EX_TWS_ADJUST \
	-D__GCC_PI32V2__ \
	-DCONFIG_NEW_ECC_ENABLE \
	-DMULTI_MEMORY_ENABLE \
	-DCONFIG_CODE_MOVABLE_ENABLE \
	-DTCFG_USER_TWS_ENABLE=0 \
	-DCONFIG_EFFECT_CORE_V2_ENABLE \
	-DTCFG_USB_SLAVE_HCI_ENABLE=0 \
	-DCONFIG_OS_AFFINITY_ENABLE=1 \
	-DEXPORT_CONFIG_USB_ENABLE=1 \
	-DEXPORT_CONFIG_SD_ENABLE=1 \
	-DEXPORT_FMNA_ENABLE=1 \
	-DCONFIG_JLUI_ENABLE=1 \
	-DCONFIG_LV84_ENABLE=0 \
	-DCONFIG_LV9_ENABLE=1 \
	-DCONFIG_VIDEO_ENABLE=1 \
	-DCONFIG_APP_UI_ENABLE=1 \
	-DEVENT_HANDLER_NUM_CONFIG=2 \
	-DEVENT_TOUCH_ENABLE_CONFIG=0 \
	-DEVENT_POOL_SIZE_CONFIG=256 \
	-DCONFIG_EVENT_KEY_MAP_ENABLE=0 \
	-DTIMER_POOL_NUM_CONFIG=15 \
	-DAPP_ASYNC_POOL_NUM_CONFIG=0 \
	-DVFS_FILE_POOL_NUM_CONFIG=1 \
	-DFS_VERSION=0x020001 \
	-DFATFS_VERSION=0x020101 \
	-DSDFILE_VERSION=0x020000 \
	-DVFS_ENABLE=1 \
	-DVM_MAX_SIZE_CONFIG=64*1024 \
	-DVM_ITEM_MAX_NUM=256 \
	-DCONFIG_NEW_CFG_TOOL_ENABLE \
	-DCONFIG_WATCH_CASE_ENABLE=1 \
	-DCONFIG_LITE_AEC_ENABLE=0 \
	-DAUDIO_REC_LITE \
	-DAUDIO_DEC_LITE \
	-DAUDIO_REC_POOL_NUM=1 \
	-DAUDIO_DEC_POOL_NUM=3 \
	-DCONFIG_BTCTRLER_TASK_DEL_ENABLE \
	-DCONFIG_LINK_DISTURB_SCAN_ENABLE=0 \
	-DCONFIG_UPDATA_ENABLE \
	-DCONFIG_OTA_UPDATA_ENABLE \
	-DEXPORT_PLATFORM_AUDIO_SMART_VOICE_ENABLE=1 \
	-DEXPORT_PLATFORM_AUDIO_PDM_ENABLE=1 \
	-DEXPORT_PLATFORM_AUDIO_ALINK_ENABLE=1 \
	-DEXPORT_PLATFORM_AUDIO_TDM_ENABLE=0 \
	-DEXPORT_PLATFORM_AUDIO_SPDIF_ENABLE=0 \
	-DEXPORT_PLATFORM_AUDIO_FM_ENABLE=0 \
	-DEXPORT_PLATFORM_AUDIO_FIR_ENABLE=1 \
	-DEXPORT_PLATFORM_AUDIO_WM8978_ENABLE=0 \
	-DEXPORT_PLATFORM_AUDIO_UART_SENDER_ENABLE=1 \
	-DEXPORT_PLATFORM_LE_AUDIO_ENABLE=0 \
	-DEXPORT_PLATFORM_AUDIO_EFFECT_DEMO_ENABLE=0 \
	-DEXPORT_PLATFORM_HW_MATH_VERSION=0 \
	-DEXPORT_PLATFORM_AUDIO_DEMO=1 \
	-DEXPORT_WATCH_LVGL_APP_CODE_ENABLE=0 \
	-DEXPORT_WATCH_JLUI_APP_CODE_ENABLE=0 \
	-DEXPORT_CSC_JLUI_APP_CODE_ENABLE=0 \
	-DEXPORT_AI_GLASSES_LVGL_APP_CODE_ENABLE=0 \
	-DEXPORT_DASHBOARD_LVGL_APP_CODE_ENABLE=0 \
	-DEXPORT_KID_CAMERA_LVGL_APP_CODE_ENABLE=0 \
	-DFREETYPE_COMMIT_HASH=\"45903920b984540bb629bc89f4c010159c23a89a\" \
	-DFT2_BUILD_LIBRARY \
	-DLVGL_COMMIT_HASH=\"release/v9.5_85aa60d18b3d5e5588d7b247abf90198f07c8a63\" \
	-D_XOPEN_SOURCE=700 \
	-D_GNU_SOURCE \
	-D__ELF__ \


export DEFINES += $(EXT_CFLAGS) # 额外的一些定义

# 头文件搜索路径
export INCLUDES := \
	@build/include_dir.txt \
	-I$(SYS_INC_DIR)



VERBOSE ?= 0
ifeq ($(VERBOSE), 1)
export QUITE :=
else
export QUITE := @
endif

# 一些旧的 make 不支持 file 函数，需要 make 的时候指定 LINK_AT=0 make
export LINK_AT ?= 1

# 表示下面的不是一个文件的名字，无论是否存在 all, clean, pre_build 这样的文件
# 还是要执行命令
# see: https://www.gnu.org/software/make/manual/html_node/Phony-Targets.html
.PHONY: all clean pre_build

# 不要使用 make 预设置的规则
# see: https://www.gnu.org/software/make/manual/html_node/Suffix-Rules.html
.SUFFIXES:

all: pre_build 
	make -f build/Makefile.mk

pre_build:
	$(info +CC build/genFileList.c)
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -D__LD__ -E -P build/genFileList.c -o build/fileList.mk


clean:
	$(QUITE) $(RM) $(OUT_ELF)
	$(QUITE) $(RM) $(BUILD_DIR)

