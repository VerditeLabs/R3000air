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

#pragma once

// Sets FDIV Flags at the proper time
microVUt(void) mVUdivSet(mV) {
	int flagReg1, flagReg2;
	if (mVUinfo.doDivFlag) {
		getFlagReg(flagReg1, sFLAG.write);
		if (!sFLAG.doFlag) { getFlagReg(flagReg2, sFLAG.lastWrite); MOV32RtoR(flagReg1, flagReg2); }
		AND32ItoR(flagReg1, 0xfff3ffff);
		OR32MtoR (flagReg1, (uptr)&mVU->divFlag);
	}
}

// Optimizes out unneeded status flag updates
microVUt(void) mVUstatusFlagOp(mV) {
	int curPC = iPC;
	int i = mVUcount;
	bool runLoop = 1;
	if (sFLAG.doFlag) { mVUlow.useSflag = 1; }
	else {
		for (; i > 0; i--) {
			incPC2(-2);
			if (mVUlow.useSflag) { runLoop = 0; break; }
			if (sFLAG.doFlag)	 { mVUlow.useSflag = 1; break; }
		}
	}
	if (runLoop) {
		for (; i > 0; i--) {
			incPC2(-2);
			if (mVUlow.useSflag) break;
			sFLAG.doFlag = 0;
		}
	}
	iPC = curPC;
	DevCon::Status("microVU%d: FSSET Optimization", params getIndex);
}

int findFlagInst(int* fFlag, int cycles) {
	int j = 0, jValue = -1;
	for (int i = 0; i < 4; i++) {
		if ((fFlag[i] <= cycles) && (fFlag[i] > jValue)) { j = i; jValue = fFlag[i]; }
	}
	return j;
}

// Setup Last 4 instances of Status/Mac/Clip flags (needed for accurate block linking)
int sortFlag(int* fFlag, int* bFlag, int cycles) {
	int lFlag = -5;
	int x = 0;
	for (int i = 0; i < 4; i++) {
		bFlag[i] = findFlagInst(fFlag, cycles);
		if (lFlag != bFlag[i]) { x++; }
		lFlag = bFlag[i];
		cycles++;
	}
	return x; // Returns the number of Valid Flag Instances
}

#define sFlagCond ((sFLAG.doFlag && !mVUsFlagHack) || mVUlow.isFSSET || mVUinfo.doDivFlag)

// Note: Flag handling is 'very' complex, it requires full knowledge of how microVU recs work, so don't touch!
microVUt(int) mVUsetFlags(mV, int* xStatus, int* xMac, int* xClip) {

	int endPC  = iPC;
	u32 aCount = 1; // Amount of instructions needed to get valid mac flag instances for block linking

	// Ensure last ~4+ instructions update mac/status flags (if next block's first 4 instructions will read them)
	for (int i = mVUcount; i > 0; i--, aCount++) {
		if (sFLAG.doFlag) { 
			if (__Mac)	  { mFLAG.doFlag = 1; } 
			if (__Status) { sFLAG.doNonSticky = 1; } 		
			if (aCount >= 4) { break; } 
		}
		incPC2(-2);
	}

	// Status/Mac Flags Setup Code
	int xS = 0, xM = 0, xC = 0;
	for (int i = 0; i < 4; i++) {
		xStatus[i] = i;
		xMac   [i] = i;
		xClip  [i] = i;
	}

	if (!(mVUpBlock->pState.needExactMatch & 0x00f)) {
		xS = (mVUpBlock->pState.flags >> 0) & 3;
		xStatus[0] = -1; xStatus[1] = -1;
		xStatus[2] = -1; xStatus[3] = -1;
		xStatus[(xS-1)&3] = 0;
	}

	if (!(mVUpBlock->pState.needExactMatch & 0xf00)) {
		xC = (mVUpBlock->pState.flags >> 2) & 3;
		xClip[0] = -1; xClip[1] = -1;
		xClip[2] = -1; xClip[3] = -1;
		xClip[(xC-1)&3] = 0;
	}

	if (!(mVUpBlock->pState.needExactMatch & 0x0f0)) {
		xMac[0] = -1; xMac[1] = -1;
		xMac[2] = -1; xMac[3] = -1;
	}

	int cycles	= 0;
	u32 xCount	= mVUcount; // Backup count
	iPC			= mVUstartPC;
	for (mVUcount = 0; mVUcount < xCount; mVUcount++) {
		if (mVUlow.isFSSET) {
			if (__Status) { // Don't Optimize out on the last ~4+ instructions
				if ((xCount - mVUcount) > aCount) { mVUstatusFlagOp(mVU); }
			}
			else mVUstatusFlagOp(mVU);
		}
		cycles += mVUstall;

		sFLAG.read = findFlagInst(xStatus, cycles);
		mFLAG.read = findFlagInst(xMac,	   cycles);
		cFLAG.read = findFlagInst(xClip,   cycles);
		
		sFLAG.write = xS;
		mFLAG.write = xM;
		cFLAG.write = xC;

		sFLAG.lastWrite = (xS-1) & 3;
		mFLAG.lastWrite = (xM-1) & 3;
		cFLAG.lastWrite = (xC-1) & 3;

		if (sFlagCond)		{ xStatus[xS] = cycles + 4;  xS = (xS+1) & 3; }
		if (mFLAG.doFlag)	{ xMac   [xM] = cycles + 4;  xM = (xM+1) & 3; }
		if (cFLAG.doFlag)	{ xClip  [xC] = cycles + 4;  xC = (xC+1) & 3; }

		cycles++;
		incPC2(2);
	}

	mVUregs.flags = ((__Clip) ? 0 : (xC << 2)) | ((__Status) ? 0 : xS);
	return cycles;
}

#define getFlagReg1(x)	((x == 3) ? gprF3 : ((x == 2) ? gprF2 : ((x == 1) ? gprF1 : gprF0)))
#define getFlagReg2(x)	((bStatus[0] == x) ? getFlagReg1(x) : gprT1)
#define getFlagReg3(x)	((gFlag == x) ? gprT1 : getFlagReg1(x))
#define getFlagReg4(x)	((gFlag == x) ? gprT1 : gprT2)
#define shuffleMac		((bMac [3]<<6)|(bMac [2]<<4)|(bMac [1]<<2)|bMac [0])
#define shuffleClip		((bClip[3]<<6)|(bClip[2]<<4)|(bClip[1]<<2)|bClip[0])

// Recompiles Code for Proper Flags on Block Linkings
microVUt(void) mVUsetupFlags(mV, int* xStatus, int* xMac, int* xClip, int cycles) {

	if (__Status) {
		int bStatus[4];
		int sortRegs = sortFlag(xStatus, bStatus, cycles);
		// DevCon::Status("sortRegs = %d", params sortRegs);
		// Note: Emitter will optimize out mov(reg1, reg1) cases...
		// There 'is' still room for small optimizations but the 
		// sorting algorithm would be really complex and not really
		// a noticeable improvement... (Most common cases are 1 & 2)
		if (sortRegs == 1) {
			MOV32RtoR(gprF0,  getFlagReg1(bStatus[0]));
			MOV32RtoR(gprF1,  getFlagReg1(bStatus[1]));
			MOV32RtoR(gprF2,  getFlagReg1(bStatus[2]));
			MOV32RtoR(gprF3,  getFlagReg1(bStatus[3]));
		}
		else if (sortRegs == 2) {
			MOV32RtoR(gprT1,  getFlagReg1(bStatus[3])); 
			MOV32RtoR(gprF0,  getFlagReg1(bStatus[0]));
			MOV32RtoR(gprF1,  getFlagReg2(bStatus[1]));
			MOV32RtoR(gprF2,  getFlagReg2(bStatus[2]));
			MOV32RtoR(gprF3,  gprT1);
		}
		else if (sortRegs == 3) {
			int gFlag = (bStatus[0] == bStatus[1]) ? bStatus[2] : bStatus[1];
			MOV32RtoR(gprT1,  getFlagReg1(gFlag)); 
			MOV32RtoR(gprT2,  getFlagReg1(bStatus[3]));
			MOV32RtoR(gprF0,  getFlagReg1(bStatus[0]));
			MOV32RtoR(gprF1,  getFlagReg3(bStatus[1]));
			MOV32RtoR(gprF2,  getFlagReg4(bStatus[2]));
			MOV32RtoR(gprF3,  gprT2);
		}
		else {
			MOV32RtoR(gprT1,  getFlagReg1(bStatus[0])); 
			MOV32RtoR(gprT2,  getFlagReg1(bStatus[1]));
			MOV32RtoR(gprR,   getFlagReg1(bStatus[2]));
			MOV32RtoR(gprF3,  getFlagReg1(bStatus[3]));
			MOV32RtoR(gprF0,  gprT1);
			MOV32RtoR(gprF1,  gprT2); 
			MOV32RtoR(gprF2,  gprR); 
			MOV32ItoR(gprR, Roffset); // Restore gprR
		}
	}

	if (__Mac) {
		int bMac[4];
		sortFlag(xMac, bMac, cycles);
		SSE_MOVAPS_M128_to_XMM(xmmT1, (uptr)mVU->macFlag);
		SSE_SHUFPS_XMM_to_XMM (xmmT1, xmmT1, shuffleMac);
		SSE_MOVAPS_XMM_to_M128((uptr)mVU->macFlag, xmmT1);
	}

	if (__Clip) {
		int bClip[4];
		sortFlag(xClip, bClip, cycles);
		SSE_MOVAPS_M128_to_XMM(xmmT1, (uptr)mVU->clipFlag);
		SSE_SHUFPS_XMM_to_XMM (xmmT1, xmmT1, shuffleClip);
		SSE_MOVAPS_XMM_to_M128((uptr)mVU->clipFlag, xmmT1);
	}
}

#define shortBranch() {											\
	if (branch == 3) {											\
		mVUflagPass(mVU, aBranchAddr, (xCount - (mVUcount+1)));	\
		mVUcount = 4;											\
	}															\
}

// Scan through instructions and check if flags are read (FSxxx, FMxxx, FCxxx opcodes)
void mVUflagPass(mV, u32 startPC, u32 xCount) {

	int oldPC	  = iPC;
	int oldCount  = mVUcount;
	int oldBranch = mVUbranch;
	int aBranchAddr;
	iPC		  = startPC / 4;
	mVUcount  = 0;
	mVUbranch = 0;
	for (int branch = 0; mVUcount < xCount; mVUcount++) {
		incPC(1);
		if (  curI & _Ebit_   )	{ branch = 1; }
		if (  curI & _DTbit_ )	{ branch = 4; }
		if (!(curI & _Ibit_)  )	{ incPC(-1); mVUopL(mVU, 3); incPC(1); }
		if		(branch >= 2)	{ shortBranch(); break; }
		else if (branch == 1)	{ branch = 2; }
		if		(mVUbranch)		{ branch = (mVUbranch >= 9) ? 5 : 3; aBranchAddr = branchAddr; mVUbranch = 0; }
		incPC(1);
	}
	if (mVUcount < 4) { mVUflagInfo |= 0xfff; }
	iPC		  = oldPC;
	mVUcount  = oldCount;
	mVUbranch = oldBranch;
	setCode();
}

#define branchType1 if		(mVUbranch <= 2)	// B/BAL
#define branchType2 else if (mVUbranch >= 9)	// JR/JALR
#define branchType3 else						// Conditional Branch

// Checks if the first 4 instructions of a block will read flags
microVUt(void) mVUsetFlagInfo(mV) {
	branchType1 { incPC(-1); mVUflagPass(mVU, branchAddr, 4); incPC(1); }
	branchType2 { mVUflagInfo |= 0xfff; }
	branchType3 {
		incPC(-1); 
		mVUflagPass(mVU, branchAddr, 4);
		int backupFlagInfo = mVUflagInfo;
		mVUflagInfo = 0;
		incPC(4); // Branch Not Taken
		mVUflagPass(mVU, xPC, 4);
		incPC(-3);		
		mVUflagInfo |= backupFlagInfo;
	}
}

