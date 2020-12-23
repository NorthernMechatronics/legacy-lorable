INCLUDES += -I$(AMBIQ_SDK)/mcu/apollo3
INCLUDES += -I$(AMBIQ_SDK)/mcu/apollo3/hal
INCLUDES += -I$(AMBIQ_SDK)/CMSIS/AmbiqMicro/Include
INCLUDES += -I$(AMBIQ_SDK)/CMSIS/ARM/Include
INCLUDES += -I$(AMBIQ_SDK)/utils
INCLUDES += -I$(FREERTOS)/Source/include
INCLUDES += -I$(NM_SDK)/features/FreeRTOS
INCLUDES += -I$(NM_SDK)/features/FreeRTOS-Plus-CLI

ifdef DEBUG
    LIBS += -lam_hal-dev
    LIBS += -lam_utils-dev
    LIBS += -lfreertos-dev
    LIBS += -lfreertos-cli-dev

    DEFINES += -DAM_DEBUG_PRINTF
else
    LIBS += -lam_hal
    LIBS += -lam_utils
    LIBS += -lfreertos
    LIBS += -lfreertos-cli
endif

LFLAGS += -Wl,--gc-sections,--entry,Reset_Handler,-Map,$(BUILDDIR)/$(TARGET).map

