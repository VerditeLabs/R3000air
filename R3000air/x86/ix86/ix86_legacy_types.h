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

#define SIB 4		// maps to ESP
#define SIBDISP 5	// maps to EBP
#define DISP32 5	// maps to EBP

// general types
typedef int x86IntRegType;

#define EAX 0
#define EBX 3
#define ECX 1
#define EDX 2
#define ESI 6
#define EDI 7
#define EBP 5
#define ESP 4

#define X86ARG1 EAX
#define X86ARG2 ECX
#define X86ARG3 EDX
#define X86ARG4 EBX

#define MM0 0
#define MM1 1
#define MM2 2
#define MM3 3
#define MM4 4
#define MM5 5
#define MM6 6
#define MM7 7

typedef int x86MMXRegType;

#define XMM0 0 
#define XMM1 1 
#define XMM2 2 
#define XMM3 3 
#define XMM4 4 
#define XMM5 5 
#define XMM6 6 
#define XMM7 7 
#define XMM8 8 
#define XMM9 9 
#define XMM10 10 
#define XMM11 11 
#define XMM12 12 
#define XMM13 13 
#define XMM14 14 
#define XMM15 15

typedef int x86SSERegType;
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

#define SIB 4		// maps to ESP
#define SIBDISP 5	// maps to EBP
#define DISP32 5	// maps to EBP

// general types
typedef int x86IntRegType;

#define EAX 0
#define EBX 3
#define ECX 1
#define EDX 2
#define ESI 6
#define EDI 7
#define EBP 5
#define ESP 4

#define X86ARG1 EAX
#define X86ARG2 ECX
#define X86ARG3 EDX
#define X86ARG4 EBX

#define MM0 0
#define MM1 1
#define MM2 2
#define MM3 3
#define MM4 4
#define MM5 5
#define MM6 6
#define MM7 7

typedef int x86MMXRegType;

#define XMM0 0 
#define XMM1 1 
#define XMM2 2 
#define XMM3 3 
#define XMM4 4 
#define XMM5 5 
#define XMM6 6 
#define XMM7 7 
#define XMM8 8 
#define XMM9 9 
#define XMM10 10 
#define XMM11 11 
#define XMM12 12 
#define XMM13 13 
#define XMM14 14 
#define XMM15 15

typedef int x86SSERegType;
