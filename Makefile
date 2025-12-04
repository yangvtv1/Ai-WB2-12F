#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := main
PROJECT_PATH := $(abspath .)
PROJECT_BOARD := evb
export PROJECT_PATH PROJECT_BOARD
#CONFIG_TOOLPREFIX :=
BLECONTROLLER_LIBS := std m0s1 m0s1s
BLECONTROLLER_LIB_DEFAULT := std
BLECONTROLLER_LIB_PREFIX := blecontroller_602_

-include ./proj_config.mk

ifeq ($(origin BL60X_SDK_PATH), undefined)
BL60X_SDK_PATH_GUESS ?= $(shell pwd)
#BL60X_SDK_PATH ?= $(BL60X_SDK_PATH_GUESS)/../../../../..
#BL60X_SDK_PATH ?= $(BL60X_SDK_PATH_GUESS)/../../../..
#BL60X_SDK_PATH ?= $(BL60X_SDK_PATH_GUESS)/../../..
BL60X_SDK_PATH ?= $(BL60X_SDK_PATH_GUESS)/../..
$(info ****** Please SET BL60X_SDK_PATH ******)
$(info ****** Trying SDK PATH [$(BL60X_SDK_PATH)])
endif

COMPONENTS_NETWORK := sntp dns_server bl60x_wifi_driver
COMPONENTS_BLSYS   := bltime blfdt blmtd blota bloop loopadc looprt loopset
COMPONENTS_VFS     := romfs cjson
COMPONENTS_MQTT    := axk_common tcp_transport http-parser axk_tls axk_mqtt

INCLUDE_COMPONENTS += freertos_riscv_ram bl602 bl602_std newlibc wifi wifi_manager wpa_supplicant bl_os_adapter wifi_hosal hosal mbedtls_lts lwip lwip_dhcpd vfs yloop utils cli dns_server netutils httpc blog blog_testc blcrypto_suite
INCLUDE_COMPONENTS += easyflash4 coredump axk_ota http-parser
INCLUDE_COMPONENTS += rfparam_adapter_tmp
INCLUDE_COMPONENTS += $(COMPONENTS_NETWORK)
INCLUDE_COMPONENTS += $(COMPONENTS_BLSYS)
INCLUDE_COMPONENTS += $(COMPONENTS_VFS)
INCLUDE_COMPONENTS += $(COMPONENTS_MQTT)
INCLUDE_COMPONENTS += $(PROJECT_NAME)
INCLUDE_COMPONENTS += axk_ota
ifeq ($(CONFIG_BT),1)
INCLUDE_COMPONENTS += $(COMPONENTS_BLE)
ifeq ($(CONFIG_BT_MESH),1)
INCLUDE_COMPONENTS += blemesh
endif
endif


ifeq ($(CONFIG_BLECONTROLLER_LIB),all)
COMPONENTS_BLECONTROLLER := $(addprefix $(BLECONTROLLER_LIB_PREFIX), $(BLECONTROLLER_LIBS))
else
ifeq ($(findstring $(CONFIG_BLECONTROLLER_LIB), $(BLECONTROLLER_LIBS)),)
COMPONENTS_BLECONTROLLER := $(addprefix $(BLECONTROLLER_LIB_PREFIX), $(BLECONTROLLER_LIB_DEFAULT))
else
COMPONENTS_BLECONTROLLER := $(addprefix $(BLECONTROLLER_LIB_PREFIX), $(CONFIG_BLECONTROLLER_LIB))
endif
endif

ifeq ($(CONFIG_BT_TL),1)
COMPONENTS_BLE     := $(COMPONENTS_BLECONTROLLER)
else
COMPONENTS_BLE     := $(COMPONENTS_BLECONTROLLER) blestack blecontroller
endif

# INCLUDE_COMPONENTS += component
EXTRA_COMPONENT_DIRS += $(PROJECT_PATH)/axk_ota

CFLAGS += -I$(PROJECT_PATH)/axk_ota

include $(BL60X_SDK_PATH)/make_scripts_riscv/project.mk













# # demo_ir
# #
# # This is a project Makefile. It is assumed the directory this Makefile resides in is a
# # project subdirectory.
# #

# PROJECT_NAME := demo_ir
# PROJECT_PATH := $(abspath .)
# PROJECT_BOARD := evb
# export PROJECT_PATH PROJECT_BOARD
# #CONFIG_TOOLPREFIX :=

# -include ./proj_config.mk

# ifeq ($(origin BL60X_SDK_PATH), undefined)
# BL60X_SDK_PATH_GUESS ?= $(shell pwd)
# BL60X_SDK_PATH ?= $(BL60X_SDK_PATH_GUESS)/../../..
# $(info ****** Please SET BL60X_SDK_PATH ******)
# $(info ****** Trying SDK PATH [$(BL60X_SDK_PATH)])
# endif

# COMPONENTS_BLSYS   := bltime blfdt blmtd bloop loopset looprt
# COMPONENTS_VFS     := romfs

# INCLUDE_COMPONENTS += freertos_riscv_ram
# INCLUDE_COMPONENTS += bl602 bl602_std
# INCLUDE_COMPONENTS += hosal mbedtls_lts lwip cli vfs yloop utils blog blog_testc newlibc
# INCLUDE_COMPONENTS += $(COMPONENTS_NETWORK)
# INCLUDE_COMPONENTS += $(COMPONENTS_BLSYS)
# INCLUDE_COMPONENTS += $(COMPONENTS_VFS)
# INCLUDE_COMPONENTS += $(PROJECT_NAME)

# include $(BL60X_SDK_PATH)/make_scripts_riscv/project.mk