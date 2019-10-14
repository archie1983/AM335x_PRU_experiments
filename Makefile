## 
# Copyright (c) 2016 Zubeen Tolani <ZeekHuge - zeekhuge@gmail.com>
# Copyright (c) 2017 Texas Instruments - Jason Kridner <jdk@ti.com>
#
PROJ_NAME=ae
#PROJ_NAME=$1
PRU0_PROJ=PRU_experiment_CAN
PRU1_PROJ=button_led1

# PRU_CGT environment variable must point to the TI PRU compiler directory.
# PRU_SUPPORT points to pru-software-support-package
# Both are set in setup.sh

#PRU_CGT:=/usr/share/ti/cgt-pru
PRU_CGT:=/home/arturs/AE_Custom_Linux_kernels_Yocto/non-yocto_build_envs/ti-cgt-pru_2.1.4
#PRU_SUPPORT:=/usr/lib/ti/pru-software-support-package
PRU_SUPPORT:=/home/arturs/AE_Custom_Linux_kernels_Yocto/non-yocto_build_envs/PRU_experiments_AM335x/pru-software-support-package

LINKER_COMMAND_FILE=./AM335x_PRU.cmd
LIBS=--library=$(PRU_SUPPORT)/lib/rpmsg_lib.lib
INCLUDE=--include_path=$(PRU_SUPPORT)/include --include_path=$(PRU_SUPPORT)/include/am335x
STACK_SIZE=0x100
HEAP_SIZE=0x100

CFLAGS=-v3 -O2 --display_error_number --endian=little --hardware_mac=on --obj_directory=$(GEN_DIR) --pp_directory=$(GEN_DIR) -ppd -ppa --asm_listing --c_src_interlist # --absolute_listing
LFLAGS=--reread_libs --warn_sections --stack_size=$(STACK_SIZE) --heap_size=$(HEAP_SIZE) -m $(GEN_DIR)/file.map

GEN_DIR=gen

PRU0_FW		=$(GEN_DIR)/$(PRU0_PROJ).out
PRU1_FW         =$(GEN_DIR)/$(PRU1_PROJ).out

# AE extra modules
AE_DCAN = $(GEN_DIR)/ae_dcan
AE_RPMSG = ae_rpmsg

# -----------------------------------------------------
# Variable to edit in the makefile
#
# add the required firmwares to TARGETS
# AE: Pay attention how AE extra modules need to come in first, so that when we get to compile and link PRU0_FW,
# AE: we already have the extra modules available.
TARGETS		= $(AE_DCAN) $(PRU0_FW) $(PRU1_FW) $(GEN_DIR)/decay95.out

#------------------------------------------------------

.PHONY: all
all: $(TARGETS)
	@echo '-	Generated firmwares are : $^'

$(GEN_DIR)/decay95.out:  $(GEN_DIR)/decay95.obj
	@echo 'LD	$^' 
	@lnkpru -i$(PRU_CGT)/lib -i$(PRU_CGT)/include $(LFLAGS) -o $@ $^  $(LINKER_COMMAND_FILE) --library=libc.a $(LIBS) $^

$(GEN_DIR)/decay95.obj: $(PRU1_PROJ).c 
	@mkdir -p $(GEN_DIR)
	@echo 'CC	$<'
	@clpru --define=DECAY_RATE=95 --include_path=$(PRU_CGT)/include $(INCLUDE) $(CFLAGS) -fe $@ $<

# AE: Adding an entry for dcan functionality. It's actually gen/dcan so that it puts the .o file into the gen directory.
# AE: via the $@.o part in the command arguments.
$(AE_DCAN): ae_dcan.c
	@mkdir -p $(GEN_DIR)
	@echo 'CC    $@   $<'
	@clpru --define=DECAY_RATE=95 --include_path=$(PRU_CGT)/include $(INCLUDE) $(CFLAGS) -fe $@.o $<

# AE: Adding the object files of AE extra modules (e.g.: $(GEN_DIR)/ae_dcan.o) to be linked in.
$(PRU0_FW): $(GEN_DIR)/$(PRU0_PROJ).obj
	@echo 'LD	$^' 
	@lnkpru -i$(PRU_CGT)/lib -i$(PRU_CGT)/include $(LFLAGS) -o $@ $^ $(AE_DCAN).o $(LINKER_COMMAND_FILE) --library=libc.a $(LIBS) $^

$(PRU1_FW): $(GEN_DIR)/$(PRU1_PROJ).obj
	@echo 'LD       $^'
	@lnkpru -i$(PRU_CGT)/lib -i$(PRU_CGT)/include $(LFLAGS) -o $@ $^  $(LINKER_COMMAND_FILE) --library=libc.a $(LIBS) $^

# AE: ae_rpmsg module (source here is referenced as $(AE_RPMSG).c) has to be compiled together with the main
# AE: firmware module because they both need the resource table (and other resources) and if we compile them
# AE: separately, then linking later fails with a resource redefinition error.
$(GEN_DIR)/$(PRU0_PROJ).obj: $(PRU0_PROJ).c 
	@mkdir -p $(GEN_DIR)
	@echo 'CC	$<'
	@clpru --include_path=$(PRU_CGT)/include $(INCLUDE) $(CFLAGS) -fe $@ $< $(AE_RPMSG).c

$(GEN_DIR)/$(PRU1_PROJ).obj: $(PRU1_PROJ).c
	@mkdir -p $(GEN_DIR)
	@echo 'CC       $<'
	@clpru --include_path=$(PRU_CGT)/include $(INCLUDE) $(CFLAGS) -fe $@ $<

.PHONY: install run run95

install:
	@echo '-	copying firmware file $(PRU0_FW) to /lib/firmware/am335x-pru0-fw'
	@cp $(PRU0_FW) /lib/firmware/am335x-pru0-fw

run: install
	@echo '-	rebooting pru core 0'
	$(shell echo "4a334000.pru0" > /sys/bus/platform/drivers/pru-rproc/unbind 2> /dev/null)
	$(shell echo "4a334000.pru0" > /sys/bus/platform/drivers/pru-rproc/bind)
	@echo "-	pru core 0 is now loaded with $(PRU0_FW)"

run95:
	@echo '-	copying firmware file $(GEN_DIR)/decay95.out to /lib/firmware/am335x-pru0-fw'
	@cp $(GEN_DIR)/decay95.out /lib/firmware/am335x-pru0-fw
	@echo '-	rebooting pru core 0'
	$(shell echo "4a334000.pru0" > /sys/bus/platform/drivers/pru-rproc/unbind 2> /dev/null)
	$(shell echo "4a334000.pru0" > /sys/bus/platform/drivers/pru-rproc/bind)
	@echo "-	pru core 0 is now loaded with $(GEN_DIR)/decay95.out"

.PHONY: clean
# Need to make sure that AE extra modules (e.g. ae_dcan) are taken care of and their asm files get deleted
clean:
	@echo 'CLEAN	.'
	@rm -rf $(GEN_DIR) $(PRU0_PROJ).asm $(PRU1_PROJ).asm ae_dcan.asm ae_rpmsg.asm
