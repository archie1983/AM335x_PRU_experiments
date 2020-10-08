#!/bin/bash

export PATH="/home/arturs/CoreLab_Work/ti-cgt-pru_2.3.3/bin/:$PATH"

make

sudo cp gen/PRU_experiment_ADC.out /home/arturs/CoreLab_Work/Yocto/NFS_sysroots/mppc/lib/firmware/pru/
