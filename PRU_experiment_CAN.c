#include <stdint.h>
#include <stdio.h>
#include <pru_cfg.h>
#include <pru_ctrl.h>
#include <pru_intc.h>
#include <rsc_types.h>
#include <pru_rpmsg.h>
#include "include/resource_table_rpmsg.h"
#include "include/ae_rpmsg.h"
#include "include/ae_inter_pru.h"
#include "include/ae_dcan.h"

volatile register uint32_t __R31;

/* NOTE:  Allocating shared_freq_x to PRU Shared Memory means that other PRU cores on
 *        the same subsystem must take care not to allocate data to that memory.
 *		  Users also cannot rely on where in shared memory these variables are placed
 *        so accessing them from another PRU core or from the ARM is an undefined behavior.
 */
PRU_SRAM volatile uint32_t shared_freq_1;
PRU_SRAM volatile uint32_t shared_freq_2;
PRU_SRAM volatile uint32_t shared_freq_3;

int main(void)
{
	volatile uint8_t *ptr_cm;

    __delay_cycles(1000000000); //# 500ms wait

	/**
	 * Infrastructure for communication to from PRU0 to PRU1
	 */
	/* Configure GPI and GPO as Mode 0 (Direct Connect) */
	CT_CFG.GPCFG0 = 0x0000;

	/* Clear GPO pins */
	//__R30 &= 0xFFFF0000;

	/* Configure INTC */
	configIntc();
	
	/**
	 * First trigger to PRU1
	 */
	pokePRU1Processor(); //# so that I can see that it's started the initialisation phase

	ptr_cm = CM_PER_BASE;

	/*****************************************************************/
	/* Access PRU peripherals using Constant Table & PRU header file */
	/*****************************************************************/

	/* Clear SYSCFG[STANDBY_INIT] to enable OCP master port */
	//CT_CFG.SYSCFG_bit.STANDBY_INIT = 0;

	/* Read IEPCLK[OCP_EN] for IEP clock source */
	//while (CT_CFG.IEPCLK_bit.OCP_EN == 1);


	/*****************************************************************/
	/* Access SoC peripherals using Constant Table                   */
	/*****************************************************************/

	/* Access PRCM (without CT) to initialize DCAN0 clock */
	ptr_cm[CM_PER_DCAN0_CLKCTRL] = ON;

	/**
	 * First we'll set up timings (i.e. bit rate)
	 */
	setUpCANTimings();

	/**
	 * Now the second part: Configuration of message objects.
	 */
	configureCANObjects();

	/*****************************************************************/
	/* Access PRU Shared RAM using Constant Table                    */
	/*****************************************************************/

	/* C28 defaults to 0x00000000, we need to set bits 23:8 to 0x0100 in order to have it point to 0x00010000	 */
	PRU0_CTRL.CTPPR0_bit.C28_BLK_POINTER = 0x0100;

	/* Define value of shared_freq_1 */
	shared_freq_1 = 1;

	/* Read PRU Shared RAM Freq_1 memory */
	if (shared_freq_1 == 1)
		shared_freq_2 = shared_freq_2 + 1;
	else
		shared_freq_2 = shared_freq_3;


	pokePRU1Processor(); //# so that I can see that it has gone through initialisation

	/* Attempting to send a CAN data frame and a remote frame once every second */
	while (1) {
		//setUpCANTimings();
		//configureCANObjects();

		//__delay_cycles(100000000); //# 500ms wait

		transmitDataFrame();
		//transmitRemoteFrame();

		//pokePRU1Processor();

		//__delay_cycles(100000000); //# 500ms wait
	}

	/* Halt PRU core */
	//__halt();
}

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

void commsWithARMCore() {
    struct pru_rpmsg_transport transport;
    uint16_t src, dst, len;
    volatile uint8_t *status;

    /* Allow OCP master port access by the PRU so the PRU can read external memories */
    CT_CFG.SYSCFG_bit.STANDBY_INIT = 0;

    /* Clear the status of the PRU-ICSS system event that the ARM will use to 'kick' us */
    CT_INTC.SICR_bit.STS_CLR_IDX = FROM_ARM_HOST;

    /* Make sure the Linux drivers are ready for RPMsg communication */
    status = &resourceTable.rpmsg_vdev.status;
    while (!(*status & VIRTIO_CONFIG_S_DRIVER_OK));

    /* Initialize the RPMsg transport structure */
    pru_rpmsg_init(&transport, &resourceTable.rpmsg_vring0, &resourceTable.rpmsg_vring1, TO_ARM_HOST, FROM_ARM_HOST);

    /* Create the RPMsg channel between the PRU and ARM user space using the transport structure. */
    while (pru_rpmsg_channel(RPMSG_NS_CREATE, &transport, CHAN_NAME, CHAN_DESC, CHAN_PORT) != PRU_RPMSG_SUCCESS);
    while (1) {
            /* Check bit 30 of register R31 to see if the ARM has kicked us */
            if (__R31 & HOST_INT) {
                    /* Clear the event status */
                    CT_INTC.SICR_bit.STS_CLR_IDX = FROM_ARM_HOST;
                    /* Receive all available messages, multiple messages can be sent per kick */
                    while (pru_rpmsg_receive(&transport, &src, &dst, payload, &len) == PRU_RPMSG_SUCCESS) {
                            /* Echo the message back to the same address from which we just received */
                            pru_rpmsg_send(&transport, dst, src, payload, len);
                    }
            }
    }
}
