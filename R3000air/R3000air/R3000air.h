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

#include <stdio.h>

#define __instinline __forceinline		// MSVC still fails to inline as much as it should

// ------------------------------------------------------------------------
//
enum IopEventType
{
	IopEvt_Counter0 = 0,
	IopEvt_Counter1,
	IopEvt_Counter2,
	IopEvt_Counter3,
	IopEvt_Counter4,
	IopEvt_Counter5,
	IopEvt_Exception,
	IopEvt_SIF0,
	IopEvt_SIF1,
	IopEvt_SIO,
	IopEvt_SIO2_Dma11,
	IopEvt_SIO2_Dma12,

	IopEvt_Cdvd,
	IopEvt_CdvdRead,
	IopEvt_Cdrom,
	IopEvt_CdromRead,
	IopEvt_SPU2_Dma4,	// Core 0 DMA
	IopEvt_SPU2_Dma7,	// Core 1 DMA
	IopEvt_SPU2,		// SPU command/event processor
	IopEvt_DEV9,
	IopEvt_USB,

	IopEvt_BreakForEE,

	IopEvt_CountNonIdle,

	// Idle state, no events scheduled.  Placed at -1 since it has no actual
	// entry in the Event System's event schedule table.
	IopEvt_Idle = IopEvt_CountNonIdle,

	IopEvt_CountAll		// total number of schedulable event types in the Iop
};

// ------------------------------------------------------------------------
union IntSign32
{
	s32 SL;
	u32 UL;

	s16 SS[2];
	u16 US[2];

	s8 SB[4];
	u8 UB[4];
};

// ------------------------------------------------------------------------
enum MipsGPRs_t
{
	GPR_Invalid = -1,
	GPR_r0 = 0,		// zero register -- it's always zero!
	GPR_at,
	GPR_v0, GPR_v1,
	GPR_a0, GPR_a1, GPR_a2, GPR_a3,
	
	GPR_t0, GPR_t1, GPR_t2, GPR_t3,
	GPR_t4, GPR_t5, GPR_t6, GPR_t7,

	GPR_s0, GPR_s1, GPR_s2, GPR_s3,
	GPR_s4, GPR_s5, GPR_s6, GPR_s7,
	
	GPR_t8, GPR_t9, GPR_k0, GPR_k1,
	GPR_gp, GPR_sp, GPR_s8,
	
	GPR_ra,		// Link Register!
	
	GPR_hi, GPR_lo
};

namespace R3000A
{

// Houses the 32 bit MIPS GPRs (R3000 style, yeah!)
struct GPRRegs32
{
	IntSign32 r[34];

	__forceinline IntSign32& operator[]( uint gpr )
	{
		jASSUME( gpr < 34 );
		return r[gpr];
	}

	__forceinline const IntSign32& operator[]( uint gpr ) const
	{
		jASSUME( gpr < 34 );
		return r[gpr];
	}
};

union CP0Regs
{
	struct
	{
		u32
			Index,     Random,    EntryLo0,  EntryLo1,
			Context,   PageMask,  Wired,     Reserved0,
			BadVAddr,  Count,     EntryHi,   Compare,
			Status,    Cause,     EPC,       PRid,
			Config,    LLAddr,    WatchLO,   WatchHI,
			XContext,  Reserved1, Reserved2, Reserved3,
			Reserved4, Reserved5, ECC,       CacheErr,
			TagLo,     TagHi,     ErrorEPC,  Reserved6;
	} n;
	IntSign32 r[32];
};

// ------------------------------------------------------------------------
//
struct CpuEventType
{
	CpuEventType* next;	// link the list?!
	void (*Execute)();

	s32 OrigDelta;		// original delta time of the scheduled event
	s32 RelativeDelta;	// delta relative to the scheduled item in front of it

	bool IsActive() const
	{
		return next != NULL;
	}
};


//////////////////////////////////////////////////////////////////////////////////////////
//
struct Registers
{
	GPRRegs32 GPR;			// General Purpose Registers
	CP0Regs CP0;			// Coprocessor0 Registers
	
	u32 BIU_Cache_Ctrl;		// Bus Interface Unit / Cache Controller [mapped to 0xfffe0130]
	u32 MysteryRegAt_FFFE0140;
	u32 MysteryRegAt_FFFE0144;

	u32 pc;					// Program counter for the next instruction fetch
	u32 VectorPC;			// pc to vector to after the next instruction fetch

	// marks the original duration of time for the current pending event.  This is
	// typically used to determine the amount of time passed since the last update
	// to iopRegs._cycle:
	//  currentcycle = _cycle + ( evtCycleDuration - evtCycleCountdown );
	s32 evtCycleDuration;

	// marks the *current* duration of time until the current pending event. In
	// other words: counts down from evtCycleDuration to 0; event is raised when 0
	// is reached.
	s32 evtCycleCountdown;

	// number of cycles pending on the current div unit instruction (mult and div both run in the same
	// unit).  Any zero-or-negative values mean the unit is free and no stalls incurred for executing
	// a new instruction on the pipeline.  Negative values are flushed to 0 during PendingEvent executions.
	// (faster than flushing to zero on every cycle update).
	s32 DivUnitCycles;

	bool IsDelaySlot;
	bool IsExecuting;

	// ------------------------------------------------------------------------
	// Internal Storage Section (Protected-style members which aren't protected
	//                  since this isn't a class type)
	// ------------------------------------------------------------------------

	// Internal cycle counter.  Depending on the type of event system used to manage
	u32 m_cycle;

	// Countdown value for each event known to the IOP scheduler.
	CpuEventType m_Events[IopEvt_CountAll];

	// list of pending events, in sorted order from nearest to furthest.
	CpuEventType* m_ActiveEvents;

	// ------------------------------------------------------------------------
	//                    Accessors and Properties Section
	// ------------------------------------------------------------------------

	// Indexer for the IOP's GPRs, using the MipsGPRs_t enumeration.
	__forceinline IntSign32& operator[]( MipsGPRs_t gpr )				{ return GPR[(uint)gpr]; }
	__forceinline const IntSign32& operator[]( MipsGPRs_t gpr ) const	{ return GPR[(uint)gpr]; }

	// [TODO] : Make an accessor array for this one?
	const CpuEventType& GetEventInfo( IopEventType evt ) const
	{
		return m_Events[evt];
	}

	u32 GetCycle() const
	{
		return m_cycle + ( evtCycleDuration - evtCycleCountdown );
	}
	
	s32 GetPendingCycles() const
	{
		return evtCycleDuration - evtCycleCountdown;
	}
	
	// ------------------------------------------------------------------------
	//            General Methods Section (modify struct contents)
	// ------------------------------------------------------------------------
	
	void StopExecution();

	void AddCycles( int amount )
	{
		evtCycleCountdown	-= amount;
		DivUnitCycles		-= amount;
	}

	// ------------------------------------------------------------------------
	// Sets a new PC in "abrupt" fashion (without consideration for delay slot).
	// Effectively cancels the delay slot instruction, making this ideal for use
	// in raising exceptions.
	__releaseinline void SetExceptionPC( u32 newpc )
	{
		//pc			= newpc;
		VectorPC	= newpc;
		IsDelaySlot = false;
	}
	
	// ------------------------------------------------------------------------
	// DivUnitStall - CPU stalls are performed accordingly and the DivUnit's cycle status
	// is updated.  The DIV pipe runs in parallel to the rest of the CPU, but if one of
	// the dependent  instructions is used, it means we need to stall the IOP to wait for
	// the result.
	//
	// newStall - non-zero values indicate an instruction which depends on the DivUnit.
	//
	__releaseinline void DivUnitStall( u32 newStall )
	{
		if( newStall == 0 ) return;		// instruction doesn't use the DivUnit?

		// anything zero or less means the DivUnit is empty
		if( DivUnitCycles > 0 )
		{
			evtCycleCountdown -= DivUnitCycles;
			//Console::Status( "Iop DivUnit Stall for %d cycles.", params DivUnitCycles );
		}

		DivUnitCycles = newStall;
	}
	
	void RaiseExtInt( uint irq );
	
	// ------------------------------------------------------------------------
	//                        Event System Section
	// ------------------------------------------------------------------------

	void ResetEvents();

	void RescheduleEvent( CpuEventType& evt, s32 delta );
	void ScheduleEvent( IopEventType evt, s32 delta );
	void ScheduleEvent( IopEventType evt, s32 delta, void (*execute)() );
	void RaiseException();
	void Dispatch( IopEventType evt );
	void ExecutePendingEvents();
	void IdleEventHandler();

	void CancelEvent( IopEventType evt );
	void CancelEvent( CpuEventType& evt );

	// ------------------------------------------------------------------------
	// RescheduleEvent - Used to re-schedule events which are *known* to be currently
	// unscheduled.  Calling this function on an event which is currently scheduled
	// is an invalid action, and will cause an exception in devel/debug builds.
	//
	void RescheduleEvent( IopEventType evt, s32 delta )
	{
		jASSUME( evt < IopEvt_CountNonIdle );
		CpuEventType& thisevt( m_Events[evt] );
		RescheduleEvent( thisevt, delta );
	}
};

PCSX2_ALIGNED16_EXTERN(Registers iopRegs);

union GprStatus
{
	u8 Value;

	struct  
	{
		bool Rd:1;
		bool Rs:1;
		bool Rt:1;
		bool Fs:1;		// the COP0 register as a source/dest
		bool Hi:1;
		bool Lo:1;
		bool Link:1;	// ra - the link register
		bool Memory:1;
	};
};

#undef _Funct_
#undef _Basecode_
#undef _Rd_
#undef _Rs_
#undef _Rt_
#undef _Sa_
#undef _PC_
#undef _Opcode_

//////////////////////////////////////////////////////////////////////////////////////////
//
#define INSTRUCTION_API() \
	void BGEZ(); \
	void BGEZAL(); \
	void BGTZ(); \
	void BLEZ(); \
	void BLTZ(); \
	void BLTZAL(); \
	void BEQ(); \
	void BNE(); \
 \
	void J(); \
	void JAL(); \
	void JR(); \
	void JALR(); \
 \
	void ADDI(); \
	void ADDIU(); \
	void ANDI(); \
	void ORI(); \
	void XORI(); \
	void SLTI(); \
	void SLTIU(); \
 \
	void ADD(); \
	void ADDU(); \
	void SUB(); \
	void SUBU(); \
	void AND(); \
	void OR(); \
	void XOR(); \
	void NOR(); \
	void SLT(); \
	void SLTU(); \
 \
	void DIV(); \
	void DIVU(); \
	void MULT(); \
	void MULTU(); \
 \
	void SLL(); \
	void SRA(); \
	void SRL(); \
	void SLLV(); \
	void SRAV(); \
	void SRLV(); \
	void LUI(); \
 \
	void MFHI(); \
	void MFLO(); \
	void MTHI(); \
	void MTLO(); \
 \
	void BREAK(); \
	void SYSCALL(); \
	void RFE(); \
 \
	void LB(); \
	void LBU(); \
	void LH(); \
	void LHU(); \
	void LW(); \
	void LWL(); \
	void LWR(); \
 \
	void SB(); \
	void SH(); \
	void SW(); \
	void SWL(); \
	void SWR(); \
 \
	void MFC0(); \
	void CFC0(); \
	void MTC0(); \
	void CTC0(); \
	void Unknown();


//////////////////////////////////////////////////////////////////////////////////////////
//
// This enumerator is used to describe the display of each instruction/opcode.
// All instructions are allowed three parameters.
enum InstParamType
{
	Param_None = 0,
	Param_Rt,
	Param_Rs,
	Param_Rd,
	Param_Sa,

	Param_Fs,			// Used by Cop0 load/store

	Param_Hi,
	Param_Lo,
	Param_HiLo,			// 64 bit operand (allowed as destination only)

	Param_Imm,

	Param_AddrImm,		// Address Immediate (Rs + Imm()), used by load/store
	Param_BranchOffset,
	Param_JumpTarget,	// 26 bit immediate used as a jump target
	Param_RsJumpTarget	// 32 bit register used as a jump target
};

struct InstDiagInfo
{
	const char* Name;			// display name
	const InstParamType Param[3];	// parameter mappings
};

enum RegField_t
{
	RF_Unused = -1,
	RF_Rd = 0,
	RF_Rt,
	RF_Rs,
	RF_Hi,
	RF_Lo,

	RF_Link,

	RF_Count		// total number of register fields
};

//////////////////////////////////////////////////////////////////////////////////////////
//
struct Opcode
{
	u32 U32;

	u8 Funct() const { return U32 & 0x3F; }
	u8 Sa() const { return (U32 >> 6) & 0x1F; }
	u8 Rd() const { return (U32 >> 11) & 0x1F; }
	u8 Rt() const { return (U32 >> 16) & 0x1F; }
	u8 Rs() const { return (U32 >> 21) & 0x1F; }
	u8 Basecode() const { return U32 >> 26; }

	// Returns the target portion of the opcode (26 bit immediate)
	uint Target() const { return U32 & 0x03ffffff; }

	// Sign-extended immediate
	s32 Imm() const { return (s16)U32; }

	// Zero-extended immediate
	u32 ImmU() const { return (u16)U32; }
	u32 TrapCode() const { return (u16)(U32 >> 6); };

	Opcode() {}

	Opcode( u32 src ) :
		U32( src ) {}
		
	__forceinline int RegField( RegField_t field ) const
	{
		switch( field )
		{
			case RF_Rd: return Rd();
			case RF_Rt: return Rt();
			case RF_Rs: return Rs();
			case RF_Hi: return GPR_hi;
			case RF_Lo: return GPR_lo;
			
			jNO_DEFAULT
		}
	}
};

//////////////////////////////////////////////////////////////////////////////////////////
// This object is immutable.  Do not modify, just create new ones. :)
// See R3000AirInstruction.cpp for code implementations.
//
// Note on R3000A cycle counting: it's best to assume *all* R3000A instructions as a single
// cycle.  Loads have a delay slot (NOP), and the mult and div instructions have their own
// pipeline that runs in parallel to the rest of the CPU (typically mult/div are paired
// with a 13 instruction computation of overflow).
//
struct Instruction 
{
public:
	// ------------------------------------------------------------------------
	// Precached values for the instruction (makes debugging and processing simpler).

	Opcode _Opcode_;	// the instruction opcode
	MipsGPRs_t _Rd_;
	MipsGPRs_t _Rt_;
	MipsGPRs_t _Rs_;
	u32 _Pc_;		// program counter for this specific instruction

protected:
	// ------------------------------------------------------------------------
	// Interpretation status vars, updated post-Process, and accessible via read-only
	// public accessors.

	u32 m_VectorPC;		// new PC after instruction has finished execution.
	u32 m_DivStall;		// indicates the cycle stall of the current instruction if the DIV pipe is already in use.
	const char* m_Name;	// text name/representation for this instruction
	bool m_HasDelaySlot;

public:
	Instruction() {}

	Instruction( const Opcode& opcode ) :
		_Opcode_( opcode )
	,	_Rd_( (MipsGPRs_t)opcode.Rd() )
	,	_Rt_( (MipsGPRs_t)opcode.Rt() )
	,	_Rs_( (MipsGPRs_t)opcode.Rs() )
	,	_Pc_( iopRegs.pc )
	,	m_VectorPC( iopRegs.VectorPC + 4 )
	,	m_DivStall( 0 )
	,	m_Name( NULL )
	,	m_HasDelaySlot( false )
	{
	}
	
	__releaseinline void Assign( const Opcode& opcode )
	{
		_Opcode_	= opcode;
		_Rd_		= (MipsGPRs_t)opcode.Rd();
		_Rt_		= (MipsGPRs_t)opcode.Rt();
		_Rs_		= (MipsGPRs_t)opcode.Rs();
		_Pc_		= iopRegs.pc;
		m_VectorPC	= iopRegs.VectorPC + 4;
		m_DivStall	= 0;
		m_Name		= NULL;
		m_HasDelaySlot = false;
	}

public:
	// ------------------------------------------------------------------------
	// Public Instances Methods and Functions

	// Returns true if this instruction is a branch/jump type (which means it
	// will have a delay slot which should execute prior to the jump but *after*
	// the branch instruction's target has been calculated).
	const bool HasDelaySlot() const { return m_HasDelaySlot; }
	
	const u32  GetVectorPC() const	{ return m_VectorPC; }
	const u32  GetDivStall() const	{ return m_DivStall; }
	const char* GetName() const		{ return m_Name; }

	// ------------------------------------------------------------------------
	// APIs for grabbing portions of the opcodes.  These are just passthrough
	// functions to the Opcode type.

	u32  Funct() const		{ return _Opcode_.Funct(); }
	u32  Basecode() const	{ return _Opcode_.Basecode(); }
	u32  Sa() const			{ return _Opcode_.Sa(); }
	// Sign-extended immediate
	s32  Imm()				{ return _Opcode_.Imm(); }
	// Zero-extended immediate
	u32  ImmU()				{ return _Opcode_.ImmU(); }

	// Returns the target portion of the opcode (26 bit immediate)
	uint Target() const		{ return _Opcode_.Target(); }

	u32  TrapCode() const	{ return _Opcode_.TrapCode(); }

	// Calculates the target for a jump instruction (26 bit immediate added with the upper 4 bits of
	// the current pc address)
	uint JumpTarget() const	{ return (Target()<<2) + ((_Pc_+4) & 0xf0000000); }
	u32  BranchTarget() const{ return (_Pc_+4) + (_Opcode_.Imm() * 4); }
	u32  AddrImm()			{ return GetRs_UL() + Imm(); }

	// ------------------------------------------------------------------------

	__forceinline int GprFromField( RegField_t field ) const 
	{
		switch( field )
		{
			case RF_Rd: return _Rd_;
			case RF_Rt: return _Rt_;
			case RF_Rs: return _Rs_;
			case RF_Hi: return GPR_hi;
			case RF_Lo: return GPR_lo;
			case RF_Link: return GPR_ra;

			jNO_DEFAULT
		}
		return -1;
	}

	// Sets the name of the instruction.
	void SetName( const char* name )
	{
		m_Name = name;
	}

	INSTRUCTION_API()

protected:
	void MultHelper( u64 result );
	bool _OverflowCheck( u64 result );

	void SetLink();
	void SetLinkRd();

	// ------------------------------------------------------------------------
	// Register value retrieval methods.  Use these to read/write the registers
	// specified by the Opcode.  There are two types: the const "RegValue()" brand
	// which do not propagate const/optimization data, and the GetReg() form,
	// which is non-const and propagates optimization flags.

	const IntSign32 RdValue() const { return iopRegs[_Rd_]; }
	const IntSign32 RtValue() const { return iopRegs[_Rt_]; }
	const IntSign32 RsValue() const { return iopRegs[_Rs_]; }
	const IntSign32 HiValue() const { return iopRegs[GPR_hi]; }
	const IntSign32 LoValue() const { return iopRegs[GPR_lo]; }
	const IntSign32 FsValue() const { return iopRegs.CP0.r[_Rd_]; }

	// ------------------------------------------------------------------------
	// Begin Virtual API
	// All succeeding methods are intended for use in the instruction interpretation
	// process, and are overridden in derived classes to collect optimization info.

	// HiLo are always written as unsigned.
	virtual void SetHiLo( u32 hi, u32 lo ) { SetLo_UL( lo ); SetHi_UL( hi ); }

	virtual s32 GetRt_SL() { return iopRegs[_Rt_].SL; }
	virtual s32 GetRs_SL() { return iopRegs[_Rs_].SL; }
	virtual s32 GetHi_SL() { return iopRegs[GPR_hi].SL; }
	virtual s32 GetLo_SL() { return iopRegs[GPR_lo].SL; }
	virtual s32 GetFs_SL() { return iopRegs.CP0.r[_Rd_].SL; }

	virtual u32 GetRt_UL() { return iopRegs[_Rt_].SL; }
	virtual u32 GetRs_UL() { return iopRegs[_Rs_].SL; }
	virtual u32 GetHi_UL() { return iopRegs[GPR_hi].SL; }
	virtual u32 GetLo_UL() { return iopRegs[GPR_lo].SL; }
	virtual u32 GetFs_UL() { return iopRegs.CP0.r[_Rd_].SL; }

	virtual u16 GetRt_US( int idx=0 ) { return iopRegs[_Rt_].US[idx]; }
	virtual u8  GetRt_UB( int idx=0 ) { return iopRegs[_Rt_].UB[idx]; }

	virtual void SetRd_SL( s32 src ) { if(!_Rd_) return; iopRegs[_Rd_].SL = src; }
	virtual void SetRt_SL( s32 src ) { if(!_Rt_) return; iopRegs[_Rt_].SL = src; }
	virtual void SetHi_SL( s32 src ) { iopRegs[GPR_hi].SL = src; }
	virtual void SetLo_SL( s32 src ) { iopRegs[GPR_lo].SL = src; }
	virtual void SetFs_SL( s32 src ) { iopRegs.CP0.r[_Rd_].SL = src; }

	virtual void SetRd_UL( u32 src ) { if(!_Rd_) return; iopRegs[_Rd_].UL = src; }
	virtual void SetRt_UL( u32 src ) { if(!_Rt_) return; iopRegs[_Rt_].UL = src; }
	virtual void SetHi_UL( u32 src ) { iopRegs[GPR_hi].UL = src; }
	virtual void SetLo_UL( u32 src ) { iopRegs[GPR_lo].UL = src; }
	virtual void SetFs_UL( u32 src ) { iopRegs.CP0.r[_Rd_].UL = src; }

	virtual void DoConditionalBranch( bool cond );
	virtual void RaiseException( uint code );
	virtual bool ConditionalException( uint code, bool cond );

	virtual u32 GetSa()		{ return _Opcode_.Sa(); }
	// Sign-extended immediate
	virtual s32 GetImm()	{ return _Opcode_.Imm(); }
	// Zero-extended immediate
	virtual u32 GetImmU()	{ return _Opcode_.ImmU(); }

	virtual u32 GetPC() { return _Pc_; }

	virtual void SetLink( u32 addr ) { iopRegs[GPR_ra].UL = addr; }
	virtual void SetNextPC( u32 addr ) { m_VectorPC = addr; }

	virtual u8  MemoryRead8( u32 addr );
	virtual u16 MemoryRead16( u32 addr );
	virtual u32 MemoryRead32( u32 addr );

	virtual void MemoryWrite8( u32 addr, u8 val );
	virtual void MemoryWrite16( u32 addr, u16 val );
	virtual void MemoryWrite32( u32 addr, u32 val );
	
	// used to flag instructions which have a "critical" side effect elsewhere in emulation-
	// land -- such as modifying COP0 registers, or other functions which cannot be safely
	// optimized.
	virtual void SetSideEffects() {}

	// Flags instructions which can conditionally or unconditionally cause exceptions.
	// Instructions which can cause exceptions should always generate x86 code, regardless
	// of the const status of read and written registers.
	virtual void SetCausesExceptions()	{ }
};


//////////////////////////////////////////////////////////////////////////////////////////
// This version of the instruction interprets the instruction and collects diagnostic
// information along the way (some info is also useful for first pass recompiler optimizations,
// however InstructionOptimizer of the R3000A's recompiler namespace is more complete).
//
class InstructionDiagnostic : public Instruction
{
protected:
	bool m_HasSideEffects:1;
	bool m_CanCauseExceptions:1;
	bool m_SignExtImm:1;
	bool m_SignExtRead:1;		// set TRUE if instruction sign extends on read from GPRs
	bool m_SignExtWrite:1;		// set TRUE if instruction sign extends on write to GPRs
	bool m_ReadsImm:1;
	bool m_ReadsSa:1;

	bool m_ReadsPC:1;			// set true by jump and link / branch and link instructions
	bool m_WritesPC:1;			// set true by all branch instructions and exceptions.

	bool m_IsDelaySlot:1;		// records the status of iopRegs.IsDelaySlot when the instruction is initialized.

	GprStatus m_ReadsGPR;
	GprStatus m_WritesGPR;

public:
	InstructionDiagnostic() {}

	InstructionDiagnostic( const Opcode& opcode ) :
		Instruction( opcode )
	,	m_HasSideEffects( false )
	,	m_CanCauseExceptions( false )
	,	m_SignExtImm( true )
	,	m_SignExtRead( false )
	,	m_SignExtWrite( false )
	,	m_ReadsImm( false )
	,	m_ReadsSa( false )
	,	m_ReadsPC( false )
	,	m_WritesPC( false )
	,	m_IsDelaySlot( iopRegs.IsDelaySlot )
	{
		m_ReadsGPR.Value	= 0;
		m_WritesGPR.Value	= 0;
	}
	
	__releaseinline void Assign( const Opcode& opcode )
	{
		Instruction::Assign( opcode );
		m_HasSideEffects		= false;
		m_CanCauseExceptions	= false;
		m_SignExtImm			= true;
		m_SignExtRead			= false;
		m_SignExtWrite			= false;
		m_ReadsImm				= false;
		m_ReadsSa				= false;

		m_ReadsPC				= false;
		m_WritesPC				= false;

		m_IsDelaySlot = iopRegs.IsDelaySlot;

		m_ReadsGPR.Value	= 0;
		m_WritesGPR.Value	= 0;
	}

public:
	MipsGPRs_t ReadsField( RegField_t field ) const;
	MipsGPRs_t WritesField( RegField_t field ) const;
	RegField_t WritesReg( int gpridx ) const;
	
	bool IsDelaySlot() const		{ return m_IsDelaySlot; }
	bool SignExtendsImm() const		{ return m_SignExtImm; }
	bool SignExtendsReads() const	{ return m_SignExtRead; }
	bool SignExtendResult() const	{ return m_SignExtWrite; }

	const bool IsConstBranch() const
	{
		// Is it even a branching instruction?
		if( !m_HasDelaySlot ) return false;

		if( m_HasSideEffects || m_CanCauseExceptions ) return false;

		// branches that don't read Imm are unconditional jumps (always!)
		if( !m_ReadsImm ) return true;

		// branches which don't read Rt are also unconditional (no comparisons made).
		// Branches with the same reg for both inputs are unconditional as well
		return !m_ReadsGPR.Rt || (_Rs_ == _Rt_);
	}

	bool ReadsRd() const		{ return m_ReadsGPR.Rd; }
	bool ReadsRt() const		{ return m_ReadsGPR.Rt; }
	bool ReadsRs() const		{ return m_ReadsGPR.Rs; }
	bool ReadsHi() const		{ return m_ReadsGPR.Hi; }
	bool ReadsLo() const		{ return m_ReadsGPR.Lo; }
	bool ReadsFs() const		{ return m_ReadsGPR.Fs; }
	bool ReadsMemory() const	{ return m_ReadsGPR.Memory; }

	bool WritesRd() const		{ return m_WritesGPR.Rd; }
	bool WritesRt() const		{ return m_WritesGPR.Rt; }
	bool WritesRs() const		{ return m_WritesGPR.Rs; }
	bool WritesHi() const		{ return m_WritesGPR.Hi; }
	bool WritesLo() const		{ return m_WritesGPR.Lo; }
	bool WritesFs() const		{ return m_WritesGPR.Fs; }
	bool WritesLink() const		{ return m_WritesGPR.Link; }
	bool WritesMemory() const	{ return m_WritesGPR.Memory; }

	bool HasSideEffects() const	{ return m_HasSideEffects; }
	bool CanCauseExceptions() const { return m_CanCauseExceptions; }

protected:
	// ------------------------------------------------------------------------
	// Optimization / Const propagation API.
	// This API is not implemented to any functionality by default, so that the standard
	// interpreter won't have to do more work than is needed.  To enable the extended optimization
	// information, use an InstructionDiagnostic instead.

	virtual u32 GetSa()		{ m_ReadsSa = true; return _Opcode_.Sa(); }
	virtual s32 GetImm()	{ m_ReadsImm = true; m_SignExtImm = false; return _Opcode_.Imm(); }
	virtual u32 GetImmU()	{ m_ReadsImm = true; m_SignExtImm = true;  return _Opcode_.ImmU(); }
	virtual u32 GetPC()		{ m_ReadsPC = true; return _Pc_; }

	virtual s32 GetRt_SL() { m_ReadsGPR.Rt = true; m_SignExtRead = true; return iopRegs[_Rt_].SL; }
	virtual s32 GetRs_SL() { m_ReadsGPR.Rs = true; m_SignExtRead = true; return iopRegs[_Rs_].SL; }
	virtual s32 GetHi_SL() { m_ReadsGPR.Hi = true; m_SignExtRead = true; return iopRegs[GPR_hi].SL; }
	virtual s32 GetLo_SL() { m_ReadsGPR.Lo = true; m_SignExtRead = true; return iopRegs[GPR_lo].SL; }
	virtual s32 GetFs_SL() { m_ReadsGPR.Fs = true; m_SignExtRead = true; return iopRegs.CP0.r[_Rd_].SL; }

	virtual u32 GetRt_UL() { m_ReadsGPR.Rt = true; m_SignExtRead = false; return iopRegs[_Rt_].UL; }
	virtual u32 GetRs_UL() { m_ReadsGPR.Rs = true; m_SignExtRead = false; return iopRegs[_Rs_].UL; }
	virtual u32 GetHi_UL() { m_ReadsGPR.Hi = true; m_SignExtRead = false; return iopRegs[GPR_hi].UL; }
	virtual u32 GetLo_UL() { m_ReadsGPR.Lo = true; m_SignExtRead = false; return iopRegs[GPR_lo].UL; }
	virtual u32 GetFs_UL() { m_ReadsGPR.Fs = true; m_SignExtRead = false; return iopRegs.CP0.r[_Rd_].UL; }

	u16 GetRt_US( int idx=0 ) { m_ReadsGPR.Rt = true; m_SignExtRead = false; return iopRegs[_Rt_].US[idx]; }
	u8  GetRt_UB( int idx=0 ) { m_ReadsGPR.Rt = true; m_SignExtRead = false; return iopRegs[_Rt_].UB[idx]; }

	virtual void SetRd_SL( s32 src ) { if(!_Rd_) return; m_WritesGPR.Rd = true; m_SignExtWrite = true; iopRegs[_Rd_].SL = src; }
	virtual void SetRt_SL( s32 src ) { if(!_Rt_) return; m_WritesGPR.Rt = true; m_SignExtWrite = true; iopRegs[_Rt_].SL = src; }
	virtual void SetHi_SL( s32 src ) { m_WritesGPR.Hi = true; m_SignExtWrite = true; iopRegs[GPR_hi].SL = src; }
	virtual void SetLo_SL( s32 src ) { m_WritesGPR.Lo = true; m_SignExtWrite = true; iopRegs[GPR_lo].SL = src; }
	virtual void SetFs_SL( s32 src ) { m_WritesGPR.Fs = true; m_SignExtWrite = true; iopRegs.CP0.r[_Rd_].SL = src; }
	virtual void SetLink( u32 addr ) { m_WritesGPR.Link = true; m_SignExtWrite = true; iopRegs[GPR_ra].UL = addr; }

	virtual void SetRd_UL( u32 src ) { if(!_Rd_) return; m_WritesGPR.Rd = true; m_SignExtWrite = false; iopRegs[_Rd_].UL = src; }
	virtual void SetRt_UL( u32 src ) { if(!_Rt_) return; m_WritesGPR.Rt = true; m_SignExtWrite = false; iopRegs[_Rt_].UL = src; }
	virtual void SetHi_UL( u32 src ) { m_WritesGPR.Hi = true; m_SignExtWrite = false; iopRegs[GPR_hi].UL = src; }
	virtual void SetLo_UL( u32 src ) { m_WritesGPR.Lo = true; m_SignExtWrite = false; iopRegs[GPR_lo].UL = src; }
	virtual void SetFs_UL( u32 src ) { m_WritesGPR.Fs = true; m_SignExtWrite = false; iopRegs.CP0.r[_Rd_].UL = src; }

	virtual void SetNextPC( u32 addr ) { m_WritesPC = true; m_VectorPC = addr; }

	virtual u8  MemoryRead8( u32 addr );
	virtual u16 MemoryRead16( u32 addr );
	virtual u32 MemoryRead32( u32 addr );

	virtual void MemoryWrite8( u32 addr, u8 val );
	virtual void MemoryWrite16( u32 addr, u16 val );
	virtual void MemoryWrite32( u32 addr, u32 val );

	virtual void SetSideEffects()		{ m_HasSideEffects = true; }
	virtual void SetCausesExceptions()	{ m_CanCauseExceptions = true; }
	virtual bool ConditionalException( uint code, bool cond );

protected:
	// ------------------------------------------------------------------------
	// Diagnostic Functions.

	template< RegField_t field >
	InstParamType AssignFieldParam() const;
	
	bool ParamIsRead( const InstParamType ptype ) const;
	void GetParamLayout( InstParamType iparam[3] ) const;

	void GetParamName( const InstParamType ptype, string& dest ) const;
	void GetParamValue( const InstParamType ptype, string& dest ) const;

public:
	void GetDisasm( string& dest ) const;
	void GetValuesComment( string& dest ) const;
};

//////////////////////////////////////////////////////////////////////////////////////////
// Diagnostic Functions

extern const char* Diag_GetGprName( MipsGPRs_t gpr );

}

extern void PSX_INT( IopEventType evt, int deltaCycles );
extern void iopExecutePendingEvents();
