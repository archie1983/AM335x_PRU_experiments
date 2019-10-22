#include "include/ae_rpmsg.h"
#include "include/ae_inter_pru.h"
#include "include/ae_dcan.h"
//#include "include/PRU_experiment.h"
#include <string.h>

uint8_t * lastReceivedMessageFromUser;
uint8_t lastReceivedDataFromCANBus[8];

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
     * Start DCAN clocks
     */
    startDCANClock();

	/*
	 * Set up DCAN timings (i.e. bit rate)
	 */
	setUpCANTimings();

	/*
	 * Now the second part: Configuration of message objects.
	 */
	configureCANObjects();

    setUpCommsWithARMCore();

	pokePRU1Processor(); //# so that I can see that it has gone through initialisation

	/* Attempting to send a CAN data frame and a remote frame once every second */
	while (1) {
		//setUpCANTimings();
		//configureCANObjects();

		//__delay_cycles(100000000); //# 500ms wait

        serveCommsWithARMCore();
        
        /*
         * If last received message matches a predefined command, then executing that
         * command. Don't forget to clear the lastReceivedMessageFromUser, otherwise
         * we'll be re-entering here non-stop.
         */
        lastReceivedMessageFromUser = getLastReceivedMessage();
        if (strncmp ((char *)lastReceivedMessageFromUser, "show", 4) == 0) {
            strncpy((char *)lastReceivedMessageFromUser, "\0\0\0\0", 4);
            readReceivedDataFrame(lastReceivedDataFromCANBus);
            sendMessageToUserSpace(lastReceivedDataFromCANBus, 8);
        } else if(strncmp ((char *)lastReceivedMessageFromUser, "send", 4) == 0) {
            strncpy((char *)lastReceivedMessageFromUser, "\0\0\0\0", 4);
            transmitDataFrame();
        } else if (strncmp ((char *)lastReceivedMessageFromUser, "test", 4) == 0) {
            strncpy((char *)lastReceivedMessageFromUser, "\0\0\0\0", 4);
            sendMessageToUserSpace("I'm UP", 6);
        } else if (getLastReceivedMessageLength() == 9) {
            loadDataToSendInDataFrame(lastReceivedMessageFromUser);
            transmitDataFrame();
            
            strncpy((char *)lastReceivedMessageFromUser, "\0\0\0\0", 4);
            resetLastReceivedMessageLength();
        } else {
            //transmitDataFrame();
            //transmitRemoteFrame();
        }

		//transmitRemoteFrame();

		//pokePRU1Processor();

		//__delay_cycles(100000000); //# 500ms wait
	}

	/* Halt PRU core */
	//__halt();
}
