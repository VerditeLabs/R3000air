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
 *	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef __MISC_H__
#define __MISC_H__

struct KeyModifiers
{
	bool control;
	bool alt;
	bool shift;
	bool capslock;
};
extern struct KeyModifiers keymodifiers;

// Per ChickenLiver, this is being used to pass the GS plugins window handle to the Pad plugins.
// So a rename to pDisplay is in the works, but it will not, in fact, be removed.
extern uptr pDsp;	//Used in  GS, MTGS, Plugins, Misc

u32 GetBiosVersion(); // Used in Misc, Memory
extern u32 BiosVersion;  //  Used in Memory, Misc, CDVD
int GetPS2ElfName(char*); // Used in Misc, System, Linux, CDVD

// Not sure what header these should go in. Probably not this one.
void SetCPUState(u32 sseMXCSR, u32 sseVUMXCSR);
extern u32 g_sseVUMXCSR, g_sseMXCSR;
extern u8 g_globalMMXSaved, g_globalXMMSaved;
extern bool g_EEFreezeRegs;

// when using mmx/xmm regs, use; 0 is load
// freezes no matter the state
#ifndef __INTEL_COMPILER
extern "C" void FreezeXMMRegs_(int save);
extern "C" void FreezeMMXRegs_(int save);
extern "C" void FreezeRegs(int save);
#else
extern void FreezeXMMRegs_(int save);
extern void FreezeMMXRegs_(int save);
extern void FreezeRegs(int save);
#endif

// these macros check to see if needs freezing
#define FreezeXMMRegs(save) if( g_EEFreezeRegs ) { FreezeXMMRegs_(save); }
#define FreezeMMXRegs(save) if( g_EEFreezeRegs ) { FreezeMMXRegs_(save); }

// If we move the rest of this stuff, we can probably move these, too.
extern void InitCPUTicks();
extern u64 GetCPUTicks();
extern u64 GetTickFrequency();

// Used in Misc,and Windows/Linux files.
extern void ProcessFKeys(int fkey,  struct KeyModifiers *keymod); // processes fkey related commands value 1-12
extern int IsBIOS(const char *filename, char *description);

char *ParseLang(char *id);
extern const char *LabelAuthors;
extern const char *LabelGreets;
void CycleFrameLimit(int dir);

void SaveGSState(const string& file);
void LoadGSState(const string& file);

#endif /* __MISC_H__ */

