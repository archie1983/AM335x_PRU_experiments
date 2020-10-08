#!/bin/bash

# In addition to the environment variables being included on the next line, I also needed to change Makefile circa line 43:
# changing line:         $(ARM_CCT)/arm-phytec-linux-gnueabi-gcc -o $(GEN_DIR)/$(PROJ_NAME) $< $(LIBS)
# into:                  $(CC) -o $(GEN_DIR)/$(PROJ_NAME) $< $(LIBS)

. /home/arturs/CoreLab_Work/Yocto/SDKs/BSP-Yocto-AM335x-PD19.1.1/environment-setup-cortexa8hf-neon-phytec-linux-gnueabi

make all

sudo cp gen/pru_adc_userspace /home/arturs/CoreLab_Work/Yocto/NFS_sysroots/mppc/home/root/
