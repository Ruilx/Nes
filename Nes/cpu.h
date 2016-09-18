#ifndef CPU_H
#define CPU_H

#include <QtCore>
#include "../typedef.h"
#include "../macro.h"

#include "nes.h"
#include "mmu.h"
#include "ppu.h"
#include "apu.h"
#include "rom.h"
#include "mapper.h"

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
typedef CpuStatusFlags::CFlag CFlag;
typedef CpuStatusFlags::ZFlag ZFlag;
typedef CpuStatusFlags::IFlag IFlag;
typedef CpuStatusFlags::DFlag DFlag;
typedef CpuStatusFlags::BFlag BFlag;
typedef CpuStatusFlags::RFlag RFlag;
typedef CpuStatusFlags::VFlag VFlag;
typedef CpuStatusFlags::NFlag NFlag;


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

	inline static uint16 rd6502(uint16 addr){
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

signals:

public slots:
};

#endif // CPU_H
