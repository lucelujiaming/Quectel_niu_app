#current path : ql-ol-sdk/ql-ol-extsdk/example/ql_network
CURR_DIR := $(shell pwd)
QL_ROOT_DIR := $(word 1, $(subst /ql-ol-extsdk, /ql-ol-extsdk,$(CURR_DIR)))
#QL_ROOT_DIR := /w/sdk/niu/EC25EFAR06A01M4G_OCPU_NIU_BETA1008_SDK/ql-ol-sdk
QL_SDK_DIR := $(QL_ROOT_DIR)/ql-ol-extsdk
QL_SYSROOT_DIR := $(QL_ROOT_DIR)/ql-ol-crosstool/sysroots/armv7a-vfp-neon-oe-linux-gnueabi
QL_TOOLCHAIN_DIR := $(QL_ROOT_DIR)/ql-ol-crosstool/sysroots/x86_64-oesdk-linux

QL_CROSSTOOLS_PREFIX := $(QL_TOOLCHAIN_DIR)/usr/bin/arm-oe-linux-gnueabi/arm-oe-linux-gnueabi-

QL_MODULE_DIR := $(word 1, $(subst /niu_app,/niu_app ,$(CURR_DIR)))

export CC:=$(QL_CROSSTOOLS_PREFIX)gcc
export CXX:=$(QL_CROSSTOOLS_PREFIX)g++
export CPP:=$(QL_CROSSTOOLS_PREFIX)gcc
export AS:=$(QL_CROSSTOOLS_PREFIX)as
export LD:=$(QL_CROSSTOOLS_PREFIX)ld
export STRIP:=$(QL_CROSSTOOLS_PREFIX)strip
export RANLIB:=$(QL_CROSSTOOLS_PREFIX)ranlib
export OBJCOPY:=$(QL_CROSSTOOLS_PREFIX)objcopy
export OBJDUMP:=$(QL_CROSSTOOLS_PREFIX)objdump
export AR:=$(QL_CROSSTOOLS_PREFIX)ar
export NM:=$(QL_CROSSTOOLS_PREFIX)nm

#vpath %.c ../../common ../../niu_mgmt ../../niu_common/aes ../../niu_common/cwpack/src ../../niu_common/mqtt/MQTTClient ../../niu_common/mqtt/MQTTPacket
#vpath %.h ../../common ../../niu_mgmt ../../niu_common/aes ../../niu_common/cwpack/src ../../niu_common/mqtt/MQTTClient ../../niu_common/mqtt/MQTTPacket
QL_SDK_FLAGS := -march=armv7-a -mfloat-abi=softfp -mfpu=neon --sysroot=$(QL_SYSROOT_DIR)

CFLAGS := $(QL_SDK_FLAGS) -DQL_LOG_TAG="\"niu_mgmt\"" 
CFLAGS += -I. -I../../common -I$(QL_SDK_DIR)/include             \
	-I$(QL_SYSROOT_DIR)/usr/include               \
	-I$(QL_SYSROOT_DIR)/usr/include/data          \
	-I$(QL_SYSROOT_DIR)/usr/include/dsutils       \
	-I$(QL_SYSROOT_DIR)/usr/include/qmi           \
	-I$(QL_SYSROOT_DIR)/usr/include/qmi-framework \



# For debug
CFLAGS += -rdynamic -funwind-tables -DQL_DEBUG_MEM_TRACK -DQL_DEBUG_MUTEX_TRACK


#CFLAGS += -Werror -Wundef -finline-functions -finline-limit=64 
#-Wall

QL_SYS_LIBS = -lqmi -lqmi_common_so -lqmi_cci -lqmiservices -ldsi_netctrl -ldsutils -lmcm -lql_common_api -lql_peripheral -lql_sys_log

LDFLAGS := $(QL_SDK_FLAGS)
LDFLAGS += -L./                          \
		   -L$(QL_SDK_DIR)/lib           \
           -L$(QL_SYSROOT_DIR)/usr/lib   \
           -L$(QL_SYSROOT_DIR)/lib       \
		   $(QL_SYS_LIBS)                \
           -lpthread


QL_TARGET_EXE = niu_daemon
QL_TARGET_OBJS = niu_daemon.o

QL_OBJS_DIR = .objs
QL_TARGET_OBJS:=$(addprefix $(QL_OBJS_DIR)/, $(QL_TARGET_OBJS))

#QL_TARGET_OBJS += thread_main.o

all: precfg $(QL_TARGET_EXE)
.PHONY: all

precfg:
	@mkdir -p $(QL_OBJS_DIR)

$(QL_OBJS_DIR)/%.o:%.c
	$(CC) -o $@ -c $^ $(CFLAGS)

$(QL_TARGET_EXE): $(QL_TARGET_OBJS)
	$(CC) -o $@ $(QL_TARGET_OBJS) $(LDFLAGS)
	$(OBJDUMP) -D $@ > $@.objdump

clean:
	rm -rf $(QL_TARGET_EXE) 
	rm -rf $(QL_TARGET_EXE).objdump 
	rm -rf $(QL_OBJS_DIR)
	rm -rf *.objdump 

.PHONY:checkmake
checkmake:  
	@echo -e "--------------------------------------------------------------------------------------"
	@echo -e "---------------------------------dump make env----------------------------------------"
	@echo -e "--------------------------------------------------------------------------------------"
	@echo -e "CURDIR            = ${CURR_DIR}"  
	@echo -e "ROOT_DIR          = ${QL_ROOT_DIR}"  
	@echo -e "SDK_DIR           = ${QL_SDK_DIR}"  
	@echo -e "SYSROOT_DIR       = ${QL_SYSROOT_DIR}"
	@echo -e "TOOLCHAIN_DIR     = ${QL_TOOLCHAIN_DIR}"
	@echo -e "CROSSTOOLS_PREFIX = ${QL_CROSSTOOLS_PREFIX}"
	@echo -e "CFLAGS            = ${CFLAGS}"
	@echo -e "LDFLAGS           = ${LDFLAGS}"
	@echo -e "QL_SYS_LIBS       = ${QL_SYS_LIBS}"
	@echo -e "QL_MODULE-DIR     = $(QL_MODULE_DIR)"

