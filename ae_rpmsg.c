#include <stdint.h>
#include <pru_intc.h>
#include <pru_rpmsg.h>
#include <pru_cfg.h>
#include "include/ae_rpmsg.h"
#include "include/PRU_experiment.h"

volatile register uint32_t __R31;
uint8_t payload[RPMSG_MESSAGE_SIZE];

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
