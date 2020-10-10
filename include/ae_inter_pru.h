#ifndef _AE_INTER_PRU_H_

#define _AE_INTER_PRU_H_

/*
 * Declarations for poking the other PRU core (PRU1)
 *
 * We need to map some event to go to PRU1. We don't want to use events 16, 17, 18, 19 because they
 * are used by RPMSG and the numbers are defined in the device tree, so unless we recompile the device
 * tree, it's best to leave them alone.
 *
 * So we're going to choose something that is free- perhaps 20 seems like a nice number.
 */
#define PRU0_PRU1_EVT (20)
/**
 * PRU can trigger events 16-31 which map to 0-15 in R31[4:0] register.
 * The bit1 in 6th position (0x100000) is the ISR bit (the bit that raises the particular event)
 *
 * See page 210 in SPRUH73O–October 2011–Revised September 2016
 */
#define PRU0_PRU1_TRIGGER (__R31 = (PRU0_PRU1_EVT - 16) | (1 << 5))
#define LONG_CYCLE (5000000)
#define SHORT_CYCLE     (500000)

#ifndef PRU_SRAM
#define PRU_SRAM __far __attribute__((cregister("PRU_SHAREDMEM", near)))
#endif

/**
 * When we 'poke' the PRU1 processor once, it will start flashing the LEDs on the Phytec board much
 * faster and it will look like they're glowing continuously. When we 'poke' it again, the LEDs
 * flashing frequency will get back to normal flashing state, that is discernible with a naked eye.
 * 'Poking' it again will repeat it again.
 */
void pokePRU1Processor(void);
void setUpInterPRU(void);
void configIntc(void);

#endif
