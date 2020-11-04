#include "include/ae_rpmsg.h"
#include "include/ae_inter_pru.h"
#include "include/ae_adc.h"
//#include "include/PRU_experiment.h"
#include "include/ae_queue.h"
#include <string.h>

uint8_t * lastReceivedMessageFromUser;
uint16_t lastADCReadings[2];
uint16_t return_buffer[MAX_UNLOAD_CNT_FROM_QUEUE];
uint16_t cnt_elements_to_return = 0;

int main(void)
{
    __delay_cycles(100000000); //# 500ms wait

    /*
     * Setting up inter communication between PRU0 and PRU1
     */
    setUpInterPRU();

    /*
     * Trigger to PRU1 so that I can see that it's started the initialisation phase
     */
    pokePRU1Processor();

    /*
     * Set up comms with ARM core
     */
    setUpCommsWithARMCore();

    pokePRU1Processor(); //# so that I can see that it has gone through initialisation
    //sendMessageToUserSpace("99", 2);

    init_queue();

    /*
     * Initialise ADC steps and other ADC configuration.
     */
    init_adc();

    //__delay_cycles(100000000); //# 500ms wait

    /* Attempting to send a CAN data frame and a remote frame once every second */
    while (1) {
        //__delay_cycles(100000000); //# 500ms wait
        //__delay_cycles(1000000); //# 5ms wait
        //__delay_cycles(200000); //# 1ms wait

        /**
         * If we have FIFO buffer threshold interrupt, then let's empty the FIFO.
         */
        if (ADC_TSC.IRQSTATUS_bit.FIFO0_THRESHOLD == 1) {
          fill_adc_queue();
          ADC_TSC.IRQSTATUS_bit.FIFO0_THRESHOLD = 1;
        }

        //fill_adc_queue();
        //fill_adc_queue_test();
        //fill_adc_queue();

        serveCommsWithARMCore();

        /*
         * If last received message matches a predefined command, then executing that
         * command. Don't forget to clear the lastReceivedMessageFromUser, otherwise
         * we'll be re-entering here non-stop.
         */
        lastReceivedMessageFromUser = getLastReceivedMessage();
        if (strncmp ((char *)lastReceivedMessageFromUser, "read", 4) == 0) {
            strncpy((char *)lastReceivedMessageFromUser, "\0\0\0\0", 4);
            //lastADCReadings[1] = 99;
            //lastADCReadings[0] = 88;
            //lastADCReadings[1] = read_adc(ADC_I_CHAN);
            //__delay_cycles(100000000);
            //lastADCReadings[0] = read_adc(ADC_V_CHAN);
            //sendMessageToUserSpace((uint8_t *)lastADCReadings, 4);
            empty_adc_queue(return_buffer, &cnt_elements_to_return);
            if (cnt_elements_to_return > 0) {
              sendMessageToUserSpace((uint8_t *)return_buffer, cnt_elements_to_return * 2);
            } else {
              return_buffer[0] = ADC_TSC.FIFO0COUNT_bit.WORDS_IN_FIFO0;
              return_buffer[1] = 1; //ADC_TSC.FIFO0DATA_bit.ADCDATA;
              return_buffer[2] = 2;
              return_buffer[3] = 3;
              return_buffer[4] = 4;
              return_buffer[5] = 500;

              sendMessageToUserSpace((uint8_t *)return_buffer, 12);
              //sendMessageToUserSpace("AAAA", 4);
            }
        } else if (strncmp ((char *)lastReceivedMessageFromUser, "test", 4) == 0) {
            strncpy((char *)lastReceivedMessageFromUser, "\0\0\0\0", 4);
            sendMessageToUserSpace("I'm UP", 6);
        }

	//__delay_cycles(100000000); //# 500ms wait
    }

	/* Halt PRU core */
	//__halt();
}
