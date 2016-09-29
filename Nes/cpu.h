#ifndef CPU_H
#define CPU_H

#include <QtCore>
#include "../typedef.h"
#include "../macro.h"

#include "nes.h"
#include "mmu.h"
//#include "ppu.h"
#include "apu.h"
//#include "rom.h"
//#include "mapper.h"

enum CpuStatusFlags{
	CFlag = 0x01, //Carry
	ZFlag = 0x02, //Zero
	IFlag = 0x04, //IrqDisabled
	DFlag = 0x08, //DecimalModeFlag(NES unused)
	BFlag = 0x10, //Break
	RFlag = 0x20, //Reserved (=1)
	VFlag = 0x40, //Overflow
	NFlag = 0x80, //Negative
};
//typedef CpuStatusFlags::CFlag CFlag;
//typedef CpuStatusFlags::ZFlag ZFlag;
//typedef CpuStatusFlags::IFlag IFlag;
//typedef CpuStatusFlags::DFlag DFlag;
//typedef CpuStatusFlags::BFlag BFlag;
//typedef CpuStatusFlags::RFlag RFlag;
//typedef CpuStatusFlags::VFlag VFlag;
//typedef CpuStatusFlags::NFlag NFlag;


enum CpuInterrupt{
	NmiFlag = 0x01,
	IrqFlag = 0x02,

	IrqFrameIrq = 0x04,
	IrqDpcm = 0x08,
	IrqMapper = 0x10,
	IrqMapper2 = 0x20,
	IrqTrigger = 0x40, //One Shot (Old IRQ())
	IrqTrigger2 = 0x80, //One Shot (Old IRQ_NotPending())

	IrqMask = (~(NmiFlag | IrqFlag)),
};

enum CpuVector{
	NmiVector = 0xFFFA,
	ResVector = 0xFFFC,
	IrqVector = 0xFFFE,
};

typedef struct{
	quint16 pc; //Program Counter
	quint8 a; //Cpu Registers
	quint8 p;
	quint8 x;
	quint8 y;
	quint8 s;

	quint8 intPending; //Interrupt pending flag
}CpuReg, R6502;

#define DpcmSyncClock FALSE

// Operation code
//#define Op6502(A) Rd6502((A))
//#define Op6502W(A) Rd6502W((A))

// Zero page read
#define Zprd(A)      (Ram[(uint8)(A)])
#define ZprdW(A)     ((uint16)Ram[(uint8)(A)] + ((uint16)Ram[(uint8)((A) +1)] << 8))
// Zero page write
#define Zpwr(A,V)    { Ram[(uint8)(A)] = (V); }
#define ZpwrW(A,V)   { *((uint16 *) &Ram[(uint8)(A)]) = (uint16)(V); }

// Cycle counter
#define AddCycle(V)  { execCycles += (V); }

// EFFECTIVE ADDRESS page boundary beyond check
#define CheckEa()    { if((Et & 0xFF00) != (Ea & 0xFF00)){ AddCycle(1); }}

// Flag operation
// --> Check the setting of the zero / negative flag
#define SetZnFlag(A) { r.p &= ~(ZFlag | NFlag); r.p |= ZnTable[(uint8)(A)]; }
// --> Flag set
#define SetFlag(V) { r.p |= (V); }
// --> Flag clear
#define ClrFlag(V) { r.p &= ~(V); }
// --> Flag test and set / clear
#define TstFlag(F,V) { r.p &= ~(V); if((F)){ r.p |= ((V)); } }
// --> Flag check
#define ChkFlag(V) (r.p & (V))

// WT ---> Word Temp
// EA ---> Effective Address
// ET ---> Effective Address Temp
// DT ---> Data
#define MrIm() { dt = op6502(r.pc++); }
#define MrZp() { ea = op6502(r.pc++); dt = Zprd(ea); }
#define MrZx() { dt = op6502(r.pc++); ea = (uint8)(dt + r.x); dt = Zprd(ea); }
#define MrZy() { dt = op6502(r.pc++); ea = (uint8)(dt + r.y); dt = Zprd(ea); }
#define MrAb() { ea = op6502W(r.pc); r.pc += 2; dt = rd6502(ea); }
#define MrAx() { et = op6502W(r.pc); r.pc += 2; ea = et + r.x; dt = rd6502(ea); }
#define MrAy() { et = op6502W(r.pc); r.pc += 2; ea = et + r.y; dt = rd6502(ea); }
#define MrIx() { st = op6502(r.pc++); ea = ZprdW(dt + r.x); dt = rd6502(ea); }
#define MrIy() { dt = op6502(r.pc++); et = ZprdW(dt); ea = et + r.y; dt = rd6502(ea); }
// EFFECTIVE ADDRESS
#define EaZp() { ea = op6502(r.pc++); }
#define EaZx() { dt = op6502(r.pc++); ea = (uint8)(dt + r.x); }
#define EaZy() { dt = op6502(r.pc++); ea = (uint8)(dt + r.y); }
#define EaAb() { ea = op6502W(r.pc); r.pc += 2; }
#define EaAx() { et = op6502W(r.pc); r.pc += 2; ea = et + r.x; }
#define EaAy() { et = op6502W(r.pc); r.pc += 2; ea = et + r.y; }
#define EaIx() { dt = op6502(r.pc++); ea = ZprdW(dt + r.x); }
#define EaIy() { dt = op6502(r.pc++); et = ZprdW(dt); ea = et + (uint16)r.y; }
// Memory write
#define MwZp() Zpwr(ea, dt)
#define MwEa() wr6502(ea, dt)
// Stack operations
// PUSH
#define Push(V) { stack[(r.s--) & 0xFF] = (V); }
// POP
#define Pop()   stack[(++r.s) & 0xFF]

// Pop & SetZnFlag
#define PopAndSetZnFlag(a) { a = Pop(); SetZnFlag(a); }

// Arithmetic operations
// Flags(NVRBDIZC)
// ADC  (NV----ZC)
#define Adc() { wt = r.a + dt + (r.p & CFlag); TstFlag(wt > 0xFF, CFlag); TstFlag(((~(r.a ^ dt)) & (r.a ^ wt) & 0x80), VFlag); r.a = (uint8)wt; SetZnFlag(r.a); }
// SBC  (NV----ZC)
#define Sbc() { wt = r.a - dt - (~r.p & CFlag); TstFlag(((r.a ^ dt) & (r.a ^ wt) & 0x80), VFlag); TstFlag(WT < 0x100, CFlag); r.a = (uint8)wt; SetZnFlag(r.a); }
// INC  (N-----Z-)
#define Inc() { dt++; SetZnFlag(dt); }
// INX  (N-----Z-)
#define Inx() { r.x++; SetZnFlag(r.x); }
// INY  (N-----Z-)
#define Iny() { r.y++; SetZnFlag(r.y); }
// DEC  (N-----Z-)
#define Dec() { dt--; SetZnFlag(dt); }
// DEX  (N-----Z-)
#define Dex() { r.x--; SetZnFlag(r.x); }
// DEY  (N-----Z-)
#define Dey() { r.y--; SetZnFlag(r.y); }
// Logical operations
// AND  (N-----Z-)
#define And() { r.a &= dt; SetZnFlag(r.a); }
// ORA  (N-----Z-)
#define Ora() { r.a |= dt; SetZnFlag(r.a); }
// EOR  (N-----Z-)
#define Eor() { r.a ^= dt; SetZnFlag(r.a); }
// ASLA (N-----ZC)
#define AslA() { TstFlag(r.a & 0x80, CFlag); r.a <<= 1; SetZnFlag(r.a); }
// ASL  (N-----ZC)
#define Asl() { TstFlag(r.a & 0x80, CFlag); dt <<= 1; SetZnFlag(dt); }
// LSRA (N-----ZC)
#define LsrA() { TstFlag(r.a & 0x01, CFlag); r.a >>= 1; SetZnFlag(r.a); }
// LSR  (N-----ZC)
#define Lsr() { TstFlag(dt & 0x01, CFlag); dt >>= 1; SetZnFlag(dt); }
// ROLA (N-----ZC)
#define RolA() { if(r.p & CFlag){ TstFlag(r.a & 0x80, CFlag); r.a = (r.a << 1) | 0x01; }else{ TstFlag(r.a & 0x80, CFlag) } SetZnFlag(r.a); }
// ROL  (N-----ZC)
#define Rol() { if(r.p & CFlag){ TstFlag(dt & 0x80, CFlag); dt = (st << 1) | 0x01; }else{ TstFlag(dt & 0x80, CFlag); dt <<= 1; } SetZnFlag(dt); }
// RORA (N-----ZC)
#define RorA() { if(r.p & CFlag){ TstFlag(r.a & 0x01, CFlag); r.a = (r.a >> 1) | 0x80; }else{ TstFlag(r.a & 0x01, CFlag); r.a >>= 1; } SetZnFlag(r.a); }
// ROR  (N-----ZC)
#define Ror() { if(r.p & CFlag){ TstFlag(dt & 0x01, CFlag); dt = (dt >> 1) | 0x80; }else{ TstFlag(dt & 0x01, CFlag); dt >>= 1; } SetZnFlag(dt); }
// BIT  (NV----Z-)
#define Bit() { TstFlag((dt & r.a) == 0, ZFlag); TstFlag(dt & 0x80, NFlag); TstFlag(dt & 0x40, VFlag); }
// Load / store operations
// LDA  (N-----Z-)
#define Lda() { r.a = dt; SetZnFlag(r.a); }
// LDX  (N-----Z-)
#define Ldx() { r.x = dt; SetZnFlag(r.x); }
// LDY  (N-----Z-)
#define Ldy() { r.y = dt; SetZnFlag(r.y); }
// STA  (--------)
#define Sta() { dt = r.a; }
// STX  (--------)
#define Stx() { dt = r.x; }
// STY  (--------)
#define Sty() { dt = r.y; }
// TAX  (N-----Z-)
#define Tax() { r.x = r.a; SetZnFlag(r.x); }
// TXA  (N-----Z-)
#define Txa() { r.a = r.x; SetZnFlag(r.a); }
// TAY  (N-----Z-)
#define Tay() { r.y = r.a; SetZnFlag(r.y); }
// TYA  (N-----Z-)
#define Tya() { r.a = r.y; SetZnFlag(r.a); }
// TSX  (N-----Z-)
#define Tsx() { r.x = r.s; SetZnFlag(r.x); }
// TXS  (--------)
#define Txs() { r.s = r.x; }
// Compare operations
// CMP  (N-----ZC)
#define Cmp() { wt = (uint16)r.a - (uint16)dt; TstFlag((wt & 0x8000) == 0, CFlag); SetZnFlag((uint8)wt); }
// CPX  (N-----ZC)
#define Cpx() { wt = (uint16)r.x - (uint16)dt; TstFlag((wt & 0x8000) == 0, CFlag); SetZnFlag((uint8)wt); }
// CPY  (N-----ZC)
#define Cpy() { wt = (uint16)r.y - (uint16)dt; TstFlag((wt & 0x8000) == 0, CFlag); SetZnFlag((uint8)wt); }
// Jump / return operations
// JMP_ID
#define JmpId() { wt = op6502W(r.pc); ea = rd6502(wt); wt = (wt & 0xFF00) | ((wt +1) & 0x00FF); r.pc = ea + rd6502(wt) * 0x100; }
// JMP
#define Jmp() { r.pc = op6502W(r.pc); }
// JSR
#define Jsr() { ea = op6502W(r.pc); r.pc++; Push(r.pc >> 8); Push(r.pc & 0xFF); r.pc = ea; }
// RTS
#define Rts() { r.pc = Pop(); r.pc |= Pop() * 0x0100; r.pc++; }
// RTI
#define Rti() { r.p = Pop() | RFlag; r.pc = Pop(); r.pc |= Pop() * 0x0100; }
// _NMI
#define _Nmi() { Push(r.pc >> 8); Push(r.pc & 0xFF); ClrFlag(BFlag); Push(r.p); SetFlag(IFlag); r.pc = rd6502W(NmiVector); execCycles += 7; }
// _IRQ
#define _Irq() { Push(r.pc >> 8); Push(r.pc & 0xFF); ClrFlag(BFlag); Push(r.p); SetFlag(IFlag); r.pc = rd6502W(IrqVector); execCycles += 7; }
// BRK
#define Brk() { r.pc++; Push(r.pc >> 8); Push(r.pc & 0xFF); SetFlag(BFlag); Push(r.p); SetFlag(IFlag); r.pc = rd6502W(IrqVector); }
// REL_JUMP
#define RelJump() { et = r.pc; ea = r.pc + (int8)dt; r.pc = ea; AddCycle(1); CheckEa(); }
// BCC
#define Bcc() { if(!(r.p & CFlag)){ RelJump(); } }
// BCS
#define Bcs() { if( (r.p & CFlag)){ RelJump(); } }
// BNE
#define Bne() { if(!(r.p & ZFlag)){ RelJump(); } }
// BEQ
#define Beq() { if( (r.p & ZFlag)){ RelJump(); } }
// BPL
#define Bpl() { if(!(r.p & NFlag)){ RelJump(); } }
// BMI
#define Bmi() { if( (r.p & NFlag)){ RelJump(); } }
// BVC
#define Bvc() { if(!(r.p & VFlag)){ RelJump(); } }
// BVS
#define Bvs() { if( (r.p & VFlag)){ RelJump(); } }
// Flag control operations
// CLC
#define Clc() { r.p &= ~CFlag; }
// CLD
#define Cld() { r.p &= ~DFlag; }
// CLI
#define Cli() { r.p &= ~IFlag; }
// CLV
#define Clv() { r.p &= ~VFlag; }
// SEC
#define Sec() { r.p |= CFlag; }
// SED
#define Sed() { r.p |= DFlag; }
// SEI
#define Sei() { r.p |= IFlag; }
// Unofficial operations
// ANC
#define Anc() { r.a &= dt; SetZnFlag(r.a); TstFlag(r.p & NFlag, CFlag); }
// ANE
#define Ane() { r.a = (r.a | 0xEE) & r.x & dt; SetZnFlag(r.a); }
// ARR
#define Arr() { dt &= r.a; r.a = (dt >> 1) | ((r.p & CFlag) << 7); SetZnFlag(r.a); TstFlag(r.a & 0x40, CFlag); TstFlag((r.a >> 6) ^ (r.a >> 5), VFlag); }
// ASR
#define Asr() { dt &= r.a; TstFlag(dt & 0x01, CFlag); r.a = dt >> 1; SetZnFlag(r.a); }
// DCP
#define Dcp() { dt--; Cmp(); }
// DOP
#define Dop() { r.pc++; }
// ISB
#define Isb() { dt++; Sbc(); }
// LAS
#define Las() { r.a = r.x = r.s = (r.s & dt); SetZnFlag(r.a); }
// LAX
#define Lax() { r.a = dt; r.x = r.a; SetZnFlag(r.a); }
// LXA
#define Lxa() { r.a = r.x = ((r.a | 0xEE) & dt); SetZnFlag(r.a); }
// RLA
#define Rla() { if(r.p & CFlag){ TstFlag(dt & 0x80, CFlag); dt = (dt << 1) | 0x01; }else{ TstFlag(dt & 0x80, CFlag); dt <<= 1; } r.a &= dt; SetZnFlag(r.a); }
// RRA
#define Rra() { if(r.p & CFlag){ TstFlag(dt & 0x01, CFlag); dt = (dt >> 1) | 0x80; }else{ TstFlag(dt & 0x01, CFlag); dt >>= 1; } Adc(); }
// SAX
#define Sax() { dt = r.a & r.x; }
// SBX
#define Sbx() { wt = (r.a & r.x) - dt; TstFlag(wt < 0x100, CFlag); r.x = wt & 0xFF; SetZnFlag(r.x); }
// SHA
#define Sha() { dt = r.a & r.x & (uint8)((ea >> 8) +1); }
// SHS
#define Shs() { r.s = r.a & r.x; dt = r.s & (uint8)((ea >> 8) +1); }
// SHX
#define Shx() { dt = r.x & (uint8)((ea >> 8) +1); }
// SHY
#define Shy() { dt = r.y & (uint8)((ea >> 8) +1); }
// SLO
#define Slo() { TstFlag(dt & 0x80, CFlag); dt <<= 1; r.a |= dt; SetZnFlag(r.a); }
// SRE
#define Sre() { TstFlag(dt & 0x01, CFlag); dt >>= 1; r.a ^= dt; SetZnFlag(r.a); }
// TOP
#define Top() { r.pc += 2; }

class Cpu : public QObject
{
	Q_OBJECT
public:
	explicit Cpu(Nes *parent, QObject *p = 0);
	virtual ~Cpu();

	quint8 readCpu(quint16 addr);
	void writeCpu(quint16 addr, quint8 data);
	quint16 readWordCpu(quint16 addr);

	void reset();

	static int nmicount;
	void nmi();
	void setIrq(quint8 mask);
	void clearIrq(quint8 mask);

	void dma(qint32 cycles);

	qint32 exec(qint32 requestCycles);

	qint32 getDmaCycles();
	void setDmaCycles(qint32 cycles);

	qint32 getTotalCycles();
	void setTotalCycles(qint32 cycles);

	void setContext(CpuReg r);
	void getContext(CpuReg &r);

	void setClockProcess(bool enable);
protected:
	Nes *nes;
	Apu *apu;
	Mapper *mapper;

	CpuReg r;

	qint32 totalCycles;
	qint32 dmaCycles;

	Ptr quint8 stack;

	quint8 znTable[256];

	bool clockProcess;

private:
	uint16 ea;
	uint16 et;
	uint16 wt;
	uint8  dt;

	inline static uint8 op6502(uint16 addr){
		return cpuMemBank[addr >> 13][addr & 0x1FFF];
	}

	inline static uint16 op6502W(uint16 addr){
#if 0
		uint16 ret;
		ret = (uint16)cpuMemBank[(addr) >> 13][(addr) & 0x1FFF];
		ret |= (uint16)cpuMemBank[(addr +1) >> 13][(addr +1) & 0x1FFF] << 8;
		return ret;
#else
		return *((uint16*) &cpuMemBank[addr >> 13][addr & 0x1FFF]);
#endif
	}

	inline static uint8 rd6502(uint16 addr){
		if(addr < 0x2000){
			// RAM (Mirror $0800, $1000, $1800)
			return ram[addr & 0x07FF];
		}else if(addr < 0x8000){
			// Others
			return this->nes->read(addr);
		}else{
			// Dummy access
			this->mapper->read(addr, cpuMemBank[addr >> 13][addr & 0x1FFF]);
		}
		// Quick bank read
		return cpuMemBank[addr >> 13][addr & 0x1FFF];
	}

	inline static uint16 rd6502W(uint16 addr){
		if(addr < 0x2000){
			// RAM (Mirror $0800, $1000, $1800)
			return *((uint16*) &ram[addr & 0x07FF]);
		}else if(addr < 0x8000){
			// Others
			return (uint16)this->nes->read(addr) + (uint16)this->nes->read(addr +1) * 0x100;
		}
		// Quick bank read
#if 0
		uint16 ret;
		ret = (uint16)cpuMemBank[(addr) >> 13][(addr) & 0x1FFF];
		ret |= (uint16)cpuMemBank[(addr +1) >> 13][(addr +1) & 0x1FFF] << 8;
		return ret;
#else
		return *((uint16*) &cpuMemBank[addr >> 13][addr & 0x1FFF]);
#endif
	}

	inline void BRKREL(){         Brk();            AddCycle(7); } // 0x00
	inline void ORAIZX(){ MrIx(); Ora();            AddCycle(6); } // 0x01
	inline void SLOIZX(){ MrIx(); Slo(); MwEa();    AddCycle(8); } // 0x03
	inline void ORAZP_(){ MrZp(); Ora();            AddCycle(3); } // 0x05
	inline void ASLZP_(){ MrZp(); Slo(); MwZp();    AddCycle(5); } // 0x06
	inline void SLOZP_(){ MrZp(); Slo(); MwZp();    AddCycle(5); } // 0x07
	inline void PHPREL(){ Push(r.p | BFlag);        AddCycle(3); } // 0x08
	inline void ORAIMM(){ MrIm(); Ora();            AddCycle(2); } // 0x09
	inline void ASLAAA(){         AslA();           AddCycle(2); } // 0x0A
	inline void ANCIMM(){ MrIm(); Anc();            AddCycle(2); } // 0x0B, 0x2B
	inline void ORAABS(){ MrAb(); Ora();            AddCycle(4); } // 0x0D
	inline void ASLABS(){ MrAb(); Asl(); MwEa();    AddCycle(6); } // 0x0E
	inline void SLOABS(){ MrAb(); Slo(); MwEa();    AddCycle(6); } // 0x0F
	inline void BPLREL(){ MrIm(); Bpl();            AddCycle(2); } // 0x10
	inline void ORAIZY(){ MrIy(); Ora(); CheckEa(); AddCycle(5); } // 0x11
	inline void SLOIZY(){ MrIy(); Slo(); MwEa();    AddCycle(8); } // 0x13
	inline void ORAZPX(){ MrZx(); Ora();            AddCycle(3); } // 0x15
	inline void ASLZPX(){ MrZx(); Asl(); MwZp();    AddCycle(6); } // 0x16
	inline void SLOZPX(){ MrZx(); Slo(); MwZp();    AddCycle(5); } // 0x17
	inline void CLCREL(){         Clc();            AddCycle(2); } // 0x18
	inline void ORAABY(){ MrAy(); Ora(); CheckEa(); AddCycle(4); } // 0x19
	inline void SLOABY(){ MrAy(); Slo(); MwEa();    AddCycle(7); } // 0x1B
	inline void ORAABX(){ MrAx(); Ora(); CheckEa(); AddCycle(4); } // 0x1D
	inline void ASLABX(){ MrAx(); Asl(); MwEa();    AddCycle(7); } // 0x1E
	inline void SLOABX(){ MrAx(); Slo(); MwEa();    AddCycle(7); } // 0x1F
	inline void JSRREL(){         Jsr();            AddCycle(6); } // 0x20
	inline void ANDIZX(){ MrIx(); And();            AddCycle(6); } // 0x21
	inline void RLAIZX(){ MrIx(); Rla(); MwEa();    AddCycle(8); } // 0x23
	inline void BITZP_(){ MrZp(); Bit();            AddCycle(3); } // 0x24
	inline void ANDZP_(){ MrZp(); And();            AddCycle(3); } // 0x25
	inline void ROLZP_(){ MrZp(); Rol(); MwZp();    AddCycle(5); } // 0x26
	inline void RLAZP_(){ MrZp(); Rla(); MwZp();    AddCycle(5); } // 0x27
	inline void PLPREL(){ r.p = Pop() | RFlag;      AddCycle(4); } // 0x28
	inline void ANDIMM(){ MrIm(); And();            AddCycle(2); } // 0x29
	inline void ROLAAA(){         RolA();           AddCycle(2); } // 0x2A
	inline void BITABS(){ MrAb(); Bit();            AddCycle(4); } // 0x2C
	inline void ANDABS(){ MrAb(); And();            AddCycle(4); } // 0x2D
	inline void ROLABS(){ MrAb(); Rol(); MwEa();    AddCycle(6); } // 0x2E
	inline void RLAABS(){ MrAb(); Rla(); MwEa();    AddCycle(6); } // 0x2F
	inline void BMIREL(){ MrIm(); Bmi();            AddCycle(2); } // 0x30
	inline void ANDIZY(){ MrIy(); And(); CheckEa(); AddCycle(5); } // 0x31
	inline void RLAIZY(){ MrIy(); Rla(); MwEa();    AddCycle(8); } // 0x33
	inline void ANDZPX(){ MrZx(); And();            AddCycle(4); } // 0x35
	inline void ROLZPX(){ MrZx(); Rol(); MwZp();    AddCycle(6); } // 0x36
	inline void RLAZPX(){ MrZx(); Rla(); MwZp();    AddCycle(6); } // 0x37
	inline void SECREL(){         Sec();            AddCycle(2); } // 0x38
	inline void ANDABY(){ MrAy(); And(); CheckEa(); AddCycle(4); } // 0x39
	inline void RLAABY(){ MrAy(); Rla(); MwEa();    AddCycle(7); } // 0x3B
	inline void ANDABX(){ MrAx(); And(); CheckEa(); AddCycle(4); } // 0x3D
	inline void ROLABX(){ MrAb(); Rol(); MwEa();    AddCycle(7); } // 0x3E
	inline void RLAABX(){ MrAx(); Rla(); MwEa();    AddCycle(7); } // 0x3F
	inline void RTIREL(){         Rti();            AddCycle(6); } // 0x40
	inline void EORIZX(){ MrIx(); Eor();            AddCycle(6); } // 0x41
	inline void SREIZX(){ MrIx(); Sre(); MwEa();    AddCycle(8); } // 0x43
	inline void EORZP_(){ MrZp(); Eor();            AddCycle(3); } // 0x45
	inline void LSRZP_(){ MrZp(); Lsr(); MwZp();    AddCycle(5); } // 0x46
	inline void SREZP_(){ MrZp(); Sre(); MwZp();    AddCycle(5); } // 0x47
	inline void PHAREL(){ Push(r.a);                AddCycle(3); } // 0x48
	inline void EORIMM(){ MrIm(); Eor();            AddCycle(2); } // 0x49
	inline void LSRAAA(){         LsrA();           AddCycle(2); } // 0x4A
	inline void ASRIMM(){ MrIm(); Asr();            AddCycle(2); } // 0x4B
	inline void JMPABS(){         Jmp();            AddCycle(3); } // 0x4C
	inline void EORABS(){ MrAb(); Eor();            AddCycle(4); } // 0x4D
	inline void LSRABS(){ MrAb(); Lsr(); MwEa();    AddCycle(6); } // 0x4E
	inline void SREABS(){ MrAb(); Sre(); MwEa();    AddCycle(6); } // 0x4F
	inline void BVCREL(){ MrIm(); Bvc();            AddCycle(2); } // 0x50
	inline void EORIZY(){ MrIy(); Eor(); CheckEa(); AddCycle(5); } // 0x51
	inline void SREIZY(){ MrIy(); Sre(); MwEa();    AddCycle(8); } // 0x53
	inline void EORZPX(){ MrZx(); Eor();            AddCycle(4); } // 0x55
	inline void LSRZPX(){ MrZx(); Lsr(); MwZp();    AddCycle(6); } // 0x56
	inline void SREZPX(){ MrZx(); Sre(); MwZp();    AddCycle(6); } // 0x57
	inline void CLIREL(){         Cli();            AddCycle(2); } // 0x58
	inline void EORABY(){ MrAy(); Eor(); CheckEa(); AddCycle(4); } // 0x59
	inline void SREABY(){ MrAy(); Sre(); MwEa();    AddCycle(7); } // 0x5B
	inline void EORABX(){ MrAx(); Eor(); CheckEa(); AddCycle(4); } // 0x5D
	inline void LSRABX(){ MrAx(); Lsr(); MwEa();    AddCycle(7); } // 0x5E
	inline void SREABX(){ MrAx(); Sre(); MwEa();    AddCycle(7); } // 0x5F
	inline void RTSREL(){         Rts();            AddCycle(6); } // 0x60
	inline void ADCIZX(){ MrIx(); Adc();            AddCycle(6); } // 0x61
	inline void RRAIZX(){ MrIx(); Rra(); MwEa();    AddCycle(8); } // 0x63
	inline void ADCZP_(){ MrZp(); Adc();            AddCycle(3); } // 0x65
	inline void RORZP_(){ MrZp(); Ror(); MwZp();    AddCycle(5); } // 0x66
	inline void RRAZP_(){ MrZp(); Rra(); MwZp();    AddCycle(5); } // 0x67
	inline void PLAREL(){ PopAndSetZnFlag(r.a);     AddCycle(4); } // 0x68
	inline void ADCIMM(){ MrIm(); Adc();            AddCycle(2); } // 0x69
	inline void RORAAA(){         RorA();           AddCycle(2); } // 0x6A
	inline void ARRIMM(){ MrIm(); Arr();            AddCycle(2); } // 0x6B
	inline void JMPIZA(){         JmpId();          AddCycle(5); } // 0x6C
	inline void ADCABS(){ MrAb(); Adc();            AddCycle(4); } // 0x6D
	inline void RORABS(){ MrAb(); Ror(); MwEa();    AddCycle(6); } // 0x6E
	inline void RRAABS(){ MrAb(); Rra(); MwEa();    AddCycle(6); } // 0x6F
	inline void BVSREL(){ MrIm(); Bvs();            AddCycle(2); } // 0x70
	inline void ADCIZY(){ MrIy(); Adc(); CheckEa(); AddCycle(4); } // 0x71
	inline void RRAIZY(){ MrIy(); Rra(); MwEa();    AddCycle(8); } // 0x73
	inline void ADCZPX(){ MrZx(); Adc();            AddCycle(4); } // 0x75
	inline void RORZPX(){ MrZx(); Ror(); MwEa();    AddCycle(6); } // 0x76
	inline void RRAZPX(){ MrZx(); Rra(); MwZp();    AddCycle(6); } // 0x77
	inline void SEIREL(){         Sei();            AddCycle(2); } // 0x78
	inline void ADCABY(){ MrAy(); Adc(); CheckEa(); AddCycle(4); } // 0x79
	inline void RRAABY(){ MrAy(); Rra(); MwEa();    AddCycle(7); } // 0x7B
	inline void ADCABX(){ MrAx(); Adc(); CheckEa(); AddCycle(4); } // 0x7D
	inline void RORABX(){ MrAx(); Ror(); MwEa();    AddCycle(7); } // 0x7E
	inline void RRAABX(){ MrAx(); Rra(); MwEa();    AddCycle(7); } // 0x7F
	inline void STAIZX(){ EaIx(); Sta(); MwEa();    AddCycle(6); } // 0x81
	inline void SAXIZX(){ MrIx(); Sax(); MwEa();    AddCycle(6); } // 0x83
	inline void STYZP_(){ EaZp(); Sty(); MwZp();    AddCycle(3); } // 0x84
	inline void STAZP_(){ EaZp(); Sta(); MwZp();    AddCycle(3); } // 0x85
	inline void STXZP_(){ EaZp(); Stx(); MwZp();    AddCycle(3); } // 0x86
	inline void SAXZP_(){ MrZp(); Sax(); MwZp();    AddCycle(3); } // 0x87
	inline void DEYREL(){         Dey();            AddCycle(2); } // 0x88
	inline void TXAREL(){         Txa();            AddCycle(2); } // 0x8A
	inline void ANEIMM(){ MrIm(); Ane();            AddCycle(2); } // 0x8B
	inline void STYABS(){ EaAb(); Sty(); MwEa();    AddCycle(4); } // 0x8C
	inline void STAABS(){ EaAb(); Sta(); MwEa();    AddCycle(4); } // 0x8D
	inline void STXABS(){ EaAb(); Stx(); MwEa();    AddCycle(4); } // 0x8E
	inline void SAXABS(){ MrAb(); Sax(); MwEa();    AddCycle(4); } // 0x8F
	inline void BCCREL(){ MrIm(); Bcc();            AddCycle(2); } // 0x90
	inline void STAIZY(){ EaIy(); Sta(); MwEa();    AddCycle(6); } // 0x91
	inline void SHAIZY(){ MrIy(); Sha(); MwEa();    AddCycle(6); } // 0x93
	inline void STYZPX(){ EaZx(); Sty(); MwZp();    AddCycle(4); } // 0x94
	inline void STAZPX(){ EaZx(); Sta(); MwZp();    AddCycle(4); } // 0x95
	inline void STXZPY(){ EaZy(); Stx(); MwZp();    AddCycle(4); } // 0x96
	inline void SAXZPY(){ MrZy(); Sax(); MwZp();    AddCycle(4); } // 0x97
	inline void TYAREL(){         Tya();            AddCycle(2); } // 0x98
	inline void STAABY(){ EaAy(); Sta(); MwEa();    AddCycle(5); } // 0x99
	inline void TXSREL(){         Txs();            AddCycle(2); } // 0x9A
	inline void SHSABY(){ MrAy(); Shs(); MwEa();    AddCycle(5); } // 0x9B
	inline void SHYABX(){ MrAx(); Shy(); MwEa();    AddCycle(5); } // 0x9C
	inline void STAABX(){ EaAx(); Sta(); MwEa();    AddCycle(5); } // 0x9D
	inline void SHXABY(){ MrAy(); Shx(); MwEa();    AddCycle(5); } // 0x9E
	inline void SHAABY(){ MrAy(); Sha(); MwEa();    AddCycle(5); } // 0x9F
	inline void LDYIMM(){ MrIm(); Ldy();            AddCycle(2); } // 0xA0
	inline void LDAIZX(){ MrIx(); Lda();            AddCycle(6); } // 0xA1
	inline void LDXIMM(){ MrIm(); Ldx();            AddCycle(2); } // 0xA2
	inline void LAXIZX(){ MrIx(); Lax();            AddCycle(6); } // 0xA3
	inline void LDYZP_(){ MrZp(); Ldy();            AddCycle(3); } // 0xA4
	inline void LDAZP_(){ MrZp(); Lda();            AddCycle(3); } // 0xA5
	inline void LDXZP_(){ MrZp(); Ldx();            AddCycle(3); } // 0xA6
	inline void LAXZP_(){ MrZp(); Lax();            AddCycle(3); } // 0xA7
	inline void TAYREL(){         Tay();            AddCycle(2); } // 0xA8
	inline void LDAIMM(){ MrIm(); Lda();            AddCycle(2); } // 0xA9
	inline void TAXREL(){         Tax();            AddCycle(2); } // 0xAA
	inline void LXAIMM(){ MrIm(); Lxa();            AddCycle(2); } // 0xAB
	inline void LDYABS(){ MrAb(); Ldy();            AddCycle(4); } // 0xAC
	inline void LDAABS(){ MrAb(); Lda();            AddCycle(4); } // 0xAD
	inline void LDXABS(){ MrAb(); Ldx();            AddCycle(4); } // 0xAE
	inline void LAXABS(){ MrAb(); Lax();            AddCycle(4); } // 0xAF
	inline void BCSREL(){ MrIm(); Bcs();            AddCycle(2); } // 0xB0
	inline void LDAIZY(){ MrIy(); Lda(); CheckEa(); AddCycle(5); } // 0xB1
	inline void LAXIZY(){ MrIy(); Lax(); CheckEa(); AddCycle(5); } // 0xB3
	inline void LDYZPX(){ MrZx(); Ldy();            AddCycle(4); } // 0xB4
	inline void LDAZPX(){ MrZx(); Lda();            AddCycle(4); } // 0xB5
	inline void LDXZPY(){ MrZy(); Ldx();            AddCycle(4); } // 0xB6
	inline void LAXZPY(){ MrZy(); Lax();            AddCycle(4); } // 0xB7
	inline void CLVREL(){         Clv();            AddCycle(2); } // 0xB8
	inline void LDAABY(){ MrAy(); Lda(); CheckEa(); AddCycle(4); } // 0xB9
	inline void TSXREL(){         Tsx();            AddCycle(2); } // 0xBA
	inline void LASABY(){ MrAy(); Las(); CheckEa(); AddCycle(4); } // 0xBB
	inline void LDYABX(){ MrAx(); Ldy(); CheckEa(); AddCycle(4); } // 0xBC
	inline void LDAABX(){ MrAx(); Lda(); CheckEa(); AddCycle(4); } // 0xBD
	inline void LDXABY(){ MrAy(); Ldx(); CheckEa(); AddCycle(4); } // 0xBE
	inline void LAXABY(){ MrAy(); Lax(); CheckEa(); AddCycle(4); } // 0xBF
	inline void CPYIMM(){ MrIm(); Cpy();            AddCycle(2); } // 0xC0
	inline void CMPIZX(){ MrIx(); Cmp(); CheckEa(); AddCycle(5); } // 0xC1
	inline void DCPIZX(){ MrIx(); Dcp(); MwEa();    AddCycle(8); } // 0xC3
	inline void CPYZP_(){ MrZp(); Cpy();            AddCycle(3); } // 0xC4
	inline void CMPZP_(){ MrZp(); Cmp();            AddCycle(3); } // 0xC5
	inline void DECZP_(){ MrZp(); Dec(); MwZp();    AddCycle(5); } // 0xC6
	inline void DCPZP_(){ MrZp(); Dcp(); MwZp();    AddCycle(5); } // 0xC7
	inline void INYREL(){         Iny();            AddCycle(2); } // 0xC8
	inline void CMPIMM(){ MrIm(); Cmp();            AddCycle(2); } // 0xC9
	inline void DEXREL(){         Dex();            AddCycle(2); } // 0xCA
	inline void SBXIMM(){ MrIm(); Sbx();            AddCycle(2); } // 0xCB
	inline void CPYABS(){ MrAb(); Cpy();            AddCycle(4); } // 0xCC
	inline void CMPABS(){ MrAb(); Cmp();            AddCycle(4); } // 0xCD
	inline void DECABS(){ MrAb(); Dec(); MwEa();    AddCycle(6); } // 0xCE
	inline void DCPABS(){ MrAb(); Dcp(); MwEa();    AddCycle(6); } // 0xCF
	inline void BNEREL(){ MrIm(); Bne();            AddCycle(2); } // 0xD0
	inline void CMPIZY(){ MrIy(); Cmp(); CheckEa(); AddCycle(5); } // 0xD1
	inline void DCPIZY(){ MrIy(); Dcp(); MwEa();    AddCycle(8); } // 0xD3
	inline void CMPZPX(){ MrZx(); Cmp();            AddCycle(4); } // 0xD5
	inline void DECZPX(){ MrZx(); Dec(); MwZp();    AddCycle(6); } // 0xD6
	inline void DCPZPX(){ MrZx(); Dcp(); MwZp();    AddCycle(6); } // 0xD7
	inline void CLDREL(){         Cld();            AddCycle(2); } // 0xD8
	inline void CMPABY(){ MrAy(); Cmp(); CheckEa(); AddCycle(4); } // 0xD9
	inline void DCPABY(){ MrAy(); Dcp(); MwEa();    AddCycle(7); } // 0xDB
	inline void CMPABX(){ MrAx(); Cmp(); CheckEa(); AddCycle(4); } // 0xDD
	inline void DECABX(){ MrAx(); Dec(); MwEa();    AddCycle(7); } // 0xDE
	inline void DCPABX(){ MrAx(); Dcp(); MwEa();    AddCycle(7); } // 0xDF
	inline void CPXIMM(){ MrIm(); Cpx();            AddCycle(2); } // 0xE0
	inline void SBCIZX(){ MrIx(); Sbc();            AddCycle(6); } // 0xE1
	inline void ISBIZX(){ MrIx(); Isb(); MwEa();    AddCycle(5); } // 0xE3
	inline void CPXZP_(){ MrZp(); Cpx();            AddCycle(3); } // 0xE4
	inline void SBCZP_(){ MrZp(); Sbc();            AddCycle(3); } // 0xE5
	inline void INCZP_(){ MrZp(); Inc(); MwZp();    AddCycle(5); } // 0xE6
	inline void ISBZP_(){ MrZp(); Isb(); MwZp();    AddCycle(5); } // 0xE7
	inline void INXREL(){         Inx();            AddCycle(2); } // 0xE8
	inline void SBCIMM(){ MrIm(); Sbc();            AddCycle(2); } // 0xE9, 0xEB
	inline void CPXABS(){ MrAb(); Cpx();            AddCycle(4); } // 0xEC
	inline void SBCABS(){ MrAb(); Sbc();            AddCycle(4); } // 0xED
	inline void INCABS(){ MrAb(); Inc(); MwEa();    AddCycle(6); } // 0xEE
	inline void ISBABS(){ MrAb(); Isb(); MwEa();    AddCycle(5); } // 0xEF
	inline void BEQREL(){ MrIm(); Beq();            AddCycle(2); } // 0xF0
	inline void SBCIZY(){ MrIy(); Sbc(); CheckEa(); AddCycle(5); } // 0xF1
	inline void ISBIZY(){ MrIy(); Isb(); MwEa();    AddCycle(5); } // 0xF3
	inline void SBCZPX(){ MrZx(); Sbc();            AddCycle(4); } // 0xF5
	inline void INCZPX(){ MrZx(); Inc(); MwZp();    AddCycle(6); } // 0xF6
	inline void ISBZPX(){ MrZx(); Isb(); MwZp();    AddCycle(5); } // 0xF7
	inline void SEDREL(){         Sed();            AddCycle(2); } // 0xF8
	inline void SBCABY(){ MrAy(); Sbc(); CheckEa(); AddCycle(4); } // 0xF9
	inline void ISBABY(){ MrAy(); Isb(); MwEa();    AddCycle(5); } // 0xFB
	inline void SBCABX(){ MrAx(); Sbc(); CheckEa(); AddCycle(4); } // 0xFD
	inline void INCABX(){ MrAx(); Inc(); MwEa();    AddCycle(7); } // 0xFE
	inline void ISBABX(){ MrAx(); Isb(); MwEa();    AddCycle(5); } // 0xFF
	inline void NOPREL(){                           AddCycle(2); } // 0x1A, 0x3A, 0x5A, 0x7A, 0xDA, 0xFA
	inline void DOP__2(){ r.pc++;                   AddCycle(2); } // 0x80, 0x82, 0x89, 0xC2, 0xE2
	inline void DOP__3(){ r.pc++;                   AddCycle(3); } // 0x04, 0x44, 0x64
	inline void DOP__4(){ r.pc++;                   AddCycle(4); } // 0x14, 0x34, 0x54, 0x74, 0xD4, 0xF4
	inline void TOPREL(){ r.pc++; r.pc++;           AddCycle(4); } // 0x0C, 0x1C, 0x3C, 0x5C, 0x7C, 0xDC, 0xFC
	inline void KILLED(){ this->IllegalOperationCodeAttached();  } // 0x02, 0x12, 0x22, 0x32, 0x42, 0x52, 0x62, 0x72, 0x92, 0xB2, 0xD2, 0xF2


	typedef void(Cpu::*operationFunc)(void);

	operationFunc operations[256] = {
//      0       1       2       3       4       5       6       7       8       9       A       B       C       D       E       F
/*0+ */	BRKREL, ORAIZX, KILLED, SLOIZX, DOP__3, ORAZP_, ASLZP_, SLOZP_, PHPREL, ORAIMM, ASLAAA, ANCIMM, TOPREL, ORAABS, ASLABS, SLOABS, //0+
/*10+*/	BPLREL, ORAIZY, KILLED, SLOIZY, DOP__4, ORAZPX, ASLZPX, SLOZPX, CLCREL, ORAABY, NOPREL, SLOABY, TOPREL, ORAABX, ASLABX, SLOABX, //10+
/*20+*/	JSRREL, ANDIZX, KILLED, RLAIZX, BITZP_, ANDZP_, ROLZP_, RLAZP_, PLPREL, ANDIMM, ROLAAA, ANCIMM, BITABS, ANDABS, ROLABS, RLAABS, //20+
/*30+*/	BMIREL, ANDIZY, KILLED, RLAIZY, DOP__4, ANDZPX, ROLZPX, RLAZPX, SECREL, ANDABY, NOPREL, RLAABY, TOPREL, ANDABX, ROLABX, RLAABX, //30+
/*40+*/	RTIREL, EORIZX, KILLED, SREIZX, DOP__3, EORZP_, LSRZP_, SREZP_, PHAREL, EORIMM, LSRAAA, ASRIMM, JMPABS, EORABS, LSRABS, SREABS, //40+
/*50+*/	BVCREL, EORIZY, KILLED, SREIZY, DOP__4, EORZPX, LSRZPX, SREZPX, CLIREL, EORABY, NOPREL, SREABY, TOPREL, EORABX, LSRABX, SREABX, //50+
/*60+*/	RTSREL, ADCIZX, KILLED, RRAIZX, DOP__3, ADCZP_, RORZP_, RRAZP_, PLAREL, ADCIMM, RORAAA, ARRIMM, JMPIZA, ADCABS, RORABS, RRAABS, //60+
/*70+*/	BVSREL, ADCIZY, KILLED, RRAIZY, DOP__4, ADCZPX, RORZPX, RRAZPX, SEIREL, ADCABY, NOPREL, RRAABY, TOPREL, ADCABX, RORABX, RRAABX, //70+
/*80+*/	DOP__2, STAIZX, DOP__2, SAXIZX, STYZP_, STAZP_, STXZP_, SAXZP_, DEYREL, DOP__2, TXAREL, ANEIMM, STYABS, STAABS, STXABS, SAXABS, //80+
/*90+*/	BCCREL, STAIZY, KILLED, SHAIZY, STYZPX, STAZPX, STXZPY, SAXZPY, TYAREL, STAABY, TXSREL, SHSABY, SHYABX, STAABX, SHXABY, SHAABY, //90+
/*A0+*/	LDYIMM, LDAIZX, LDXIMM, LAXIZX, LDYZP_, LDAZP_, LDXZP_, LAXZP_, TAYREL, LDAIMM, TAXREL, LXAIMM, LDYABS, LDAABS, LDXABS, LAXABS, //A0+
/*B0+*/	BCSREL, LDAIZY, KILLED, LAXIZY, LDYZPX, LDAZPX, LDXZPY, LAXZPY, CLVREL, LDAABY, TSXREL, LASABY, LDYABX, LDAABX, LDXABY, LAXABY, //B0+
/*C0+*/	CPYIMM, CMPIZX, DOP__2, DCPIZX, CPYZP_, CMPZP_, DECZP_, DCPZP_, INYREL, CMPIMM, DEXREL, SBXIMM, CPYABS, CMPABS, DECABS, DCPABS, //C0+
/*D0+*/	BNEREL, CMPIZY, KILLED, DCPIZY, DOP__4, CMPZPX, DECZPX, DCPZPX, CLDREL, CMPABY, NOPREL, DCPABY, TOPREL, CMPABX, DECABX, DCPABX, //D0+
/*E0+*/	CPXIMM, SBCIZX, DOP__2, ISBIZX, CPXZP_, SBCZP_, INCZP_, ISBZP_, INXREL, SBCIMM, NOPREL, SBCIMM, CPXABS, SBCABS, INCABS, ISBABS, //E0+
/*F0+*/	BEQREL, SBCIZY, KILLED, ISBIZY, DOP__4, SBCZPX, INCZPX, ISBZPX, SEDREL, SBCABY, NOPREL, ISBABY, TOPREL, SBCABX, INCABX, ISBABX, //F0+

////      0       1       2       3       4       5       6       7       8       9       A       B       C       D       E       F
///*0+ */	&BRKREL, &ORAIZX, &KILLED, &SLOIZX, &DOP__3, &ORAZP_, &ASLZP_, &SLOZP_, &PHPREL, &ORAIMM, &ASLAAA, &ANCIMM, &TOPREL, &ORAABS, &ASLABS, &SLOABS, //0+
///*10+*/	&BPLREL, &ORAIZY, &KILLED, &SLOIZY, &DOP__4, &ORAZPX, &ASLZPX, &SLOZPX, &CLCREL, &ORAABY, &NOPREL, &SLOABY, &TOPREL, &ORAABX, &ASLABX, &SLOABX, //10+
///*20+*/	&JSRREL, &ANDIZX, &KILLED, &RLAIZX, &BITZP_, &ANDZP_, &ROLZP_, &RLAZP_, &PLPREL, &ANDIMM, &ROLAAA, &ANCIMM, &BITABS, &ANDABS, &ROLABS, &RLAABS, //20+
///*30+*/	&BMIREL, &ANDIZY, &KILLED, &RLAIZY, &DOP__4, &ANDZPX, &ROLZPX, &RLAZPX, &SECREL, &ANDABY, &NOPREL, &RLAABY, &TOPREL, &ANDABX, &ROLABX, &RLAABX, //30+
///*40+*/	&RTIREL, &EORIZX, &KILLED, &SREIZX, &DOP__3, &EORZP_, &LSRZP_, &SREZP_, &PHAREL, &EORIMM, &LSRAAA, &ASRIMM, &JMPABS, &EORABS, &LSRABS, &SREABS, //40+
///*50+*/	&BVCREL, &EORIZY, &KILLED, &SREIZY, &DOP__4, &EORZPX, &LSRZPX, &SREZPX, &CLIREL, &EORABY, &NOPREL, &SREABY, &TOPREL, &EORABX, &LSRABX, &SREABX, //50+
///*60+*/	&RTSREL, &ADCIZX, &KILLED, &RRAIZX, &DOP__3, &ADCZP_, &RORZP_, &RRAZP_, &PLAREL, &ADCIMM, &RORAAA, &ARRIMM, &JMPIZA, &ADCABS, &RORABS, &RRAABS, //60+
///*70+*/	&BVSREL, &ADCIZY, &KILLED, &RRAIZY, &DOP__4, &ADCZPX, &RORZPX, &RRAZPX, &SEIREL, &ADCABY, &NOPREL, &RRAABY, &TOPREL, &ADCABX, &RORABX, &RRAABX, //70+
///*80+*/	&DOP__2, &STAIZX, &DOP__2, &SAXIZX, &STYZP_, &STAZP_, &STXZP_, &SAXZP_, &DEYREL, &DOP__2, &TXAREL, &ANEIMM, &STYABS, &STAABS, &STXABS, &SAXABS, //80+
///*90+*/	&BCCREL, &STAIZY, &KILLED, &SHAIZY, &STYZPX, &STAZPX, &STXZPY, &SAXZPY, &TYAREL, &STAABY, &TXSREL, &SHSABY, &SHYABX, &STAABX, &SHXABY, &SHAABY, //90+
///*A0+*/	&LDYIMM, &LDAIZX, &LDXIMM, &LAXIZX, &LDYZP_, &LDAZP_, &LDXZP_, &LAXZP_, &TAYREL, &LDAIMM, &TAXREL, &LXAIMM, &LDYABS, &LDAABS, &LDXABS, &LAXABS, //A0+
///*B0+*/	&BCSREL, &LDAIZY, &KILLED, &LAXIZY, &LDYZPX, &LDAZPX, &LDXZPY, &LAXZPY, &CLVREL, &LDAABY, &TSXREL, &LASABY, &LDYABX, &LDAABX, &LDXABY, &LAXABY, //B0+
///*C0+*/	&CPYIMM, &CMPIZX, &DOP__2, &DCPIZX, &CPYZP_, &CMPZP_, &DECZP_, &DCPZP_, &INYREL, &CMPIMM, &DEXREL, &SBXIMM, &CPYABS, &CMPABS, &DECABS, &DCPABS, //C0+
///*D0+*/	&BNEREL, &CMPIZY, &KILLED, &DCPIZY, &DOP__4, &CMPZPX, &DECZPX, &DCPZPX, &CLDREL, &CMPABY, &NOPREL, &DCPABY, &TOPREL, &CMPABX, &DECABX, &DCPABX, //D0+
///*E0+*/	&CPXIMM, &SBCIZX, &DOP__2, &ISBIZX, &CPXZP_, &SBCZP_, &INCZP_, &ISBZP_, &INXREL, &SBCIMM, &NOPREL, &SBCIMM, &CPXABS, &SBCABS, &INCABS, &ISBABS, //E0+
///*F0+*/	&BEQREL, &SBCIZY, &KILLED, &ISBIZY, &DOP__4, &SBCZPX, &INCZPX, &ISBZPX, &SEDREL, &SBCABY, &NOPREL, &ISBABY, &TOPREL, &SBCABX, &INCABX, &ISBABX, //F0+
	};

	void IllegalOperationCodeAttached(){
		if(!Config.emulator.illegalOp){
			// TODO: Throw Error String: CApp class.
		}else{
			r.pc--;
			AddCycle(4);
		}
	}


signals:
	ErrorOpcodeOccurredSignal();

public slots:
};

#endif // CPU_H
