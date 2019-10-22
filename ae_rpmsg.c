#include <stdint.h>
#include <pru_intc.h>
#include <pru_rpmsg.h>
#include <pru_cfg.h>
#include "include/ae_rpmsg.h"
#include "include/PRU_experiment.h"
#include "include/ae_inter_pru.h"

volatile register uint32_t __R31;
uint8_t payload[RPMSG_MESSAGE_SIZE];
uint8_t lastReceivedMessage[RPMSG_MESSAGE_SIZE];
uint16_t lastMessageLength;
uint16_t lastMessageSrc, lastMessageDest;

/*
 * A global transport structure -- we'll set it up in setUpCommsWithARMCore() and use it
 * in serveCommsWithARMCore().
 */
struct pru_rpmsg_transport transport;

/**
 * Sets up the communication infrastructure between ARM and PRU core
 */
void setUpCommsWithARMCore() {
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
}

/**
 * Serves the comms between PRU and ARM cores -- if we have an interrupt pending for incoming message, then
 * reads the incoming message and sends out the response.
 */
void serveCommsWithARMCore() {
    /* Check bit 30 of register R31 to see if the ARM has kicked us */
    if (__R31 & HOST_INT) {
            /* Clear the event status */
            CT_INTC.SICR_bit.STS_CLR_IDX = FROM_ARM_HOST;
            /* Receive all available messages, multiple messages can be sent per kick and store the last one into lastReceivedMessage. */
            while (pru_rpmsg_receive(&transport, &lastMessageSrc, &lastMessageDest, lastReceivedMessage, &lastMessageLength) == PRU_RPMSG_SUCCESS);
    }
}

/**
 * Returns last received message (a pointer to it actually) from Linux user space running on ARM core.
 */
uint8_t * getLastReceivedMessage() {
    return lastReceivedMessage;
}

/**
 * Returns length (including the \0 symbol) of the last received message from Linux user space running on ARM core.
 */
uint16_t getLastReceivedMessageLength() {
    return lastMessageLength;
}

/**
 * Resets length of the last received message from Linux user space running on ARM core.
 */
void resetLastReceivedMessageLength() {
    lastMessageLength = 0;
}

/**
 * Sends out a message to the user space in Linux running on the ARM core.
 */
void sendMessageToUserSpace(uint8_t * message, uint16_t messageLength) {
    /* Send the message to the address, that we last received something from. */
    pru_rpmsg_send(&transport, lastMessageDest, lastMessageSrc, message, messageLength);
}
