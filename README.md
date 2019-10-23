This repo serves as a trace for my experiments with PRU cores on TI's Sitara AM335x processors. Specifically I used AM3359, but it should work on others too.

The master branch contains no code, because I created one branch for each experiment. At the moment there are two experiments and branches with the appropriate names:

can_experiment and uart_experiment

Please check out one of them and you will find source code in there along with a more in-depth README.md file.

At the moment can_experiment is the one that is more polished, while UART experiment is more basic.

All code runs on PRU cores of AM335x processors and require other dependencies to be compiled. Namely pru-software-support-package project and PRU code generation tools - compiler, but please see README.md in can_experiment for more details.

You will also need to compile a Linux distribution to run on your board. That distribution must include remoteproc and rpmsg drivers. Also the device tree has to assign resources to remoteproc driver. I used Yocto Sumo for that, but there are other ways.
