#include "cpu.h"

// Constructor / Destructor
Cpu::Cpu(Nes *parent, QObject *p) : QObject(p), nes(parent)
{
	//this->nes = parent;
	Q_UNUSED(parent);
	this->clockProcess = false;
}

Cpu::~Cpu()
{

}
// Reset
void Cpu::reset()
{
	this->apu = this->nes->apu;
	this->mapper = this->nes->mapper;

	this->r.a = 0x00;
	this->r.x = 0x00;
	this->r.y = 0x00;
	this->r.s = 0xFF;
	this->r.p = ZFlag | RFlag;
	this->r.pc = rd6502(ResVector);
	this->r.intPending = 0;
	this->totalCycles = 0;
	this->dmaCycles = 0;
	// Stack quick access
	this->stack = &ram[0x0100];
	// Zero / Negative Flag
	this->znTable[0] = ZFlag;
	for(int i = 1; i < 256; i++){
		this->znTable[i] = (i & 0x80)? NFlag: 0;
	}
}

// Interrupt
void Cpu::nmi()
{
	r.intPending |= NmiFlag;
	this->nmicount = 0;
}

void Cpu::setIrq(quint8 mask)
{
	r.intPending |= mask;
}

void Cpu::clearIrq(quint8 mask)
{
	r.intPending &= ~mask;
}

// DMA pending cycle setting
void Cpu::dma(qint32 cycles)
{
	this->dmaCycles += cycles;
}

// Instruction execution >_<!!
qint32 Cpu::exec(qint32 requestCycles)
{
	uint8 opcode; // Operation code
	int oldCycles = this->totalCycles;
	int execCycles;
	uint8 nmiRequest;
	uint8 irqRequest;
	bool clockProcess = this->clockProcess;

	// Temp
//	register uint16 ea;
//	register uint16 et;
//	register uint16 wt;
//	register uint8  dt;

	while(requestCycles > 0){
		execCycles = 0;
		if(this->dmaCycles){
			if(requestCycles <= this->dmaCycles){
				this->dmaCycles -= requestCycles;
				totalCycles += requestCycles;

				// Clock synchronization process
				this->mapper->clock(requestCycles);
#if DpcmSyncClock
				this->apu->syncDpcm(requestCycles);
#endif
				if(clockProcess){
					this->nes->clock(requestCycles);
				}
				goto ExecuteExit;
			}else{
				execCycles += dmaCycles;
				//requestCycles -= dmaCycles;
				dmaCycles = 0;
			}
		}
		nmiRequest = irqRequest = 0;
		opcode = op6502(r.pc);
		r.pc++;
		if(r.intPending){
			if(r.intPending & NmiFlag){
				nmiRequest = 0xFF;
				r.intPending &= ~NmiFlag;
			}else if(r.intPending & IrqMask){
				r.intPending &= ~IrqTrigger2;
				if(!(r.p & IFlag) && opcode != 0x40){
					irqRequest = 0xFF;
					r.intPending &= ~IrqTrigger;
				}
			}
		}

		switch(opcode){

		}

	}
}

qint32 Cpu::getDmaCycles()
{
	return this->dmaCycles;
}

void Cpu::setDmaCycles(qint32 cycles)
{
	this->dmaCycles = cycles;
}

qint32 Cpu::getTotalCycles()
{
	return this->totalCycles;
}

void Cpu::setTotalCycles(qint32 cycles)
{
	this->totalCycles = cycles;
}




