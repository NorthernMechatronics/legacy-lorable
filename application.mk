#******************************************************************************
#
# Step 1
# Define the locations of the various SDKs and libraries.
#
#******************************************************************************
NM_SDK    ?= $(shell pwd)/../nmsdk
AMBIQ_SDK ?= $(shell pwd)/../AmbiqSuite-R2.5.1
FREERTOS  ?= $(shell pwd)/../FreeRTOS-Kernel/FreeRTOS/Source
CORDIO    ?= $(shell pwd)/../AmbiqSuite-R2.5.1/third_party/cordio
UECC      ?= $(shell pwd)/../AmbiqSuite-R2.5.1/third_party/uecc
LORAMAC   ?= $(shell pwd)/../LoRaMac-node

#******************************************************************************
#
# Step 2
# Specify the location of the board support package to be used.
#
#******************************************************************************
BSP_DIR := $(NM_SDK)/bsp/nm180100evb

#******************************************************************************
#
# Step 3
# Specify output target name and target version
#
# When the on-chip bootloader is enabled (either for wired update or over the
# air update), the target version must be the same as the version number
# specified in the info0 region or the device will not boot properly.
#
#******************************************************************************
TARGET_VERSION := 0x00

ifdef DEBUG
    TARGET      := lorable-dev
    TARGET_OTA  := lorable_ota-dev
    TARGET_WIRE := lorable_wire-dev
else
    TARGET      := lorable
    TARGET_OTA  := lorable_ota
    TARGET_WIRE := lorable_wire
endif

#******************************************************************************
#
# Step 4
# Include additional source, header, libraries or paths below.
#
# Examples:
#   INCLUDES += -Iadditional_include_path
#   VPATH    += additional_source_path
#   LIBS     += -ladditional_library
#******************************************************************************

#########################
#
# Ambiq BLE Over-The-Air
#
#########################
INCLUDES += -I$(AMBIQ_SDK)/ambiq_ble/services
INCLUDES += -I$(AMBIQ_SDK)/ambiq_ble/apps/barebone
INCLUDES += -I$(AMBIQ_SDK)/ambiq_ble/apps/amota
INCLUDES += -I$(AMBIQ_SDK)/ambiq_ble/profiles/amota
INCLUDES += -I$(AMBIQ_SDK)/bootloader

VPATH += $(AMBIQ_SDK)/ambiq_ble/services
VPATH += $(AMBIQ_SDK)/ambiq_ble/apps/barebone
VPATH += $(AMBIQ_SDK)/ambiq_ble/apps/amota
VPATH += $(AMBIQ_SDK)/ambiq_ble/profiles/amota
VPATH += $(AMBIQ_SDK)/bootloader

SRC += barebone_main.c
SRC += svc_amotas.c
SRC += amota_main.c
SRC += amotas_main.c
SRC += am_bootloader.c
SRC += am_multi_boot.c

#########################

INCLUDES += -I$(NM_SDK)/platform/console
INCLUDES += -I./soft-se

VPATH += $(NM_SDK)/platform/console
VPATH += ./soft-se

SRC += console_task.c
SRC += gpio_service.c
SRC += iom_service.c

DEFINES += -DSOFT_SE
SRC += aes.c
SRC += cmac.c
SRC += soft-se.c
SRC += soft-se-hal.c

SRC += ble.c
SRC += lorawan.c
SRC += lorawan_cli.c
SRC += application.c

SRC += amota_cli.c