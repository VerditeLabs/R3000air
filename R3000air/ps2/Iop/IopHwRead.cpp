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

#include "PrecompiledHeader.h"
#include "IopHw_Internal.h"

namespace IopMemory
{
using namespace Internal;

//////////////////////////////////////////////////////////////////////////////////////////
//
mem8_t __fastcall iopHwRead8_Page1( u32 addr )
{
	HwAddrPrep( 1 );

	u32 masked_addr = addr & 0x0fff;

	mem8_t ret;		// using a return var can be helpful in debugging.
	switch( masked_addr )
	{		
		mcase(HW_SIO_DATA): ret = sioRead8(); break;

		// for use of serial port ignore for now
		//case 0x50: ret = serial_read8(); break;

		mcase(HW_DEV9_DATA): ret = DEV9read8( addr ); break;

		mcase(HW_CDR_DATA0): ret = cdrRead0(); break;
		mcase(HW_CDR_DATA1): ret = cdrRead1(); break;
		mcase(HW_CDR_DATA2): ret = cdrRead2(); break;
		mcase(HW_CDR_DATA3): ret = cdrRead3(); break;

		default:
			if( masked_addr >= 0x100 && masked_addr < 0x130 )
			{
				DevCon::Notice( "HwRead8 from Counter16 [ignored], addr 0x%08x = 0x%02x", params addr, psxHu8(addr) );
				ret = psxHu8( addr );
			}
			else if( masked_addr >= 0x480 && masked_addr < 0x4a0 )
			{
				DevCon::Notice( "HwRead8 from Counter32 [ignored], addr 0x%08x = 0x%02x", params addr, psxHu8(addr) );
				ret = psxHu8( addr );
			}
			else if( (masked_addr >= pgmsk(HW_USB_START)) && (masked_addr < pgmsk(HW_USB_END)) )
			{
				ret = USBread8( addr );
				PSXHW_LOG( "HwRead8 from USB, addr 0x%08x = 0x%02x", addr, ret );
			}
			else
			{
				ret = psxHu8(addr);
				PSXHW_LOG( "HwRead8 from Unknown, addr 0x%08x = 0x%02x", addr, ret );
			}
		return ret;
	}

	PSXHW_LOG( "HwRead8 from %s, addr 0x%08x = 0x%02x", _log_GetIopHwName<mem8_t>( addr ), addr, ret );
	return ret;
}


//////////////////////////////////////////////////////////////////////////////////////////
//
mem8_t __fastcall iopHwRead8_Page3( u32 addr )
{
	// all addresses are assumed to be prefixed with 0x1f803xxx:
	HwAddrPrep( 3 );

	mem8_t ret;
	if( addr == 0x1f803100 )	// PS/EE/IOP conf related
		ret = 0x10; // Dram 2M
	else
		ret = psxHu8( addr );

	PSXHW_LOG( "HwRead8 from %s, addr 0x%08x = 0x%02x", _log_GetIopHwName<mem8_t>( addr ), addr, psxHu8(addr) );
	return ret;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
mem8_t __fastcall iopHwRead8_Page8( u32 addr )
{
	HwAddrPrep( 8 );

	mem8_t ret;

	if( addr == HW_SIO2_FIFO )
		ret = sio2_fifoOut();//sio2 serial data feed/fifo_out
	else
		ret = psxHu8( addr );

	PSXHW_LOG( "HwRead8 from %s, addr 0x%08x = 0x%02x", _log_GetIopHwName<mem8_t>( addr ), addr, psxHu8(addr) );
	return ret;	
}

/*template< typename T >
static __forceinline T _read_hwreg( u32 addr, const u32 src )
{
	const int alignpos = (addr & 0x3);
	return (T)( src >> (alignpos*8) );
}*/

template< typename T >
static __forceinline T _read_hwreg( u32 addr, const u32& src )
{
	const int alignpos = (addr & 0x3);
	return *(T*)( ((u8*)&src) + alignpos );
}

//////////////////////////////////////////////////////////////////////////////////////////
//
template< typename T >
static __forceinline T _HwRead_16or32_Page1( u32 addr )
{
	HwAddrPrep( 1 );

	// all addresses should be aligned to the data operand size:
	jASSUME(
		( sizeof(T) == 2 && (addr & 1) == 0 ) ||
		( sizeof(T) == 4 && (addr & 3) == 0 )
	);

	u32 masked_addr = pgmsk( addr );
	T ret;

	// ------------------------------------------------------------------------
	// Counters, 16-bit varieties!
	//
	// Note: word reads/writes to the uppoer halfword of the 16 bit registers should 
	// just map to the HW memory map (tested on real IOP) -- ie, a write to the upper
	// halfword of 0xcccc will have those upper values return 0xcccc always.
	//
	if( masked_addr >= 0x100 && masked_addr < 0x130 )
	{
		int cntidx = ( masked_addr >> 4 ) & 0xf;
		switch( masked_addr & 0xf )
		{
			case 0x0:
				ret = (T)IopCounters::ReadCount16( cntidx );
			break;

			case 0x4:
				ret = IopCounters::ReadMode( cntidx );
			break;

			case 0x8:
				ret = (T)IopCounters::ReadTarget16( cntidx );
			break;
			
			default:
				ret = psxHu32(addr);
			break;
		}
	}
	// ------------------------------------------------------------------------
	// Counters, 32-bit varieties!
	//
	else if( masked_addr >= 0x480 && masked_addr < 0x4b0 )
	{
		int cntidx = (( masked_addr >> 4 ) & 0xf) - 5;
		switch( masked_addr & 0xf )
		{
			case 0x0:
				ret = IopCounters::ReadCount32( cntidx );
			break;

			case 0x4:
				ret = IopCounters::ReadMode( cntidx );
			break;

			case 0x8:
				ret = IopCounters::ReadTarget32( cntidx );
			break;

			default:
				ret = psxHu32(addr);
			break;
		}
	}
	// ------------------------------------------------------------------------
	// USB, with both 16 and 32 bit interfaces
	//
	else if( masked_addr >= pgmsk(HW_USB_START) && masked_addr < pgmsk(HW_USB_END) )
	{
		ret = (sizeof(T) == 2) ? USBread16( addr ) : USBread32( addr );
	}
	// ------------------------------------------------------------------------
	// SPU2, accessible in 16 bit mode only!
	//
	else if( masked_addr >= pgmsk(HW_SPU2_START) && masked_addr < pgmsk(HW_SPU2_END) )
	{
		if( sizeof(T) == 2 )
			ret = SPU2read( addr );
		else
		{
			DevCon::Notice( "HwRead32 from SPU2? (addr=0x%08X) .. What manner of trickery is this?!", params addr );
			ret = psxHu32(addr);
		}
	}
	else
	{
		switch( masked_addr )
		{
			// ------------------------------------------------------------------------
			mcase(HW_SIO_DATA):
				ret  = sioRead8();
				ret |= sioRead8() << 8;
				if( sizeof(T) == 4 )
				{
					ret |= sioRead8() << 16;
					ret |= sioRead8() << 24;
				}
			break;

			mcase(HW_SIO_STAT):
				ret = sio.StatReg;
			break;

			mcase(HW_SIO_MODE):
				ret = sio.ModeReg;
				if( sizeof(T) == 4 )
				{
					// My guess on 32-bit accesses.  Dunno yet what the real hardware does. --air
					ret |= sio.CtrlReg << 16;
				}
			break;

			mcase(HW_SIO_CTRL):
				ret = sio.CtrlReg;
			break;

			mcase(HW_SIO_BAUD):
				ret = sio.BaudReg;
			break;

			// ------------------------------------------------------------------------
			//Serial port stuff not support now ;P
			// case 0x050: hard = serial_read32(); break;
			//	case 0x054: hard = serial_status_read(); break;
			//	case 0x05a: hard = serial_control_read(); break;
			//	case 0x05e: hard = serial_baud_read(); break;

			mcase(HW_ICTRL):
				ret = psxHu32(0x1078);
				psxHu32(0x1078) = 0;
			break;

			mcase(HW_ICTRL+2):
				ret = psxHu16(0x107a);
				psxHu32(0x1078) = 0;	// most likely should clear all 32 bits here.
			break;

			// ------------------------------------------------------------------------
			// Soon-to-be outdated SPU2 DMA hack (spu2 manages its own DMA MADR).
			//
			mcase(0x1f8010C0):
				ret = SPU2ReadMemAddr(0);
			break;

			mcase(0x1f801500):
				ret = SPU2ReadMemAddr(1);
			break;

			// ------------------------------------------------------------------------
			// Legacy GPU  emulation (not needed).
			// The IOP emulates the GPU itself through the EE's hardware.

			/*case 0x810:
				PSXHW_LOG("GPU DATA 32bit write %lx", value);
				GPU_writeData(value); return;
			case 0x814:
				PSXHW_LOG("GPU STATUS 32bit write %lx", value);
				GPU_writeStatus(value); return;

			case 0x820:
				mdecWrite0(value); break;
			case 0x824:
				mdecWrite1(value); break;*/

			// ------------------------------------------------------------------------

			mcase(0x1f80146e):
				ret = DEV9read16( addr );
			break;

			default:
				ret = psxHu32(addr);
			break;
		}
	}
	
	PSXHW_LOG( "HwRead%s from %s, addr 0x%08x = 0x%04x",
		(sizeof(T) == 2) ? "16" : "32", _log_GetIopHwName<T>( addr ), addr, ret
	);
	
	//return _read_hwreg<T>( masked_addr, ret );
	return ret;
}

// Some Page 2 mess?  I love random question marks for comments!
//case 0x1f802030: hard =   //int_2000????
//case 0x1f802040: hard =//dip switches...??

//////////////////////////////////////////////////////////////////////////////////////////
//
mem16_t __fastcall iopHwRead16_Page1( u32 addr )
{
	return _HwRead_16or32_Page1<mem16_t>( addr );
}

//////////////////////////////////////////////////////////////////////////////////////////
//
mem16_t __fastcall iopHwRead16_Page3( u32 addr )
{
	HwAddrPrep( 3 );

	mem16_t ret = psxHu16(addr);
	PSXHW_LOG( "HwRead16 from %s, addr 0x%08x = 0x%04x", _log_GetIopHwName<mem16_t>( addr ), addr, ret );
	return ret;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
mem16_t __fastcall iopHwRead16_Page8( u32 addr )
{
	HwAddrPrep( 8 );

	mem16_t ret = psxHu16(addr);
	PSXHW_LOG( "HwRead16 from %s, addr 0x%08x = 0x%04x", _log_GetIopHwName<mem16_t>( addr ), addr, ret );
	return ret;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
mem32_t __fastcall iopHwRead32_Page1( u32 addr )
{
	return _HwRead_16or32_Page1<mem32_t>( addr );
}

//////////////////////////////////////////////////////////////////////////////////////////
//
mem32_t __fastcall iopHwRead32_Page3( u32 addr )
{
	HwAddrPrep( 3 );
	const mem32_t ret = psxHu32(addr);
	PSXHW_LOG( "HwRead32 from %s, addr 0x%08x = 0x%08x", _log_GetIopHwName<mem32_t>( addr ), addr, ret );
	return ret;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
mem32_t __fastcall iopHwRead32_Page8( u32 addr )
{
	HwAddrPrep( 8 );

	u32 masked_addr = addr & 0x0fff;
	mem32_t ret;

	if( masked_addr >= 0x200 )
	{
		if( masked_addr < 0x240 )
		{
			const int parm = (masked_addr-0x200) / 4;
			ret = sio2_getSend3( parm );
		}
		else if( masked_addr < 0x260 )
		{
			// SIO2 Send commands alternate registers.  First reg maps to Send1, second
			// to Send2, third to Send1, etc.  And the following clever code does this:
			
			const int parm = (masked_addr-0x240) / 8;
			ret = (masked_addr & 4) ? sio2_getSend2( parm ) : sio2_getSend1( parm );
		}
		else if( masked_addr <= 0x280 )
		{
			switch( masked_addr )
			{
				mcase(HW_SIO2_CTRL):	ret = sio2_getCtrl();	break;
				mcase(HW_SIO2_RECV1):	ret = sio2_getRecv1();	break;
				mcase(HW_SIO2_RECV2):	ret = sio2_getRecv2();	break;
				mcase(HW_SIO2_RECV3):	ret = sio2_getRecv3();	break;
				mcase(0x1f808278):		ret = sio2_get8278();	break;
				mcase(0x1f80827C):		ret = sio2_get827C();	break;
				mcase(HW_SIO2_INTR):	ret = sio2_getIntr();	break;
				
				// HW_SIO2_FIFO -- A yet unknown: Should this be ignored on 32 bit writes, or handled as a
				// 4-byte FIFO input?
				// The old IOP system just ignored it, so that's what we do here.  I've included commented code
				// for treating it as a 16/32 bit write though [which si what the SIO does, for example).
				mcase(HW_SIO2_FIFO):
					//ret = sio2_fifoOut();
					//ret |= sio2_fifoOut() << 8;
					//ret |= sio2_fifoOut() << 16;
					//ret |= sio2_fifoOut() << 24;
				//break;

				default:
					ret = psxHu32(addr);
				break;
			}
		}
		else if( masked_addr >= pgmsk(HW_FW_START) && masked_addr <= pgmsk(HW_FW_END) )
		{
			ret = FWread32( addr );
		}
	}
	else ret = psxHu32(addr);

	PSXHW_LOG( "HwRead32 from %s, addr 0x%08x = 0x%02x", _log_GetIopHwName<mem32_t>( addr ), addr, ret );
	return ret;
}


//////////////////////////////////////////////////////////////////////////////////////////
//
__forceinline u8 __fastcall IopMemory::iopHw4Read8(u32 add) 
{
	u16 mem = add & 0xFF;
	u8 ret = cdvdRead(mem);
	PSXHW_LOG("HwRead8 from Cdvd [segment 0x1f40], addr 0x%02x = 0x%02x", mem, ret);
	return ret;
}


}

