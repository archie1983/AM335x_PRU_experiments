#include <stdint.h>
#include "include/ae_adc.h"
//#include "include/sys_tscAdcSs.h"
#include <sys_tscAdcSs.h>
#include "include/ae_queue.h"

volatile register uint32_t __R31;

/**
 * What we really want is a software continuous-shot mode, which averages 16 samples AND
 * puts it all into the FIFO0 for V and FIFO1 for I. Then this PRU code should act on
 * FIFO0COUNT and read the FIFO until it's empty and analyze each result with
 * a Kallman filter.
 *
 * When in continuous mode, both the open delay and sample delay apply for each step
 * and sample delay applies for each sample that will be averaged.
 *
 * TSC_ADC_SS:
 * STEPCONFIG1: Averaging, channel, FIFO and reference selection
 * STEPDELAY1: Sample delay and open delay
 * IRQSTATUS_RAW: Unmasked interrup states
 * IRQENABLE_SET: Set (unmask) interrupts
 * IRQENABLE_CLR: Unset (mask) interrupts
 * ADC_CLKDIV: ADC clock division
 * STEPENABLE: Enabling steps
 * (IDLECONFIG, TS_CHARGE_STEPCONFIG,
 * TS_CHARGE_DELAY)
 * FIFO0COUNT: How many values are there in the FIFO
 * FIFO0THRESHOLD: How many values do we want to accumulate in the FIFO before raising an interrupt
 * FIFO0DATA: Read this to get the next value in FIFO
 *
 ADC_TSC : 0x44E0_D000
 */
void init_adc()
{

	/* set the always on clock domain to NO_SLEEP. Enable ADC_TSC clock */
	while (!(CM_WKUP_ADC_TSC_CLKCTRL == 0x02)) {
		CM_WKUP_CLKSTCTRL = 0;
		CM_WKUP_ADC_TSC_CLKCTRL = 0x02;
		/* Optional: implement timeout logic. */
	}

	/*
	 * How frequently will we be taking measurements?
	 * Our ADC clock is 24MHz if we're not dividing it down.
	 * To get 1 sample, we'll need to wait the following:
	 * 1x "open delay" clock cycles
	 * 16x ("sample delay" + 1) clock cycles (if we're averaging 16 values)
	 * 1x "conversion time" clock cycles, which includes digitizing sample and outputting data. This is always 13 cycles long
	 *
	 * So maximum measurement frequency with opend delay=0 and sample delay=0 and no averaging would be:
	 * 24MHz / (1 + 13) = around 1.7MHz
	 * If we average 16x, then: 24MHz / (16 * 1 + 13) = around 800KHz
	 *
	 * If we wanted around 10KHz, then we could introduce sample delay of 148 cycles
	 * and open delay of 3 cycles: 24MHz / (16 * (148 + 1) + 3 + 13) = 10KHz
	 */

	/*
	 * Set the ADC_TSC CTRL register.
	 * Disable TSC_ADC_SS module so we can program it.
	 * Set step configuration registers to writable.
	 */
	ADC_TSC.CTRL_bit.ENABLE = 0;
	ADC_TSC.CTRL_bit.STEPCONFIG_WRITEPROTECT_N_ACTIVE_LOW = 1;

	//# STEP1 for V channel
	ADC_TSC.STEPCONFIG1_bit.MODE = 1; //# Software enabled, continuous
	ADC_TSC.STEPCONFIG1_bit.AVERAGING = 4; //# Average 16 samples
	ADC_TSC.STEPCONFIG1_bit.SEL_INP_SWC_3_0 = ADC_V_CHAN;
	ADC_TSC.STEPCONFIG1_bit.SEL_INM_SWC_3_0 = 8; //# No negative differential input, because DIFF_CNTRL = 0 (by default, hence not even set)
	ADC_TSC.STEPCONFIG1_bit.SEL_RFM_SWC_1_0 = 0; //# SEL_RFM pins SW configuration: 0 = VSSA, 3 = VREFN. Experiment with this.
	ADC_TSC.STEPCONFIG1_bit.SEL_RFP_SWC_2_0 = 0; //# SEL_RFP pins SW configutation: 0 = VDDA_ADC, 3 = VREFP. Experiment with this.
	ADC_TSC.STEPDELAY1_bit.OPENDELAY = 3; //# OPENDELAY = 3 for 10KHz measurement frequency
	ADC_TSC.STEPDELAY1_bit.SAMPLEDELAY = 148; //# SAMPLEDELAY = 148 for 10KHz measurement frequency
	ADC_TSC.STEPCONFIG1_bit.FIFO_SELECT = 0; //# FIFO0 for data

	//# STEP2 for I channel
	ADC_TSC.STEPCONFIG2_bit.MODE = 1; //# Software enabled, continuous
	ADC_TSC.STEPCONFIG2_bit.AVERAGING = 4; //# Average 16 samples
	ADC_TSC.STEPCONFIG2_bit.SEL_INP_SWC_3_0 = ADC_I_CHAN;
	ADC_TSC.STEPCONFIG2_bit.SEL_INM_SWC_3_0 = 8; //# No negative differential input, because DIFF_CNTRL = 0 (by default, hence not even set)
	ADC_TSC.STEPCONFIG2_bit.SEL_RFM_SWC_1_0 = 0; //# SEL_RFM pins SW configuration: 0 = VSSA, 3 = VREFN. Experiment with this.
	ADC_TSC.STEPCONFIG2_bit.SEL_RFP_SWC_2_0 = 0; //# SEL_RFP pins SW configutation: 0 = VDDA_ADC, 3 = VREFP. Experiment with this.
	ADC_TSC.STEPDELAY2_bit.OPENDELAY = 3; //# OPENDELAY = 3 for 10KHz measurement frequency
	ADC_TSC.STEPDELAY2_bit.SAMPLEDELAY = 148; //# SAMPLEDELAY = 148 for 10KHz measurement frequency
	ADC_TSC.STEPCONFIG2_bit.FIFO_SELECT = 1; //# FIFO1 for data

	/**
	 * Mask the FIFO buffer interrupts - both for overflow and underflow
	 */
	 ADC_TSC.IRQENABLE_CLR_bit.FIFO0_OVERRUN = 1;
	 ADC_TSC.IRQENABLE_CLR_bit.FIFO0_UNDERFLOW = 1;
	 ADC_TSC.IRQENABLE_CLR_bit.FIFO0_THRESHOLD = 1;
	 ADC_TSC.IRQENABLE_CLR_bit.FIFO1_OVERRUN = 1;
	 ADC_TSC.IRQENABLE_CLR_bit.FIFO1_UNDERFLOW = 1;
	 ADC_TSC.IRQENABLE_CLR_bit.FIFO1_THRESHOLD = 1;

	 ADC_TSC.FIFO1THRESHOLD_bit.FIFO0_THRESHOLD_LEVEL = 50;
	 ADC_TSC.FIFO0THRESHOLD_bit.FIFO0_THRESHOLD_LEVEL = 50;

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

uint8_t values_in_fifo = 0;

/**
 * Takes the values from FIFO and puts them into the queue.
 */
void fill_adc_queue() {
	/*
	 * Read FIFO0 until there's nothing left and put all new data into the queue.
	 */
	values_in_fifo = ADC_TSC.FIFO0COUNT_bit.WORDS_IN_FIFO0;
	while (values_in_fifo > 0) {
    enqueue_adc_value(ADC_TSC.FIFO0DATA_bit.ADCDATA);
		values_in_fifo--;
	}
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
		case ADC_V_CHAN :
			ADC_TSC.STEPENABLE_bit.STEP1 = 1;
		case ADC_I_CHAN :
			ADC_TSC.STEPENABLE_bit.STEP2 = 1;
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
