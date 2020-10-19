#ifndef _AE_QUEUE_H_

#define _AE_QUEUE_H_
#ifndef _PRU_RPMSG_H_

/**
 * We need this header for the value of RPMSG_MESSAGE_SIZE- that's the upper
 * limit of bytes that we want to unload into the buffer when we empty the queue.
 *
 * NOTE: It is easier to just define the single constant that we need - it's easier
 * for unit tests.
 */
//#include <pru_rpmsg.h>
/* The maximum size of the buffer message */
#define RPMSG_MESSAGE_SIZE			496
#include <stdint.h>

#endif

/**
 * We'll keep 5000 values in the queue where we unload the fifo to. That's
 * about 500ms time before the data is overwritten.
 *
 * This must be greater than 128 - the ADC FIFO size for the queue logic to work.
 */
#define ADC_QUEUE_LENGTH 500

/**
 * This is the upper limit of elements that we want to unload into the buffer
 * when we empty the queue, because we can't send more than this to the user
 * space. The limit of bytes is RPMSG_MESSAGE_SIZE, but we're sending uint16_t
 * values.
 */
#define MAX_UNLOAD_CNT_FROM_QUEUE RPMSG_MESSAGE_SIZE / 2

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
#define rectify_overflow() q_start=(q_end+1)%ADC_QUEUE_LENGTH

/**
 * Enqueues one value.
 */
void enqueue_adc_value(uint16_t value);

/**
 * Initialises queue
 */
void init_queue();

/**
 * This function will store as many values as possible from the queue to the
 * passed buffer buffer_for_values. It will store the number of values
 * transferred into the cnt_values_transferred parameter.
 */
void empty_adc_queue(uint16_t* buffer_for_values, uint16_t* cnt_values_transferred);


#endif
