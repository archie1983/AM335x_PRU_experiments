 #include <stdio.h>
 #include "minunit.h"
 #include "../include/ae_queue.h"

 int tests_run = 0;

 int foo = 7;
 int bar = 4;

 static char * test_foo() {
     mu_assert("error, foo != 7", foo == 7);
     return 0;
 }

 static char * queue_fill() {
     int i = 0;
     for (i = 0; i < 5; i++) {
       enqueue_adc_value(i);
       //printf("q_start=%d, q_end=%d, is_q_empty()=%d\n", q_start, q_end, is_q_empty());
     }

     uint16_t return_buffer[ADC_QUEUE_LENGTH];
     uint16_t returned_elem_cnt;
     empty_adc_queue(return_buffer, &returned_elem_cnt);

     //printf("returned_elem_cnt=%d, is_q_empty()=%d\n", returned_elem_cnt, is_q_empty());
     //printf("q_start=%d, q_end=%d, is_q_empty()=%d\n", q_start, q_end, is_q_empty());
     mu_assert("Returned element count not 5", returned_elem_cnt == 5);

     for (i = 0; i < returned_elem_cnt; i++) {
       mu_assert("Returned elements in buffer are wrong", return_buffer[i] == i);
       //printf("q_start=%d, q_end=%d, is_q_empty()=%d\n", q_start, q_end, is_q_empty());
     }

     return 0;
 }

 static char * queue_overfill() {
     int i = 0;
     for (i = 0; i < ADC_QUEUE_LENGTH; i++) {
       enqueue_adc_value(i);
       //printf("q_start=%d, q_end=%d, is_q_empty()=%d\n", q_start, q_end, is_q_empty());
     }

     uint16_t return_buffer[ADC_QUEUE_LENGTH];
     uint16_t returned_elem_cnt;
     empty_adc_queue(return_buffer, &returned_elem_cnt);

     //printf("returned_elem_cnt=%d, is_q_empty()=%d\n", returned_elem_cnt, is_q_empty());
     //printf("q_start=%d, q_end=%d, is_q_empty()=%d\n", q_start, q_end, is_q_empty());
     mu_assert("Returned element count not 5", returned_elem_cnt == MAX_UNLOAD_CNT_FROM_QUEUE);

     for (i = 0; i < returned_elem_cnt; i++) {
       printf("returned_elem_cnt=%d, return_buffer[i]=%d, i+1=%d\n", returned_elem_cnt, return_buffer[i], i+1);
       mu_assert("Returned elements in buffer are wrong", return_buffer[i] == i+1);
     }

     return 0;
 }

 static char * all_tests() {
     mu_run_test(test_foo);
     mu_run_test(queue_fill);
     mu_run_test(queue_overfill);
     return 0;
 }

 int main(int argc, char **argv) {
     char *result = all_tests();
     if (result != 0) {
         printf("%s\n", result);
     }
     else {
         printf("ALL TESTS PASSED\n");
     }
     printf("Tests run: %d\n", tests_run);

     return result != 0;
 }
