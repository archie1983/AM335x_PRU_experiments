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

/**
 * We'll keep 5000 values in the queue where we unload the fifo to. That's
 * about 500ms time before the data is overwritten.
 *
 * This must be greater than 128 - the ADC FIFO size for the queue logic to work.
 */
#define ADC_QUEUE_LENGTH 5000


void init_adc();
uint16_t read_adc(uint16_t adc_chan);

/**
 * q_start points at the element in the queue, which should be returned next
 * when reading it in normal conditions. If buffer overflow has occured, then
 * we will want to return the whole buffer starting from q_end and then make
 * both q_start and q_end point at the same element.
 */
uint16_t q_start;

/**
 * q_end points at the element in the queue, which 1 past the last that should
 * be returned, when reading it in normal conditions. If buffer overflow has
 * happened, then we will want to read the whole buffer from starting from
 * q_end.
 *
 * If we're writing to the queue, then we'll write the element, that is pointed
 * to by this index and advance the index by 1.
 */
uint16_t q_end;

/**
 * This array will be the actual queue.
 */
uint16_t adcQueue[ADC_QUEUE_LENGTH];

/**
 * If queue overflows, we want this to be true.
 */
uint8_t q_overflowed;

/**
 * Some useful macro statements.
 */
#define is_q_empty() (q_start == q_end)
#define advance_q_start() q_start=(q_start+1)%ADC_QUEUE_LENGTH
#define advance_q_end() q_end=(q_end+1)%ADC_QUEUE_LENGTH
#define q_start_element adcQueue[q_start]
#define q_end_element adcQueue[q_end]

/**
 * This function will service the queue in which we're offloading the FIFO values.
 * So basically this function reads the FIFO and puts all the data into the
 * queue with the appropriate pointer updates.
 */
void fill_adc_queue();

/**
 * This function will store as many values as possible from the queue to the
 * passed buffer unload_values. It will return the number of values transferred.
 */
uint16_t empty_adc_queue(uint16_t* unload_values);
