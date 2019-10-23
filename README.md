This is an experiment that I've done with a TI Sitara AM3359 processor, it's PRU cores and it's DCAN peripheral.

Basically I've modified examples that were available in the TI's "PROCESSOR-SDK-LINUX-AM335X" package and created a solution that allows user to interact with the PRU from Linux user space and as a result drive the DCAN peripheral of the AM335x processor.

To compile my solution:

1) Git clone pru-software-support-package project: git://git.ti.com/pru-software-support-package/pru-software-support-package.git. I used a tag: v5.3.0, but there might be more up to date versions available at the time of reading this README.md.

2) Download from TI's site "PRU code generation tools - compiler" and install it somewhere. You will end up with a folder named "ti-cgt-pru_x.y.z", where x, y and z are version numbers. I used version 2.1.4, but at the time of writing this, newer versions are available.

3) Edit "compile.sh" script to reference path to your PRU code generation tools downloaded in step (2). Don't forget the "/bin/" at the end of the ti-pru-cgt package.

4) Edit "Makefile", variable PRU_CGT must point to your PRU code generation tools downloaded in step (2). This time DO NOT add the "/bin/" at the end.

5) Edit "Makefile", variable PRU_SUPPORT must point to your pru-software-support-package downloaded in step (1).

6) To compile, run: "./compile.sh"

7) To clean run: "make clean"


To run my solution:

1) Compile a Linux distribution to run on your board (either from linux-ti or mainline) and make sure that it compiles the "remoteproc" and "rpmsg" drivers. I compiled them as modules, but they can probably be compiled into the kernel too. For mainline Linux I needed to manually migrate these drivers in from linux-ti and of course enable them in kernel configuration. Also the device tree needs to be configured to actually assign resources to launch the drivers - this was a small stumbling block for me in mainline linux. Drop me an email if you want help with that, but that's likely to be specific to your hardware platform and kernel version. Also I used Yocto Sumo for that.

2) Launch your board with the compiled kernel.

3) transport the "gen/PRU_experiment_CAN.out" file to your target platform with your Linux kernel running and place it in "/lib/firmware/pru/" directory. You can also transport the "gen/button_led1.out" file and place it in the same folder. I used button_led1.out to have a simple feedback and debug mechanism with a blinking LED, but it shouldn't be neccessary.

4) Still on the target platform: in "/lib/firmware/" Create a symbolic link named "am335x-pru0-fw" that points to "/lib/firmware/pru/PRU_experiment_CAN.out". If you want to use button_led1.out, then create another symbolic link named "am335x-pru1-fw" that points to "/lib/firmware/pru/button_led1.out".

Example: ln -s /lib/firmware/pru/PRU_experiment_CAN.out /lib/firmware/am335x-pru0-fw

5) Now load and launch the PRU firmware:

echo 'am335x-pru0-fw' > /sys/class/remoteproc/remoteproc1/firmware
echo "start" > /sys/class/remoteproc/remoteproc1/state

If you use my button_led1.out, then launch that first:

echo 'am335x-pru1-fw' > /sys/class/remoteproc/remoteproc2/firmware
echo "start" > /sys/class/remoteproc/remoteproc2/state

6) Now it should all be running. You should have a file descriptor available in /dev/rpmsg_pru30, that can be used to communicate with the PRU and send and receive CAN packets. For example to send something to another board running these PRU codes, do this:

echo "12345678" > /dev/rpmsg_pru30

That will send a CAN bus packet containing "12345678" to the other node. The node IDs are already configured so that the packet actually gets accepted and handled. At the moment though the length of the data to send MUST be 8 bytes long, so "12345678" is fine, but "1234567" is not. I will probably NOT implement any kind of input sanitisation, because my goal was to demonstrate packet transfer between two CAN nodes and I've got to press on with other more interesting projects.

Then on the other board do this:

echo "show" > /dev/rpmsg_pru30
cat /dev/rpmsg_pru30

The "show" command will make the PRU read DCAN registers and extract the received data. Then reading "/dev/rpmsg_pru30" should show you the data, that you sent from the first node, so in this case: "12345678"



That's about all there is to say about this CAN experiment. There are a few more commands that I have implemented, please check them out if interested. Probably the biggest value of this all is in the "ae_dcan.c" file, where I show how to configure a DCAN peripheral on AM335x with extensive comments.
