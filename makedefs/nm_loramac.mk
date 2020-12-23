ifndef NM_SDK
    $(error NM SDK location NM_SDK not defined)
endif

INCLUDES += -I$(LORAMAC)/src/mac
INCLUDES += -I$(LORAMAC)/src/mac/region
INCLUDES += -I$(LORAMAC)/src/boards
INCLUDES += -I$(LORAMAC)/src/radio
INCLUDES += -I$(LORAMAC)/src/system
INCLUDES += -I$(LORAMAC)/src/apps/LoRaMac/common
INCLUDES += -I$(LORAMAC)/src/apps/LoRaMac/common/LmHandler
INCLUDES += -I$(LORAMAC)/src/apps/LoRaMac/common/LmHandler/packages

ifdef DEBUG
    LIBS += -lloramac-dev
else
    LIBS += -lloramac
endif
