#include "include/ae_rpmsg.h"
#include "include/ae_inter_pru.h"
#include "include/ae_dcan.h"
//#include "include/PRU_experiment.h"

int main(void)
{
    __delay_cycles(1000000000); //# 500ms wait

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
		//transmitDataFrame();
		//transmitRemoteFrame();

		//pokePRU1Processor();

		//__delay_cycles(100000000); //# 500ms wait
	}

	/* Halt PRU core */
	//__halt();
}
