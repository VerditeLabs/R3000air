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

#include "Common.h"
#include "iR5900.h"
#include "VUmicro.h"
#include "IopMem.h"

// The full suite of hardware APIs:
#include "IPU/IPU.h"
#include "GS.h"
#include "Counters.h"
#include "Vif.h"
#include "VifDma.h"
#include "SPR.h"
#include "Sif.h"

using namespace R5900;

static __forceinline void IntCHackCheck()
{
	// Sanity check: To protect from accidentally "rewinding" the cyclecount
	// on the few times nextBranchCycle can be behind our current cycle.
	s32 diff = g_nextBranchCycle - cpuRegs.cycle;
	if( diff > 0 ) cpuRegs.cycle = g_nextBranchCycle;

	
	// Threshold method, might fix games that have problems with the simple
	// implementation above (none known that break yet)
	/*if( ( g_nextBranchCycle - cpuRegs.cycle ) > 500 )
		cpuRegs.cycle += 498;
	else
		cpuRegs.cycle = g_nextBranchCycle - 2;*/
}

/////////////////////////////////////////////////////////////////////////
// Hardware READ 8 bit

__forceinline mem8_t hwRead8(u32 mem)
{
	u8 ret;

	if( mem >= IPU_CMD && mem < D0_CHCR )
		DevCon::Notice("Unexpected hwRead8 from 0x%x", params mem);

	switch (mem)
	{
		case RCNT0_COUNT: ret = (u8)rcntRcount(0); break;
		case RCNT0_MODE: ret = (u8)counters[0].modeval; break;
		case RCNT0_TARGET: ret = (u8)counters[0].target; break;
		case RCNT0_HOLD: ret = (u8)counters[0].hold; break;
		case RCNT0_COUNT + 1: ret = (u8)(rcntRcount(0)>>8); break;
		case RCNT0_MODE + 1: ret = (u8)(counters[0].modeval>>8); break;
		case RCNT0_TARGET + 1: ret = (u8)(counters[0].target>>8); break;
		case RCNT0_HOLD + 1: ret = (u8)(counters[0].hold>>8); break;

		case RCNT1_COUNT: ret = (u8)rcntRcount(1); break;
		case RCNT1_MODE: ret = (u8)counters[1].modeval; break;
		case RCNT1_TARGET: ret = (u8)counters[1].target; break;
		case RCNT1_HOLD: ret = (u8)counters[1].hold; break;
		case RCNT1_COUNT + 1: ret = (u8)(rcntRcount(1)>>8); break;
		case RCNT1_MODE + 1: ret = (u8)(counters[1].modeval>>8); break;
		case RCNT1_TARGET + 1: ret = (u8)(counters[1].target>>8); break;
		case RCNT1_HOLD + 1: ret = (u8)(counters[1].hold>>8); break;

		case RCNT2_COUNT: ret = (u8)rcntRcount(2); break;
		case RCNT2_MODE: ret = (u8)counters[2].modeval; break;
		case RCNT2_TARGET: ret = (u8)counters[2].target; break;
		case RCNT2_COUNT + 1: ret = (u8)(rcntRcount(2)>>8); break;
		case RCNT2_MODE + 1: ret = (u8)(counters[2].modeval>>8); break;
		case RCNT2_TARGET + 1: ret = (u8)(counters[2].target>>8); break;

		case RCNT3_COUNT: ret = (u8)rcntRcount(3); break;
		case RCNT3_MODE: ret = (u8)counters[3].modeval; break;
		case RCNT3_TARGET: ret = (u8)counters[3].target; break;
		case RCNT3_COUNT + 1: ret = (u8)(rcntRcount(3)>>8); break;
		case RCNT3_MODE + 1: ret = (u8)(counters[3].modeval>>8); break;
		case RCNT3_TARGET + 1: ret = (u8)(counters[3].target>>8); break;

		default:
			if ((mem & 0xffffff0f) == SBUS_F200)
			{
				switch (mem)
				{
					case SBUS_F240: 
						ret = psHu32(mem);
						//psHu32(mem) &= ~0x4000;
						break;
					
					case SBUS_F260:
						ret = 0;
						break;
					
					default:
						ret = psHu32(mem);
						break;
				}
				return (u8)ret;
			}

			ret = psHu8(mem);
			HW_LOG("Unknown Hardware Read 8 from 0x%x = 0x%x", mem, ret);
			break;
	}

	return ret;
}

/////////////////////////////////////////////////////////////////////////
// Hardware READ 16 bit

__forceinline mem16_t hwRead16(u32 mem)
{
	u16 ret;

	if( mem >= IPU_CMD && mem < D0_CHCR )
		Console::Notice("Unexpected hwRead16 from 0x%x", params mem);

	switch (mem)
	{
		case RCNT0_COUNT: ret = (u16)rcntRcount(0); break;
		case RCNT0_MODE: ret = (u16)counters[0].modeval; break;
		case RCNT0_TARGET: ret = (u16)counters[0].target; break;
		case RCNT0_HOLD: ret = (u16)counters[0].hold; break;

		case RCNT1_COUNT: ret = (u16)rcntRcount(1); break;
		case RCNT1_MODE: ret = (u16)counters[1].modeval; break;
		case RCNT1_TARGET: ret = (u16)counters[1].target; break;
		case RCNT1_HOLD: ret = (u16)counters[1].hold; break;

		case RCNT2_COUNT: ret = (u16)rcntRcount(2); break;
		case RCNT2_MODE: ret = (u16)counters[2].modeval; break;
		case RCNT2_TARGET: ret = (u16)counters[2].target; break;

		case RCNT3_COUNT: ret = (u16)rcntRcount(3); break;
		case RCNT3_MODE: ret = (u16)counters[3].modeval; break;
		case RCNT3_TARGET: ret = (u16)counters[3].target; break;

		default:
			if ((mem & 0xffffff0f) == SBUS_F200)
			{
				switch (mem)
				{
					case SBUS_F240: 
						ret = psHu16(mem) | 0x0102;
						psHu32(mem) &= ~0x4000; // not commented out like in  bit mode?
						break;
					
					case SBUS_F260:
						ret = 0;
						break;
					
					default:
						ret = psHu32(mem);
						break;
				}
				return (u16)ret;
			}
			ret = psHu16(mem);
			HW_LOG("Hardware Read16 at 0x%x, value= 0x%x", ret, mem);
			break;
	}
	return ret;
}

/////////////////////////////////////////////////////////////////////////
// Hardware READ 32 bit

// Reads hardware registers for page 0 (counters 0 and 1)
mem32_t __fastcall hwRead32_page_00(u32 mem)
{
	mem &= 0xffff;
	switch( mem )
	{
		case 0x00: return (u16)rcntRcount(0);
		case 0x10: return (u16)counters[0].modeval;
		case 0x20: return (u16)counters[0].target;
		case 0x30: return (u16)counters[0].hold;

		case 0x800: return (u16)rcntRcount(1);
		case 0x810: return (u16)counters[1].modeval;
		case 0x820: return (u16)counters[1].target;
		case 0x830: return (u16)counters[1].hold;
	}

	return *((u32*)&PS2MEM_HW[mem]);
}

// Reads hardware registers for page 1 (counters 2 and 3)
mem32_t __fastcall hwRead32_page_01(u32 mem)
{
	mem &= 0xffff;
	switch( mem )
	{
		case 0x1000: return (u16)rcntRcount(2);
		case 0x1010: return (u16)counters[2].modeval;
		case 0x1020: return (u16)counters[2].target;

		case 0x1800: return (u16)rcntRcount(3);
		case 0x1810: return (u16)counters[3].modeval;
		case 0x1820: return (u16)counters[3].target;
	}

	return *((u32*)&PS2MEM_HW[mem]);
}

// Reads hardware registers for page 15 (0x0F).
// This is used internally to produce two inline versions, one with INTC_HACK, and one without.
static __forceinline mem32_t __hwRead32_page_0F( u32 mem, bool intchack )
{
	// *Performance Warning*  This function is called -A-LOT.  Be wary when making changes.  It
	// could impact FPS significantly.

	mem &= 0xffff;

	// INTC_STAT shortcut for heavy spinning.
	// Performance Note: Visual Studio handles this best if we just manually check for it here,
	// outside the context of the switch statement below.  This is likely fixed by PGO also,
	// but it's an easy enough conditional to account for anyways.
	
	static const uint ics = INTC_STAT & 0xffff;
	if( mem == ics )		// INTC_STAT
	{
		if( intchack ) IntCHackCheck();
		return *((u32*)&PS2MEM_HW[ics]);
	}

	switch( mem )
	{
		case 0xf010:
			HW_LOG("INTC_MASK Read32, value=0x%x", psHu32(INTC_MASK));
		break;

		case 0xf130:	// SIO_ISR
		case 0xf260:	// SBUS_F260
		case 0xf410:	// 0x1000f410
		case 0xf430:	// MCH_RICM
			return 0;

		case 0xf240:	// SBUS_F240
			return psHu32(0xf240) | 0xF0000102;

		case 0xf440:	// MCH_DRD
			if( !((psHu32(0xf430) >> 6) & 0xF) )
			{
				switch ((psHu32(0xf430)>>16) & 0xFFF)
				{
					//MCH_RICM: x:4|SA:12|x:5|SDEV:1|SOP:4|SBC:1|SDEV:5

					case 0x21://INIT
						if(rdram_sdevid < rdram_devices)
						{
							rdram_sdevid++;
							return 0x1F;
						}
					return 0;

					case 0x23://CNFGA
						return 0x0D0D;	//PVER=3 | MVER=16 | DBL=1 | REFBIT=5

					case 0x24://CNFGB
						//0x0110 for PSX  SVER=0 | CORG=8(5x9x7) | SPT=1 | DEVTYP=0 | BYTE=0
						return 0x0090;	//SVER=0 | CORG=4(5x9x6) | SPT=1 | DEVTYP=0 | BYTE=0

					case 0x40://DEVID
						return psHu32(0xf430) & 0x1F;	// =SDEV
				}
			}
			return 0;
	}
	return *((u32*)&PS2MEM_HW[mem]);
}

mem32_t __fastcall hwRead32_page_0F(u32 mem)
{
	return __hwRead32_page_0F( mem, false );
}

mem32_t __fastcall hwRead32_page_0F_INTC_HACK(u32 mem)
{
	return __hwRead32_page_0F( mem, true );
}

mem32_t __fastcall hwRead32_page_02(u32 mem)
{
	return ipuRead32( mem );
}

// Used for all pages not explicitly specified above.
mem32_t __fastcall hwRead32_generic(u32 mem)
{
	const u16 masked_mem = mem & 0xffff;

	switch( masked_mem>>12 )		// switch out as according to the 4k page of the access.
	{
		///////////////////////////////////////////////////////
		// Most of the following case handlers are for developer builds only (logging).
		// It'll all optimize to ziltch in public release builds.

		case 0x03:
		case 0x04:
		case 0x05:
		case 0x06:
		case 0x07:
		case 0x08:
		case 0x09:
		case 0x0a:
		case 0x0d:
		{
			const char* regName = "Unknown";

			switch( mem )
			{
				case D2_CHCR: regName = "DMA2_CHCR"; break;
				case D2_MADR: regName = "DMA2_MADR"; break;
				case D2_QWC: regName = "DMA2_QWC"; break;
				case D2_TADR: regName = "DMA2_TADDR"; break;
				case D2_ASR0: regName = "DMA2_ASR0"; break;
				case D2_ASR1: regName = "DMA2_ASR1"; break;
				case D2_SADR: regName = "DMA2_SADDR"; break;
			}

			HW_LOG( "Hardware Read32 at 0x%x (%s), value=0x%x", mem, regName, psHu32(mem) );
		}
		break;

		case 0x0b:
			if( mem == D4_CHCR )
				HW_LOG("Hardware Read32 at 0x%x (IPU1:DMA4_CHCR), value=0x%x", mem, psHu32(mem));
		break;

		case 0x0c:
		case 0x0e:
			if( mem == DMAC_STAT)
				HW_LOG("DMAC_STAT Read32, value=0x%x", psHu32(DMAC_STAT));
		break;

		jNO_DEFAULT;
	}

	return *((u32*)&PS2MEM_HW[masked_mem]);
}

/////////////////////////////////////////////////////////////////////////
// Hardware READ 64 bit

void __fastcall hwRead64_page_00(u32 mem, mem64_t* result )
{
	*result = hwRead32_page_00( mem );
}

void __fastcall hwRead64_page_01(u32 mem, mem64_t* result )
{
	*result = hwRead32_page_01( mem );
}

void __fastcall hwRead64_page_02(u32 mem, mem64_t* result )
{
	*result = ipuRead64(mem);
}

void __fastcall hwRead64_generic_INTC_HACK(u32 mem, mem64_t* result )
{
	if (mem == INTC_STAT) IntCHackCheck();

	*result = psHu64(mem);
	HW_LOG("Unknown Hardware Read 64 at %x",mem);
}

void __fastcall hwRead64_generic(u32 mem, mem64_t* result )
{
	*result = psHu64(mem);
	HW_LOG("Unknown Hardware Read 64 at %x",mem);
}

/////////////////////////////////////////////////////////////////////////
// Hardware READ 128 bit

void __fastcall hwRead128_page_00(u32 mem, mem128_t* result )
{
	result[0] = hwRead32_page_00( mem );
	result[1] = 0;
}

void __fastcall hwRead128_page_01(u32 mem, mem128_t* result )
{
	result[0] = hwRead32_page_01( mem );
	result[1] = 0;
}

void __fastcall hwRead128_page_02(u32 mem, mem128_t* result )
{
	// IPU is currently unhandled in 128 bit mode.
	HW_LOG("Unknown Hardware Read 128 at %x (IPU)",mem);
}

void __fastcall hwRead128_generic(u32 mem, mem128_t* out)
{
	out[0] = psHu64(mem);
	out[1] = psHu64(mem+8);

	HW_LOG("Unknown Hardware Read 128 at %x",mem);
}
