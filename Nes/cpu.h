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
signals:

public slots:
};

#endif // CPU_H
