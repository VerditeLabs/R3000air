/******************************************************************************

 Copyright (c) 2001 Advanced Micro Devices, Inc.

 LIMITATION OF LIABILITY:  THE MATERIALS ARE PROVIDED *AS IS* WITHOUT ANY
 EXPRESS OR IMPLIED WARRANTY OF ANY KIND INCLUDING WARRANTIES OF MERCHANTABILITY,
 NONINFRINGEMENT OF THIRD-PARTY INTELLECTUAL PROPERTY, OR FITNESS FOR ANY
 PARTICULAR PURPOSE.  IN NO EVENT SHALL AMD OR ITS SUPPLIERS BE LIABLE FOR ANY
 DAMAGES WHATSOEVER (INCLUDING, WITHOUT LIMITATION, DAMAGES FOR LOSS OF PROFITS,
 BUSINESS INTERRUPTION, LOSS OF INFORMATION) ARISING OUT OF THE USE OF OR
 INABILITY TO USE THE MATERIALS, EVEN IF AMD HAS BEEN ADVISED OF THE POSSIBILITY
 OF SUCH DAMAGES.  BECAUSE SOME JURISDICTIONS PROHIBIT THE EXCLUSION OR LIMITATION
 OF LIABILITY FOR CONSEQUENTIAL OR INCIDENTAL DAMAGES, THE ABOVE LIMITATION MAY
 NOT APPLY TO YOU.

 AMD does not assume any responsibility for any errors which may appear in the
 Materials nor any responsibility to support or update the Materials.  AMD retains
 the right to make changes to its test specifications at any time, without notice.

 NO SUPPORT OBLIGATION: AMD is not obligated to furnish, support, or make any
 further information, software, technical information, know-how, or show-how
 available to you.

 So that all may benefit from your experience, please report  any  problems
 or  suggestions about this software to 3dsdk.support@amd.com

 AMD Developer Technologies, M/S 585
 Advanced Micro Devices, Inc.
 5900 E. Ben White Blvd.
 Austin, TX 78741
 3dsdk.support@amd.com
******************************************************************************/

#include "PrecompiledHeader.h"

#ifdef _MSC_VER
#pragma warning(disable:4414)
#endif

/*****************************************************************************
MEMCPY_AMD.CPP
******************************************************************************/

// Very optimized memcpy() routine for AMD Athlon and Duron family.
// This code uses any of FOUR different basic copy methods, depending
// on the transfer size.
// NOTE:  Since this code uses MOVNTQ (also known as "Non-Temporal MOV" or
// "Streaming Store"), and also uses the software prefetch instructions,
// be sure you're running on Athlon/Duron or other recent CPU before calling!

#define TINY_BLOCK_COPY 64       // upper limit for movsd type copy
// The smallest copy uses the X86 "movsd" instruction, in an optimized
// form which is an "unrolled loop".

#define IN_CACHE_COPY 2 * 1024  // upper limit for movq/movq copy w/SW prefetch
// Next is a copy that uses the MMX registers to copy 8 bytes at a time,
// also using the "unrolled loop" optimization.   This code uses
// the software prefetch instruction to get the data into the cache.

#define UNCACHED_COPY 4 * 1024 // upper limit for movq/movntq w/SW prefetch
// For larger blocks, which will spill beyond the cache, it's faster to
// use the Streaming Store instruction MOVNTQ.   This write instruction
// bypasses the cache and writes straight to main memory.  This code also
// uses the software prefetch instruction to pre-read the data.
// USE 64 * 1024 FOR THIS VALUE IF YOU'RE ALWAYS FILLING A "CLEAN CACHE"

#define BLOCK_PREFETCH_COPY  infinity // no limit for movq/movntq w/block prefetch 
#define CACHEBLOCK 80h // number of 64-byte blocks (cache lines) for block prefetch
// For the largest size blocks, a special technique called Block Prefetch
// can be used to accelerate the read operations.   Block Prefetch reads
// one address per cache line, for a series of cache lines, in a short loop.
// This is faster than using software prefetch.  The technique is great for
// getting maximum read bandwidth, especially in DDR memory systems.

#include "Misc.h"

// Inline assembly syntax for use with Visual C++

#if defined(_MSC_VER)

#ifdef PCSX2_DEBUG
extern u8 g_globalMMXSaved;

void checkregs()
{
	if( g_EEFreezeRegs ) assert( g_globalMMXSaved );
}
#endif


PCSX2_ALIGNED16( static u8 _xmm_backup[16*2] );
PCSX2_ALIGNED16( static u8 _mmx_backup[8*4] );

static __declspec(naked) void __fastcall _memcpy_raz_usrc(void *dest, const void *src, size_t bytes)
{
	// MOVSRC = opcode used to read. I use the same code for the aligned version, with a different define :)
	#define MOVSRC movdqu
	#define MOVDST movdqa

	__asm
	{
		//Reads before reads, to avoid stalls
		mov eax,[esp+4];
		//Make sure to save xmm0, it must be preserved ...
		movaps [_xmm_backup],xmm0;

		//if >=128 bytes use 128 byte unrolled loop
		//i use cmp ..,127 + jna because 127 is encodable using the simm8 form
		cmp eax,127;
		jna _loop_1;

		//since this is a common branch target it could be good to align it -- no idea if it has any effect :p
		align 16

		//128 byte unrolled loop
_loop_8:

		MOVSRC xmm0,[edx+0x00];	//read first to avoid read-after-write stalls
		MOVDST [ecx+0x00],xmm0; //then write :p
		MOVSRC xmm0,[edx+0x10];
		MOVDST [ecx+0x10],xmm0;
		sub edx,-128;			//edx won't be used for a while, so update it here. sub/-128 for simm8 encoding
		sub ecx,-128;			//ecx won't be used for a while, so update it here. sub/-128 for simm8 encoding

		MOVSRC xmm0,[edx+0x20-128];
		MOVDST [ecx+0x20-128],xmm0;
		MOVSRC xmm0,[edx+0x30-128];
		MOVDST [ecx+0x30-128],xmm0;
		add eax,-128;			//eax won't be used for a while, so update it here. add/-128 for simm8 encoding

		MOVSRC xmm0,[edx+0x40-128];
		MOVDST [ecx+0x40-128],xmm0;
		MOVSRC xmm0,[edx+0x50-128];
		MOVDST [ecx+0x50-128],xmm0;

		MOVSRC xmm0,[edx+0x60-128];
		MOVDST [ecx+0x60-128],xmm0;
		MOVSRC xmm0,[edx+0x70-128];
		MOVDST [ecx+0x70-128],xmm0;

		//127~ja, 127 is encodable as simm8 :)
		cmp eax,127;
		ja _loop_8;
		
		//direct copy for 0~7 qwords
		//in order to avoid the inc/dec of all 3 registers
		//i use negative relative addressing from the top of the buffers
		//[top-current index]

_loop_1:
		//prepare the regs for 'negative relative addressing'
		add edx,eax;
		add ecx,eax;
		neg eax;
		jz cleanup;	//exit if nothing to do

_loop_1_inner:
		MOVSRC xmm0,[edx+eax];
		MOVDST [ecx+eax],xmm0;
		
		add eax,16;		//while the offset is still negative we have data to copy
		js _loop_1_inner;

		//done !
cleanup:
		//restore xmm and exit ~)
		movaps xmm0,[_xmm_backup];
		ret 4;
	}
	#undef MOVSRC
	#undef MOVDST
}


static __declspec(naked) void __fastcall _memcpy_raz_udst(void *dest, const void *src, size_t bytes)
{
	// MOVDST = opcode used to read. I use the same code for the aligned version, with a different define :)
	#define MOVSRC movaps
	#define MOVDST movups
	__asm
	{
		//Reads before reads, to avoid stalls
		mov eax,[esp+4];
		//Make sure to save xmm0, it must be preserved ...
		movaps [_xmm_backup],xmm0;

		//if >=128 bytes use 128 byte unrolled loop
		//i use cmp ..,127 + jna because 127 is encodable using the simm8 form
		cmp eax,127;
		jna _loop_1;

		//since this is a common branch target it could be good to align it -- no idea if it has any effect :p
		align 16

		//128 byte unrolled loop
_loop_8:

		MOVSRC xmm0,[edx+0x00];	//read first to avoid read-after-write stalls
		MOVDST [ecx+0x00],xmm0; //then write :p
		MOVSRC xmm0,[edx+0x10];
		MOVDST [ecx+0x10],xmm0;
		sub edx,-128;			//edx won't be used for a while, so update it here. sub/-128 for simm8 encoding
		sub ecx,-128;			//ecx won't be used for a while, so update it here. sub/-128 for simm8 encoding

		MOVSRC xmm0,[edx+0x20-128];
		MOVDST [ecx+0x20-128],xmm0;
		MOVSRC xmm0,[edx+0x30-128];
		MOVDST [ecx+0x30-128],xmm0;
		add eax,-128;			//eax won't be used for a while, so update it here. add/-128 for simm8 encoding

		MOVSRC xmm0,[edx+0x40-128];
		MOVDST [ecx+0x40-128],xmm0;
		MOVSRC xmm0,[edx+0x50-128];
		MOVDST [ecx+0x50-128],xmm0;

		MOVSRC xmm0,[edx+0x60-128];
		MOVDST [ecx+0x60-128],xmm0;
		MOVSRC xmm0,[edx+0x70-128];
		MOVDST [ecx+0x70-128],xmm0;

		//127~ja, 127 is encodable as simm8 :)
		cmp eax,127;
		ja _loop_8;

		//direct copy for 0~7 qwords
		//in order to avoid the inc/dec of all 3 registers
		//i use negative relative addressing from the top of the buffers
		//[top-current index]

_loop_1:
		//prepare the regs for 'negative relative addressing'
		add edx,eax;
		add ecx,eax;
		neg eax;
		jz cleanup;	//exit if nothing to do

_loop_1_inner:
		MOVSRC xmm0,[edx+eax];
		movaps [ecx+eax],xmm0;

		add eax,16;		//while the offset is still negative we have data to copy
		js _loop_1_inner;

		//done !
cleanup:
		//restore xmm and exit ~)
		movaps xmm0,[_xmm_backup];
		ret 4;
	}
	#undef MOVSRC
	#undef MOVDST
}

// Custom memcpy, only for 16 byte aligned stuff (used for mtgs)
// This function is optimized for medium-small transfer sizes (<2048, >=128). No prefetching is
// used since the reads are linear and the cache logic can predict em :)
// *OBSOLETE* -- memcpy_amd_ has been optimized and is now faster.
__declspec(naked) void __fastcall memcpy_raz_(void *dest, const void *src, size_t bytes)
{
	// Code Implementation Notes:
	// Uses a forward copy, in 128 byte blocks, and then does the remaining in 16 byte blocks :)

	// MOVSRC = opcode used to read. I use the same code for the unaligned version, with a different define :)
	#define MOVSRC movaps
	#define MOVDST movaps
	__asm
	{
		//Reads before reads, to avoid stalls
		mov eax,[esp+4];
		//Make sure to save xmm0, it must be preserved ...
		movaps [_xmm_backup],xmm0;

		//if >=128 bytes use 128 byte unrolled loop
		//i use cmp ..,127 + jna because 127 is encodable using the simm8 form
		cmp eax,127;
		jna _loop_1;

		//since this is a common branch target it could be good to align it -- no idea if it has any effect :p
		align 16

		//128 byte unrolled loop
_loop_8:

		MOVSRC xmm0,[edx+0x00];	//read first to avoid read-after-write stalls
		MOVDST [ecx+0x00],xmm0; //then write :p
		MOVSRC xmm0,[edx+0x10];
		MOVDST [ecx+0x10],xmm0;
		sub edx,-128;			//edx won't be used for a while, so update it here. sub/-128 for simm8 encoding
		sub ecx,-128;			//ecx won't be used for a while, so update it here. sub/-128 for simm8 encoding

		MOVSRC xmm0,[edx+0x20-128];
		MOVDST [ecx+0x20-128],xmm0;
		MOVSRC xmm0,[edx+0x30-128];
		MOVDST [ecx+0x30-128],xmm0;
		add eax,-128;			//eax won't be used for a while, so update it here. add/-128 for simm8 encoding

		MOVSRC xmm0,[edx+0x40-128];
		MOVDST [ecx+0x40-128],xmm0;
		MOVSRC xmm0,[edx+0x50-128];
		MOVDST [ecx+0x50-128],xmm0;

		MOVSRC xmm0,[edx+0x60-128];
		MOVDST [ecx+0x60-128],xmm0;
		MOVSRC xmm0,[edx+0x70-128];
		MOVDST [ecx+0x70-128],xmm0;

		//127~ja, 127 is encodable as simm8 :)
		cmp eax,127;
		ja _loop_8;
		
		//direct copy for 0~7 qwords
		//in order to avoid the inc/dec of all 3 registers
		//i use negative relative addressing from the top of the buffers
		//[top-current index]

_loop_1:
		//prepare the regs for 'negative relative addressing'
		add edx,eax;
		add ecx,eax;
		neg eax;
		jz cleanup;	//exit if nothing to do

_loop_1_inner:
		MOVSRC xmm0,[edx+eax];
		MOVDST [ecx+eax],xmm0;
		
		add eax,16;		//while the offset is still negative we have data to copy
		js _loop_1_inner;

		//done !
cleanup:
		//restore xmm and exit ~)
		movaps xmm0,[_xmm_backup];
		ret 4;
	}
	#undef MOVSRC
	#undef MOVDST
}

// This memcpy routine is for use in situations where the source buffer's alignment is indeterminate.
__forceinline void __fastcall memcpy_raz_usrc(void *dest, const void *src, size_t bytes)
{
	if( ((uptr)src & 0xf) == 0 )
		memcpy_raz_( dest, src, bytes );
	else
		_memcpy_raz_usrc( dest, src, bytes );
}

// This memcpy routine is for use in situations where the destination buffer's alignment is indeterminate.
__forceinline void __fastcall memcpy_raz_udst(void *dest, const void *src, size_t bytes)
{
	if( ((uptr)dest & 0xf) == 0 )
		memcpy_raz_( dest, src, bytes );
	else
		_memcpy_raz_udst( dest, src, bytes );
}


//////////////////////////////////////////////////////////////////////////
// Fast memcpy as coded by AMD, and thn improved by air.
//
// This routine preserves mmx registers!  It's the complete real deal!
__declspec(naked) void __fastcall memcpy_amd_(void *dest, const void *src, size_t n)
{
    __asm
	{
	push    edi  
	push    esi  

	mov		edi, ecx		; destination
	mov		esi, edx		; source
	mov		ecx, [esp+12]	; number of bytes to copy
	mov		eax, ecx		; keep a copy of count

	cld
	cmp		eax, TINY_BLOCK_COPY
	jb		$memcpy_ic_3	; tiny? skip mmx copy

	cmp		eax, 32*1024		; don't align between 32k-64k because
	jbe		$memcpy_do_align	;  it appears to be slower
	cmp		eax, 64*1024
	jbe		$memcpy_align_done
$memcpy_do_align:
	mov		eax, 8			; a trick that's faster than rep movsb...
	sub		eax, edi		; align destination to qword
	and		eax, 111b		; get the low bits
	sub		ecx, eax		; update copy count
	neg		eax				; set up to jump into the array
	add		eax, offset $memcpy_align_done
	jmp		eax				; jump to array of movsb's

align 4
	movsb
	movsb
	movsb
	movsb
	movsb
	movsb
	movsb
	movsb

$memcpy_align_done:			; destination is dword aligned
	mov		eax, ecx		; number of bytes left to copy
	shr		eax, 6			; get 64-byte block count
	jz		$memcpy_ic_2	; finish the last few bytes

	mov     edx, offset _mmx_backup ; will probably need this to save/restore mmx
	cmp		eax, IN_CACHE_COPY/64	; too big 4 cache? use uncached copy
	jae		$memcpy_uc_test

	movq	[edx+0x00],mm0
	movq	[edx+0x08],mm1
	movq	[edx+0x10],mm2
	movq	[edx+0x18],mm3

// This is small block copy that uses the MMX registers to copy 8 bytes
// at a time.  It uses the "unrolled loop" optimization, and also uses
// the software prefetch instruction to get the data into the cache.
align 16
$memcpy_ic_1:			; 64-byte block copies, in-cache copy

	prefetchnta [esi + (200*64/34+192)]		; start reading ahead

	movq	mm0, [esi+0]	; read 64 bits
	movq	mm1, [esi+8]
	movq	[edi+0], mm0	; write 64 bits
	movq	[edi+8], mm1	;    note:  the normal movq writes the
	movq	mm2, [esi+16]	;    data to cache; a cache line will be
	movq	mm3, [esi+24]	;    allocated as needed, to store the data
	movq	[edi+16], mm2
	movq	[edi+24], mm3
	movq	mm0, [esi+32]
	movq	mm1, [esi+40]
	movq	[edi+32], mm0
	movq	[edi+40], mm1
	movq	mm2, [esi+48]
	movq	mm3, [esi+56]
	movq	[edi+48], mm2
	movq	[edi+56], mm3

	add		esi, 64			; update source pointer
	add		edi, 64			; update destination pointer
	dec		eax				; count down
	jnz		$memcpy_ic_1	; last 64-byte block?

	movq	mm0,[edx+0x00]
	movq	mm1,[edx+0x08]
	movq	mm2,[edx+0x10]
	movq	mm3,[edx+0x18]

$memcpy_ic_2:
	mov		eax, ecx		; has valid low 6 bits of the byte count
$memcpy_ic_3:
	shr		eax, 2			; dword count
	and		eax, 1111b		; only look at the "remainder" bits
	neg		eax				; set up to jump into the array
	add		eax, offset $memcpy_last_few
	jmp		eax				; jump to array of movsd's

$memcpy_uc_test:
	or		eax, eax		; tail end of block prefetch will jump here
	jz		$memcpy_ic_2	; no more 64-byte blocks left

// For larger blocks, which will spill beyond the cache, it's faster to
// use the Streaming Store instruction MOVNTQ.   This write instruction
// bypasses the cache and writes straight to main memory.  This code also
// uses the software prefetch instruction to pre-read the data.

	movq	[edx+0x00],mm0
	movq	[edx+0x08],mm1
	movq	[edx+0x10],mm2

align 16
$memcpy_uc_1:				; 64-byte blocks, uncached copy

	prefetchnta [esi + (200*64/34+192)]		; start reading ahead

	movq	mm0,[esi+0]		; read 64 bits
	add		edi,64			; update destination pointer
	movq	mm1,[esi+8]
	add		esi,64			; update source pointer
	movq	mm2,[esi-48]
	movntq	[edi-64], mm0	; write 64 bits, bypassing the cache
	movq	mm0,[esi-40]	;    note: movntq also prevents the CPU
	movntq	[edi-56], mm1	;    from READING the destination address
	movq	mm1,[esi-32]	;    into the cache, only to be over-written
	movntq	[edi-48], mm2	;    so that also helps performance
	movq	mm2,[esi-24]
	movntq	[edi-40], mm0
	movq	mm0,[esi-16]
	movntq	[edi-32], mm1
	movq	mm1,[esi-8]
	movntq	[edi-24], mm2
	movntq	[edi-16], mm0
	dec		eax
	movntq	[edi-8], mm1
	jnz		$memcpy_uc_1	; last 64-byte block?

	movq	mm0,[edx+0x00]
	movq	mm1,[edx+0x08]
	movq	mm2,[edx+0x10]

	jmp		$memcpy_ic_2		; almost done  (not needed because large copy below was removed)

// For the largest size blocks, a special technique called Block Prefetch
// can be used to accelerate the read operations.   Block Prefetch reads
// one address per cache line, for a series of cache lines, in a short loop.
// This is faster than using software prefetch.  The technique is great for
// getting maximum read bandwidth, especially in DDR memory systems.

// Note: Pcsx2 rarely invokes large copies, so this mode has been disabled to
// help keep the code cache footprint of memcpy_fast to a minimum.
/*
$memcpy_bp_1:			; large blocks, block prefetch copy

	cmp		ecx, CACHEBLOCK			; big enough to run another prefetch loop?
	jl		$memcpy_64_test			; no, back to regular uncached copy

	mov		eax, CACHEBLOCK / 2		; block prefetch loop, unrolled 2X
	add		esi, CACHEBLOCK * 64	; move to the top of the block
align 16
$memcpy_bp_2:
	mov		edx, [esi-64]		; grab one address per cache line
	mov		edx, [esi-128]		; grab one address per cache line
	sub		esi, 128			; go reverse order to suppress HW prefetcher
	dec		eax					; count down the cache lines
	jnz		$memcpy_bp_2		; keep grabbing more lines into cache

	mov		eax, CACHEBLOCK		; now that it's in cache, do the copy
align 16
$memcpy_bp_3:
	movq	mm0, [esi   ]		; read 64 bits
	movq	mm1, [esi+ 8]
	movq	mm2, [esi+16]
	movq	mm3, [esi+24]
	movq	mm4, [esi+32]
	movq	mm5, [esi+40]
	movq	mm6, [esi+48]
	movq	mm7, [esi+56]
	add		esi, 64				; update source pointer
	movntq	[edi   ], mm0		; write 64 bits, bypassing cache
	movntq	[edi+ 8], mm1		;    note: movntq also prevents the CPU
	movntq	[edi+16], mm2		;    from READING the destination address 
	movntq	[edi+24], mm3		;    into the cache, only to be over-written,
	movntq	[edi+32], mm4		;    so that also helps performance
	movntq	[edi+40], mm5
	movntq	[edi+48], mm6
	movntq	[edi+56], mm7
	add		edi, 64				; update dest pointer

	dec		eax					; count down

	jnz		$memcpy_bp_3		; keep copying
	sub		ecx, CACHEBLOCK		; update the 64-byte block count
	jmp		$memcpy_bp_1		; keep processing chunks
*/

// The smallest copy uses the X86 "movsd" instruction, in an optimized
// form which is an "unrolled loop".   Then it handles the last few bytes.
align 16
	movsd
	movsd			; perform last 1-15 dword copies
	movsd
	movsd
	movsd
	movsd
	movsd
	movsd
	movsd
	movsd			; perform last 1-7 dword copies
	movsd
	movsd
	movsd
	movsd
	movsd
	movsd

$memcpy_last_few:		; dword aligned from before movsd's
	and		ecx, 11b	; the last few cows must come home
	jz		$memcpy_final	; no more, let's leave
	rep		movsb		; the last 1, 2, or 3 bytes

$memcpy_final: 
	emms				; clean up the MMX state
	sfence				; flush the write buffer
	//mov		eax, [dest]	; ret value = destination pointer

	pop    esi  
	pop    edi

	ret 4
    }
}

// mmx mem-compare implementation, size has to be a multiple of 8
// returns 0 is equal, nonzero value if not equal
// ~10 times faster than standard memcmp
// (zerofrog)
u8 memcmp_mmx(const void* src1, const void* src2, int cmpsize)
{
	assert( (cmpsize&7) == 0 );

	__asm {
		push esi
		mov ecx, cmpsize
		mov edx, src1
		mov esi, src2

		cmp ecx, 32
		jl Done4

		// custom test first 8 to make sure things are ok
		movq mm0, [esi]
		movq mm1, [esi+8]
		pcmpeqd mm0, [edx]
		pcmpeqd mm1, [edx+8]
		pand mm0, mm1
		movq mm2, [esi+16]
		pmovmskb eax, mm0
		movq mm3, [esi+24]

		// check if eq
		cmp eax, 0xff
		je NextComp
		mov eax, 1
		jmp End

NextComp:
		pcmpeqd mm2, [edx+16]
		pcmpeqd mm3, [edx+24]
		pand mm2, mm3
		pmovmskb eax, mm2

		sub ecx, 32
		add esi, 32
		add edx, 32

		// check if eq
		cmp eax, 0xff
		je ContinueTest
		mov eax, 1
		jmp End

		cmp ecx, 64
		jl Done8

Cmp8:
		movq mm0, [esi]
		movq mm1, [esi+8]
		movq mm2, [esi+16]
		movq mm3, [esi+24]
		movq mm4, [esi+32]
		movq mm5, [esi+40]
		movq mm6, [esi+48]
		movq mm7, [esi+56]
		pcmpeqd mm0, [edx]
		pcmpeqd mm1, [edx+8]
		pcmpeqd mm2, [edx+16]
		pcmpeqd mm3, [edx+24]
		pand mm0, mm1
		pcmpeqd mm4, [edx+32]
		pand mm0, mm2
		pcmpeqd mm5, [edx+40]
		pand mm0, mm3
		pcmpeqd mm6, [edx+48]
		pand mm0, mm4
		pcmpeqd mm7, [edx+56]
		pand mm0, mm5
		pand mm0, mm6
		pand mm0, mm7
		pmovmskb eax, mm0
		
		// check if eq
		cmp eax, 0xff
		je Continue
		mov eax, 1
		jmp End

Continue:
		sub ecx, 64
		add esi, 64
		add edx, 64
ContinueTest:
		cmp ecx, 64
		jge Cmp8

Done8:
		test ecx, 0x20
		jz Done4
		movq mm0, [esi]
		movq mm1, [esi+8]
		movq mm2, [esi+16]
		movq mm3, [esi+24]
		pcmpeqd mm0, [edx]
		pcmpeqd mm1, [edx+8]
		pcmpeqd mm2, [edx+16]
		pcmpeqd mm3, [edx+24]
		pand mm0, mm1
		pand mm0, mm2
		pand mm0, mm3
		pmovmskb eax, mm0
		sub ecx, 32
		add esi, 32
		add edx, 32

		// check if eq
		cmp eax, 0xff
		je Done4
		mov eax, 1
		jmp End

Done4:
		cmp ecx, 24
		jne Done2
		movq mm0, [esi]
		movq mm1, [esi+8]
		movq mm2, [esi+16]
		pcmpeqd mm0, [edx]
		pcmpeqd mm1, [edx+8]
		pcmpeqd mm2, [edx+16]
		pand mm0, mm1
		pand mm0, mm2
		pmovmskb eax, mm0

		// check if eq
		cmp eax, 0xff
		setne al
		jmp End

Done2:
		cmp ecx, 16
		jne Done1

		movq mm0, [esi]
		movq mm1, [esi+8]
		pcmpeqd mm0, [edx]
		pcmpeqd mm1, [edx+8]
		pand mm0, mm1
		pmovmskb eax, mm0

		// check if eq
		cmp eax, 0xff
		setne al
		jmp End

Done1:
		cmp ecx, 8
		jne Done

		mov eax, [esi]
		mov esi, [esi+4]
		cmp eax, [edx]
		je Next
		mov eax, 1
		jmp End

Next:
		cmp esi, [edx+4]
		setne al
		jmp End

Done:
		xor eax, eax

End:
		pop esi
		emms
	}
}


// returns the xor of all elements, cmpsize has to be mult of 8
void memxor_mmx(void* dst, const void* src1, int cmpsize)
{
	FreezeMMXRegs(1);
	assert( (cmpsize&7) == 0 );

	__asm {
		mov ecx, cmpsize
		mov eax, src1
		mov edx, dst

		cmp ecx, 64
		jl Setup4

		movq mm0, [eax]
		movq mm1, [eax+8]
		movq mm2, [eax+16]
		movq mm3, [eax+24]
		movq mm4, [eax+32]
		movq mm5, [eax+40]
		movq mm6, [eax+48]
		movq mm7, [eax+56]
		sub ecx, 64
		add eax, 64
		cmp ecx, 64
		jl End8

Cmp8:
		pxor mm0, [eax]
		pxor mm1, [eax+8]
		pxor mm2, [eax+16]
		pxor mm3, [eax+24]
		pxor mm4, [eax+32]
		pxor mm5, [eax+40]
		pxor mm6, [eax+48]
		pxor mm7, [eax+56]

		sub ecx, 64
		add eax, 64
		cmp ecx, 64
		jge Cmp8

End8:
		pxor mm0, mm4
		pxor mm1, mm5
		pxor mm2, mm6
		pxor mm3, mm7

		cmp ecx, 32
		jl End4
		pxor mm0, [eax]
		pxor mm1, [eax+8]
		pxor mm2, [eax+16]
		pxor mm3, [eax+24]
		sub ecx, 32
		add eax, 32
		jmp End4

Setup4:
		cmp ecx, 32
		jl Setup2

		movq mm0, [eax]
		movq mm1, [eax+8]
		movq mm2, [eax+16]
		movq mm3, [eax+24]
		sub ecx, 32
		add eax, 32

End4:
		pxor mm0, mm2
		pxor mm1, mm3

		cmp ecx, 16
		jl End2
		pxor mm0, [eax]
		pxor mm1, [eax+8]
		sub ecx, 16
		add eax, 16
		jmp End2

Setup2:
		cmp ecx, 16
		jl Setup1

		movq mm0, [eax]
		movq mm1, [eax+8]
		sub ecx, 16
		add eax, 16

End2:
		pxor mm0, mm1

		cmp ecx, 8
		jl End1
		pxor mm0, [eax]
End1:
		movq [edx], mm0
		jmp End

Setup1:
		movq mm0, [eax]
		movq [edx], mm0
End:
		emms
	}
	FreezeMMXRegs(0);
}

#endif
