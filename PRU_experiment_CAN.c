#include <stdint.h>
#include <pru_cfg.h>
#include <pru_ctrl.h>
#include <pru_intc.h>
#include "resource_table_empty.h"

/**
 * Defines for communication with the other PRU core (PRU1)
 */
#define PRU0_PRU1_EVT		(16)
/**
 * PRU can trigger events 16-31 which map to 0-15 in R31[4:0] register.
 * The bit1 in 6th position (0x100000) is the ISR bit (the bit that raises the particular event)
 *
 * See page 210 in SPRUH73O–October 2011–Revised September 2016
 */
#define PRU0_PRU1_TRIGGER	(__R31 = (PRU0_PRU1_EVT - 16) | (1 << 5))
#define LONG_CYCLE      (5000000)
#define SHORT_CYCLE     (500000)

/* Mapping Constant Table (CT) registers to variables. DCAN0 = 0x481CC000 */
volatile far uint8_t CT_DCAN0 __attribute__((cregister("DCAN0", near), peripheral));

#ifndef PRU_SRAM
#define PRU_SRAM __far __attribute__((cregister("PRU_SHAREDMEM", near)))
#endif

/* NOTE:  Allocating shared_freq_x to PRU Shared Memory means that other PRU cores on
 *        the same subsystem must take care not to allocate data to that memory.
 *		  Users also cannot rely on where in shared memory these variables are placed
 *        so accessing them from another PRU core or from the ARM is an undefined behavior.
 */
PRU_SRAM volatile uint32_t shared_freq_1;
PRU_SRAM volatile uint32_t shared_freq_2;
PRU_SRAM volatile uint32_t shared_freq_3;

/**
 * PRCM Registers
 * ### AE: ###
 * Pages 179 and 1250 in SPRUH73P–October 2011–Revised March 2017
 * 
 * From here on we'll refer to SPRUH73P–October 2011–Revised March 2017 as "user's guide"
 */
#define CM_PER_BASE	((volatile uint8_t *)(0x44E00000))

#define CONTROL_MODULE ((volatile uint8_t *)(0x44E10000))
#define CTRL_MOD_DCAN_RAMINIT_offset (0x644)

/**
 * ### AE: ###
 * Pages 1250 and 1285 in user's guide
 */
#define CM_PER_DCAN0_CLKCTRL (0xC0)

/**
 * ### AE: ###
 * Page 1285 in user's guide
 */
#define ON (0x2)

/**
 * ### AE: ###
 * Taking CT_DCAN0 register address (0x481CC000), adding 0x0 to that address, 
 * then casting it to a volatile uint16_t pointer and finally taking 
 * the value that is pointed to by the address held in the pointer 
 * and using that value. That should give us a direct access to the CTL
 * register of the CT_DCAN0, which we can use to control the peripheral.
 * Page 4813 in user's guide
 */
#define DCAN0_CTL (*((volatile uint16_t*)(&CT_DCAN0 + 0x0)))

/* DCAN0 bit timing register as described in page 4819 of user's guide */
#define DCAN0_BTR (*((volatile uint16_t*)(&CT_DCAN0 + 0x0C)))

/**
 * Test mode register.
 */
#define DCAN0_TEST (*((volatile uint32_t*)(&CT_DCAN0 + 0x14)))

/**
 * IF1 command register to specify what we want to update in the CAN data 
 * object (receive object, transmit object or remote frame receive object)
 */
#define IF1CMD (*((volatile uint32_t*)(&CT_DCAN0 + 0x100)))

/** 
 * Arbitration bits (kind of like address, which is also a priority) for message 
 * object as described in page 4852 of user's guide 
 */
#define IF1ARB (*((volatile uint32_t*)(&CT_DCAN0 + 0x108)))

/**
 * Mask register bits (affecting arbitration bits to decide what we will accept and what not).
 */
#define IF1MSK (*((volatile uint32_t*)(&CT_DCAN0 + 0x104)))

/**
 * Message control register (allows us to set the frame data and start transmission amongst other things).
 */
#define IF1MCTL (*((volatile uint16_t*)(&CT_DCAN0 + 0x10C)))

/**
 * The first 4 bytes of the actual message data.
 */
#define IF1DATA (*((volatile uint32_t*)(&CT_DCAN0 + 0x110)))

/**
 * The last 4 bytes of the actual message data.
 */
#define IF1DATB (*((volatile uint32_t*)(&CT_DCAN0 + 0x114)))

/**
 * Same for IF2 and IF3 interfaces
 */
#define IF2CMD (*((volatile uint32_t*)(&CT_DCAN0 + 0x120)))
#define IF2MSK (*((volatile uint32_t*)(&CT_DCAN0 + 0x124)))
#define IF2ARB (*((volatile uint32_t*)(&CT_DCAN0 + 0x128)))
#define IF2MCTL (*((volatile uint16_t*)(&CT_DCAN0 + 0x12C)))
#define IF2DATA (*((volatile uint32_t*)(&CT_DCAN0 + 0x130)))
#define IF2DATB (*((volatile uint32_t*)(&CT_DCAN0 + 0x134)))

/**
 * In IF3 case we don't have an IF3CMD register and we're allowed to set
 * IF3 registers on the go as we please. We do have IF3OBS register that
 * tells us of received data (fields that have changed).
 */
#define IF3MSK (*((volatile uint32_t*)(&CT_DCAN0 + 0x144)))
#define IF3ARB (*((volatile uint32_t*)(&CT_DCAN0 + 0x148)))
#define IF3MCTL (*((volatile uint16_t*)(&CT_DCAN0 + 0x14C)))
#define IF3DATA (*((volatile uint32_t*)(&CT_DCAN0 + 0x150)))
#define IF3DATB (*((volatile uint32_t*)(&CT_DCAN0 + 0x154)))

/* This is a char so that I can force access to R31.b0 for the host interrupt */
volatile register uint8_t __R31;

/* PRU-to-ARM interrupt */
#define PRU_ARM_INTERRUPT (19+16)

void setUpCANTimings(void);
void configureCANObjects(void);
void configIntc(void);
void pokePRU1Processor(void);
void transmitDataFrame(void);
void init_dcan_ram(void);
void disable_unused_dcan0_objects(void);

int main(void)
{
	uint32_t result;
	volatile uint8_t *ptr_cm;

	/**
	 * Infrastructure for communication to PRU1
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
	pokePRU1Processor();

	ptr_cm = CM_PER_BASE;

	/*****************************************************************/
	/* Access PRU peripherals using Constant Table & PRU header file */
	/*****************************************************************/

	/* Clear SYSCFG[STANDBY_INIT] to enable OCP master port */
	CT_CFG.SYSCFG_bit.STANDBY_INIT = 0;

	/* Read IEPCLK[OCP_EN] for IEP clock source */
	result = CT_CFG.IEPCLK_bit.OCP_EN;


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


	/* Attempting to send a CAN data frame and a remote frame once every second */
	while (1) {
		setUpCANTimings();
		configureCANObjects();

		__delay_cycles(100000000); //# 500ms wait

		transmitDataFrame();

		pokePRU1Processor();

		__delay_cycles(100000000); //# 500ms wait

		//tmp_val = IF2CMD;
		//IF2CMD = tmp_val | 0x840000; //# Setting IF2CMD[23] = WR_RD = 1 and IF2CMD[18] = TxRqst_NewDat = 1 to start a data frame transmission
		//tmp_val = IF2CMD;
		//IF2CMD = tmp_val | 0x1; //# Setting IF2CMD[7:0] = message_number to start data transfer from IF register to RAM, which will
								//# also set the TxRqst flag and actually start the data frame transmission
		//while (IF2CMD & 0x8000 > 0); //# waiting until the IF1 registers are transmitted to DCAN RAM, which will start CAN data frame transmission
	}

	/* Halt PRU core */
	__halt();
}

void transmitDataFrame() {
	uint32_t tmp_val;

	//IF1MCTL = tmp_val | 0x8100; //# Requesting a transmit by setting the TxRqst and NewDat flags
	tmp_val = IF1CMD; //# 0xF30001
	IF1CMD = tmp_val | 0x840000; //# Setting IF1CMD[23] = WR_RD = 1 and IF1CMD[18] = TxRqst_NewDat = 1 to start a data frame transmission
	tmp_val = IF1CMD;
	IF1CMD = tmp_val | 0x1; //# Setting IF1CMD[7:0] = message_number to start data transfer from IF register to RAM, which will
							//# also set the TxRqst flag and actually start the data frame transmission
	while ((IF1CMD & 0x8000) == 0x8000); //# waiting until the IF1 registers are transmitted to DCAN RAM, which will start CAN data frame transmission
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

void setUpCANTimings() {
	uint32_t tmp_val;

	/**
	 * According to page 4775 in user's guide
	 * A general CAN module initialization would mean the following two critical steps:
	 * -> Configuration of the CAN bit timing
	 * -> Configuration of message objects
	 *
	 * Let's start with CAN bit timing:
	 *
	 * Now we want to reset the DCAN0 peripheral so that we start from a clean sheet
	 * and get the init bit set in control register as described on page 4814 in user's manual.
     *
	 * DCAN0_CTL[31:26] : Reserved
	 * DCAN0_CTL[25] = WUBA = 0 : Don't define auto wake-up from low-power mode upon receiving dominant CAN bus level
	 * DCAN0_CTL[24] = PDR = 0 : Not requesting local low-power mode
	 * DCAN0_CTL[23:21] : Reserved
	 * DCAN0_CTL[20] = DE3 = 0 : No DMA request line for IF3
	 * DCAN0_CTL[19] = DE2 = 0 : No DMA request line for IF2
	 * DCAN0_CTL[18] = DE1 = 0 : No DMA request line for IF1
	 * DCAN0_CTL[17] = IE1 = 0 : Interrupt line 1 not enabled
	 * DCAN0_CTL[16] = InitDbg = 0 : Not in debug mode
	 * DCAN0_CTL[15] = SWR = 1 : Module is forced to reset state
	 * DCAN0_CTL[14] : Reserved
	 * DCAN0_CTL[13:10] = PMD = 0 : Parity on atm (anything other than 5 here switches parity on), but will probably disable later
	 * DCAN0_CTL[9] = ABO = 0 : Auto-Bus-On feature (to automatically try to recover from errors): Off for now, but will set it later
	 * DCAN0_CTL[8] = IDS = 0 : Not to enter debug mode immediately, but to wait for frame completion (only if debug/suspend mode is requested)
	 * DCAN0_CTL[7] = TEST = 0 : Test mode (off for now, but will set it later)
	 * DCAN0_CTL[6] = CCE = 0 : Configuration changes (disabled for now, but will enable later)
	 * DCAN0_CTL[5] = DAR = 0 : Auto retransmission of unsuccessful messages (off for now, but will enable later)
	 * DCAN0_CTL[4] : Reserved
	 * DCAN0_CTL[3] = EIE = 0 : Errors to not generate interrupts
	 * DCAN0_CTL[2] = SIE = 0 : Status changes to not generate interrupts
	 * DCAN0_CTL[1] = IE0 = 0 : Interrupt line 1 disabled
	 * DCAN0_CTL[0] = Init = 1 : Initialisation mode is entered
	 */
	DCAN0_CTL = 0x8001;
	//DCAN0_CTL = 0x0001;

	DCAN0_CTL = 0x0041;
	
	/**
	 * waiting for init bit to be set
	 */
	while ((DCAN0_CTL & 0x1) == 0);

	/**
	 * Following workflow in figure 23-4 on page 4776 in user's manual to configure bit timing.
	 * CCE = 1. Init bit too has to remain set.
	 *
	 * DCAN0_CTL[31:26] : Reserved
	 * DCAN0_CTL[25] = WUBA = 0 : Don't define auto wake-up from low-power mode upon receiving dominant CAN bus level
	 * DCAN0_CTL[24] = PDR = 0 : Not requesting local low-power mode
	 * DCAN0_CTL[23:21] : Reserved
	 * DCAN0_CTL[20] = DE3 = 1 : No DMA request line for IF3
	 * DCAN0_CTL[19] = DE2 = 1 : No DMA request line for IF2
	 * DCAN0_CTL[18] = DE1 = 1 : No DMA request line for IF1
	 * DCAN0_CTL[17] = IE1 = 0 : Interrupt line 1 not enabled
	 * DCAN0_CTL[16] = InitDbg = 0 : Not in debug mode
	 * DCAN0_CTL[15] = SWR = 0 : Not resetting
	 * DCAN0_CTL[14] : Reserved
	 * DCAN0_CTL[13:10] = PMD = 0101 : Parity off
	 * DCAN0_CTL[9] = ABO = 1 : Auto-Bus-On feature enabled
	 * DCAN0_CTL[8] = IDS = 0 : Not to enter debug mode immediately, but to wait for frame completion (only if debug/suspend mode is requested)
	 * DCAN0_CTL[7] = TEST = 1 : Test mode enabled
	 * DCAN0_CTL[6] = CCE = 1 : Configuration changes enabled
	 * DCAN0_CTL[5] = DAR = 1 : Auto retransmission of unsuccessful messages enabled
	 * DCAN0_CTL[4] : Reserved
	 * DCAN0_CTL[3] = EIE = 0 : Errors to not generate interrupts
	 * DCAN0_CTL[2] = SIE = 0 : Status changes to not generate interrupts
	 * DCAN0_CTL[1] = IE0 = 0 : Interrupt line 1 disabled
	 * DCAN0_CTL[0] = Init = 1 : Initialisation mode is entered
	 */
	//DCAN0_CTL = 0x41;
	//DCAN0_CTL = 0xC1; //# with test mode
	DCAN0_CTL = 0x1C16E1; //# with test mode enabled and parity mode disabled and auto retransmission disabled

	/**
	 * waiting for init bit to be set
	 */
	while ((DCAN0_CTL & 0x1) == 0);

	/**
	 * Putting the DCAN0 peripheral into the external loopback mode.
	 */
	//DCAN0_TEST = 0x300;
	DCAN0_TEST = 0x10;
	
	/**
	 * Now the actual bit timing register value:
	 * 
	 * DCAN module in AM3359 is supplied with two clocks: interface clock and functional clock.
	 * Functional clock runs the actual module and encodes / decodes packet frames, etc.. It is
	 * equal to CLK_M_OSC - the high speed system clock, which can be 19.2 - 26 MHz. In the case
	 * of Phytec's AM335x modules it is: 25 MHz
	 *
	 * Interface clock puts the bits onto the physical interface (the actual can bus wires). It
	 * taken from pd_per_l3_gclk (a.k.a. CORE_CLKOUTM4 and ocp_clk) dividing it by two. Usually
	 * (and in case of Phytec's AM335x modules) CORE_CLKOUTM4 = 200 MHz, so our interface clock
	 * is 100 MHz.
	 *
	 * Out of these two we need interface clock to calculate tq (time quanta).
	 * writing bit time values to BTR.
	 * 
	 * As per page 4801 in user's manual:
	 * Usually, the calculation of the bit timing configuration starts with a desired bit rate or bit time. The resulting
	 * bit time (1 / Bit rate) must be an integer multiple of the CAN clock period. 
	 * 
	 * AE: For our 100 MHz interface clock, the clock period = 1 / (10^8) = 10ns = cp
	 * AE: Let's say we want 1 MBit/s, so our bit time will be 1 / (10^6) = 1us = 100 * cp
	 * 
	 * The bit time may consist of 8 to 25 time quanta. The length of the time quantum tq is defined by the baud rate 
	 * prescaler with tq = (Baud Rate Prescaler) / CAN_CLK. Several combinations may lead to the desired bit time, 
	 * allowing iterations of the following steps.
	 * 
	 * AE: Let's make our 1us bit time to be convenient 10 tq. So 1tq = 100ns
	 * 
	 * First part of the bit time to be defined is the Prop_Seg. Its length depends on the delay times measured in
	 * the system. A maximum bus length as well as a maximum node delay has to be defined for expandible CAN bus systems. 
	 * The resulting time for Prop_Seg is converted into time quanta (rounded up to the nearest integer multiple of tq).
	 * 
	 * AE: Let's assume for now that Prop_Seg = 230ns, so 3tq. Prop_Seg = 3tq.
	 * 
	 * The Sync_Seg is 1 tq long (fixed), leaving (bit time – Prop_Seg – 1) tq for the two Phase Buffer Segments. If the 
	 * number of remaining tq is even, the Phase Buffer Segments have the same length, Phase_Seg2 = Phase_Seg1, else 
	 * Phase_Seg2 = Phase_Seg1 + 1.
	 * 
	 * AE: So we have (10 - 3 - 1) = 6tq left for the two phase buffer segments => Phase_Seg2 = Phase_Seg1 = 3tq
	 * 
	 * The minimum nominal length of Phase_Seg2 has to be regarded as well. Phase_Seg2 may not be shorter than any CAN 
	 * controller’s Information Processing Time in the network, which is device dependent and can be in the range of [0…2] tq.
	 * 
	 * The length of the synchronization jump width is set to its maximum value, which is the minimum of four (4) and Phase_Seg1.
	 * 
	 * AE: So SJW = Phase_Seg1 = 3tq
	 * 
	 * The oscillator tolerance range necessary for the resulting configuration is calculated by the formulas given 
	 * in Section 23.3.16.2.3. 
	 * 
	 * If more than one configurations are possible to reach a certain Bit rate, it is recommended to choose the configuration 
	 * which allows the highest oscillator tolerance range. 
	 * 
	 * CAN nodes with different clocks require different configurations to come to the same bit rate. The calculation of the 
	 * propagation time in the CAN network, based on the nodes with the longest delay times, is done once for the whole network. 
	 * 
	 * The CAN system’s oscillator tolerance range is limited by the node with the lowest tolerance range. The calculation may 
	 * show that bus length or bit rate have to be decreased or that the oscillator frequencies’ stability has to be increased 
	 * in order to find a protocol compliant configuration of the CAN bit timing.
	 * 
	 * The resulting configuration is written into the bit timing register:
	 * Tseg2 = Phase_Seg2-1
	 * Tseg1 = Phase_Seg1+ Prop_Seg-1
	 * SJW = SynchronizationJumpWidth-1
	 * BRP = Prescaler-1
	 * 
	 * AE: So we get:
	 * Tseg2 = 3 - 1 = 2
	 * Tseg1 = 3 + 3 - 1 = 5
	 * SJW = 3 - 1 = 2
	 * BRP = 1 - 1 = 0
	 * 
	 * AE: A note on BRP consideration as per user's manual:
	 * Baud rate prescaler. Value by which the CAN_CLK frequency is divided for generating the bit time quanta. The bit time is built 
	 * up from a multiple of this quanta. Valid programmed values are 0 to 63. The actual BRP value interpreted for the bit timing 
	 * will be the programmed BRP value + 1.
	 * 
	 * AE: Let's also calculate oscillator tolerance:
	 * 
	 * The tolerance range df for an oscillator’s frequency fosc around the nominal frequency fnom with:
	 * 
	 * (1 - df) * Fnom < Fosc < Fnom * (1 + df)
	 * 
	 * depends on the proportions of Phase_Seg1, Phase_Seg2, SJW, and the bit time. The maximum tolerance df is the defined by 
	 * two conditions (both shall be met):
	 * 
	 * 1) df <= min(TSeg1, TSeg2) / (2 * (13 * (bit_time - TSeg2)))
	 * 2) df <= SJW / (20 * bit_time)
	 * 
	 * AE: So: 
	 * AE: df <= 2 / 2 * 13 * 8 = 0.0096
	 * AE: df <= 2 / (20 * 10) = 0.01
	 * 
	 * AE: df = 0.96%
	 * 
	 * AE: Turning all that into a register value according to page 4819 (BTR register) in user's manual:
	 * 
	 * BTR = 0xA40;
	 */
	DCAN0_BTR = 0xA40;

	/**
	 * Clear the CCE and Init bit.
	 */
	tmp_val = DCAN0_CTL;
	DCAN0_CTL = tmp_val & 0xffbe; //# resetting bits DCAN0_CTL[0] = Init = 0 and DCAN0_CTL[6] = CCE = 0

	/**
	 * waiting for init bit to be unset
	 */
	while ((DCAN0_CTL & 0x1) == 1);
}

void configureCANObjects() {
	uint32_t tmp_val;
	/**
	 * Now the second part: Configuration of message objects.
	 * Let's configure one receive object and one transmit object in the beginning.
	 * The receive object will also serve as a configuration for remote frames according
	 * to page 4788 in user's guide.
	 *
	 * Let's start with the transmit object.
	 */

	//init_dcan_ram();
	disable_unused_dcan0_objects();
	/**
	 * IF1ARB[31] = MsgVal = 0 : Message not valid yet. We need to set this to 1 after setting up mask bits and Umask = 1
	 * IF1ARB[30] = Xtd = 0 : Standard identifier (not extended) is used 
	 * IF1ARB[29] = Dir = 1 : Direction is "transmit" (on setting TxRqst to 1 this will send
	 * out a data frame or a receipt of a matching ID remote frame will also trigger a send).
	 * IF1ARB[28:18] = ID28_to_ID18 = 0xAE : our node ID in standard mode : 1010 1110
	 * IF1ARB[17:0] = ID17_to_ID0 = 0 : we don't use extended ID mode, so extended ID bits are 0
	 */
	IF1ARB = 0x35C00000;

	/**
	 * IF1MSK[31] = MXtd = 0 : Standard identifier (not extended):
	 * IF1MSK[30] = MDir = 0 : Message direction NOT masked
	 * IF1MSK[28:0] = MSk_28:0_ = 0 : Validate message, whose ID starts with 0xAE
	 */
	IF1MSK = 0x3FE00000; //# We will only accept remote frames from address that contains 0xAE in the beginning

	/**
	 * IF1MCTL[31:16] = 0 : Reserved
	 * IF1MCTL[15] = NewDat = 0 : Setting "new data" flag to false
	 * IF1MCTL[14] = MsgLst = 0 : No message has been lost
	 * IF1MCTL[13] = IntPnd = 0 : No interrupt is pending
	 * IF1MCTL[12] = UMask = 1 : Use acceptance mask (has to be true if we want to automatically answer remote frames).
	 * IF1MCTL[11] = TxIE = 0 : Transmit interrupt not enabled (IntPnd not triggered after transmit of a frame)
	 * IF1MCTL[10] = RxIE = 0 : Receive interrupt not enabled (IntPnd not triggered after reception of a frame)
	 * IF1MCTL[9] = RmtEn = 1 : Automatically reply to a matching remote frame
	 * IF1MCTL[8] = TxRqst = 0 : Transmit request not active at this point (don't want to send anything yet)
	 * IF1MCTL[7] = EoB = 1 : This is the "end of buffer" data frame- i.e. we won't have a FIFO buffer as we only have one object anyway.
	 * IF1MCTL[6:4] = 0 : Reserved
	 * IF1MCTL[3:0] = 0x8 : DLC (data length code) : Data frame will have 8 bytes.
	 */
	IF1MCTL = 0x1288;

	/**
	 * The first 4 bytes of data in the object. Since it's the transmit object,
	 * we want to set the data before we make it active.
	 */
	IF1DATA = 0xAEAEAEAE;

	/**
	 * The last 4 bytes of data in the object. Since it's the transmit object,
	 * we want to set the data before we make it active.
	 */
	IF1DATB = 0xEAEAEAEA;

	tmp_val = IF1ARB;
	IF1ARB = 0x80000000 | tmp_val;

	/**
	 * Similarly we set up the IF2 registers for the "receive" object
	 */
	IF2ARB = 0x15C80000; //# ID will be AE4 : 1010 1110 0100
	IF2MSK = 0x3FE00000; //# We will only accept remote frames from address that contains 0xAE in the beginning
	IF2MCTL = 0x1088; //# RmtEn == 0 because this object will not handle received remote frames
	IF2DATA = 0x0;
	IF2DATB = 0x0;

	tmp_val = IF2ARB;
	IF2ARB = 0x80000000 | tmp_val;

	/**
	 * And finally the IF3 registers for the "receive" object for remote frames
	 */
	IF3ARB = 0x95D00000; //# ID will be AE8 : 1010 1110 1000
	IF3MSK = 0x3FE00000; //# We will only accept remote frames from address that contains 0xAE in the beginning
	IF3MCTL = 0x1088;

	/**
	 * IF1CMD[31:24] = 0 : Reserved
	 * IF1CMD[23] = 1 : Direction- write (from IF1 register set to the message object
	 * addressed by Message Number bits (7:0)
	 * IF1CMD[22] = 1 : Change Mask bits in the message object
	 * IF1CMD[21] = 1 : Change Arbitration bits in the message object
	 * IF1CMD[20] = 1 : Change Access control bits in the message object
	 * IF1CMD[19] = 0 : Ignore this bit. In write mode IntPnd is set by access control register later
	 * IF1CMD[18] = 0 : In write mode TxRqst and NewDat will be set by access control register later
	 * IF1CMD[17] = 1 : Change Data bits[3:0] in the message object
	 * IF1CMD[16] = 1 : Change Data bits[7:4] in the message object
	 * IF1CMD[15] = 0 : Clear the "busy" bit of this register (probably should be r/o anyway)
	 * IF1CMD[14] = 0 : Not going to use DMA yet
	 * IF1CMD[13:8] = 0 : Reserved
	 * IF1CMD[7:0] = 1 : This will be message #1
	 */
	IF1CMD = 0xF30001;
	IF2CMD = 0xF30002;

	/**
	 * Just in case the IF registers are still transferring the data into the DCAN RAM,
	 * we need to wait a little before sending off first data frame.
	 */
	 while ((IF1CMD & 0x8000) == 0x8000);
	 while ((IF2CMD & 0x8000) == 0x8000);
}

void disable_unused_dcan0_objects() {
	uint32_t tmp_val;
	uint8_t current_address = 3;
	
	while (current_address < 0x81) {
		/**
		 * IF1ARB[31] = MsgVal = 0 : Message not valid yet. We need to set this to 1 after setting up mask bits and Umask = 1
		 * IF1ARB[30] = Xtd = 0 : Standard identifier (not extended) is used 
		 * IF1ARB[29] = Dir = 0 : Direction is "transmit" (on setting TxRqst to 1 this will send
		 * out a data frame or a receipt of a matching ID remote frame will also trigger a send).
		 * IF1ARB[28:18] = ID28_to_ID18 = 0xAE : our node ID in standard mode : 1010 1110
		 * IF1ARB[17:0] = ID17_to_ID0 = 0 : we don't use extended ID mode, so extended ID bits are 0
		 */
		IF1ARB = 0;

		/**
		 * IF1MSK[31] = MXtd = 0 : Standard identifier (not extended):
		 * IF1MSK[30] = MDir = 0 : Message direction NOT masked
		 * IF1MSK[28:0] = MSk_28:0_ = 0 : Validate message, whose ID starts with 0xAE
		 */
		IF1MSK = 0;

		/**
		 * IF1MCTL[31:16] = 0 : Reserved
		 * IF1MCTL[15] = NewDat = 0 : Setting "new data" flag to false
		 * IF1MCTL[14] = MsgLst = 0 : No message has been lost
		 * IF1MCTL[13] = IntPnd = 0 : No interrupt is pending
		 * IF1MCTL[12] = UMask = 0 : Use acceptance mask (has to be true if we want to automatically answer remote frames).
		 * IF1MCTL[11] = TxIE = 0 : Transmit interrupt not enabled (IntPnd not triggered after transmit of a frame)
		 * IF1MCTL[10] = RxIE = 0 : Receive interrupt not enabled (IntPnd not triggered after reception of a frame)
		 * IF1MCTL[9] = RmtEn = 0 : Automatically reply to a matching remote frame
		 * IF1MCTL[8] = TxRqst = 0 : Transmit request not active at this point (don't want to send anything yet)
		 * IF1MCTL[7] = EoB = 1 : This is the "end of buffer" data frame- i.e. we won't have a FIFO buffer as we only have one object anyway.
		 * IF1MCTL[6:4] = 0 : Reserved
		 * IF1MCTL[3:0] = 0x8 : DLC (data length code) : Data frame will have 8 bytes.
		 */
		IF1MCTL = 0x80;

		/**
		 * The first 4 bytes of data in the object. Since it's the transmit object,
		 * we want to set the data before we make it active.
		 */
		IF1DATA = 0;

		/**
		 * The last 4 bytes of data in the object. Since it's the transmit object,
		 * we want to set the data before we make it active.
		 */
		IF1DATB = 0;

		/**
		 * Similarly we set up the IF2 registers for the "receive" object
		 */
		IF2ARB = 0;
		IF2MSK = 0;
		IF2MCTL = 0x80;
		IF2DATA = 0;
		IF2DATB = 0;

		/**
		 * And finally the IF3 registers for the "receive" object for remote frames
		 */
		IF3ARB = 0;
		IF3MSK = 0;
		IF3MCTL = 0x80;

		/**
		 * IF1CMD[31:24] = 0 : Reserved
		 * IF1CMD[23] = 1 : Direction- write (from IF1 register set to the message object
		 * addressed by Message Number bits (7:0)
		 * IF1CMD[22] = 1 : Change Mask bits in the message object
		 * IF1CMD[21] = 1 : Change Arbitration bits in the message object
		 * IF1CMD[20] = 1 : Change Access control bits in the message object
		 * IF1CMD[19] = 0 : Ignore this bit. In write mode IntPnd is set by access control register later
		 * IF1CMD[18] = 0 : In write mode TxRqst and NewDat will be set by access control register later
		 * IF1CMD[17] = 1 : Change Data bits[3:0] in the message object
		 * IF1CMD[16] = 1 : Change Data bits[7:4] in the message object
		 * IF1CMD[15] = 0 : Clear the "busy" bit of this register (probably should be r/o anyway)
		 * IF1CMD[14] = 0 : Not going to use DMA yet
		 * IF1CMD[13:8] = 0 : Reserved
		 * IF1CMD[7:0] = xxxxxxxx : Message number
		 */
		IF1CMD = 0xF30000 + current_address;
		IF2CMD = 0xF30000 + current_address;

		/**
		 * Just in case the IF registers are still transferring the data into the DCAN RAM,
		 * we need to wait a little before sending off first data frame.
		 */
		while ((IF1CMD & 0x8000) == 0x8000);
		while ((IF2CMD & 0x8000) == 0x8000);

		current_address++;
	}
}

/**
 * Initialise DCAN0 ram. Doesn't work, because we need the main processor (Cortex A8) to be in privileged mode
 * and I'm not sure yet how to set it to that mode from a PRU core (may not even be possible). I may need to
 * do this kind of initialisation in the device tree or bootloader.
 */
void init_dcan_ram() {
	volatile uint8_t *ptr_cm;

	ptr_cm = CONTROL_MODULE;
	ptr_cm[CTRL_MOD_DCAN_RAMINIT_offset] = 0x1;
	while ((ptr_cm[CTRL_MOD_DCAN_RAMINIT_offset] & 0x100) == 0x0);
}

/* INTC configuration
 * We are going to map User event 16 to Host 1
 * PRU1 will then wait for r31 bit 31 (designates Host 1) to go high
 * */
void configIntc(void)
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
