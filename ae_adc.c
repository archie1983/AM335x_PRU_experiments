#include <stdint.h>
#include "include/ae_adc.h"
#include "include/sys_tscAdcSs.h"

volatile register uint32_t __R31;

void init_adc()
{

	/* set the always on clock domain to NO_SLEEP. Enable ADC_TSC clock */
	while (!(CM_WKUP_ADC_TSC_CLKCTRL == 0x02)) {
		CM_WKUP_CLKSTCTRL = 0;
		CM_WKUP_ADC_TSC_CLKCTRL = 0x02;
		/* Optional: implement timeout logic. */
	}

	/* 
	 * Set the ADC_TSC CTRL register. 
	 * Disable TSC_ADC_SS module so we can program it.
	 * Set step configuration registers to writable.
	 */
	ADC_TSC.CTRL_bit.ENABLE = 0;
	ADC_TSC.CTRL_bit.STEPCONFIG_WRITEPROTECT_N_ACTIVE_LOW = 1;

	/* 
	 * set the ADC_TSC STEPCONFIG1 register for channel 5  
	 * Mode = 0; SW enabled, one-shot
	 * Averaging = 0x3; 8 sample average
	 * SEL_INP_SWC_3_0 = 0x4 = Channel 5
         * SEL_INM_SWC_3_0 = 1xxx = VREFN (reduces noise in single ended mode)
	 * use FIFO0
	 */
	ADC_TSC.STEPCONFIG1_bit.MODE = 0;
	ADC_TSC.STEPCONFIG1_bit.AVERAGING = 3;
	ADC_TSC.STEPCONFIG1_bit.SEL_INP_SWC_3_0 = 4;
	ADC_TSC.STEPCONFIG1_bit.SEL_INM_SWC_3_0 = 8;
	ADC_TSC.STEPCONFIG1_bit.FIFO_SELECT = 0;

	/*
	 * set the ADC_TSC STEPCONFIG2 register for channel 6
	 * Mode = 0; SW enabled, one-shot
	 * Averaging = 0x3; 8 sample average
	 * SEL_INP_SWC_3_0 = 0x5 = Channel 6
         * SEL_INM_SWC_3_0 = 1xxx = VREFN (reduces noise in single ended mode)
	 * use FIFO0
	 */
	ADC_TSC.STEPCONFIG2_bit.MODE = 0;
	ADC_TSC.STEPCONFIG2_bit.AVERAGING = 3;
	ADC_TSC.STEPCONFIG2_bit.SEL_INP_SWC_3_0 = 5;
	ADC_TSC.STEPCONFIG2_bit.SEL_INM_SWC_3_0 = 8;
	ADC_TSC.STEPCONFIG2_bit.FIFO_SELECT = 0;

	/* 
	 * set the ADC_TSC STEPCONFIG3 register for channel 7
	 * Mode = 0; SW enabled, one-shot
	 * Averaging = 0x3; 8 sample average
	 * SEL_INP_SWC_3_0 = 0x6 = Channel 7
         * SEL_INM_SWC_3_0 = 1xxx = VREFN (reduces noise in single ended mode)
	 * use FIFO0
	 */
	ADC_TSC.STEPCONFIG3_bit.MODE = 0;
	ADC_TSC.STEPCONFIG3_bit.AVERAGING = 3;
	ADC_TSC.STEPCONFIG3_bit.SEL_INP_SWC_3_0 = 6;
	ADC_TSC.STEPCONFIG3_bit.SEL_INM_SWC_3_0 = 8;
	ADC_TSC.STEPCONFIG3_bit.FIFO_SELECT = 0;

	/* 
	 * set the ADC_TSC STEPCONFIG4 register for channel 8
	 * Mode = 0; SW enabled, one-shot
	 * Averaging = 0x3; 8 sample average
	 * SEL_INP_SWC_3_0 = 0x7= Channel 8
         * SEL_INM_SWC_3_0 = 1xxx = VREFN (reduces noise in single ended mode)
	 * use FIFO0
	 */
	ADC_TSC.STEPCONFIG4_bit.MODE = 0;
	ADC_TSC.STEPCONFIG4_bit.AVERAGING = 3;
	ADC_TSC.STEPCONFIG4_bit.SEL_INP_SWC_3_0 = 7;
	ADC_TSC.STEPCONFIG4_bit.SEL_INM_SWC_3_0 = 8;
	ADC_TSC.STEPCONFIG4_bit.FIFO_SELECT = 0;

	/* 
	 * set the ADC_TSC CTRL register
	 * set step configuration registers to protected
	 * store channel ID tag if needed for debug
	 * Enable TSC_ADC_SS module
	 */
	ADC_TSC.CTRL_bit.STEPCONFIG_WRITEPROTECT_N_ACTIVE_LOW = 0;
	ADC_TSC.CTRL_bit.STEP_ID_TAG = 1;
	ADC_TSC.CTRL_bit.ENABLE = 1;
}

uint16_t read_adc(uint16_t adc_chan)
{
	/* 
	 * Clear FIFO0 by reading from it
	 * We are using single-shot mode. 
	 * It should not usually enter the for loop
	 */
	uint32_t count = ADC_TSC.FIFO0COUNT;
	uint32_t data;
	uint32_t i;
	for (i = 0; i < count; i++) {
		data = ADC_TSC.FIFO0DATA;
	}

	/* read from the specified ADC channel */
	switch (adc_chan) {
		case 5 :
			ADC_TSC.STEPENABLE_bit.STEP1 = 1;
		case 6 :
			ADC_TSC.STEPENABLE_bit.STEP2 = 1;
		case 7 :
			ADC_TSC.STEPENABLE_bit.STEP3 = 1;
		case 8 :
			ADC_TSC.STEPENABLE_bit.STEP4 = 1;
		default :
			/* 
			 * this error condition should not occur because of
			 * checks in userspace code
			 */
			ADC_TSC.STEPENABLE_bit.STEP1 = 1;
	}

	while (ADC_TSC.FIFO0COUNT == 0) {
		/*
		 * loop until value placed in fifo.
		 * Optional: implement timeout logic.
		 *
		 * Other potential options include: 
		 *   - configure PRU to receive an ADC interrupt. Note that 
		 *     END_OF_SEQUENCE does not mean that the value has been
		 *     loaded into the FIFO yet
		 *   - perform other actions, and periodically poll for 
		 *     FIFO0COUNT > 0
		 */
	}

	/* read the voltage */
	uint16_t voltage = ADC_TSC.FIFO0DATA_bit.ADCDATA;

	return voltage;
}
