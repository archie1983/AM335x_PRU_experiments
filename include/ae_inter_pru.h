/*
 * Declarations for poking the other PRU core
 */ /**
 * Defines for communication with the other PRU core (PRU1)
 */
#define PRU0_PRU1_EVT (16)
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
