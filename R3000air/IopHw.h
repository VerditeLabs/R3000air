/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2009  Pcsx2 Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#pragma once

#include "IopMem.h"

static const u32
	HW_USB_START	= 0x1f801600,
	HW_USB_END		= 0x1f801700,
	HW_FW_START		= 0x1f808400,
	HW_FW_END		= 0x1f808550,	// end addr for FW is a guess...
	HW_SPU2_START	= 0x1f801c00,
	HW_SPU2_END		= 0x1f801e00;
	
static const u32
	HW_SSBUS_SPD_ADDR	= 0x1f801000,
	HW_SSBUS_PIO_ADDR	= 0x1f801004,
	HW_SSBUS_SPD_DELAY	= 0x1f801008,
	HW_SSBUS_DEV1_DELAY	= 0x1f80100C,
	HW_SSBUS_ROM_DELAY	= 0x1f801010,
	HW_SSBUS_SPU_DELAY	= 0x1f801014,
	HW_SSBUS_DEV5_DELAY	= 0x1f801018,
	HW_SSBUS_PIO_DELAY	= 0x1f80101c,
	HW_SSBUS_COM_DELAY	= 0x1f801020,
	
	HW_SIO_DATA			= 0x1f801040,	// SIO read/write register
	HW_SIO_STAT			= 0x1f801044,
	HW_SIO_MODE			= 0x1f801048,
	HW_SIO_CTRL			= 0x1f80104a,
	HW_SIO_BAUD			= 0x1f80104e,

	HW_IREG				= 0x1f801070,
	HW_IMASK			= 0x1f801074,
	HW_ICTRL			= 0x1f801078,

	HW_SSBUS_DEV1_ADDR	= 0x1f801400,
	HW_SSBUS_SPU_ADDR	= 0x1f801404,
	HW_SSBUS_DEV5_ADDR	= 0x1f801408,
	HW_SSBUS_SPU1_ADDR	= 0x1f80140c,
	HW_SSBUS_DEV9_ADDR3	= 0x1f801410,
	HW_SSBUS_SPU1_DELAY	= 0x1f801414,
	HW_SSBUS_DEV9_DELAY2= 0x1f801418,
	HW_SSBUS_DEV9_DELAY3= 0x1f80141c,
	HW_SSBUS_DEV9_DELAY1= 0x1f801420,

	HW_ICFG				= 0x1f801450,
	HW_DEV9_DATA		= 0x1f80146e,	// DEV9 read/write register

	// CDRom registers are used for various command, status, and data stuff.

	HW_CDR_DATA0		= 0x1f801800,	// CDROM multipurpose data register 1
	HW_CDR_DATA1		= 0x1f801801,	// CDROM multipurpose data register 2
	HW_CDR_DATA2		= 0x1f801802,	// CDROM multipurpose data register 3
	HW_CDR_DATA3		= 0x1f801803,	// CDROM multipurpose data register 4

	// SIO2 is a DMA interface for the SIO.

	HW_SIO2_DATAIN		= 0x1F808260,
	HW_SIO2_FIFO		= 0x1f808264,
	HW_SIO2_CTRL		= 0x1f808268,
	HW_SIO2_RECV1		= 0x1f80826c,
	HW_SIO2_RECV2		= 0x1f808270,
	HW_SIO2_RECV3		= 0x1f808274,
	HW_SIO2_INTR		= 0x1f808280;


/* Registers for the IOP Counters */
enum IOPCountRegs
{
	IOP_T0_COUNT = 0x1f801100,
	IOP_T1_COUNT = 0x1f801110,
	IOP_T2_COUNT = 0x1f801120,
	IOP_T3_COUNT = 0x1f801480,
	IOP_T4_COUNT = 0x1f801490,
	IOP_T5_COUNT = 0x1f8014a0,
			
	IOP_T0_MODE = 0x1f801104,
	IOP_T1_MODE = 0x1f801114,
	IOP_T2_MODE = 0x1f801124,
	IOP_T3_MODE = 0x1f801484,
	IOP_T4_MODE = 0x1f801494,
	IOP_T5_MODE = 0x1f8014a4,
			
	IOP_T0_TARGET = 0x1f801108,
	IOP_T1_TARGET = 0x1f801118,
	IOP_T2_TARGET = 0x1f801128,
	IOP_T3_TARGET = 0x1f801488,
	IOP_T4_TARGET = 0x1f801498,
	IOP_T5_TARGET = 0x1f8014a8
};

// fixme: I'm sure there's a better way to do this. --arcum42
#define DmaExec(n) { \
	if (HW_DMA##n##_CHCR & 0x01000000 && \
		HW_DMA_PCR & (8 << (n * 4))) { \
		psxDma##n(HW_DMA##n##_MADR, HW_DMA##n##_BCR, HW_DMA##n##_CHCR); \
	} \
}

#define DmaExec2(n) { \
	if (HW_DMA##n##_CHCR & 0x01000000 && \
		HW_DMA_PCR2 & (8 << ((n-7) * 4))) { \
		psxDma##n(HW_DMA##n##_MADR, HW_DMA##n##_BCR, HW_DMA##n##_CHCR); \
	} \
}

#ifdef ENABLE_NEW_IOPDMA
#define DmaExecNew(n) { \
	if (HW_DMA##n##_CHCR & 0x01000000 && \
		HW_DMA_PCR & (8 << (n * 4))) { \
		IopDmaStart(n, HW_DMA##n##_CHCR, HW_DMA##n##_MADR, HW_DMA##n##_BCR); \
	} \
}

#define DmaExecNew2(n) { \
	if (HW_DMA##n##_CHCR & 0x01000000 && \
		HW_DMA_PCR2 & (8 << ((n-7) * 4))) { \
		IopDmaStart(n, HW_DMA##n##_CHCR, HW_DMA##n##_MADR, HW_DMA##n##_BCR); \
	} \
}
#else
#define DmaExecNew(n) DmaExec(n)
#define DmaExecNew2(n) DmaExec2(n)
#endif

#define HW_DMA0_MADR (psxHu32(0x1080)) // MDEC in DMA
#define HW_DMA0_BCR  (psxHu32(0x1084))
#define HW_DMA0_CHCR (psxHu32(0x1088))

#define HW_DMA1_MADR (psxHu32(0x1090)) // MDEC out DMA
#define HW_DMA1_BCR  (psxHu32(0x1094))
#define HW_DMA1_CHCR (psxHu32(0x1098))

#define HW_DMA2_MADR (psxHu32(0x10a0)) // GPU DMA
#define HW_DMA2_BCR  (psxHu32(0x10a4))
#define HW_DMA2_CHCR (psxHu32(0x10a8))
#define HW_DMA2_TADR (psxHu32(0x10ac))

#define HW_DMA3_MADR (psxHu32(0x10b0)) // CDROM DMA
#define HW_DMA3_BCR  (psxHu32(0x10b4))
#define HW_DMA3_BCR_L16 (psxHu16(0x10b4))
#define HW_DMA3_BCR_H16 (psxHu16(0x10b6))
#define HW_DMA3_CHCR (psxHu32(0x10b8))

#define HW_DMA4_MADR (psxHu32(0x10c0)) // SPU DMA
#define HW_DMA4_BCR  (psxHu32(0x10c4))
#define HW_DMA4_CHCR (psxHu32(0x10c8))
#define HW_DMA4_TADR (psxHu32(0x10cc))

#define HW_DMA6_MADR (psxHu32(0x10e0)) // GPU DMA (OT)
#define HW_DMA6_BCR  (psxHu32(0x10e4))
#define HW_DMA6_CHCR (psxHu32(0x10e8))

#define HW_DMA7_MADR (psxHu32(0x1500)) // SPU2 DMA
#define HW_DMA7_BCR  (psxHu32(0x1504))
#define HW_DMA7_CHCR (psxHu32(0x1508))

#define HW_DMA8_MADR (psxHu32(0x1510)) // DEV9 DMA
#define HW_DMA8_BCR  (psxHu32(0x1514))
#define HW_DMA8_CHCR (psxHu32(0x1518))

#define HW_DMA9_MADR (psxHu32(0x1520)) // SIF0 DMA
#define HW_DMA9_BCR  (psxHu32(0x1524))
#define HW_DMA9_CHCR (psxHu32(0x1528))
#define HW_DMA9_TADR (psxHu32(0x152c))

#define HW_DMA10_MADR (psxHu32(0x1530)) // SIF1 DMA
#define HW_DMA10_BCR  (psxHu32(0x1534))
#define HW_DMA10_CHCR (psxHu32(0x1538))

#define HW_DMA11_MADR (psxHu32(0x1540)) // SIO2 in
#define HW_DMA11_BCR  (psxHu32(0x1544))
#define HW_DMA11_CHCR (psxHu32(0x1548))

#define HW_DMA12_MADR (psxHu32(0x1550)) // SIO2 out
#define HW_DMA12_BCR  (psxHu32(0x1554))
#define HW_DMA12_CHCR (psxHu32(0x1558))

#define HW_DMA_PCR   (psxHu32(0x10f0))
#define HW_DMA_ICR   (psxHu32(0x10f4))

#define HW_DMA_PCR2  (psxHu32(0x1570))
#define HW_DMA_ICR2  (psxHu32(0x1574))

// ------------------------------------------------------------------------
// IopInterrupts : These represent the bits in the 0x1070 (INTC_STAT) register, and
// when enabled raise exceptions to the R3000A cpu.
//
// Parts commented out are unused in the IOP, and are likely from PSX/PS1 origins.
//
enum IopInterrupts
{
	IopInt_VBlank	= 0,
	IopInt_GM		= 1,
	IopInt_CDROM	= 2,
	IopInt_DMA		= 3,

	IopInt_RTC0		= 4,
	IopInt_RTC1		= 5,
	IopInt_RTC2		= 6,

	IopInt_SIO0		= 7,
	IopInt_SIO1		= 8,
	IopInt_SPU2		= 9,
	IopInt_PIO		= 10,
	IopInt_VBlankEnd= 11,
	IopInt_DVD		= 12,
	IopInt_DEV9		= 13,

	IopInt_RTC3		= 14,
	IopInt_RTC4		= 15,
	IopInt_RTC5		= 16,

	IopInt_SIO2		= 17,

	IopInt_HTR0		= 18,
	IopInt_HTR1		= 19,
	IopInt_HTR2		= 20,
	IopInt_HTR3		= 21,

	IopInt_USB		= 22,
	IopInt_EXTR		= 23,
	IopInt_FireWire	= 24,
	IopInt_FDMA		= 25  
};


extern void psxHwReset();
extern void psxDmaInterrupt(int n);
extern void psxDmaInterrupt2(int n);
