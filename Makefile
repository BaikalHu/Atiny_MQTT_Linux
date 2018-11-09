##########################################################################################################################
# Cloud_STM32F429IGTx_FIRE GCC compiler Makefile
##########################################################################################################################

# ------------------------------------------------
# Generic Makefile (based on gcc)
# ------------------------------------------------

######################################
# target
######################################
TARGET = Huawei_LiteOS
######################################
# building variables
######################################
# debug build?
DEBUG = 1
# optimization
OPT = -Og


#######################################
# binaries
#######################################
PREFIX    =
CC        = $(PREFIX)gcc
AS        = $(PREFIX)gcc -x assembler-with-cpp
OBJCOPY   = $(PREFIX)objcopy
OBJDUMP   = $(PREFIX)objdump
AR        = $(PREFIX)ar
SZ        = $(PREFIX)size
LD        = $(PREFIX)ld
HEX       = $(OBJCOPY) -O ihex
BIN       = $(OBJCOPY) -O binary -S


PROJECTBASE = $(PWD)
override PROJECTBASE    := $(abspath $(PROJECTBASE))
TOP_DIR = $(PROJECTBASE)


#######################################
# paths
#######################################
# firmware library path
PERIFLIB_PATH =

# Build path
BUILD_DIR = build

######################################
# source
######################################
# C sources




MBEDTLS_SRC = \
        ${wildcard $(TOP_DIR)/mbedtls/mbedtls-2.6.0/library/*.c}
        C_SOURCES += $(MBEDTLS_SRC)

MBEDTLS_PORT_SRC = \
        ${wildcard $(TOP_DIR)/mbedtls/mbedtls_port/*.c}
        C_SOURCES += $(MBEDTLS_PORT_SRC)


ATINY_LOG_SRC = \
        ${wildcard $(TOP_DIR)/agent_tiny/lwm2m_client/atiny_log.c}
        C_SOURCES += $(ATINY_LOG_SRC)

MQTT_SRC = \
        ${wildcard $(TOP_DIR)/paho.mqtt.embedded-c-1.1.0/MQTTPacket/src/*.c} \
        $(TOP_DIR)/paho.mqtt.embedded-c-1.1.0/MQTTClient-C/src/MQTTClient.c \
        $(TOP_DIR)/paho.mqtt.embedded-c-1.1.0/MQTTClient-C/src/linux/MQTTLinux.c
        C_SOURCES += $(MQTT_SRC)

OS_ADAPTER_SRC = \
        ${wildcard $(TOP_DIR)/agent_tiny/osadapter/*.c}
        C_SOURCES += $(OS_ADAPTER_SRC)

ATINY_TINY_SRC = \
        ${wildcard $(TOP_DIR)/agent_tiny/mqtt_client/*.c}
        C_SOURCES += $(ATINY_TINY_SRC)

AGENT_DEMO_SRC = \
        ${wildcard $(TOP_DIR)/agent_tiny/examples/mqtt_demo/*.c}
        C_SOURCES += $(AGENT_DEMO_SRC)

CJSON_SRC = \
        ${wildcard $(TOP_DIR)/cJSON/cJSON*.c}
        C_SOURCES += $(CJSON_SRC)

USER_SRC =  \
        $(TOP_DIR)/main.c
        C_SOURCES += $(USER_SRC)


# ASM sources



######################################
# firmware library
######################################
PERIFLIB_SOURCES =


#######################################
# CFLAGS
#######################################
# cpu
# fpu
# float-abi
# mcu

# macros for gcc
# AS defines
AS_DEFS =

# C defines
C_DEFS =  \
        -D WITH_LINUX \
        -D LWM2M_LITTLE_ENDIAN \
        -D LWM2M_CLIENT_MODE \
        -D ATINY_DEBUG \
        -D USE_MBED_TLS \
        -D WITH_DTLS  \
        -D WITH_CA_BI \
        -D MBEDTLS_CONFIG_FILE=\"los_mbedtls_config_x509.h\" \
        -D LWIP_TIMEVAL_PRIVATE=0

        #-D MBEDTLS_DEBUG_C \

# AS includes
AS_INCLUDES =

# C includes

MBEDTLS_INC = \
        -I $(TOP_DIR)/mbedtls/mbedtls-2.6.0/include \
        -I $(TOP_DIR)/mbedtls/mbedtls-2.6.0/include/mbedtls
        C_INCLUDES += $(MBEDTLS_INC)

MBEDTLS_PORT_INC = \
        -I $(TOP_DIR)/mbedtls/mbedtls_port
        C_INCLUDES += $(MBEDTLS_PORT_INC)



MQTT_INC = \
	-I $(TOP_DIR)/paho.mqtt.embedded-c-1.1.0/MQTTPacket/src \
        -I $(TOP_DIR)/paho.mqtt.embedded-c-1.1.0/MQTTClient-C/src/linux \
        -I $(TOP_DIR)/paho.mqtt.embedded-c-1.1.0/MQTTClient-C/src
        C_INCLUDES += $(MQTT_INC)

OS_ADAPTER_INC = \
        -I $(TOP_DIR)/agent_tiny/osadapter
        C_INCLUDES += $(OS_ADAPTER_INC)


ATINY_TINY_INC = \
	-I $(TOP_DIR)/agent_tiny/mqtt_client \
        -I $(TOP_DIR)/agent_tiny/comm/include
        C_INCLUDES += $(ATINY_TINY_INC)

AGENT_DEMO_INC = \
        -I $(TOP_DIR)/agent_tiny/examples/mqtt_demo
        C_INCLUDES += $(AGENT_DEMO_INC)

CJSON_INC = \
        -I $(TOP_DIR)/cJSON
        C_INCLUDES += $(CJSON_INC)


# compile gcc flags
ASFLAGS = $(MCU) $(AS_DEFS) $(AS_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections

CFLAGS = $(MCU) $(C_DEFS) $(C_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections

ifeq ($(DEBUG), 1)
CFLAGS += -g -gdwarf-2
endif


# Generate dependency information
CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)"


#######################################
# LDFLAGS
#######################################
# link script

# libraries
LIBS = -lc -lm -lpthread
LIBDIR =
LDFLAGS = $(MCU) $(LIBDIR) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections

# default action: build all
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin


#######################################
# build the application
#######################################
# list of objects
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
# list of ASM program objects
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES_s:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES_s)))
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES_S:.S=.o)))
vpath %.S $(sort $(dir $(ASM_SOURCES_S)))

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	$(AS) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.S Makefile | $(BUILD_DIR)
	$(AS) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SZ) $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(HEX) $< $@

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(BIN) $< $@

$(BUILD_DIR):
	mkdir $@

#######################################
# clean up
#######################################
clean:
	-rm -fR .dep $(BUILD_DIR)

#######################################
# dependencies
#######################################
-include $(shell mkdir .dep 2>/dev/null) $(wildcard .dep/*)

# *** EOF ***
