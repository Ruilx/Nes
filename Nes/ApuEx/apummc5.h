#ifndef APUMMC5_H
#define APUMMC5_H

#include <QObject>

#include "macro.h"
#include "typedef.h"
#include "apuinterface.h"

class ApuMmc5 : public QObject, public ApuInterface
{
	Q_OBJECT
public:
	explicit ApuMmc5(QObject *parent = 0);
	~ApuMmc5();

	void reset(float clock, int rate);
	void setup(float clock, int rate);
	void write(uint16 addr, uint8 data);
	int process(int channel);

	void syncWrite(uint16 addr, uint8 data);
	uint8 syncRead(uint16 addr);
	bool sync(int cycles);

	int getFreq(int channel);

	int getStateSize();
	void saveState(quint8 *p);
	void loadState(quint8 *p);
protected:
	typedef struct{
		uint8 reg[4];
		uint8 enable;
		int vblLength;
		int phaseacc;
		int freq;
		int outputVol;
		uint8 fixedEnvelope;
		uint8 holdnote;
		uint8 volume;
		uint8 envVol;
		int envPhase;
		int envDecay;
		int adder;
		int dutyFlip;
	}Rectangle, *lpRectangle;
	typedef struct{
		// For sync
		uint8 reg[4];
		uint8 enable;
		uint8 holdnote;
		uint8 dummy[2];
		int vblLength;
	}SyncRectangle, *lpSyncRectangle;

	int rectangleRender(Rectangle &ch);

	Rectangle ch0;
	Rectangle ch1;
	SyncRectangle sch0;
	SyncRectangle sch1;

	uint8 reg5010 = 0;
	uint8 reg5011 = 0;
	uint8 reg5015 = 0;
	int cycleRate;

	int frameCycle;
	uint8 syncReg5015 = 0;

	float cpuClock;

	// Tables
	static int vblLength[32] = {
		5, 127,   10,   1,   19,   2,   40,   3,
	   80,   4,   30,   5,    7,   6,   13,   7,
		6,   8,   12,   9,   24,  10,   48,  11,
	   96,  12,   36,  13,    8,  14,   16,  15
	};
	static int dutyLut[4] = {
		2,  4,  8, 12
	};
	static int decayLut[16];
	static int vblLut[32];
signals:

public slots:
};

#define RectangleVolShift 8
#define DaoutVolShift 6

#endif // APUMMC5_H
