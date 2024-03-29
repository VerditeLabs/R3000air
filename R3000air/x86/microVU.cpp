/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2009  Pcsx2 Team
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

// Micro VU recompiler! - author: cottonvibes(@gmail.com)

#include "PrecompiledHeader.h"
#include "microVU.h"

//------------------------------------------------------------------
// Micro VU - Global Variables
//------------------------------------------------------------------

PCSX2_ALIGNED16(microVU microVU0);
PCSX2_ALIGNED16(microVU microVU1);

declareAllVariables // Declares All Global Variables :D

//------------------------------------------------------------------
// Micro VU - Main Functions
//------------------------------------------------------------------

// Only run this once per VU! ;)
microVUt(void) mVUinit(VURegs* vuRegsPtr, int vuIndex) {

	microVU* mVU	  = mVUx;
	mVU->regs		  = vuRegsPtr;
	mVU->index		  = vuIndex;
	mVU->vuMemSize	  = (vuIndex ? 0x4000 : 0x1000);
	mVU->microMemSize = (vuIndex ? 0x4000 : 0x1000);
	mVU->progSize	  = (vuIndex ? 0x4000 : 0x1000) / 4;
	mVU->cache		  = NULL;
	mVU->cacheSize	  = mVUcacheSize;
	memset(&mVU->prog, 0, sizeof(mVU->prog));
	mVUprint((vuIndex) ? "microVU1: init" : "microVU0: init");

	mVU->cache = SysMmapEx((vuIndex ? 0x5f240000 : 0x5e240000), mVU->cacheSize + 0x1000, 0, (vuIndex ? "Micro VU1" : "Micro VU0"));
	if (!mVU->cache) throw Exception::OutOfMemory(fmt_string("microVU Error: Failed to allocate recompiler memory!"));

	mVUemitSearch();
	mVUreset(mVU);
}

// Resets Rec Data
microVUt(void) mVUreset(mV) {

	mVUprint((mVU->index) ? "microVU1: reset" : "microVU0: reset");

	// Delete Block Managers
	for (int i = 0; i <= mVU->prog.max; i++) {
		for (u32 j = 0; j < (mVU->progSize / 2); j++) {
			microBlockManager::Delete( mVU->prog.prog[i].block[j] );
		}
	}

	// Dynarec Cache
	memset(mVU->cache, 0xcc, mVU->cacheSize + 0x1000);

	// Setup Entrance/Exit Points
	x86SetPtr(mVU->cache);
	mVUdispatcherA(mVU);
	mVUdispatcherB(mVU);

	// Clear All Program Data
	memset(&mVU->prog, 0, sizeof(mVU->prog));

	// Program Variables
	mVU->prog.isSame = -1;
	mVU->prog.cleared = 1;
	mVU->prog.cur = -1;
	mVU->prog.total = -1;
	memset(&mVU->prog.lpState, 0, sizeof(mVU->prog.lpState));
	//mVU->prog.lpState = &mVU->prog.prog[15].allocInfo.block.pState; // Blank Pipeline State (ToDo: finish implementation)

	// Setup Dynarec Cache Limits for Each Program
	u8* z = (mVU->cache + 0x1000); // Dispatcher Code is in first page of cache
	for (int i = 0; i <= mVU->prog.max; i++) {
		mVU->prog.prog[i].x86start = z;
		mVU->prog.prog[i].x86ptr = z;
		z += (mVU->cacheSize / (mVU->prog.max + 1));
		mVU->prog.prog[i].x86end = z;
		mVU->prog.prog[i].range[0] = -1; // Set range to 
		mVU->prog.prog[i].range[1] = -1; // indeterminable status
	}
}

// Free Allocated Resources
microVUt(void) mVUclose(mV) {

	mVUprint((mVU->index) ? "microVU1: close" : "microVU0: close");

	if (mVU->cache) { HostSys::Munmap(mVU->cache, mVU->cacheSize); mVU->cache = NULL; }

	// Delete Block Managers
	for (int i = 0; i <= mVU->prog.max; i++) {
		for (u32 j = 0; j < (mVU->progSize / 2); j++) {
			if (mVU->prog.prog[i].block[j]) {
				microBlockManager::Delete( mVU->prog.prog[i].block[j] );
			}
		}
	}
}

// Clears Block Data in specified range
microVUt(void) mVUclear(mV, u32 addr, u32 size) {
	//if (!mVU->prog.cleared) {
		//memset(&mVU->prog.lpState, 0, sizeof(mVU->prog.lpState));
		mVU->prog.cleared = 1; // Next execution searches/creates a new microprogram
	//}
}

//------------------------------------------------------------------
// Micro VU - Private Functions
//------------------------------------------------------------------

// Clears program data (Sets used to 1 because calling this function implies the program will be used at least once)
microVUf(void) mVUclearProg(int progIndex) {
	microVU* mVU = mVUx;
	mVU->prog.prog[progIndex].used = 1;
	mVU->prog.prog[progIndex].last_used = 3;
	mVU->prog.prog[progIndex].range[0] = -1;
	mVU->prog.prog[progIndex].range[1] = -1;
	mVU->prog.prog[progIndex].x86ptr = mVU->prog.prog[progIndex].x86start;
	for (u32 i = 0; i < (mVU->progSize / 2); i++) {
		if (mVU->prog.prog[progIndex].block[i])
			mVU->prog.prog[progIndex].block[i]->reset();
	}
}

// Caches Micro Program
microVUf(void) mVUcacheProg(int progIndex) {
	microVU* mVU = mVUx;
	memcpy_fast(mVU->prog.prog[progIndex].data, mVU->regs->Micro, mVU->microMemSize);
	mVUdumpProg(progIndex);
}

// Finds the least used program, (if program list full clears and returns an old program; if not-full, returns free program)
microVUf(int) mVUfindLeastUsedProg() {
	microVU* mVU = mVUx;
	if (mVU->prog.total < mVU->prog.max) {
		mVU->prog.total++;
		mVUcacheProg<vuIndex>(mVU->prog.total); // Cache Micro Program
		mVU->prog.prog[mVU->prog.total].used = 1;
		mVU->prog.prog[mVU->prog.total].last_used = 3;
		Console::Notice("microVU%d: Cached MicroPrograms = %d", params vuIndex, mVU->prog.total+1);
		return mVU->prog.total;
	}
	else {
	
		const int pMax	=  mVU->prog.max;
		int smallidx	= (mVU->prog.cur+1)&pMax;
		u32 smallval	=  mVU->prog.prog[smallidx].used;

		for (int i = 1, j = (smallidx+1)&pMax; i <= pMax; i++, j=(j+1)&pMax) {
			if (smallval > mVU->prog.prog[j].used) {
				smallval = mVU->prog.prog[j].used;
				smallidx = j;
			}
		}

		mVUclearProg<vuIndex>(smallidx); // Clear old data if overwriting old program
		mVUcacheProg<vuIndex>(smallidx); // Cache Micro Program
		//Console::Notice("microVU%d: Overwriting existing program in slot %d [%d times used]", params vuIndex, smallidx, smallval);
		return smallidx;
	}
}

// mVUvsyncUpdate -->
// This should be run at 30fps intervals from Counters.cpp (or 60fps works too, but 30fps is
// probably all we need for accurate results)
//
// To fix the program cache to more efficiently dispose of "obsolete" programs, we need to use a
// frame-based decrementing system in combination with a program-execution-based incrementing
// system.  In English:  if last_used >= 2 it means the program has been used for the current
// or prev frame.  if it's 0, the program hasn't been used for a while.
microVUt(void) mVUvsyncUpdate(mV) {

	if (mVU->prog.total < mVU->prog.max) return;

	for (int i = 0; i <= mVU->prog.max; i++) {
		if (mVU->prog.prog[i].last_used != 0) {
			if (mVU->prog.prog[i].last_used >= 3) {

				if (mVU->prog.prog[i].used < 0x4fffffff) // program has been used recently. Give it a
					mVU->prog.prog[i].used += 0x200;	 // 'weighted' bonus signifying it's importance
			}
			mVU->prog.prog[i].last_used--;
		}
		else mVU->prog.prog[i].used /= 2; // penalize unused programs.
	}
}

// Compare Cached microProgram to mVU->regs->Micro
microVUf(int) mVUcmpProg(int progIndex, bool progUsed, bool needOverflowCheck, bool cmpWholeProg) {
	microVU* mVU = mVUx;
	
	if (progUsed) {
		if (cmpWholeProg && (!memcmp_mmx((u8*)mVUprogI.data, mVU->regs->Micro, mVU->microMemSize)) ||
		  (!cmpWholeProg && (!memcmp_mmx(cmpOffset(mVUprogI.data), cmpOffset(mVU->regs->Micro), ((mVUprogI.range[1] + 8) - mVUprogI.range[0]))))) {
			mVU->prog.cur = progIndex;
			mVU->prog.cleared = 0;
			mVU->prog.isSame = cmpWholeProg ? 1 : -1;
			mVU->prog.prog[progIndex].last_used = 3;
			if (needOverflowCheck && (mVU->prog.prog[progIndex].used < 0x7fffffff)) {
				mVU->prog.prog[progIndex].used++; // increment 'used' (avoiding overflows if necessary)
			}
			return 1;
		}
	}
	return 0;
}

// Searches for Cached Micro Program and sets prog.cur to it (returns 1 if program found, else returns 0)
microVUf(int) mVUsearchProg() {
	microVU* mVU = mVUx;

	if (mVU->prog.cleared) { // If cleared, we need to search for new program
		for (int i = 0; i <= mVU->prog.total; i++) {
			if (mVUcmpProg<vuIndex>(i, !!mVU->prog.prog[i].used, 1, 0))
				return 1; // Check Recently Used Programs
		}
		for (int i = 0; i <= mVU->prog.total; i++) {
			if (mVUcmpProg<vuIndex>(i,  !mVU->prog.prog[i].used, 0, 0))
				return 1; // Check Older Programs
		}
		mVU->prog.cur = mVUfindLeastUsedProg<vuIndex>(); // If cleared and program not found, make a new program instance
		mVU->prog.cleared = 0;
		mVU->prog.isSame  = 1;
		return 0;
	}
	mVU->prog.prog[mVU->prog.cur].used++;
	mVU->prog.prog[mVU->prog.cur].last_used = 3;
	return 1; // If !cleared, then we're still on the same program as last-time ;)
}

//------------------------------------------------------------------
// Wrapper Functions - Called by other parts of the Emu
//------------------------------------------------------------------

void initVUrec (VURegs* vuRegs, const int vuIndex) { mVUinit(vuRegs, vuIndex); }
void closeVUrec(const int vuIndex)				   { mVUclose(mVUx); }
void resetVUrec(const int vuIndex)				   { mVUreset(mVUx); }
void vsyncVUrec(const int vuIndex)				   { mVUvsyncUpdate(mVUx); }

void __fastcall clearVUrec(u32 addr, u32 size, const int vuIndex) { 
	mVUclear(mVUx, addr, size); 
}

void __fastcall runVUrec(u32 startPC, u32 cycles, const int vuIndex) {
	if (!vuIndex) ((mVUrecCall)microVU0.startFunct)(startPC, cycles);
	else		  ((mVUrecCall)microVU1.startFunct)(startPC, cycles);
}

