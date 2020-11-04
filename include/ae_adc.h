#ifndef _AE_ADC_H_

#define _AE_ADC_H_
/*
 * Declarations for ADC functionality
 */
#include <stdint.h>
#include <sys_tscAdcSs.h>

#ifndef PRU_SRAM
#define PRU_SRAM __far __attribute__((cregister("PRU_SHAREDMEM", near)))
#endif

/* Control Module registers to enable the ADC peripheral */
#define CM_WKUP_CLKSTCTRL  (*((volatile unsigned int *)0x44E00400))
#define CM_WKUP_ADC_TSC_CLKCTRL  (*((volatile unsigned int *)0x44E004BC))

/*
 * The ADC channels that we'll be using - one for voltage and one for current.
 */
#define ADC_V_CHAN 3
#define ADC_I_CHAN 4

void init_adc();
uint16_t read_adc(uint16_t adc_chan);

/**
 * This function will service the queue in which we're offloading the FIFO values.
 * So basically this function reads the FIFO and puts all the data into the
 * queue with the appropriate pointer updates.
 */
void fill_adc_queue();
void fill_adc_queue_test();

#endif
