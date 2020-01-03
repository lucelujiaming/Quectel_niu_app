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

vpath %.c ../common ../niu_common/aes ../niu_common/cwpack/src ../niu_common/mqtt/MQTTClient ../niu_common/mqtt/MQTTPacket ../niu_daemon 
vpath %.h ../common ../niu_common/aes ../niu_common/cwpack/src ../niu_common/mqtt/MQTTClient ../niu_common/mqtt/MQTTPacket ../niu_common/libmga 
QL_SDK_FLAGS := -march=armv7-a -mfloat-abi=softfp -mfpu=neon --sysroot=$(QL_SYSROOT_DIR)

CFLAGS := $(QL_SDK_FLAGS) -DQL_LOG_TAG="\"niu_mgmt\""
CFLAGS += -I. -I../common -I$(QL_SDK_DIR)/include             \
	-I$(QL_SYSROOT_DIR)/usr/include               \
	-I$(QL_SYSROOT_DIR)/usr/include/data          \
	-I$(QL_SYSROOT_DIR)/usr/include/dsutils       \
	-I$(QL_SYSROOT_DIR)/usr/include/qmi           \
	-I$(QL_SYSROOT_DIR)/usr/include/qmi-framework \
	-I$(QL_MODULE_DIR)/niu_common/aes \
	-I$(QL_MODULE_DIR)/niu_common/cwpack/src \
	-I$(QL_MODULE_DIR)/niu_common/mqtt/MQTTClient \
	-I$(QL_MODULE_DIR)/niu_common/libmga			\
	-I$(QL_MODULE_DIR)/niu_common/libmga/mbedtls/include			\
	-I$(QL_MODULE_DIR)/niu_common/mqtt/MQTTPacket

export BUILD_TIMESTAMP="$(shell date +"%Y-%m-%d %k:%M:%S")"

# For debug
CFLAGS += -rdynamic -funwind-tables -DQL_DEBUG_MEM_TRACK -DQL_DEBUG_MUTEX_TRACK -DNIU_BUILD_TIMESTAMP=\"$(BUILD_TIMESTAMP)\"

CFLAGS += -Wall -Wundef -finline-functions -finline-limit=64
#CFLAGS += -Werror -Wundef -finline-functions -finline-limit=64 
#-Wall

QL_SYS_LIBS = -lqmi -lqmi_common_so -lqmi_cci -lqmiservices -ldsi_netctrl -ldsutils -lmcm -lql_common_api -lql_peripheral -lql_sys_log -lql_lib_audio -lql_mgmt_client -lrt

LDFLAGS := $(QL_SDK_FLAGS)
LDFLAGS += -L./                          \
		   -L$(QL_SDK_DIR)/lib           \
           -L$(QL_SYSROOT_DIR)/usr/lib   \
           -L$(QL_SYSROOT_DIR)/lib       \
		   $(QL_SYS_LIBS)                \
		   -L$(QL_MODULE_DIR)/niu_common/libmga/mbedtls/bin	\
		   -L$(QL_MODULE_DIR)/niu_common/libmga	\
           -lpthread -lm -lmbedtls -lmbedx509 -lmbedcrypto -lstdc++ -lmbedagps

QL_TARGET_EXE = niu_mgmtd 
QL_TARGET_OBJS = main.o ql_cmdq.o ql_tmrq.o ql_common.o ql_mem.o ql_mutex.o \
				 thread_cmd.o thread_tmr.o sys_core.o async_task.o ql_virtual_at.o \
				 MQTTClient.o MQTTLinux.o \
				 MQTTConnectClient.o MQTTConnectServer.o MQTTDeserializePublish.o \
				 MQTTFormat.o MQTTPacket.o MQTTSerializePublish.o \
				 MQTTSubscribeClient.o MQTTSubscribeServer.o\
				 MQTTUnsubscribeClient.o MQTTUnsubscribeServer.o \
				 niu_thread.o aes2.o cwpack.o \
				 cJSON.o upload_error_lbs.o niu_fota.o \
				 niu_data.o mcu_data.o utils.o niu_device.o niu_data_config.o \
				 niu_packet.o niu_data_pri.o gps_data.o nmea_parse.o  niu_entry.o niu_data_trace.o \
				 thread_main.o low_power.o niu_smt.o 
				 

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

