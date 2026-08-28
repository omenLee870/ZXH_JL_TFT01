
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
MKDIR := mkdir_win -p
RM    := rm -rf

SYS_LIB_DIR := C:/JL/pi32/pi32v2-lib/r3
SYS_INC_DIR := C:/JL/pi32/pi32v2-include
EXT_CFLAGS  := # Windows 下不需要 -D__SHELL__
export PATH:=$(TOOL_DIR);$(PATH)

## 后处理脚本
FIXBAT          := tools/utils/fixbat.exe # 用于处理 utf8->gbk 编码问题
POST_SCRIPT     := apps/bsp/cpu/br33/post_build/download.bat
RUN_POST_SCRIPT := apps\bsp\cpu\br33\post_build\download.bat
else
# Linux 下工具链位置
TOOL_DIR := /opt/jieli/pi32v2/bin
CC    := clang
CXX   := clang
LD    := lto-wrapper
AR    := lto-ar
MKDIR := mkdir -p
RM    := rm -rf
export OBJDUMP := $(TOOL_DIR)/objdump
export OBJCOPY := $(TOOL_DIR)/objcopy
export OBJSIZEDUMP := $(TOOL_DIR)/objsizedump

SYS_LIB_DIR := $(TOOL_DIR)/../lib/r3
SYS_INC_DIR := $(TOOL_DIR)/../include
EXT_CFLAGS  := -D__SHELL__ # Linux 下需要这个保证正确处理 download.c
export PATH:=$(TOOL_DIR):$(PATH)

## 后处理脚本
FIXBAT          := touch # Linux下不需要处理 bat 编码问题
POST_SCRIPT     := apps/bsp/cpu/br33/post_build/download.sh
RUN_POST_SCRIPT := bash $(POST_SCRIPT)
endif

CC  := $(TOOL_DIR)/$(CC)
CXX := $(TOOL_DIR)/$(CXX)
LD  := $(TOOL_DIR)/$(LD)
AR  := $(TOOL_DIR)/$(AR)
# 输出文件设置
OUT_ELF   := apps/bsp/cpu/br33/post_build/p11.elf
OBJ_FILE  := $(OUT_ELF).objs.txt
# 编译路径设置
BUILD_DIR := objs

# 编译参数设置
CFLAGS := \
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
	-fms-extensions \
	-fdiscrete-bitfield-abi \


# C++额外的编译参数
CXXFLAGS :=


# 宏定义
DEFINES := \
	-DSUPPORT_MS_EXTENSIONS \
	-DCONFIG_RELEASE_ENABLE \
	-D__GCC_Q32S__ \
	-DPMU_SYSTEM \
	-DCPU_CORE_NUM=1 \
	-DCONFIG_CPU_BR33 \
	-DCONFIG_BR33_FPGA_VERIFY \
	-DCONFIG_UCOS_ENABLE \
	-DBREDR_PMC_CODE \
	-DBT_PMC_BT_FUN_OVERWRITE \
	-DBREDR_Q32F \
	-DCONFIG_NEW_BREDR_ENABLE \
	-DCONFIG_NEW_MODEM_ENABLE \
	-DCONFIG_SUPPORT_AES_CCM_FOR_EDR \
	-DCONFIG_SUPPORT_EX_TWS_ADJUST \


DEFINES += $(EXT_CFLAGS) # 额外的一些定义

# 头文件搜索路径
INCLUDES := \
	-Iapps \
	-Iapps/bsp/cpu/br33/config \
	-Iinclude/driver \
	-Iinclude/driver/cpu/br33 \
	-Iinclude/driver/cpu/br33/power/pmc \
	-Iinclude/system \
	-Iinclude/system/typedef \
	-Iinclude/system/cpu/br33 \
	-Iinclude/utils \
	-Iinclude/utils/generic \
	-Iapps/sensor \
	-Iapps/sensor/driver_vc_hr_11 \
	-Iapps/btctrler/soft_baseband \
	-Iapps/sensor/driver \
	-Iapps/sensor/driver/driver_vc_hr_11 \
	-Iapps/sensor/driver/driver_hr_vc30fx \
	-Iapps/sensor/hal \
	-Iapps/sensor/service \
	-I$(SYS_INC_DIR) \


# 需要编译的 .c 文件
c_SRC_FILES := \
	apps/battery/battery_pmc.c \
	apps/bsp/cpu/br33/bsp.c \
	apps/bsp/cpu/br33/config/lib_config.c \
	apps/bsp/cpu/br33/config/log_config.c \
	apps/bsp/cpu/br33/iic_api.c \
	apps/bsp/cpu/br33/iic_soft.c \
	apps/bsp/cpu/br33/ipc_spin_lock.c \
	apps/bsp/cpu/br33/pmc_pmu_msg.c \
	apps/bsp/cpu/br33/power/lib_power_config.c \
	apps/bsp/cpu/br33/power/power_app.c \
	apps/bsp/cpu/br33/power/power_config.c \
	apps/bsp/cpu/br33/setup.c \
	apps/bsp/cpu/br33/uart.c \
	apps/bsp/cpu/br33/vad_api.c \
	apps/bsp/cpu/br33/vad_develop.c \
	apps/bsp/cpu/demo/touch_demo.c \
	apps/btctrler/aes128.c \
	apps/btctrler/aes_hw.c \
	apps/btctrler/bt_low_power.c \
	apps/btctrler/bt_pmc_msg.c \
	apps/debug/debug.c \
	apps/main.c \
	apps/msg/msg.c \
	apps/sensor/algo_motion_config.c \
	apps/sensor/default.c \
	apps/sensor/driver/driver_hr_vc30fx/vc30fx_core.c \
	apps/sensor/driver/driver_hr_vc30fx/vc30fx_driver.c \
	apps/sensor/driver/driver_hr_vc30fx/vc30fx_hw_driver.c \
	apps/sensor/driver/driver_hr_vc30fx/vc30fx_user_app.c \
	apps/sensor/driver/driver_mmc5603/driver_mmc5603.c \
	apps/sensor/driver/driver_sc7a20/driver_sc7a20.c \
	apps/sensor/driver/driver_vc_hr_11/driver_vc_hr_11.c \
	apps/sensor/driver/driver_vc_hr_11/vcHr11Hci.c \
	apps/sensor/driver/sensor_driver.c \
	apps/sensor/hal/sensor_hal_acceler.c \
	apps/sensor/hal/sensor_hal_vc30fx.c \
	apps/sensor/hal/sensor_hal_vc_hr_11.c \
	apps/sensor/hal/sensor_hub.c \
	apps/sensor/health_manager.c \
	apps/sensor/module/shm_algo_gsensor.c \
	apps/sensor/module/shm_algo_heart_rate.c \
	apps/sensor/module/shm_algo_spo2.c \
	apps/sensor/module/shm_det_wrist.c \
	apps/sensor/service/sensor_service.c \
	apps/sensor/service/sensor_service_hr_vc30fx.c \
	apps/sensor/service/sensor_service_motion.c \
	apps/sensor/service/sensor_service_vc_hr_spo2.c \
	apps/sensor/service/sensor_service_wrist_tilt.c \


# 需要编译的 .S 文件
S_SRC_FILES :=


# 需要编译的 .s 文件
s_SRC_FILES :=


# 需要编译的 .cpp 文件
cpp_SRC_FILES :=


# 需要编译的 .cc 文件
cc_SRC_FILES :=


# 需要编译的 .cxx 文件
cxx_SRC_FILES :=


# 链接参数
LFLAGS := \
	--plugin-opt=-pi32v2-always-use-itblock=false \
	--plugin-opt=-enable-ipra=true \
	--plugin-opt=-pi32v2-merge-max-offset=4096 \
	--plugin-opt=-pi32v2-enable-simd=true \
	--plugin-opt=mcpu=r3 \
	--plugin-opt=-global-merge-on-const \
	--plugin-opt=-inline-threshold=5 \
	--plugin-opt=-inline-max-allocated-size=32 \
	--plugin-opt=-inline-normal-into-special-section=true \
	--plugin-opt=-dont-used-symbol-list=malloc,free,sprintf,printf,puts,putchar \
	--plugin-opt=save-temps \
	--plugin-opt=-pi32v2-enable-rep-memop \
	--plugin-opt=-warn-stack-size=1024 \
	--sort-common \
	--dont-complain-call-overflow \
	--plugin-opt=-floating-point-may-trap=true \
	--gc-sections \
	--plugin-opt=-pi32v2-csync-before-rti=true \
	--start-group \
	apps/bsp/cpu/br33/liba/bt/watch/btctrler_pmc.a \
	apps/bsp/cpu/br33/liba/system.a \
	apps/bsp/cpu/br33/liba/cpu.a \
	apps/bsp/cpu/br33/liba/utils.a \
	apps/bsp/cpu/br33/liba/jl_motion_v0.a \
	--end-group \
	-Tapps/bsp/cpu/br33/p11.ld \
	-M=apps/bsp/cpu/br33/post_build/p11.map \
	--plugin-opt=mcpu=r3 \
	--plugin-opt=-mattr=+fprev1 \


LIBPATHS := \
	-L$(SYS_LIB_DIR) \


LIBS := \
	$(SYS_LIB_DIR)/libm.a \
	$(SYS_LIB_DIR)/libc.a \
	$(SYS_LIB_DIR)/libm.a \
	$(SYS_LIB_DIR)/libcompiler-rt.a \



c_OBJS    := $(c_SRC_FILES:%.c=%.c.o)
S_OBJS    := $(S_SRC_FILES:%.S=%.S.o)
s_OBJS    := $(s_SRC_FILES:%.s=%.s.o)
cpp_OBJS  := $(cpp_SRC_FILES:%.cpp=%.cpp.o)
cxx_OBJS  := $(cxx_SRC_FILES:%.cxx=%.cxx.o)
cc_OBJS   := $(cc_SRC_FILES:%.cc=%.cc.o)

OBJS      := $(c_OBJS) $(S_OBJS) $(s_OBJS) $(cpp_OBJS) $(cxx_OBJS) $(cc_OBJS)
DEP_FILES := $(OBJS:%.o=%.d)


OBJS      := $(addprefix $(BUILD_DIR)/, $(OBJS))
DEP_FILES := $(addprefix $(BUILD_DIR)/, $(DEP_FILES))


VERBOSE ?= 0
ifeq ($(VERBOSE), 1)
QUITE :=
else
QUITE := @
endif

# 一些旧的 make 不支持 file 函数，需要 make 的时候指定 LINK_AT=0 make
LINK_AT ?= 1

# 表示下面的不是一个文件的名字，无论是否存在 all, clean, pre_build 这样的文件
# 还是要执行命令
# see: https://www.gnu.org/software/make/manual/html_node/Phony-Targets.html
.PHONY: all clean pre_build

# 不要使用 make 预设置的规则
# see: https://www.gnu.org/software/make/manual/html_node/Suffix-Rules.html
.SUFFIXES:

all: pre_build $(OUT_ELF)
	$(info +POST-BUILD)
	$(QUITE) $(RUN_POST_SCRIPT) p11

pre_build:
	$(info +PRE-BUILD)
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -D__LD__ -E -P apps/bsp/cpu/br33/p11_used_list.c -o apps/bsp/cpu/br33/p11_used_list.used
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -D__LD__ -E -P apps/bsp/cpu/br33/p11_ld.c -o apps/bsp/cpu/br33/p11.ld
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -D__LD__ -E -P apps/bsp/cpu/br33/post_build/download_wid.c -o $(POST_SCRIPT)
	$(QUITE) $(FIXBAT) $(POST_SCRIPT)

clean:
	$(QUITE) $(RM) $(OUT_ELF)
	$(QUITE) $(RM) $(BUILD_DIR)



ifeq ($(LINK_AT), 1)
$(OUT_ELF): $(OBJS)
	$(info +LINK $@)
	$(shell $(MKDIR) $(@D))
	$(file >$(OBJ_FILE), $(OBJS))
	$(QUITE) $(LD) -o $(OUT_ELF) @$(OBJ_FILE) $(LFLAGS) $(LIBPATHS) $(LIBS)
else
$(OUT_ELF): $(OBJS)
	$(info +LINK $@)
	$(shell $(MKDIR) $(@D))
	$(QUITE) $(LD) -o $(OUT_ELF) $(OBJS) $(LFLAGS) $(LIBPATHS) $(LIBS)
endif


$(BUILD_DIR)/%.c.o : %.c
	$(info +CC $<)
	$(QUITE) $(MKDIR) $(@D)
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -MMD -MF $(@:.o=.d) -c $< -o $@

$(BUILD_DIR)/%.S.o : %.S
	$(info +AS $<)
	$(QUITE) $(MKDIR) $(@D)
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -MMD -MF $(@:.o=.d) -c $< -o $@

$(BUILD_DIR)/%.s.o : %.s
	$(info +AS $<)
	$(QUITE) $(MKDIR) $(@D)
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -MMD -MF $(@:.o=.d) -c $< -o $@

$(BUILD_DIR)/%.cpp.o : %.cpp
	$(info +CXX $<)
	$(QUITE) $(MKDIR) $(@D)
	$(QUITE) $(CXX) $(CXXFLAGS) $(CFLAGS) $(DEFINES) $(INCLUDES) -MMD -MF $(@:.o=.d) -c $< -o $@

$(BUILD_DIR)/%.cxx.o : %.cxx
	$(info +CXX $<)
	$(QUITE) $(MKDIR) $(@D)
	$(QUITE) $(CXX) $(CXXFLAGS) $(CFLAGS) $(DEFINES) $(INCLUDES) -MMD -MF $(@:.o=.d) -c $< -o $@

$(BUILD_DIR)/%.cc.o : %.cc
	$(info +CXX $<)
	$(QUITE) $(MKDIR) $(@D)
	$(QUITE) $(CXX) $(CXXFLAGS) $(CFLAGS) $(DEFINES) $(INCLUDES) -MMD -MF $(@:.o=.d) -c $< -o $@

-include $(DEP_FILES)
