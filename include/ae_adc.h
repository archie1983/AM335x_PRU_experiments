/*
 * Declarations for CAN functionality
 */
#include <stdint.h>

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

/* shared_struct is used to pass data between ARM and PRU */
typedef struct shared_struct{
	uint16_t voltage;
	uint16_t channel;
} shared_struct;

void init_adc();
uint16_t read_adc(uint16_t adc_chan);
