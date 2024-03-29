
#pragma once

// Specialized function pointers for each read type
typedef  mem8_t __fastcall vtlbMemR8FP(u32 addr);
typedef  mem16_t __fastcall vtlbMemR16FP(u32 addr);
typedef  mem32_t __fastcall vtlbMemR32FP(u32 addr);
typedef  void __fastcall vtlbMemR64FP(u32 addr,mem64_t* data);
typedef  void __fastcall vtlbMemR128FP(u32 addr,mem128_t* data);

// Specialized function pointers for each write type
typedef  void __fastcall vtlbMemW8FP(u32 addr,mem8_t data);
typedef  void __fastcall vtlbMemW16FP(u32 addr,mem16_t data);
typedef  void __fastcall vtlbMemW32FP(u32 addr,mem32_t data);
typedef  void __fastcall vtlbMemW64FP(u32 addr,const mem64_t* data);
typedef  void __fastcall vtlbMemW128FP(u32 addr,const mem128_t* data);

typedef u32 vtlbHandler;

extern void vtlb_Core_Alloc();
extern void vtlb_Core_Shutdown();
extern void vtlb_Init();
extern void vtlb_Reset();
extern void vtlb_Term();
extern u8* vtlb_malloc( uint size, uint align );
extern void vtlb_free( void* pmem, uint size );


//physical stuff
vtlbHandler vtlb_RegisterHandler(	vtlbMemR8FP* r8,vtlbMemR16FP* r16,vtlbMemR32FP* r32,vtlbMemR64FP* r64,vtlbMemR128FP* r128,
									vtlbMemW8FP* w8,vtlbMemW16FP* w16,vtlbMemW32FP* w32,vtlbMemW64FP* w64,vtlbMemW128FP* w128);

extern void vtlb_MapHandler(vtlbHandler handler,u32 start,u32 size);
extern void vtlb_MapBlock(void* base,u32 start,u32 size,u32 blocksize=0);
extern void* vtlb_GetPhyPtr(u32 paddr);
//extern void vtlb_Mirror(u32 new_region,u32 start,u32 size); // -> not working yet :(

//virtual mappings
extern void vtlb_VMap(u32 vaddr,u32 paddr,u32 sz);
extern void vtlb_VMapBuffer(u32 vaddr,void* buffer,u32 sz);
extern void vtlb_VMapUnmap(u32 vaddr,u32 sz);

//Memory functions

extern mem8_t __fastcall vtlb_memRead8(u32 mem);
extern mem16_t __fastcall vtlb_memRead16(u32 mem);
extern u32 __fastcall vtlb_memRead32(u32 mem);
extern void __fastcall vtlb_memRead64(u32 mem, u64 *out);
extern void __fastcall vtlb_memRead128(u32 mem, u64 *out);
extern void __fastcall vtlb_memWrite8 (u32 mem, mem8_t  value);
extern void __fastcall vtlb_memWrite16(u32 mem, mem16_t value);
extern void __fastcall vtlb_memWrite32(u32 mem, u32 value);
extern void __fastcall vtlb_memWrite64(u32 mem, const u64* value);
extern void __fastcall vtlb_memWrite128(u32 mem, const u64* value);

extern void vtlb_DynGenWrite(u32 sz);
extern void vtlb_DynGenRead32(u32 bits, bool sign);
extern void vtlb_DynGenRead64(u32 sz);

extern void vtlb_DynGenWrite_Const( u32 bits, u32 addr_const );
extern void vtlb_DynGenRead64_Const( u32 bits, u32 addr_const );
extern void vtlb_DynGenRead32_Const( u32 bits, bool sign, u32 addr_const );

namespace vtlb_private
{
	static const uint VTLB_ALLOC_SIZE = 0x2900000;	//this is a bit more than required

	static const uint VTLB_PAGE_BITS = 12;
	static const uint VTLB_PAGE_MASK = 4095;
	static const uint VTLB_PAGE_SIZE = 4096;

	static const uint VTLB_PMAP_ITEMS = 0x20000000 / VTLB_PAGE_SIZE;
	static const uint VTLB_PMAP_SZ = 0x20000000;
	static const uint VTLB_VMAP_ITEMS = 0x100000000ULL / VTLB_PAGE_SIZE;

	struct MapData
	{
		u8* alloc_base;			//base of the memory array
		int alloc_current;		//current base

		s32 pmap[VTLB_PMAP_ITEMS];	//512KB
		s32 vmap[VTLB_VMAP_ITEMS];   //4MB

		// first indexer -- 8/16/32/64/128 bit tables [values 0-4]
		// second indexer -- read/write  [0 or 1]
		// third indexer -- 128 possible handlers!
		void* RWFT[5][2][128];
	};

	PCSX2_ALIGNED_EXTERN( 64, MapData vtlbdata );
}
