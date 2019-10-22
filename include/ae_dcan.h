/*
 * Declarations for CAN functionality
 */
#include <stdint.h>

#ifndef PRU_SRAM
#define PRU_SRAM __far __attribute__((cregister("PRU_SHAREDMEM", near)))
#endif

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

void startDCANClock(void);
void setUpCANTimings(void);
void configureCANObjects(void);
void pokePRU1Processor(void);
void transmitDataFrame(void);
void transmitRemoteFrame(void);
void init_dcan_ram(void);
void disable_unused_dcan0_objects(void);
void readReceivedDataFrame(uint8_t *);
void loadDataToSendInDataFrame(uint8_t *);
