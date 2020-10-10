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
     enqueue_adc_value(1);
     enqueue_adc_value(3);
     enqueue_adc_value(4);
     enqueue_adc_value(5);
     enqueue_adc_value(6);

     uint16_t return_buffer[ADC_QUEUE_LENGTH];
     uint16_t returned_elem_cnt;
     empty_adc_queue(return_buffer, &returned_elem_cnt);

     printf("returned_elem_cnt=%d\n", returned_elem_cnt);
     mu_assert("Returned element count not 5", returned_elem_cnt == 5);
     return 0;
 }

 static char * all_tests() {
     mu_run_test(test_foo);
     mu_run_test(queue_fill);
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
