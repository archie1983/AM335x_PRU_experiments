#include "include/ae_queue.h"

/**
 * Enqueues a signle value.
 */
void enqueue_adc_value(uint16_t value) {
  q_end_element = value;
  advance_q_end();

  /**
   * If the queue is empty after adding and element to it, then we've
   * overflowed.
   */
  if (is_q_empty()) {
    q_overflowed = 1;
  }
}

/**
 * Dequeues as many values as possible and puts them into the provided buffer.
 */
void empty_adc_queue(uint16_t* buffer_for_values, uint16_t* cnt_values_transferred) {
	(*cnt_values_transferred) = 0;

	/*
	 * If queue has overflowed, then we need to read from q_end to q_end until
	 * we've read it all. In other words, the q_start
	 */
	if (q_overflowed) {
		rectify_overflow();
		q_overflowed = 0;
	}

	while(!is_q_empty() && (*cnt_values_transferred) < MAX_UNLOAD_CNT_FROM_QUEUE) {
		buffer_for_values[(*cnt_values_transferred)] = q_start_element;
		advance_q_start();
    (*cnt_values_transferred)++;
	}
}

/**
 * Initialises queue
 */
void init_queue() {
  q_start = 0;
  q_end = 0;
  q_overflowed = 0;
}
