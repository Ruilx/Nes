#ifndef APU_H
#define APU_H

#include <QtCore>
#include "typedef.h"
#include "macro.h"

#include "ApuEx/apuinternal.h"
#include "ApuEx/apuvrc6.h"
#include "ApuEx/apuvrc7.h"
#include "ApuEx/apummc5.h"
#include "ApuEx/apufds.h"
#include "ApuEx/apun106.h"
#include "ApuEx/apufme7.h"

//#include "app.h"
//#include "config.h"

#include "nes.h"
#include "mmu.h"
#include "cpu.h"
//#include "ppu.h"
//#include "rom.h"
#include "apu.h"


//#define QueueLength 4096
#define QueueLength 8192

// class prototype
//class Nes;

class Apu : public QObject
{
	Q_OBJECT
public:
	explicit Apu(Nes *nesParent, QObject *parent = 0);
	virtual ~Apu();

	void soundSetup();
	void selectExSound(uint8 data);
	bool setChannelMute(int ch){
		this->mute[ch] = !mute[ch];
		return mute[ch];
	}
	void queueClear();

	void reset();
	uint8 read(uint16 addr);
	void write(uint16 addr, uint8 data);
	uint8 exRead(uint16 addr);
	void exWrite(uint16 addr, uint8 data);

	void sync();
	void syncDpcm(int cycles);

	void process(uint8 *buffer, uint32 size);

	// For NSF player
	int getChannelFrequency(int ch);
	int16 *getSoundBuffer(){ return this->soundBuffer; }

	// For State
	void getFrameIrq(int &cycle, uint8 &count, uint8 &type, uint8 &irq, uint8 &occur){
		internal.getFrameIrq(cycle, count, type, irq, occur);
	}
	void setFrameIrq(int cycle, uint8 count, uint8 type, uint8 irq, uint8 occur){
		internal.setFrameIrq(cycle, count, type, irq, occur);
	}
	void saveState(uint8 *p);
	void loadState(uint8 *p);
protected:
	typedef struct{
		int time;
		uint16 addr;
		uint8 data;
		uint8 reserved;
	}QueueData, *lpQueueData;
	typedef struct{
		int rdPtr;
		int wrPtr;
		QueueData data[QueueLength];
	}Queue, *lpQueue;

	void setQueue(int writetime, uint16 addr, uint8 data);
	bool getQueue(int writetime, QueueData &ret);
	void setExQueue(int writetime, uint16 addr, uint8 data);
	bool getExQueue(int writetime, QueueData &ret);

	void queueFlush();

	void writeProcess(uint16 addr, uint8 data);
	void writeExProcess(uint16 addr, uint8 data);

	Nes *nes;

	Queue queue;
	Queue exqueue;

	uint8 exsoundSelect;

	double elapsedTime;
	//int elapsedTime;

	// Filter
	int lastData;
	int lastDiff;
	int lowpassFilter[4];

	// Sound core
	ApuInternal internal;
	ApuVrc6 vrc6;
	ApuVrc7 vrc7;
	ApuMmc5 mmc5;
	ApuFds fds;
	ApuN106 n106;
	ApuFme7 fme7;

	// Channel mute
	bool mute[16];

	// Bonus
	int16 soundBuffer[0x100];

signals:

public slots:
};

// Volume adjust
// Internal sounds
#define RectangleVol (0x00F0)
#define TriangleVol (0x0130)
#define NoiseVol (0x00C0)
#define DpcmVol (0x00F0)
// Extra sounds
#define Vrc6Vol (0x00F0)
#define Vrc7Vol (0x0130)
#define FdsVol (0x00F0)
#define Mmc5Vol (0x00F0)
#define N106Vol (0x0088)
#define Fme7Vol (0x0130)

#endif // APU_H
