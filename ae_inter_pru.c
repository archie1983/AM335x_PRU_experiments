#include <stdint.h>
#include "include/ae_inter_pru.h"

volatile register uint32_t __R31;

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
