#include <stdint.h>
#include <pru_cfg.h>
#include <pru_intc.h>
#include "include/ae_inter_pru.h"

volatile register uint32_t __R31;

/* NOTE:  Allocating shared_freq_x to PRU Shared Memory means that other PRU cores on
 *        the same subsystem must take care not to allocate data to that memory.
 *		  Users also cannot rely on where in shared memory these variables are placed
 *        so accessing them from another PRU core or from the ARM is an undefined behavior.
 */
PRU_SRAM volatile uint32_t shared_freq_1;
PRU_SRAM volatile uint32_t shared_freq_2;
PRU_SRAM volatile uint32_t shared_freq_3;

/**
 * When we 'poke' the PRU1 processor once, it will start flashing the LEDs on the Phytec board much
 * faster and it will look like they're glowing continuously. When we 'poke' it again, the LEDs
 * flashing frequency will get back to normal flashing state, that is discernible with a naked eye.
 * 'Poking' it again will repeat it again.
 */
void pokePRU1Processor() {
	PRU0_PRU1_TRIGGER;
	__delay_cycles(100000000); //# 500ms wait
}

void setUpInterPRU() {
    /*****************************************************************/
	/* Access PRU Shared RAM using Constant Table                    */
	/*****************************************************************/

	/* C28 defaults to 0x00000000, we need to set bits 23:8 to 0x0100 in order to have it point to 0x00010000	 */
	//PRU0_CTRL.CTPPR0_bit.C28_BLK_POINTER = 0x0100;

	/* Define value of shared_freq_1 */
//	shared_freq_1 = 1;

	/* Read PRU Shared RAM Freq_1 memory */
//	if (shared_freq_1 == 1)
//		shared_freq_2 = shared_freq_2 + 1;
//	else
//		shared_freq_2 = shared_freq_3;

	/**
	 * Infrastructure for communication from PRU0 to PRU1
	 */
	/* Configure GPI and GPO as Mode 0 (Direct Connect) */
	CT_CFG.GPCFG0 = 0x0000;

	/* Clear GPO pins */
	//__R30 &= 0xFFFF0000;
    
    configIntc();
}

/* INTC configuration
 * We are going to map User event 16 to Host 1
 * PRU1 will then wait for r31 bit 31 (designates Host 1) to go high
 * */
void configIntc()
{
	/* Clear any pending PRU-generated events */
	__R31 = 0x00000000;

	/* Map event 16 to channel 1
	 *
	 * ??? We are setting up the event 16 to be routed to channel 1 (which goes to PRU1 as a PRU host interrupt)
	 * */
	CT_INTC.CMR4_bit.CH_MAP_16 = 1;

	/* Map channel 1 to host 1
	 *
	 * ??? We are setting up the channel 1 to be routed to host1 (which is PRU1)
	 * */
	CT_INTC.HMR0_bit.HINT_MAP_1 = 1;

	/* Ensure event 16 is cleared */
	CT_INTC.SICR = 16;
	/* Delay to ensure the event is cleared in INTC */
	__delay_cycles(5);

	/* Enable event 16 */
	CT_INTC.EISR = 16;

	/* Enable Host interrupt 1 */
	CT_INTC.HIEISR |= (1 << 0);

	// Globally enable host interrupts
	CT_INTC.GER = 1;
}
