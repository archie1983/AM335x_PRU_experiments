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
