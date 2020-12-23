ifndef NM_SDK
    $(error NM SDK location NM_SDK not defined)
endif

CORDIO_STACK    = $(CORDIO)/ble-host
CORDIO_PROFILES = $(CORDIO)/ble-profiles
CORDIO_WSF      = $(CORDIO)/wsf

INCLUDES += -I$(NM_SDK)/features/Cordio
INCLUDES += -I$(CORDIO_STACK)/include
INCLUDES += -I$(CORDIO_STACK)/sources/hci/ambiq
INCLUDES += -I$(CORDIO_STACK)/sources/hci/ambiq/apollo3

INCLUDES += -I$(CORDIO_STACK)/sources/sec/common
INCLUDES += -I$(CORDIO_STACK)/sources/sec/uecc
INCLUDES += -I$(UECC)

INCLUDES += -I$(CORDIO_STACK)/sources/stack/att
INCLUDES += -I$(CORDIO_STACK)/sources/stack/cfg
INCLUDES += -I$(CORDIO_STACK)/sources/stack/dm
INCLUDES += -I$(CORDIO_STACK)/sources/stack/hci
INCLUDES += -I$(CORDIO_STACK)/sources/stack/l2c
INCLUDES += -I$(CORDIO_STACK)/sources/stack/smp

INCLUDES += -I$(CORDIO_WSF)/include
INCLUDES += -I$(CORDIO_WSF)/sources
INCLUDES += -I$(CORDIO_WSF)/sources/port/freertos
INCLUDES += -I$(CORDIO_WSF)/sources/util

INCLUDES += -I$(CORDIO_PROFILES)/include/app
INCLUDES += -I$(CORDIO_PROFILES)/sources/services
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/anpc
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/atpc
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/atps
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/bas
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/blpc
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/blps
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/cpp
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/cscp
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/dis
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/fmpl
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/gap
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/gatt
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/glpc
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/glps
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/hid
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/hrpc
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/hrps
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/htpc
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/htps
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/include
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/paspc
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/plxpc
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/plxps
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/rscp
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/scpps
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/sensor
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/tipc
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/udsc
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/uribeacon
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/wdxc
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/wdxs
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/wpc

ifdef DEBUG
    LIBS += -lcordio-dev
    DEFINES += -DWSF_TRACE_ENABLED
else
    LIBS += -lcordio
endif


DEFINES += -DSEC_ECC_CFG=SEC_ECC_CFG_UECC
