/*
 * Copyright (C) 2015 Texas Instruments Incorporated - http://www.ti.com/
 *
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *	* Redistributions of source code must retain the above copyright
 *	  notice, this list of conditions and the following disclaimer.
 *
 *	* Redistributions in binary form must reproduce the above copyright
 *	  notice, this list of conditions and the following disclaimer in the
 *	  documentation and/or other materials provided with the
 *	  distribution.
 *
 *	* Neither the name of Texas Instruments Incorporated nor the names of
 *	  its contributors may be used to endorse or promote products derived
 *	  from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>
#include <pru_cfg.h>
#include <pru_ctrl.h>
#include "resource_table_empty.h"

/* Mapping Constant Table (CT) registers to variables */
volatile far uint8_t CT_UART1 __attribute__((cregister("UART1", near), peripheral));

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
 * Page 1273 in SPRUH73P–October 2011–Revised March 2017
 */
#define CM_PER_BASE	((volatile uint8_t *)(0x44E00000))

/**
 * ### AE: ###
 * Page 1250 in SPRUH73P–October 2011–Revised March 2017
 */
#define UART1_CLKCTRL (0x6C)

/**
 * ### AE: ###
 * Page 1273 in SPRUH73P–October 2011–Revised March 2017
 */
#define ON (0x2)

/**
 * ### AE: ###
 * Taking CT_UART1 register address, adding 0x0 to that address, 
 * then casting it to a volatile uint16_t pointer and finally taking 
 * the value that is pointed to by the address held in the pointer 
 * and using that value. That should give us a direct access to the THR
 * register of the UART1. So if we write to it, then the UART should shift
 * out the written data.
 */
#define UART1_THR (*((volatile uint16_t*)(&CT_UART1 + 0x0)))

/* UART 1 system control as described in pages 4374 and 4421 of SPRUH73P–October 2011–Revised March 2017 */
#define UART1_SYSC (*((volatile uint16_t*)(&CT_UART1 + 0x54)))
/* UART 1 system status */
#define UART1_SYSS (*((volatile uint16_t*)(&CT_UART1 + 0x58)))

/* other UART1 registers as defined in pages 4374 and below of SPRUH73P–October 2011–Revised March 2017 */
#define UART1_MDR1 (*((volatile uint16_t*)(&CT_UART1 + 0x20))) //# mode definition register 1
#define UART1_LCR (*((volatile uint16_t*)(&CT_UART1 + 0x0C))) //# line control register
#define UART1_EFR (*((volatile uint16_t*)(&CT_UART1 + 0x08))) //# enhanced feature register
#define UART1_IER (*((volatile uint16_t*)(&CT_UART1 + 0x04))) //# interrupt enable register
#define UART1_DLL (*((volatile uint16_t*)(&CT_UART1 + 0x0))) //# divisor latches low register
#define UART1_DLH (*((volatile uint16_t*)(&CT_UART1 + 0x04))) //# divisor latches high register


/* This is a char so that I can force access to R31.b0 for the host interrupt */
volatile register uint8_t __R31;

/* PRU-to-ARM interrupt */
#define PRU_ARM_INTERRUPT (19+16)

int main(void)
{
	uint32_t result;
	volatile uint8_t *ptr_cm;
	uint16_t tmp_val, uart_efr_val;

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

	/* Access PRCM (without CT) to initialize UART1 clock */
	ptr_cm[UART1_CLKCTRL] = ON;

	/* Read McSPI0_MODULCTRL (offset 0x128)*/
	//result = UART1_MODULCTRL;
	/**
	 * Somewhere here we should set up UART parameters (baud rate and stuff)
	 */

	/**
	 * Let's start by resetting UART and clearing its registers according to page 4365 in SPRUH73P–October 2011–Revised March 2017 
	 */
	tmp_val = UART1_SYSC;
	UART1_SYSC = tmp_val & 0xfffd; //# setting bit 1 to 0.
	while (UART1_SYSS & 0x1 == 0); //# keep monitoring UART1_SYSS.RESETDONE bit until it is no longer 0.
	  
	/**
	 * UART has now been reset. We need to configure its protocol, baud rate and interrupt settings
	 * as explained in on page 4366 in SPRUH73P–October 2011–Revised March 2017 
	 */
	tmp_val = UART1_MDR1;
	UART1_MDR1 = (tmp_val & 0xfff8) | 0x7; //# setting the bottom 3 bits to 1 to disable UART
	UART1_LCR = 0xbf; //# entering configuration mode B to access UART_EFR register
	uart_efr_val = UART1_EFR; //# saving UART_EFR, because later we'll need to restore the UART_EFR.ENHANCED_EN value (UART1_EFR >> 4 & 0x1)
	UART1_EFR = uart_efr_val & 0xffef; //# setting bit 4 (UART_EFR.ENHANCED_EN) to 1 to enable access to UART_IER[7:4] register
	UART1_LCR = 0x0; //# switching to register operational mode to access the UART_IER register.
	UART1_IER = 0x0; //# clearing UART_IER register (setting UART_IER[4] SLEEP_MODE bit to 0) to change UART_DLL and UART_DLH registers
	UART1_LCR = 0xbf; //# entering configuration mode B to access UART_DLL and UART_DLH registers
	//# Setting UART_DLL[7:0] CLOCK_LSB and UART_DLH[5:0] CLOCK_MSB fields to the desired values to load the new divisor value
	//# We're aiming for 115200 baud rate, so DLL = 0x1A and DLH = 0x0 as described in section 19.3.8.1.2, page 4344 of
	//# SPRUH73P–October 2011–Revised March 2017
	UART_DLL = 0x1a;
	UART_DLH = 0x0;
	UART1_LCR = 0x0; //# switching to register operational mode to access the UART_IER register.
	//# Loading the new interrupt configuration and other UART settings
	//# UARTi.UART_IER[7] CTS_IT
	//# UARTi.UART_IER[6] RTS_IT
	//# UARTi.UART_IER[5] XOFF_IT
	//# UARTi.UART_IER[4] SLEEP_MODE
	//# UARTi.UART_IER[3] MODEM_STS_IT
	//# UARTi.UART_IER[2] LINE_STS_IT
	//# UARTi.UART_IER[1] THR_IT
	//# UARTi.UART_IER[0] RHR_IT
	UART_IER = 0x0; //# for now disabling all interupts (as we only have TX, RX and ground lines anyway) and disabling sleep mode
	UART1_LCR = 0xbf; //# entering configuration mode B to access UART_EFR register
	UART_EFR = (UART_EFR & 0xffef) | (uart_efr_val & 0x10); //# restoring UART_EFR[4] ENHANCED_EN value saved earlier
	//# loading new protocol formatting: 
	//# UART_LCR[7] DIV_EN = 0 (normal operation)
	//# UART_LCR[6] BREAK_EN = 0 (normal operation)
	//# UART_LCR[5] PARITY_TYPE_2 = 0 (parity: none)
	//# UART_LCR[4] PARITY_TYPE_1 = 0 (parity: none)
	//# UART_LCR[3] PARITY_EN = 0 (parity: none)
	//# UART_LCR[2] NB_STOP = 0 (1 stop bit)
	//# UART_LCR[1:0] CHAR_LENGTH = 0x3 (data bits = 8)
	UART1_LCR = 0x3;
	//# refer to 19.3.8, page 4342; 19.2.2, page 4320 and 19.3.7.2, page 4339.
	UART1_MDR1 = 0x0; //# loading the new UART mode: UART_MDR1[2:0] MODE_SELECT = 0 to enable UART 16x mode

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


	/* Attempting to write lower case letter "a" to the TX of UART1 once every second */
	while (1) {
		__delay_cycles(100000000); //# 500ms wait
		UART1_THR = 0x60;
	}

	/* Halt PRU core */
	__halt();
}
