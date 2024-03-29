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

u8  *psH; // hw mem

int rdram_devices = 2;	// put 8 for TOOL and 2 for PS2 and PSX
int rdram_sdevid = 0;

void hwInit()
{
	gsInit();
	vif0Init();
	vif1Init();
	vifDmaInit();
	sifInit();
	sprInit();
	ipuInit();
}

void hwShutdown() {
	ipuShutdown();
}

void hwReset()
{
	hwInit();

	memzero_ptr<Ps2MemSize::Hardware>( PS2MEM_HW );
	//memset(PS2MEM_HW+0x2000, 0, 0x0000e000);

	psHu32(0xf520) = 0x1201;
	psHu32(0xf260) = 0x1D000060;
	// i guess this is kinda a version, it's used by some bioses
	psHu32(0xf590) = 0x1201;

	gsReset();
	ipuReset();
}

__forceinline void  intcInterrupt()
{
	if ((cpuRegs.CP0.n.Status.val & 0x400) != 0x400) return;

	if ((psHu32(INTC_STAT)) == 0) {
		DevCon::Notice("*PCSX2*: intcInterrupt already cleared");
        return;
	}
	if ((psHu32(INTC_STAT) & psHu32(INTC_MASK)) == 0) return;

	HW_LOG("intcInterrupt %x", psHu32(INTC_STAT) & psHu32(INTC_MASK));
	if(psHu32(INTC_STAT) & 0x2){
		counters[0].hold = rcntRcount(0);
		counters[1].hold = rcntRcount(1);
	}

	cpuException(0x400, cpuRegs.branch);
}

__forceinline void  dmacInterrupt()
{
    if ((cpuRegs.CP0.n.Status.val & 0x10807) != 0x10801) return;

	if( ((psHu16(0xe012) & psHu16(0xe010)) == 0 ) && 
		( psHu16(0xe010) & 0x8000) == 0 ) return;

	if((psHu32(DMAC_CTRL) & 0x1) == 0) return;
	
	HW_LOG("dmacInterrupt %x", (psHu16(0xe012) & psHu16(0xe010) || 
								  psHu16(0xe010) & 0x8000));

	cpuException(0x800, cpuRegs.branch);
}

void hwIntcIrq(int n) {
	psHu32(INTC_STAT)|= 1<<n;
	cpuTestINTCInts();
}

void hwDmacIrq(int n) {
	psHu32(DMAC_STAT)|= 1<<n;
	cpuTestDMACInts();	
}

/* Write 'size' bytes to memory address 'addr' from 'data'. */
int hwMFIFOWrite(u32 addr, u8 *data, u32 size) {
	u32 msize = psHu32(DMAC_RBOR) + psHu32(DMAC_RBSR)+16;
	u8 *dst;


	addr = psHu32(DMAC_RBOR) + (addr & psHu32(DMAC_RBSR));
	/* Check if the transfer should wrap around the ring buffer */
	if ((addr+size) >= msize) {
		int s1 = msize - addr;
		int s2 = size - s1;

		/* it does, so first copy 's1' bytes from 'data' to 'addr' */
		dst = (u8*)PSM(addr);
		if (dst == NULL) return -1;
		memcpy_fast(dst, data, s1);

		/* and second copy 's2' bytes from '&data[s1]' to 'maddr' */
		dst = (u8*)PSM(psHu32(DMAC_RBOR));
		if (dst == NULL) return -1;
		memcpy_fast(dst, &data[s1], s2);
	} 
	else {
		/* it doesn't, so just copy 'size' bytes from 'data' to 'addr' */
		dst = (u8*)PSM(addr);
		if (dst == NULL) return -1;
		memcpy_fast(dst, data, size);
	}

	return 0;
}


bool hwDmacSrcChainWithStack(DMACh *dma, int id) {
	switch (id) {
		case 0: // Refe - Transfer Packet According to ADDR field
			return true;										//End Transfer

		case 1: // CNT - Transfer QWC following the tag.
			dma->madr = dma->tadr + 16;						//Set MADR to QW after Tag            
			dma->tadr = dma->madr + (dma->qwc << 4);			//Set TADR to QW following the data
			return false;

		case 2: // Next - Transfer QWC following tag. TADR = ADDR
		{
			u32 temp = dma->madr;								//Temporarily Store ADDR
			dma->madr = dma->tadr + 16; 					  //Set MADR to QW following the tag
			dma->tadr = temp;								//Copy temporarily stored ADDR to Tag
			return false;
		}
		case 3: // Ref - Transfer QWC from ADDR field
		case 4: // Refs - Transfer QWC from ADDR field (Stall Control) 
			dma->tadr += 16;									//Set TADR to next tag
			return false;

		case 5: // Call - Transfer QWC following the tag, save succeeding tag
		{
			u32 temp = dma->madr;								//Temporarily Store ADDR
															
			dma->madr = dma->tadr + 16;						//Set MADR to data following the tag
			
			if ((dma->chcr & 0x30) == 0x0) {						//Check if ASR0 is empty
				dma->asr0 = dma->madr + (dma->qwc << 4);			//If yes store Succeeding tag
				dma->chcr = (dma->chcr & 0xffffffcf) | 0x10; //1 Address in call stack
			}
			else if((dma->chcr & 0x30) == 0x10){
				dma->chcr = (dma->chcr & 0xffffffcf) | 0x20; //2 Addresses in call stack
				dma->asr1 = dma->madr + (dma->qwc << 4);	//If no store Succeeding tag in ASR1
			}else {
				Console::Notice("Call Stack Overflow (report if it fixes/breaks anything)");
				return true;										//Return done
			}
			dma->tadr = temp;								//Set TADR to temporarily stored ADDR
											
			return false;
		}
		case 6: // Ret - Transfer QWC following the tag, load next tag
			dma->madr = dma->tadr + 16;						//Set MADR to data following the tag

			if ((dma->chcr & 0x30) == 0x20) {							//If ASR1 is NOT equal to 0 (Contains address)
				dma->chcr = (dma->chcr & 0xffffffcf) | 0x10; //1 Address left in call stack
				dma->tadr = dma->asr1;						//Read ASR1 as next tag
				dma->asr1 = 0;								//Clear ASR1
			} 
			else {										//If ASR1 is empty (No address held)
				if((dma->chcr & 0x30) == 0x10) {						   //Check if ASR0 is NOT equal to 0 (Contains address)
					dma->chcr = (dma->chcr & 0xffffffcf);  //No addresses left in call stack
					dma->tadr = dma->asr0;					//Read ASR0 as next tag
					dma->asr0 = 0;							//Clear ASR0
				} else {									//Else if ASR1 and ASR0 are empty
					//dma->tadr += 16;						   //Clear tag address - Kills Klonoa 2
					return 1;								//End Transfer
				}
			}
			return false;

		case 7: // End - Transfer QWC following the tag 
			dma->madr = dma->tadr + 16;						//Set MADR to data following the tag
			//Dont Increment tadr, breaks Soul Calibur II and III
			return true;										//End Transfer
	}

	return false;
}

bool hwDmacSrcChain(DMACh *dma, int id) {
	u32 temp;

	switch (id) {
		case 0: // Refe - Transfer Packet According to ADDR field
			return true;										//End Transfer

		case 1: // CNT - Transfer QWC following the tag.
			dma->madr = dma->tadr + 16;						//Set MADR to QW after Tag            
			dma->tadr = dma->madr + (dma->qwc << 4);			//Set TADR to QW following the data
			return false;

		case 2: // Next - Transfer QWC following tag. TADR = ADDR
			temp = dma->madr;								//Temporarily Store ADDR
			dma->madr = dma->tadr + 16; 					  //Set MADR to QW following the tag
			dma->tadr = temp;								//Copy temporarily stored ADDR to Tag
			return false;

		case 3: // Ref - Transfer QWC from ADDR field
		case 4: // Refs - Transfer QWC from ADDR field (Stall Control) 
			dma->tadr += 16;									//Set TADR to next tag
			return false;

		case 7: // End - Transfer QWC following the tag
			dma->madr = dma->tadr + 16;						//Set MADR to data following the tag
			//Dont Increment tadr, breaks Soul Calibur II and III
			return true;										//End Transfer
	}

	return false;
}
