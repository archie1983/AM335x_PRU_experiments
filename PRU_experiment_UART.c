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

/* This is a char so that I can force access to R31.b0 for the host interrupt */
volatile register uint8_t __R31;

/* PRU-to-ARM interrupt */
#define PRU_ARM_INTERRUPT (19+16)

int main(void)
{
	uint32_t result;
	volatile uint8_t *ptr_cm;

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

	/* Access PRCM (without CT) to initialize McSPI0 clock */
	ptr_cm[UART1_CLKCTRL] = ON;

	/* Read McSPI0_MODULCTRL (offset 0x128)*/
	//result = UART1_MODULCTRL;
	/**
	 * Somewhere here we should set up UART parameters (baud rate and stuff)
	 */

	/* Attempting to write lower case letter "a" to the TX of UART1 */
	UART1_THR = 0x60;

	/* Reset MCSPI0_MODULCTRL[MS] to original value */
	//MCSPI0_MODULCTRL = result;

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

	/* Halt PRU core */
	__halt();
}
