#ifndef APUINTERNAL_H
#define APUINTERNAL_H

#include <QObject>

#include "apuinterface.h"
#include "../nes.h"
#include "../cpu.h"

class ApuInternal : public QObject, public ApuInterface
{
	Q_OBJECT
public:
	ApuInternal(QObject *parent = 0);
	~ApuInternal();

	void setParent(Nes *parent) { nes = parent; }

	void reset(float clock, int rate);
	void setup(float clock, int rate);

	void toneTableLoad();

	int process(int channel);

	void write(uint16 addr, uint8 data);
	uint8 read(uint16 addr);

	void syncWrite(uint16 addr, uint8 data);
	uint8 syncRead(uint16 addr);
	bool sync(int cycles);

	int getFreq(int channel);

	void getFrameIrq(int &cycle, uint8 &count, uint8 &type, uint8 &irq, uint8 &occur){
		cycle = frameCycle;
		count = (uint8)frameCount;
		type = (uint8)frameType;
		irq = frameIrq;
		occur = frameIrqOccur;
	}

	void setFrameIrq(int cycle, uint8 count, uint8 type, uint8 irq, uint8 occur){
		frameCycle = cycle;
		frameCount = (int)count;
		frameType = (int)type;
		frameIrq = irq;
		frameIrqOccur = occur;
	}

	int getStateSize();
	void saveState(uint8 *p);
	void loadState(uint8 *p);
protected:
	typedef struct{
		uint8 reg[4];

		uint8 enable;
		uint8 holdnote;
		uint8 volume;
		uint8 complement;

		// For Render
		int phaseacc;
		int freq;
		int freqlimit;
		int adder;
		int duty;
		int lenCount;
		int nowvolume;

		// For Envelope
		uint8 envFixed;
		uint8 envDecay;
		uint8 envCount;
		uint8 dummy0;
		int envVol;

		// For Sweep
		uint8 swpOn;
		uint8 swpInc;
		uint8 swpShift;
		uint8 swpDecay;
		uint8 swpCount;
		uint8 dummy1[3];

		// For sync
		uint8 syncReg[4];
		uint8 syncOutputEnable;
		uint8 syncEnable;
		uint8 syncHoldnote;
		uint8 dummy2;
		int syncLenCount;
	} Rectangle, *lpRectangle;

	typedef struct{
		uint8 reg[4];

		uint8 enable;
		uint8 holdnote;
		uint8 counterStart;
		uint8 dummy0;

		int phaseacc;
		int freq;
		int lenCount;
		int linCount;
		int adder;

		int nowvolume;

		// For sync
		uint8 syncReg[4];
		uint8 syncEnable;
		uint8 syncHoldnote;
		uint8 syncCounterStart;
		//uint8 dummy1;
		int syncLenCount;
		int syncLinCount;
	}Triangle, *lpTriangle;

	typedef struct{
		uint8 reg[4];

		uint8 enable;
		uint8 holdnote;
		uint8 volume;
		uint8 xorTap;
		int shiftReg;

		// For Render
		int phaseacc;
		int freq;
		int lenCount;

		int nowvolume;
		int output;

		// For Envelope
		uint8 envFixed;
		uint8 envDecay;
		uint8 envCount;
		uint8 dummy0;
		int envVol;

		// For sync
		uint8 syncReg[4];
		uint8 syncOutputEnable;
		uint8 syncEnable;
		uint8 syncHoldnote;
		uint8 dummy1;
		int syncLenCount;
	}Noise, *lpNoise;

	typedef struct{
		uint8 reg[4];
		uint8 enable;
		uint8 looping;
		uint8 curByte;
		uint8 dpcmValue;

		int freq;
		int phaseacc;
		int output;

		uint16 address;
		uint16 cacheAddr;
		int dmalength;
		int cacheDmalength;
		int dpcmOutputReal;
		int dpcmOutputFake;
		int dpcmOutputOld;
		int dpcmOutputOffset;

		// For sync
		uint8 syncReg[4];
		uint8 syncEnable;
		uint8 syncLooping;
		uint8 syncIrqGen;
		uitn8 syncIrqEnable;
		int syncCycles;
		int syncCacheCycles;
		int syncDmalength;
		int syncCacheDmalength;
	}Dpcm, *lpDpcm;

	void syncWrite4017(uint8 data);
	void updateFrame();

	// Rectangle
	void writeRectangle(int no, uint16 addr, uint8 data);
	void updateRectangle(Rectangle &ch, int type);
	int renderRectangle(Rectangle &ch);
	// For Sync
	void syncWriteRectangle(int no, uint16 addr, uint8 data);
	void syncUpdateRectangle(Rectangle &ch, int type);

	// Triangle
	void writeTriangle(uint16 addr, uint8 data);
	void updateTriangle(int type);
	int renderTriangle();
	// For Sync
	void syncWriteTriangle(qint16 addr, uint8 data);
	void syncUpdateTriangle(int type);

	// Noise
	void writeNoise(uint16 addr, uint8 data);
	void updateNoise(int type);
	uint8 noiseShiftreg(uint8 xorTap);
	int renderNoise();
	// For Sync
	void syncWriteNoise(uint16 addr, uint8 data);
	void syncUpdateNoise(int type);

	// DPCM
	void writeDpcm(uint16 addr, uint8 data);
	//void updateDpcm();
	int renderDpcm();
	// For Sync
	void syncWriteDpcm(uint16 addr, uint8 data);
	bool syncUpdateDpcm(int cycles);

	// Frame Counter
	int frameCycle = 0;
	int frameCount = 0;
	int frameType = 0;
	uint8 frameIrq = 0xC0;
	uint8 frameIrqOccur = 0;

	// Channels
	Rectangle ch0;
	Rectangle ch1;
	Triangle ch2;
	Noise ch3;
	Dpcm ch4;

	// $4015 Reg
	uint8 reg4015 = 0;
	uint8 syncReg4015 = 0;

	// NES object(ref)
	Nes *nes = nullptr;

	// Sound
	float cpuClock = ApuClock;
	int samplingRate = 22050;
	int cycleRate = int(cpuClock * 65536.0f / 22050.0f);

	// Tables
	static int vblLength[32] = {
		5, 127,   10,   1,   19,   2,   40,   3,
	   80,   4,   30,   5,    7,   6,   13,   7,
		6,   8,   12,   9,   24,  10,   48,  11,
	   96,  12,   36,  13,    8,  14,   16,  15,
	};
	static int freqLimit[8] = {
		0x03FF, 0x0555, 0x0666, 0x071C, 0x0787, 0x07C1, 0x07E0, 0x07F0
	};
	static int dutyLut[4] = {
		2,  4,  8, 12
	};
	static int noiseFreq[16] = {
		4,    8,   16,   32,   64,   96,  128,  160,
	  202,  254,  380,  508,  762, 1016, 2034, 4068
	};
	// DMC transfer clock number table
	static int dpcmCycles[16] = {
		428, 380, 340, 320, 286, 254, 226, 214,
		190, 160, 142, 128, 106,  85,  72,  54
	};
	static int dpcmCyclesPal[16] = {
		397, 353, 315, 297, 265, 235, 209, 198,
		176, 148, 131, 118,  98,  78,  66,  50
	};
	//static int volEffect[16] = {
	//	100,  94,  88,  83,  78,  74,  71,  67,
	//	 64,  61,  59,  56,  54,  52,  50,  48
	//};

	// Wave Tables
	const int ToneDataMax = 16;
	const int ToneDataLen = 32;
	const int ChannelMax = 3;
	const int ToneMax = 4;
	bool toneTableEnable[ToneDataMax];
	int toneTable[ToneDataMax][ToneDataLen];
	int channelTone[ChannelMax][ToneMax];

signals:

public slots:
};

#endif // APUINTERNAL_H































































