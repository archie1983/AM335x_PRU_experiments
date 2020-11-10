# CC environment variable must be set using the environment script of our Yocto build SDK,
# e.g.: /home/arturs/CoreLab_Work/Yocto/SDKs/BSP-Yocto-AM335x-PD19.1.1/environment-setup-cortexa8hf-neon-phytec-linux-gnueabi
#
# in the end CC environment variable should be something like this:
# CC=arm-phytec-linux-gnueabi-gcc  -march=armv7-a -marm -mfpu=neon -mfloat-abi=hard -mcpu=cortex-a8 --sysroot=/home/arturs/CoreLab_Work/Yocto/SDKs/BSP-Yocto-AM335x-PD19.1.1/sysroots/cortexa8hf-neon-phytec-linux-gnueabi
ifndef CC
define ERROR_BODY

************************************************************
CC cross-compiler environment variable is not set.
Please source your Yocto build SDK environment variables' setting script,
e.g.: /home/arturs/CoreLab_Work/Yocto/SDKs/BSP-Yocto-AM335x-PD19.1.1/environment-setup-cortexa8hf-neon-phytec-linux-gnueabi
************************************************************

endef
$(error $(ERROR_BODY))
endif

MKFILE_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
CURRENT_DIR := $(notdir $(patsubst %/,%,$(dir $(MKFILE_PATH))))
PROJ_NAME=$(CURRENT_DIR)
LIBS=-lm -lpng
GEN_DIR=gen
SOURCES=pru_adc_userspace.c
#H_INCLUDES=-isystem ${HOME}/CoreLab_Work/ti-processor-sdk-linux-am335x-evm-06.03.00.106/linux-devkit/sysroots/armv7at2hf-neon-linux-gnueabi/usr/include
H_INCLUDES=-isystem ${HOME}/CoreLab_Work/Yocto/SDKs/BSP-Yocto-AM335x-PD19.1.1/sysroots/cortexa8hf-neon-phytec-linux-gnueabi/usr/include

all: printStart $(GEN_DIR)/$(PROJ_NAME) printEnd

printStart:
	@echo ''
	@echo '************************************************************'
	@echo 'Building project: $(PROJ_NAME)'

printEnd:
	@echo ''
	@echo 'Finished building project: $(PROJ_NAME)'
	@echo '************************************************************'
	@echo ''

# Invokes the compiler on all c files in the directory
$(GEN_DIR)/$(PROJ_NAME): $(SOURCES)
	@mkdir -p $(GEN_DIR)
	@echo ''
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	#$(ARM_CCT)/arm-linux-gnueabihf-gcc $(H_INCLUDES) -o $(GEN_DIR)/$(PROJ_NAME) $< $(LIBS)
	#$(ARM_CCT)/arm-phytec-linux-gnueabi-gcc -o $(GEN_DIR)/$(PROJ_NAME) $< $(LIBS)
	#$(ARM_CCT)/arm-phytec-linux-gnueabi-gcc $(H_INCLUDES) -o $(GEN_DIR)/$(PROJ_NAME) $< $(LIBS)
	$(CC) -o $(GEN_DIR)/$(PROJ_NAME) $< $(LIBS)
	@echo 'Finished building target: $@'
	@echo ''
	@echo 'Output files can be found in the "$(GEN_DIR)" directory'

.PHONY: all clean

# Remove the $(GEN_DIR) directory
clean:
	@echo ''
	@echo '************************************************************'
	@echo 'Cleaning project: $(PROJ_NAME)'
	@echo ''
	@echo 'Removing files in the "$(GEN_DIR)" directory'
	@rm -rf $(GEN_DIR)
	@echo ''
	@echo 'Finished cleaning project: $(PROJ_NAME)'
	@echo '************************************************************'
	@echo ''
