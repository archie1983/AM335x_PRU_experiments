/*
 * Copyright (C) 2018 Texas Instruments Incorporated - http://www.ti.com/
 *
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 	* Redistributions of source code must retain the above copyright
 * 	  notice, this list of conditions and the following disclaimer.
 *
 * 	* Redistributions in binary form must reproduce the above copyright
 * 	  notice, this list of conditions and the following disclaimer in the
 * 	  documentation and/or other materials provided with the
 * 	  distribution.
 *
 * 	* Neither the name of Texas Instruments Incorporated nor the names of
 * 	  its contributors may be used to endorse or promote products derived
 * 	  from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include <poll.h>
#include <inttypes.h>
#include <unistd.h>
#include <errno.h>

#define RPMSG_MESSAGE_SIZE           496

/**
 * This is the upper limit of elements that we want to unload into the buffer
 * when we empty the queue, because we can't send more than this to the user
 * space. The limit of bytes is RPMSG_MESSAGE_SIZE, but we're sending uint16_t
 * values.
 */
#define MAX_UNLOAD_CNT_FROM_QUEUE RPMSG_MESSAGE_SIZE / 2

//char payload[RPMSG_MESSAGE_SIZE];

uint16_t value_buf_from_pru[MAX_UNLOAD_CNT_FROM_QUEUE];

/*
 * We'll send this command when we want the ADC channels to be read by the PRU.
 */
const char* readCmd = "read";

/* shared_struct is used to pass data between ARM and PRU */
typedef struct shared_struct{
	uint16_t voltage;
	uint16_t channel;
} shared_struct;

char charVoltageVal[] = "0.0000";

static char * convertVoltage(const uint16_t rawVoltage);
static uint16_t readADCchannel(const char *adcChannel);
static size_t readADCValuesForSentinel(uint16_t* value_buf_from_pru);

int main(int argc, char *argv[])
{
	char opt;
	char *charVoltage;

	/* Parse the command line options  */
	opt = getopt(argc, argv, "cr:");

	int argIndex = 0;
	if (opt == 'c') {
		/* ADC channel number is stored in argv[2] */
		if (*argv[2] == '5' || *argv[2] == '6' || *argv[2] == '7') {
			printf("Reading voltage at ADC Channel: %s NO LONGER SUPPORTED!!!\n", argv[2]);

			/* readADCchannel receives raw ADC voltage value from PRU */
			/* convertVoltage converts voltage to a decimal value for display */
			//charVoltage = convertVoltage(readADCchannel(argv[2]));
			//printf("Voltage on ADC Channel %s is %sV\n",argv[2],charVoltage);
			return EXIT_FAILURE;
		} else {
			printf("(ADC Channel should = 5, 6, or 7)\n");
			return EXIT_FAILURE;
		}
	} else if (opt == 'r') {
		printf("2Reading V and I in Sentinel context...\n");
		printf("AAAA\n");
		size_t cnt_returned_values = readADCValuesForSentinel(value_buf_from_pru);
		// do {
		// 	cnt_returned_values--;
		// 	printf("CNT=%d , V=%d\n", cnt_returned_values, value_buf_from_pru[cnt_returned_values]);
		// } while (cnt_returned_values > 0);

		printf("CNT=%d\n", cnt_returned_values);

	} else {
		printf("ERROR: Command not recognized: %s", argv[0]);
		for (argIndex = 1; argv[argIndex]; argIndex++) {
			printf(" %s", argv[argIndex]);
		}
		printf("\n");
		printf("usage: ./pru_adc_userspace -c <ChannelNum>\n");
		printf("Where ChannelNum = 5 OR 6 OR 7\n");

		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

/*
 * readADCValuesForSentinel sends the "read" command to PRU0 by writing to rpmsg_pru30
 * and receives two ADC channel readings (16 bit each, but packed as one 32-bit value).
 */
static size_t readADCValuesForSentinel(uint16_t* value_buf_from_pru)
{
	printf("HERE00-1\n");
	/* use character device /dev/rpmsg_pru30 */
	char outputFilename[] = "/dev/rpmsg_pru30";
printf("HERE00-2\n");
	/* test that /dev/rpmsg_pru30 exists */
	FILE *ofp;
	uint32_t returnedData;
	ofp = fopen(outputFilename, "r");
printf("HERE00-3\n");
	if (ofp == NULL) {

		printf("/dev/rpmsg_pru30 could not be opened. \n");
		printf("Trying to initialize PRU using sysfs interface.\n");

		FILE *sysfs_node;
		char firmware[] = "/sys/class/remoteproc/remoteproc1/firmware";
		char firmwareName[] = "PRU_experiment_ADC.out";
		sysfs_node = fopen(firmware, "r+");
		if (sysfs_node == NULL) {
			printf("cannot open firmware sysfs_node");
			return EXIT_FAILURE;
		}

		fwrite(&firmwareName, sizeof(uint8_t), sizeof(firmwareName), sysfs_node);
		fclose(sysfs_node);

		char pruState[] = "/sys/class/remoteproc/remoteproc1/state";
		char start[] = "start";
		sysfs_node = fopen(pruState, "r+");
		if (sysfs_node == NULL) {
			printf("cannot open state sysfs_node");
			return EXIT_FAILURE;
		}
		fwrite(&start, sizeof(uint8_t), sizeof(start), sysfs_node);
		fclose(sysfs_node);

		/* give RPMSG time to initialize */
		sleep(3);

		ofp = fopen(outputFilename, "r");

		if (ofp == NULL) {
			printf("ERROR: Could not open /dev/rpmsg_pru30\n");
			exit(EXIT_FAILURE);
		}
	}
printf("HERE0\n");
	/* now we know that the character device exists */
	fclose(ofp);
printf("HERE0-1\n");
	/* open the character device for read/write */
	struct pollfd pfds[1];
	pfds[0].fd = open(outputFilename, O_RDWR);
	if (pfds[0].fd < 0) {
		printf("failed to open /dev/rpmsg_pru30");
		exit(EXIT_FAILURE);
	}
printf("HERE0-2\n");
	/* write the read command to the PRU. */
	size_t result = write(pfds[0].fd, readCmd, sizeof(readCmd));
printf("HERE0-3\n");
	/* poll for the received message */
	pfds[0].events = POLLIN | POLLRDNORM;
	int pollResult = 0;

  printf("HERE1\n");
	uint32_t count = 0;
	/* loop while rpmsg_pru_poll says there are no kfifo messages. */
	while (pollResult <= 0) {
		pollResult = poll(pfds,1,0);

		/*
		 * users may prefer to write code that does not block the ARM
		 * core from addressing other tasks, and that contains timeout
		 * logic to avoid an infinite lockup.
		 */
	}
printf("HERE2\n");
	/* read voltage and channel back */
	size_t freadResult = read(pfds[0].fd, value_buf_from_pru, RPMSG_MESSAGE_SIZE);
	//returnedData = *((uint32_t*) payload);
printf("HERE3\n");
	close(pfds[0].fd);
printf("HERE4\n");
	return freadResult;
}

/*
 * Function converts 12 bit raw voltage value into base 10 fraction
 */
static char * convertVoltage(const uint16_t rawVoltage)
{
	char * charVoltage = charVoltageVal;
	double doubleVoltage = (double) rawVoltage;

	/* Vin = (DigitalValue * Vref)/(2^(numOfBits) - 1) */
	/* where numOfBits = 12 */
	doubleVoltage = (doubleVoltage * 1.8 )/((double)(4096 -1));

	int charNum = snprintf(charVoltage, 7, "%.4f", doubleVoltage);
	if (charNum > 7) {
		printf("charVoltage value larger than expected.\n");
	}

	return charVoltage;
}
