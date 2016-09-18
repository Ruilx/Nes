#include "cpu.h"

#define DpcmSyncClock false

// Operation code
//#define Op6502(A) Rd6502((A))
//#define Op6502W(A) Rd6502W((A))

// Zero page read
#define Zprd(A)      (Ram[(uint8)(A)])
#define ZprdW(A)     ((uint16)Ram[(uint8)(A)] + ((uint16)Ram[(uint8)((A) +1)] << 8))

#define Zpwr(A,V)    { Ram[(uint8)(A)] = (V) }
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

Cpu::Cpu(Nes *parent, QObject *p) : QObject(p)
{

}
